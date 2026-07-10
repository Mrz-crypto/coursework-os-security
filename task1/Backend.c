#define _GNU_SOURCE

#include <errno.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/coursework_auth.sock"
#define MAX_USERNAME 64
#define MAX_PASSWORD 128

struct auth_request {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
};

struct auth_response {
    int success;
    char message[128];
};

static void clear_memory(void *data, size_t size)
{
    volatile unsigned char *p = data;
    while (size--) {
        *p++ = 0;
    }
}

static void die(const char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

static void show_proc_uid(const char *label)
{
    char line[256];
    FILE *file = fopen("/proc/self/status", "r");

    printf("[backend] %s effective uid: %d\n", label, geteuid());

    if (file == NULL) {
        return;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        if (strncmp(line, "Uid:", 4) == 0) {
            printf("[backend] %s /proc: %s", label, line);
            break;
        }
    }

    fclose(file);
}

static int make_server_socket(void)
{
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket");
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    unlink(SOCKET_PATH);
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        die("bind");
    }

    if (getenv("SUDO_UID") != NULL) {
        uid_t user_uid = (uid_t)strtoul(getenv("SUDO_UID"), NULL, 10);
        if (chown(SOCKET_PATH, user_uid, getgid()) < 0) {
            die("chown");
        }
    }

    if (chmod(SOCKET_PATH, 0600) < 0) {
        die("chmod");
    }

    if (listen(fd, 5) < 0) {
        die("listen");
    }

    return fd;
}

static uid_t nobody_uid(void)
{
    struct passwd *nobody = getpwnam("nobody");
    if (nobody != NULL) {
        return nobody->pw_uid;
    }

    return getuid();
}

static void drop_privileges(void)
{
    uid_t uid = nobody_uid();

    show_proc_uid("before drop");

    if (setresuid(uid, uid, uid) != 0) {
        die("setresuid");
    }

    show_proc_uid("after drop");

    if (setuid(0) == 0) {
        fprintf(stderr, "[backend] error: root privileges came back\n");
        exit(EXIT_FAILURE);
    }

    printf("[backend] root privileges cannot be regained\n");
}

static int peer_uid(int fd)
{
    struct ucred cred;
    socklen_t len = sizeof(cred);

    if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cred, &len) != 0) {
        return -1;
    }

    return (int)cred.uid;
}

static bool valid_login(const struct auth_request *request)
{
    return strcmp(request->username, "student") == 0 &&
           strcmp(request->password, "Coursework123!") == 0;
}

static void reply(int fd, int success, const char *message)
{
    struct auth_response response;

    memset(&response, 0, sizeof(response));
    response.success = success;
    snprintf(response.message, sizeof(response.message), "%s", message);

    if (write(fd, &response, sizeof(response)) != sizeof(response)) {
        perror("write");
    }
}

static void handle_client(int fd)
{
    struct auth_request request;
    ssize_t bytes;
    int uid;

    memset(&request, 0, sizeof(request));

    uid = peer_uid(fd);
    printf("[backend] request from uid: %d\n", uid);
    if (uid < 0) {
        reply(fd, 0, "Rejected: peer credentials not available");
        return;
    }

    bytes = read(fd, &request, sizeof(request));
    if (bytes != sizeof(request)) {
        reply(fd, 0, "Rejected: malformed request");
        clear_memory(&request, sizeof(request));
        return;
    }

    request.username[MAX_USERNAME - 1] = '\0';
    request.password[MAX_PASSWORD - 1] = '\0';

    if (request.username[0] == '\0' || request.password[0] == '\0') {
        reply(fd, 0, "Rejected: empty username or password");
    } else if (valid_login(&request)) {
        reply(fd, 1, "Authentication successful");
    } else {
        reply(fd, 0, "Authentication failed");
    }

    clear_memory(&request, sizeof(request));
}

int main(void)
{
    int server_fd;

    printf("[backend] pid: %d\n", getpid());
    printf("[backend] socket: %s\n", SOCKET_PATH);

    server_fd = make_server_socket();

    if (geteuid() == 0) {
        drop_privileges();
    } else {
        show_proc_uid("running without sudo");
        printf("[backend] run with sudo to show setresuid evidence\n");
    }

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            if (errno == EINTR) {
                continue;
            }
            die("accept");
        }

        handle_client(client_fd);
        close(client_fd);
    }
}
