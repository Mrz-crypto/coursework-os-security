# Report Outline

## 1. Introduction

Briefly explain that the coursework investigates operating system security mechanisms through C programs.

## 2. Task 1: Privilege Separated Password Validation

### 2.1 Aim

Explain the purpose of separating user input handling from privileged password validation.

### 2.2 Design

Describe `Frontend.c`, `Backend.c`, the UNIX domain socket, and the authentication flow.

### 2.3 Implementation

Discuss:

- Independent processes.
- `socket`, `bind`, `listen`, `accept`, and `connect`.
- `SO_PEERCRED`.
- `setresuid`.
- `geteuid`.
- Explicit password buffer clearing.

### 2.4 Testing and Evidence

Add screenshots/logs showing compilation, backend startup, UID change, socket permissions, and login results.

### 2.5 Investigation Questions

Answer the 12 Task 1 questions from the assignment brief.

## 3. Task 2: User Space Malware Analysis Sandbox

### 3.1 Aim

Explain the purpose of running untrusted binaries under parent-process supervision.

### 3.2 Design

Describe the parent-child model, monitor threads, resource policies, and signal-based termination.

### 3.3 Implementation

Discuss:

- `fork`.
- `execve`.
- `pthread_create`.
- Atomic shared state.
- `/proc/<pid>/stat` CPU measurement.
- Wall-clock monitoring.
- `SIGTERM` and `SIGKILL`.

### 3.4 Testing and Evidence

Add logs from:

- `safe_program`.
- `cpu_hog`.
- `slow_program`.
- `infinite_loop`.

### 3.5 Investigation Questions

Answer the 11 Task 2 questions from the assignment brief.

### 3.6 Limitations

Explain that this is a user-space sandbox and does not provide full kernel-level containment like containers, VMs, seccomp, namespaces, or AppArmor.

## 4. Conclusion

Summarise what the programs demonstrated about process isolation, privilege management, IPC, signals, and concurrency.

## 5. References

Use Harvard or IEEE style consistently. Suggested sources:

- Michael Kerrisk, The Linux Programming Interface.
- Linux man pages for `fork`, `execve`, `setresuid`, `unix`, `socket`, `pthread_create`, `kill`, and `proc`.
- SEI CERT C Coding Standard.
- OWASP guidance on least privilege and secure memory handling.
