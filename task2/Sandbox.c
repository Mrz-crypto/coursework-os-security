#define _GNU_SOURCE

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_TIME_LIMIT_SECONDS 5
#define DEFAULT_CPU_LIMIT_SECONDS 3
#define POLL_INTERVAL_MS 200

struct sandbox_state {
    pid_t child_pid;
    int time_limit_seconds;
    int cpu_limit_seconds;
    atomic_bool child_finished;
    atomic_bool termination_requested;
    atomic_int exit_status;
    pthread_mutex_t log_lock;
    struct timespec started_at;
};

static void usage(const char *program_name)
{
    fprintf(stderr,
            "Usage: %s <program> [args...]\n"
            "Example: %s ./test-binaries/infinite_loop\n",
            program_name, program_name);
}

static long elapsed_ms(const struct timespec *start)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    long seconds = now.tv_sec - start->tv_sec;
    long nanoseconds = now.tv_nsec - start->tv_nsec;

    return (seconds * 1000L) + (nanoseconds / 1000000L);
}

static void log_event(struct sandbox_state *state, const char *message)
{
    pthread_mutex_lock(&state->log_lock);
    printf("[sandbox] %s\n", message);
    fflush(stdout);
    pthread_mutex_unlock(&state->log_lock);
}

static void terminate_child(struct sandbox_state *state, const char *reason)
{
    bool expected = false;
    if (!atomic_compare_exchange_strong(&state->termination_requested, &expected, true)) {
        return;
    }

    char line[256];
    snprintf(line, sizeof(line), "termination requested: %s", reason);
    log_event(state, line);

    if (kill(state->child_pid, SIGTERM) == -1 && errno != ESRCH) {
        perror("kill(SIGTERM)");
        return;
    }

    usleep(300 * 1000);

    if (!atomic_load(&state->child_finished)) {
        log_event(state, "child did not exit after SIGTERM; sending SIGKILL");
        if (kill(state->child_pid, SIGKILL) == -1 && errno != ESRCH) {
            perror("kill(SIGKILL)");
        }
    }
}

static void *wall_clock_monitor(void *arg)
{
    struct sandbox_state *state = arg;

    while (!atomic_load(&state->child_finished)) {
        long ms = elapsed_ms(&state->started_at);

        if (ms > state->time_limit_seconds * 1000L) {
            terminate_child(state, "wall-clock time limit exceeded");
            return NULL;
        }

        usleep(POLL_INTERVAL_MS * 1000);
    }

    return NULL;
}

static long read_clock_ticks(pid_t pid)
{
    char path[64];
    char buffer[4096];
    FILE *file;
    unsigned long utime = 0;
    unsigned long stime = 0;

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

    char *after_name = strrchr(buffer, ')');
    if (after_name == NULL) {
        return -1;
    }

    char state;
    long skipped;
    int scanned = sscanf(after_name + 2,
                         "%c %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld %lu %lu",
                         &state,
                         &skipped, &skipped, &skipped, &skipped, &skipped,
                         &skipped, &skipped, &skipped, &skipped, &skipped,
                         &utime, &stime);

    if (scanned != 13) {
        return -1;
    }

    return (long)(utime + stime);
}

static void *cpu_monitor(void *arg)
{
    struct sandbox_state *state = arg;
    long ticks_per_second = sysconf(_SC_CLK_TCK);

    while (!atomic_load(&state->child_finished)) {
        long ticks = read_clock_ticks(state->child_pid);
        if (ticks >= 0 && ticks_per_second > 0) {
            double total = (double)ticks / (double)ticks_per_second;

            if (total > state->cpu_limit_seconds) {
                terminate_child(state, "CPU time limit exceeded");
                return NULL;
            }
        }

        usleep(POLL_INTERVAL_MS * 1000);
    }

    return NULL;
}

static void *wait_monitor(void *arg)
{
    struct sandbox_state *state = arg;
    int status = 0;

    if (waitpid(state->child_pid, &status, 0) == -1) {
        perror("waitpid");
        atomic_store(&state->exit_status, EXIT_FAILURE);
    } else {
        atomic_store(&state->exit_status, status);
    }

    atomic_store(&state->child_finished, true);
    log_event(state, "child process has exited");
    return NULL;
}

static void run_child(char **child_argv)
{
    char *child_env[] = {
        "PATH=/usr/bin:/bin",
        NULL
    };

    execve(child_argv[0], child_argv, child_env);
    perror("execve");
    _exit(127);
}

static void report_status(struct sandbox_state *state)
{
    int status = atomic_load(&state->exit_status);

    if (WIFEXITED(status)) {
        printf("[sandbox] child exited normally with code %d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("[sandbox] child killed by signal %d\n", WTERMSIG(status));
    } else {
        printf("[sandbox] child ended with unknown status: %d\n", status);
    }
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    struct sandbox_state state;
    memset(&state, 0, sizeof(state));
    state.time_limit_seconds = DEFAULT_TIME_LIMIT_SECONDS;
    state.cpu_limit_seconds = DEFAULT_CPU_LIMIT_SECONDS;
    atomic_init(&state.child_finished, false);
    atomic_init(&state.termination_requested, false);
    atomic_init(&state.exit_status, 0);
    pthread_mutex_init(&state.log_lock, NULL);

    clock_gettime(CLOCK_MONOTONIC, &state.started_at);

    pid_t child = fork();
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
           state.time_limit_seconds, state.cpu_limit_seconds);

    pthread_t wall_thread;
    pthread_t cpu_thread;
    pthread_t wait_thread;

    pthread_create(&wall_thread, NULL, wall_clock_monitor, &state);
    pthread_create(&cpu_thread, NULL, cpu_monitor, &state);
    pthread_create(&wait_thread, NULL, wait_monitor, &state);

    pthread_join(wait_thread, NULL);
    pthread_join(wall_thread, NULL);
    pthread_join(cpu_thread, NULL);

    report_status(&state);
    pthread_mutex_destroy(&state.log_lock);

    return atomic_load(&state.termination_requested) ? EXIT_FAILURE : EXIT_SUCCESS;
}
