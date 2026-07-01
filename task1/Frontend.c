#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <termios.h>
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

static void trim_newline(char *value)
{
    value[strcspn(value, "\n")] = '\0';
}

static void read_password(char *buffer, size_t size)
{
    struct termios old_term;
    struct termios new_term;

    if (tcgetattr(STDIN_FILENO, &old_term) != 0) {
        perror("tcgetattr");
        exit(EXIT_FAILURE);
    }

    new_term = old_term;
    new_term.c_lflag &= ~(ECHO);

    printf("Password: ");
    fflush(stdout);

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_term) != 0) {
        perror("tcsetattr");
        exit(EXIT_FAILURE);
    }

    if (fgets(buffer, size, stdin) == NULL) {
        buffer[0] = '\0';
    }

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &old_term);
    printf("\n");
    trim_newline(buffer);
}

static int connect_to_backend(void)
{
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        fprintf(stderr, "Is Backend running? Try: sudo ./Backend\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    return fd;
}

int main(void)
{
    struct auth_request request;
    struct auth_response response;

    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));

    printf("Username: ");
    fflush(stdout);
    if (fgets(request.username, sizeof(request.username), stdin) == NULL) {
        fprintf(stderr, "Failed to read username\n");
        return EXIT_FAILURE;
    }
    trim_newline(request.username);

    read_password(request.password, sizeof(request.password));

    int fd = connect_to_backend();

    if (write(fd, &request, sizeof(request)) != sizeof(request)) {
        perror("write");
        secure_bzero(&request, sizeof(request));
        close(fd);
        return EXIT_FAILURE;
    }

    secure_bzero(&request, sizeof(request));

    ssize_t bytes = read(fd, &response, sizeof(response));
    if (bytes != sizeof(response)) {
        perror("read");
        close(fd);
        return EXIT_FAILURE;
    }

    printf("%s\n", response.message);
    close(fd);

    return response.success ? EXIT_SUCCESS : EXIT_FAILURE;
}
