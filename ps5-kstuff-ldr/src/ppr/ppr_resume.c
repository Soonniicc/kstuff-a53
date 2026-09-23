#include "ppr_resume.h"
#include "ppr_install_bridge.h"

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <ps5/klog.h>

typedef struct app_info {
    uint32_t app_id;
    uint64_t unknown1;
    char title_id[14];
    char unknown2[0x3c];
} app_info_t;

extern int sceKernelGetAppInfo(pid_t pid, app_info_t *info);

static pthread_mutex_t resume_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t resume_cond = PTHREAD_COND_INITIALIZER;
static unsigned requested_generation;

/* Fail closed if the process list cannot be read or parsed. The installer
 * requires PackageRead/APR to be idle, so never run it with a game open. */
static int no_game_processes(void) {
    int mib[4] = {1, 14, 8, 0};
    size_t size = 0;
    if (sysctl(mib, 4, NULL, &size, NULL, 0) != 0 || size == 0)
        return 0;

    uint8_t *buffer = malloc(size);
    if (!buffer)
        return 0;
    if (sysctl(mib, 4, buffer, &size, NULL, 0) != 0) {
        free(buffer);
        return 0;
    }

    int idle = 1;
    for (size_t offset = 0; offset < size;) {
        int entry_size;
        if (size - offset < sizeof(entry_size)) {
            idle = 0;
            break;
        }
        memcpy(&entry_size, buffer + offset, sizeof(entry_size));
        if (entry_size < 72 + (int)sizeof(pid_t) ||
            (size_t)entry_size > size - offset) {
            idle = 0;
            break;
        }
        pid_t pid;
        memcpy(&pid, buffer + offset + 72, sizeof(pid));
        if (pid > 0 && pid != getpid()) {
            app_info_t info = {0};
            if (sceKernelGetAppInfo(pid, &info) == 0 &&
                (!memcmp(info.title_id, "PPSA", 4) ||
                 !memcmp(info.title_id, "CUSA", 4))) {
                idle = 0;
                break;
            }
        }
        offset += (size_t)entry_size;
    }
    free(buffer);
    return idle;
}

static void *resume_worker(void *unused) {
    (void)unused;
    unsigned handled = 0;
    for (;;) {
        pthread_mutex_lock(&resume_lock);
        while (requested_generation == handled)
            pthread_cond_wait(&resume_cond, &resume_lock);
        unsigned generation = requested_generation;
        pthread_mutex_unlock(&resume_lock);

        /* ShellUI can appear before the A53 and package service have settled. */
        sleep(3);
        unsigned idle_samples = 0;
        for (unsigned wait_count = 0;
             wait_count < 120 && idle_samples < 3; ++wait_count) {
            if (no_game_processes())
                ++idle_samples;
            else
                idle_samples = 0;
            if (idle_samples < 3) {
                if (wait_count == 0)
                    klog_printf("[PPR] resume: waiting for idle system\n");
                sleep(2);
            }
        }
        if (idle_samples != 3 || !no_game_processes()) {
            klog_printf("[PPR] resume: idle state not confirmed; patch skipped\n");
        } else {
            for (unsigned attempt = 1; attempt <= 3; ++attempt) {
                klog_printf("[PPR] resume: checking/installing patch attempt %u\n",
                            attempt);
                if (run_ppr_install() == 0) {
                    klog_printf("[PPR] resume: patch verified\n");
                    break;
                }
                if (attempt == 3)
                    klog_printf("[PPR] resume: patch unavailable after retries\n");
                else
                    sleep(3);
            }
        }
        handled = generation;
    }
    return NULL;
}

int ppr_resume_start(void) {
    pthread_t thread;
    int result = pthread_create(&thread, NULL, resume_worker, NULL);
    if (result != 0)
        return -1;
    pthread_detach(thread);
    return 0;
}

void ppr_resume_request(void) {
    pthread_mutex_lock(&resume_lock);
    ++requested_generation;
    pthread_cond_signal(&resume_cond);
    pthread_mutex_unlock(&resume_lock);
    klog_printf("[PPR] resume: new ShellUI detected\n");
}
