# Task 2 Log Guide

Generate logs on Linux or WSL after building the sandbox and test binaries.

```bash
cd task2
make clean
make
./Sandbox ./test-binaries/safe_program | tee logs/safe_program.txt
./Sandbox ./test-binaries/cpu_hog | tee logs/cpu_hog_termination.txt
./Sandbox ./test-binaries/slow_program | tee logs/slow_program_termination.txt
./Sandbox ./test-binaries/infinite_loop | tee logs/infinite_loop_termination.txt
```

The report should use these logs to show:

- The parent supervisor knows the child PID.
- The safe program exits normally.
- CPU-heavy or endless programs are terminated by the parent.
- Long-running programs are stopped by the wall-clock limit.
- Termination is enforced with signals, not by the child program itself.
