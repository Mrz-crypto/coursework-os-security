# Operating Systems Security Coursework

This repository contains the practical implementation for two coursework tasks:

1. Privilege separated password validation using independent C processes.
2. A user-space malware analysis sandbox using process control, resource monitoring, signals, and pthread concurrency.

The code is written for Linux because the coursework requires Unix/Linux operating system features such as `fork`, `execve`, UNIX domain sockets, `setresuid`, POSIX signals, pthreads, and `/proc`.
The implementation is intentionally simple and direct so it can be explained clearly in the coursework report.

## Requirements

- Linux, WSL Ubuntu/Kali, or a Linux virtual machine
- GCC
- Make

Install tools on Ubuntu/Debian/WSL:

```bash
sudo apt update
sudo apt install build-essential make
```

## Task 1: Privilege Separated Password Validation

Files:

- `task1/Frontend.c`
- `task1/Backend.c`
- `task1/Makefile`

Build:

```bash
cd task1
make
```

Run the backend in one terminal:

```bash
sudo ./Backend
```

Run the frontend in another terminal:

```bash
./Frontend
```

Test credentials:

```text
Username: student
Password: Coursework123!
```

Evidence to capture:

- Backend process ID.
- Backend effective UID before and after privilege dropping.
- Backend `/proc/self/status` UID line before and after privilege dropping.
- Backend log showing a peer UID from the UNIX domain socket.
- Frontend successful and failed authentication attempts.
- Socket permissions:

```bash
ls -l /tmp/coursework_auth.sock
```

More guidance is in `task1/evidence/README.md`.

## Task 2: User-Space Malware Analysis Sandbox

Files:

- `task2/Sandbox.c`
- `task2/test-binaries/safe_program.c`
- `task2/test-binaries/infinite_loop.c`
- `task2/test-binaries/cpu_hog.c`
- `task2/test-binaries/slow_program.c`
- `task2/Makefile`

Build:

```bash
cd task2
make
```

Run a safe program:

```bash
./Sandbox ./test-binaries/safe_program
```

Run a CPU-heavy program:

```bash
./Sandbox ./test-binaries/cpu_hog
```

Run a slow program:

```bash
./Sandbox ./test-binaries/slow_program
```

Run an infinite loop:

```bash
./Sandbox ./test-binaries/infinite_loop
```

Save logs:

```bash
./Sandbox ./test-binaries/safe_program | tee logs/safe_program.txt
./Sandbox ./test-binaries/cpu_hog | tee logs/cpu_hog_termination.txt
./Sandbox ./test-binaries/slow_program | tee logs/slow_program_termination.txt
./Sandbox ./test-binaries/infinite_loop | tee logs/infinite_loop_termination.txt
```

Evidence to capture:

- Parent process supervising the child PID.
- Safe program finishing normally.
- Child killed after wall-clock or CPU limit.
- Signal used for termination.
- Logs in `task2/logs/`.

More guidance is in `task2/logs/README.md`.

## Repository Layout

```text
.
|-- README.md
|-- assignment-brief
|   `-- coursework-brief.txt
|-- diagrams
|   |-- task1-auth-flow.mmd
|   `-- task2-sandbox-flow.mmd
|-- report
|   |-- marking-checklist.md
|   `-- report-outline.md
|-- task1
|   |-- Backend.c
|   |-- Frontend.c
|   |-- Makefile
|   `-- evidence
|       `-- README.md
`-- task2
    |-- Sandbox.c
    |-- Makefile
    |-- logs
    |   `-- README.md
    `-- test-binaries
```

## Important Coursework Notes

- Task 1 uses two independent executables, not threads.
- Task 1 uses a UNIX domain socket for local IPC.
- Task 1 demonstrates privilege dropping with `setresuid`.
- Task 1 prints runtime UID evidence with `geteuid`, `getresuid`, and `/proc/self/status`.
- Task 1 clears password buffers using an explicit volatile memory wipe.
- Task 2 uses `fork` and `execve` to execute untrusted binaries.
- Task 2 keeps monitoring in the parent process.
- Task 2 uses multiple pthread monitor threads.
- Task 2 uses atomics to synchronize shared state safely.
- Task 2 uses signals to terminate unsafe children.

## Suggested Report Evidence

Include terminal screenshots or copied logs showing:

- Successful compilation of both tasks.
- Task 1 backend privilege state before and after dropping privileges.
- Task 1 successful and failed login attempts.
- Task 2 safe binary finishing normally.
- Task 2 CPU-heavy, slow, and infinite-loop binaries being terminated.
- Public GitHub repository URL.

Also use:

- `assignment-brief/coursework-brief.txt` for a short requirement summary.
- `report/marking-checklist.md` before final submission.
- `diagrams/task1-auth-flow.mmd` and `diagrams/task2-sandbox-flow.mmd` in the report.
