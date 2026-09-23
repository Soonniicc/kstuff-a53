# PPR after rest mode: investigation

The `kstuff-a53-restmode-test1.elf` experiment is unsafe on the tested PS5:
after a game was closed, rest mode was entered and resumed, starting the game
froze the console with a black screen. No klog was captured for this run, so
the exact failing instruction is unknown. Do not ship or retest that ELF.

The experimental change has been reverted on `fix/ppr-restmode`. It inferred
resume from a new SceShellUI process and invoked the complete A53/PPR installer
after only a process-list idle heuristic. That heuristic does not establish
that PackageRead/APR or the A53 transport are safe to modify. The installer
also changes the kernel `hz` temporarily for each DECI5S request.

The user-supplied `kstuff-fpkg-1.13-dr-test1.elf` (SHA-256
`79e9e48de83d6229c82b7d0d2487e69d1b8fad59ef5d3923b337b5fc992c713b`)
contains `SceSystemStateMgrInfo` and imports `sceKernelOpenEventFlag`,
`sceKernelPollEventFlag` and `sceKernelCloseEventFlag`. Disassembly shows it
polls the low 16 bits of that event's result for state 1000 (WORKING), handles
open/poll errors and reopens the event, and calls its own PPR installer after
additional checks. Its installer is different from this repository's, so the
binary does not provide a drop-in source implementation.

Before another console test, obtain the matching source if available. Adapt
the explicit power-state monitor and coordinate A53/PPR writes with system
readiness; do not equate a ShellUI restart with a safe PPR write window. Record
klog from before entering rest mode through the game launch, including PPR
transport, power-state transitions and install result. A new candidate needs
static validation and a fresh-boot hardware test before being called fixed.

## PuTTY capture from the failed test

The file C:/Users/PC/Music/putty.log includes the sleep/wake sequence. Line 49 shows the kernel in `suspend phase3` while kstuff reports `PPR install: 446 ms`; line 51 reports `PPR install result: 0`; line 55 reports `resume: patch verified`. The kernel does not finish `suspend phase2_pre_sync` until line 87. Resume begins at line 287. Therefore the experimental worker ran during suspend, before a safe post-resume state. From line 1377 onward the capture shows IOD/BFS read and write errors (`errno=5`), then a SELF block load failure and process coredump. The timing strongly implicates the premature PPR operation, but the capture does not identify the exact write that caused the storage failures.

The PuTTY file was later extended (199,345 bytes, last modified 19:42:19). It continues with repeated `bfs_device_nsid1_read_` failures and failed `ssd0.system_data` writes (`errno=5`) through its final lines. No later PPR retry appears in the appended portion. This confirms persistent I/O failure after wake; it does not establish whether the console's internal storage itself is damaged or whether the running patch/transport caused transient I/O errors. Do not run the test ELF again.

## Rest-mode test 2 candidate

A new local branch `fix/ppr-restmode-v2` observes `SceSystemStateMgrInfo` directly and only schedules PPR verification after it has seen a sleep state (200, 300 or 500) followed by WORKING (1000). It requires five seconds of stable WORKING and three seconds with no visible game process. Unlike test 1, a ShellUI restart is not a resume signal. A fresh A53 transport discovery is required and the resumed path does not enable the kernel-hz acceleration. The transport refuses a packet if the power flag is no longer WORKING. One attempt is made per observed wake, with no automatic retry. Compilation is not evidence of PS5 safety; the candidate remains experimental pending a clean-boot hardware test and klog capture.
