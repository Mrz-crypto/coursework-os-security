#define _GNU_SOURCE

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define TIME_LIMIT_SECONDS 5
#define CPU_LIMIT_SECONDS 3
#define POLL_MS 200

struct sandbox_state {
    pid_t child_pid;
    atomic_bool child_done;
    atomic_bool stopping;
    atomic_int child_status;
    pthread_mutex_t log_lock;
    struct timespec started;
};

static long elapsed_ms(struct timespec *started)
{
    struct timespec now;

    clock_gettime(CLOCK_MONOTONIC, &now);

    return (now.tv_sec - started->tv_sec) * 1000L +
           (now.tv_nsec - started->tv_nsec) / 1000000L;
}

static void log_line(struct sandbox_state *state, const char *message)
{
    pthread_mutex_lock(&state->log_lock);
    printf("[sandbox] %s\n", message);
    fflush(stdout);
    pthread_mutex_unlock(&state->log_lock);
}

static void stop_child(struct sandbox_state *state, const char *reason)
{
    bool expected = false;
    char line[160];

    if (!atomic_compare_exchange_strong(&state->stopping, &expected, true)) {
        return;
    }

    snprintf(line, sizeof(line), "termination requested: %s", reason);
    log_line(state, line);

    if (kill(state->child_pid, SIGTERM) != 0 && errno != ESRCH) {
        perror("kill(SIGTERM)");
        return;
    }

    usleep(300000);

    if (!atomic_load(&state->child_done)) {
        log_line(state, "child ignored SIGTERM, sending SIGKILL");
        if (kill(state->child_pid, SIGKILL) != 0 && errno != ESRCH) {
            perror("kill(SIGKILL)");
        }
    }
}

static void *time_monitor(void *arg)
{
    struct sandbox_state *state = arg;

    while (!atomic_load(&state->child_done)) {
        if (elapsed_ms(&state->started) > TIME_LIMIT_SECONDS * 1000L) {
            stop_child(state, "wall-clock time limit exceeded");
            return NULL;
        }

        usleep(POLL_MS * 1000);
    }

    return NULL;
}

static long read_cpu_ticks(pid_t pid)
{
    char path[64];
    char buffer[4096];
    FILE *file;
    char *field;
    char *after_name;
    int field_number = 3;
    unsigned long user_ticks = 0;
    unsigned long system_ticks = 0;

    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    file = fopen(path, "r");
    if (file == NULL) {
        return -1;
    }

    if (fgets(buffer, sizeof(buffer), file) == NULL) {
        fclose(file);
        return -1;
    }
    fclose(file);

    after_name = strrchr(buffer, ')');
    if (after_name == NULL) {
        return -1;
    }

    field = strtok(after_name + 2, " ");
    while (field != NULL) {
        if (field_number == 14) {
            user_ticks = strtoul(field, NULL, 10);
        } else if (field_number == 15) {
            system_ticks = strtoul(field, NULL, 10);
            break;
        }

        field = strtok(NULL, " ");
        field_number++;
    }

    return (long)(user_ticks + system_ticks);
}

static void *cpu_monitor(void *arg)
{
    struct sandbox_state *state = arg;
    long ticks_per_second = sysconf(_SC_CLK_TCK);

    while (!atomic_load(&state->child_done)) {
        long ticks = read_cpu_ticks(state->child_pid);

        if (ticks >= 0 && ticks_per_second > 0) {
            if (ticks / ticks_per_second > CPU_LIMIT_SECONDS) {
                stop_child(state, "CPU time limit exceeded");
                return NULL;
            }
        }

        usleep(POLL_MS * 1000);
    }

    return NULL;
}

static void *wait_for_child(void *arg)
{
    struct sandbox_state *state = arg;
    int status = 0;

    if (waitpid(state->child_pid, &status, 0) < 0) {
        perror("waitpid");
    }

    atomic_store(&state->child_status, status);
    atomic_store(&state->child_done, true);
    log_line(state, "child process has exited");

    return NULL;
}

static void run_child(char **argv)
{
    char *envp[] = { "PATH=/usr/bin:/bin", NULL };

    execve(argv[0], argv, envp);
    perror("execve");
    _exit(127);
}

static void print_result(struct sandbox_state *state)
{
    int status = atomic_load(&state->child_status);

    if (WIFEXITED(status)) {
        printf("[sandbox] child exited with code %d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("[sandbox] child killed by signal %d\n", WTERMSIG(status));
    } else {
        printf("[sandbox] child ended with status %d\n", status);
    }
}

int main(int argc, char **argv)
{
    struct sandbox_state state;
    pthread_t time_thread;
    pthread_t cpu_thread;
    pthread_t wait_thread;
    pid_t child;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <program> [args...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    memset(&state, 0, sizeof(state));
    atomic_init(&state.child_done, false);
    atomic_init(&state.stopping, false);
    atomic_init(&state.child_status, 0);
    pthread_mutex_init(&state.log_lock, NULL);
    clock_gettime(CLOCK_MONOTONIC, &state.started);

    child = fork();
    if (child < 0) {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (child == 0) {
        run_child(&argv[1]);
    }

    state.child_pid = child;
    printf("[sandbox] supervising child pid=%d\n", child);
    printf("[sandbox] wall-clock limit=%d seconds, CPU limit=%d seconds\n",
           TIME_LIMIT_SECONDS, CPU_LIMIT_SECONDS);

    pthread_create(&time_thread, NULL, time_monitor, &state);
    pthread_create(&cpu_thread, NULL, cpu_monitor, &state);
    pthread_create(&wait_thread, NULL, wait_for_child, &state);

    pthread_join(wait_thread, NULL);
    pthread_join(time_thread, NULL);
    pthread_join(cpu_thread, NULL);

    print_result(&state);
    pthread_mutex_destroy(&state.log_lock);

    return atomic_load(&state.stopping) ? EXIT_FAILURE : EXIT_SUCCESS;
}
