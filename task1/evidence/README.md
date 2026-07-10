# Task 1 Evidence Guide

Capture evidence on Linux or WSL after building the programs.

Build:

```bash
cd task1
make clean
make
```

Start the backend:

```bash
sudo ./Backend
```

In another terminal, run the frontend:

```bash
./Frontend
```

Useful evidence to include in the report:

- Backend startup message and PID.
- UID output before and after `setresuid()`.
- `/proc/self/status` UID line before and after dropping privileges.
- Message showing the backend cannot regain root.
- Socket permissions from `ls -l /tmp/coursework_auth.sock`.
- A successful login using `student` and `Coursework123!`.
- A failed login using an incorrect password.
- Backend log showing the peer UID from `SO_PEERCRED`.

Example socket permission command:

```bash
ls -l /tmp/coursework_auth.sock
```

The `*_non_root.txt` files in this folder show local IPC, successful login,
failed login, and socket permissions without root privileges. For the final
report, also capture one manual run of `sudo ./Backend` so the `setresuid()`
privilege drop appears in your evidence.
