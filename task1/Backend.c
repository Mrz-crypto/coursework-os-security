#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
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

static void secure_bzero(void *ptr, size_t len)
{
    volatile unsigned char *p = (volatile unsigned char *)ptr;
    while (len--) {
        *p++ = 0;
    }
}

static void die(const char *message)
{
    perror(message);
    exit(EXIT_FAILURE);
}

static int open_server_socket(void)
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

    const char *sudo_uid = getenv("SUDO_UID");
    if (sudo_uid != NULL) {
        uid_t owner = (uid_t)strtoul(sudo_uid, NULL, 10);
        if (chown(SOCKET_PATH, owner, getgid()) < 0) {
            die("chown");
        }
    }

    if (chmod(SOCKET_PATH, 0600) < 0) {
        die("chmod");
    }

    if (listen(fd, 8) < 0) {
        die("listen");
    }

    return fd;
}

static uid_t choose_unprivileged_uid(void)
{
    struct passwd *nobody = getpwnam("nobody");
    if (nobody != NULL) {
        return nobody->pw_uid;
    }

    return getuid();
}

static void permanently_drop_privileges(void)
{
    uid_t target = choose_unprivileged_uid();

    printf("[backend] effective uid before drop: %d\n", geteuid());

    if (setresuid(target, target, target) != 0) {
        die("setresuid");
    }

    printf("[backend] effective uid after drop: %d\n", geteuid());

    if (setuid(0) == 0) {
        fprintf(stderr, "[backend] ERROR: process regained root privileges\n");
        exit(EXIT_FAILURE);
    }

    printf("[backend] privilege drop is irreversible in this process\n");
}

static bool validate_credentials(const struct auth_request *request)
{
    const char expected_user[] = "student";
    const char expected_password[] = "Coursework123!";

    return strcmp(request->username, expected_user) == 0 &&
           strcmp(request->password, expected_password) == 0;
}

static int get_peer_uid(int client_fd)
{
    struct ucred cred;
    socklen_t len = sizeof(cred);

    if (getsockopt(client_fd, SOL_SOCKET, SO_PEERCRED, &cred, &len) == -1) {
        return -1;
    }

    return (int)cred.uid;
}

static void handle_client(int client_fd)
{
    struct auth_request request;
    struct auth_response response;

    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));

    int peer_uid = get_peer_uid(client_fd);
    printf("[backend] request from peer uid: %d\n", peer_uid);

    ssize_t bytes = read(client_fd, &request, sizeof(request));
    if (bytes != sizeof(request)) {
        response.success = 0;
        snprintf(response.message, sizeof(response.message),
                 "Rejected: malformed authentication request");
        write(client_fd, &response, sizeof(response));
        secure_bzero(&request, sizeof(request));
        return;
    }

    request.username[MAX_USERNAME - 1] = '\0';
    request.password[MAX_PASSWORD - 1] = '\0';

    if (strlen(request.username) == 0 || strlen(request.password) == 0) {
        response.success = 0;
        snprintf(response.message, sizeof(response.message),
                 "Rejected: empty username or password");
    } else if (validate_credentials(&request)) {
        response.success = 1;
        snprintf(response.message, sizeof(response.message),
                 "Authentication successful");
    } else {
        response.success = 0;
        snprintf(response.message, sizeof(response.message),
                 "Authentication failed");
    }

    if (write(client_fd, &response, sizeof(response)) != sizeof(response)) {
        perror("write");
    }

    secure_bzero(&request, sizeof(request));
}

int main(void)
{
    printf("[backend] starting authentication backend, pid=%d\n", getpid());
    printf("[backend] socket path: %s\n", SOCKET_PATH);

    int server_fd = open_server_socket();

    if (geteuid() == 0) {
        permanently_drop_privileges();
    } else {
        printf("[backend] not running as root; current effective uid: %d\n", geteuid());
        printf("[backend] setresuid demonstration skipped because root is required\n");
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
