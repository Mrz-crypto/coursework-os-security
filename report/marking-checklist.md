# Coursework Marking Checklist

Use this as a final review before submitting the report and GitHub link.

## Task 1

- [x] `Frontend.c` exists.
- [x] `Backend.c` exists.
- [x] Two independent executables are built.
- [x] UNIX domain socket IPC is used.
- [x] Backend checks peer credentials with `SO_PEERCRED`.
- [x] Backend rejects malformed or empty requests.
- [x] Backend calls `setresuid()` when started with root privileges.
- [x] Backend prints runtime UID evidence.
- [x] Password buffers are explicitly cleared.
- [ ] Add screenshots or copied terminal logs to the written report.
- [ ] Answer all 12 Task 1 investigation questions in the report.

## Task 2

- [x] `Sandbox.c` exists.
- [x] Test binaries are included.
- [x] Sandbox uses `fork()`.
- [x] Child uses `execve()`.
- [x] Parent supervises the child from outside.
- [x] Wall-clock time is monitored.
- [x] CPU time is monitored through `/proc/<pid>/stat`.
- [x] Signals are used for forced termination.
- [x] Multiple pthread monitor threads are used.
- [x] Shared monitor state uses atomics and a mutex.
- [ ] Add generated logs to the written report.
- [ ] Answer all 11 Task 2 investigation questions in the report.

## Repository and report

- [x] Public GitHub repository exists.
- [x] README explains how to build and run both tasks.
- [x] Diagrams are included.
- [ ] Final report includes references.
- [ ] Final report uses one consistent referencing style.
- [ ] Final report includes the GitHub repository URL.
