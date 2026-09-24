/* Adapted from BestPig/BackPork (GPL-3.0). */
#include <stdint.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <time.h>
#include <sys/event.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <ps5/klog.h>

#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/sysctl.h>
#include <sys/user.h>


#define IOVEC_ENTRY(x) {x ? (char *)x : 0, x ? strlen(x) + 1 : 0}
#define IOVEC_SIZE(x) (sizeof(x) / sizeof(struct iovec))

#define BP_LOG(...) klog_printf("[BP] " __VA_ARGS__)

typedef struct app_info {
    uint32_t app_id;
    uint64_t unknown1;
    char title_id[14];
    char unknown2[0x3c];
} app_info_t;

extern int sceKernelGetAppInfo(pid_t pid, app_info_t *info);

// from john-tornblom
static pid_t find_pid(const char *name) {
    int mib[4] = {1, 14, 8, 0};
    pid_t mypid = getpid();
    pid_t pid = -1;
    size_t buf_size;
    uint8_t *buf;

    if (sysctl(mib, 4, 0, &buf_size, 0, 0)) {
        return -1;
    }

    if (!(buf = malloc(buf_size))) {
        return -1;
    }

    if (sysctl(mib, 4, buf, &buf_size, 0, 0)) {
        free(buf);
        return -1;
    }

    for (uint8_t *ptr = buf; ptr < (buf + buf_size);) {
        int ki_structsize = *(int *)ptr;
        pid_t ki_pid = *(pid_t *)&ptr[72];
        char *ki_tdname = (char *)&ptr[447];

        ptr += ki_structsize;
        if (!strcmp(name, ki_tdname) && ki_pid != mypid) {
            pid = ki_pid;
        }
    }

    free(buf);

    return pid;
}

static int mount2(const char *src, const char *dst, const char *type) {
    struct iovec iov[] = {
        IOVEC_ENTRY("fstype"),
        IOVEC_ENTRY(type),
        IOVEC_ENTRY("from"),
        IOVEC_ENTRY(src),
        IOVEC_ENTRY("fspath"),
        IOVEC_ENTRY(dst),
    };

    return nmount(iov, IOVEC_SIZE(iov), 0);
}


static char *mount_fakelibs(const char *sandbox_id, const char *cwd, pid_t pid, char *random_folder) {
    char fake_path[PATH_MAX + 1];
    snprintf(fake_path, sizeof(fake_path), "%s/fakelib", cwd);

    struct stat st;
    if (stat(fake_path, &st) != 0) {
        BP_LOG("fakelib stat failed pid=%d errno=%d path=%s\n", pid, errno, fake_path);
        printf("[WARNING] stat on %s failed (errno: %d, %s)\n", fake_path, errno, strerror(errno));
        return NULL;
    }

    char *fake_mount_path = (char *)malloc(PATH_MAX + 1);
    if (!fake_mount_path) {
        return NULL;
    }

    snprintf(fake_mount_path, PATH_MAX + 1, "/mnt/sandbox/%s/%s/common/lib", sandbox_id, random_folder);

    int res = mount2(fake_path, fake_mount_path, "unionfs");
    if (res != 0) {
        BP_LOG("unionfs mount failed pid=%d errno=%d dst=%s\n", pid, errno, fake_mount_path);
        printf("[WARNING] mount_unionfs failed: %d (errno: %d, %s)\n", res, errno, strerror(errno));
        /* No mount was created; do not force-unmount this path. */
        free(fake_mount_path);
        return NULL;
    }

    printf("[INFO] Mounted fakelibs from %s to %s\n", fake_path, fake_mount_path);
    BP_LOG("mounted pid=%d src=%s dst=%s\n", pid, fake_path, fake_mount_path);
    return fake_mount_path;
}

static int find_highest_sandbox_number(const char* title_id) {
    char base_path[PATH_MAX];
    int highest = -1;

    for (int i = 0; i < 1000; i++) {
        snprintf(base_path, sizeof(base_path), "/mnt/sandbox/%s_%03d", title_id, i);

        struct stat st;
        if (stat(base_path, &st) == 0 && S_ISDIR(st.st_mode)) {
            highest = i;
        } else {
            break;
        }
    }

    return highest;
}

static char* find_random_folder(const char* title_id, int sandbox_num) {
    char base_path[PATH_MAX];
    snprintf(base_path, sizeof(base_path), "/mnt/sandbox/%s_%03d", title_id, sandbox_num);

    DIR* dir = opendir(base_path);
    if (!dir) {
        return NULL;
    }

    struct dirent* entry;
    while ((entry = readdir(dir))) {
        if (entry->d_name[0] == '.') {
            continue;
        }

        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s/common/lib", base_path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) {
            closedir(dir);
            return strdup(entry->d_name);
        }
    }

    closedir(dir);
    return NULL;
}

/* Adapted from BestPig/BackPork. Keep all work in the loader ELF process. */
static char *try_mount_game(pid_t pid, const char *title) {
    int number = find_highest_sandbox_number(title);
    if (number < 0) return NULL;
    char sandbox[14], app0[PATH_MAX];
    snprintf(sandbox, sizeof(sandbox), "%s_%03d", title, number);
    snprintf(app0, sizeof(app0), "/mnt/sandbox/%s/app0", sandbox);
    char fakelib[PATH_MAX];
    struct stat st;
    snprintf(fakelib, sizeof(fakelib), "%s/fakelib", app0);
    if (stat(fakelib, &st) != 0 || !S_ISDIR(st.st_mode))
        return NULL;
    char *folder = find_random_folder(title, number);
    if (!folder) return NULL;
    char *mounted = mount_fakelibs(sandbox, app0, pid, folder);
    free(folder);
    return mounted;
}

#define MAX_BP_WORKERS 16
static volatile int workers;

static void *watch_game(void *opaque) {
    pid_t pid = (pid_t)(intptr_t)opaque;
    int kq = kqueue();
    if (kq < 0) goto done;
    struct kevent change, event;
    EV_SET(&change, pid, EVFILT_PROC, EV_ADD | EV_ENABLE | EV_CLEAR,
           NOTE_EXEC | NOTE_EXIT, 0, NULL);
    if (kevent(kq, &change, 1, NULL, 0, NULL) < 0) {
        BP_LOG("process watch failed pid=%d errno=%d\n", pid, errno);
        close(kq);
        goto done;
    }
    BP_LOG("exec watch armed pid=%d\n", pid);
    struct timespec timeout = {5, 0};
    int event_count = kevent(kq, NULL, 0, &event, 1, &timeout);
    if (event_count <= 0 || (event.fflags & NOTE_EXIT) ||
        !(event.fflags & NOTE_EXEC)) {
        BP_LOG("exec not observed pid=%d events=%d flags=0x%x\n",
               pid, event_count, event_count > 0 ? event.fflags : 0);
        close(kq);
        goto done;
    }
    BP_LOG("exec observed pid=%d\n", pid);
    char title[10] = {0};
    char *mounted = NULL;
    struct timespec pause = {0, 1000000};
    /* Match the original BackPork's post-exec mount point. */
    for (int attempt = 0; attempt < 100; ++attempt) {
        struct timespec zero = {0, 0};
        int pending = kevent(kq, NULL, 0, &event, 1, &zero);
        if (pending > 0 && (event.fflags & NOTE_EXIT)) break;
        app_info_t info = {0};
        if (sceKernelGetAppInfo(pid, &info) == 0) {
            memcpy(title, info.title_id, 9);
            if (strncmp(title, "PPSA", 4) && strncmp(title, "CUSA", 4))
                break;
            mounted = try_mount_game(pid, title);
            if (mounted) {
                BP_LOG("mounted after exec pid=%d title=%s attempt=%d dst=%s\n",
                       pid, title, attempt, mounted);
                break;
            }
        }
        nanosleep(&pause, NULL);
    }
    if (!mounted && (!strncmp(title, "PPSA", 4) ||
                     !strncmp(title, "CUSA", 4)))
        BP_LOG("post-exec mount missed pid=%d title=%s\n", pid, title);
    if (mounted) {
        while (kevent(kq, NULL, 0, &event, 1, NULL) > 0) {
            if (event.fflags & NOTE_EXIT) break;
        }
        /* Unmount before SysCore tries to remove common/lib. Never rmdir it. */
        int rc = unmount(mounted, 0);
        BP_LOG("unmount pid=%d rc=%d errno=%d dst=%s\n", pid, rc,
               rc ? errno : 0, mounted);
        free(mounted);
    }
    close(kq);
done:
    __sync_sub_and_fetch(&workers, 1);
    return NULL;
}

int backpork_main(void) {
    BP_LOG("native monitor started pid=%d\n", getpid());
    for (;;) {
        pid_t syscore = find_pid("SceSysCore.elf");
        if (syscore < 0) { sleep(1); continue; }
        int kq = kqueue();
        if (kq < 0) { sleep(1); continue; }
        struct kevent change;
        EV_SET(&change, syscore, EVFILT_PROC,
               EV_ADD | EV_ENABLE | EV_CLEAR,
               NOTE_FORK | NOTE_TRACK | NOTE_EXIT, 0, NULL);
        if (kevent(kq, &change, 1, NULL, 0, NULL) < 0) {
            BP_LOG("syscore watch failed pid=%d errno=%d\n", syscore, errno);
            close(kq);
            sleep(1);
            continue;
        }
        BP_LOG("monitor active syscore=%d\n", syscore);
        for (;;) {
            struct kevent event;
            int n = kevent(kq, NULL, 0, &event, 1, NULL);
            if (n < 0) break;
            if (!n) continue;
            if (event.ident == (uintptr_t)syscore &&
                (event.fflags & NOTE_EXIT)) break;
            if (!(event.fflags & NOTE_CHILD)) continue;
            pid_t pid = (pid_t)event.ident;
            if (__sync_add_and_fetch(&workers, 1) > MAX_BP_WORKERS) {
                __sync_sub_and_fetch(&workers, 1);
                BP_LOG("worker limit reached pid=%d\n", pid);
                continue;
            }
            pthread_t thread;
            int err = pthread_create(&thread, NULL, watch_game,
                                     (void *)(intptr_t)pid);
            if (err) {
                __sync_sub_and_fetch(&workers, 1);
                BP_LOG("worker failed pid=%d error=%d\n", pid, err);
            } else {
                pthread_detach(thread);
                BP_LOG("child detected pid=%d\n", pid);
            }
        }
        close(kq);
        sleep(1);
    }
    return 0;
}

void *backpork_thread_entry(void *unused) {
    (void)unused;
    backpork_main();
    return NULL;
}
