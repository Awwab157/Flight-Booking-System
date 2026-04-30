#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/wait.h>

static int log_pipe[2];
static pid_t logger_pid;

void init_logger() {
    if (pipe(log_pipe) == -1) {
        perror("pipe failed");
        exit(EXIT_FAILURE);
    }

    logger_pid = fork();
    if (logger_pid < 0) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (logger_pid == 0) {
        // Child process (Logger)
        close(log_pipe[1]); // Close write end
        FILE *log_file = fopen("server.log", "a");
        if (!log_file) {
            perror("fopen log file");
            exit(EXIT_FAILURE);
        }

        char buffer[256];
        ssize_t n;
        while ((n = read(log_pipe[0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[n] = '\0';
            fprintf(log_file, "%s", buffer);
            fflush(log_file);
        }

        fclose(log_file);
        close(log_pipe[0]);
        exit(EXIT_SUCCESS);
    } else {
        // Parent process (Server)
        close(log_pipe[0]); // Close read end
    }
}

void log_message(const char *format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    write(log_pipe[1], buffer, strlen(buffer));
}

void close_logger() {
    close(log_pipe[1]);
}
