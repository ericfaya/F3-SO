#include "md5functions.h"




char *calculateMD5(const char *filename) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open");
        return NULL;
    }

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        close(fd);
        return NULL;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        close(fd);
        close(pipefd[0]);
        close(pipefd[1]);
        return NULL;
    }

    if (pid == 0) { // Process fill
        close(pipefd[0]);
        if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        close(pipefd[1]);
        close(fd);
        execlp("md5sum", "md5sum", filename, NULL);
        perror("execlp"); // 
        exit(EXIT_FAILURE);
    }

    // Process pare
    close(pipefd[1]);
    close(fd);

    char buffer[128]; // Buffer per llegir sortida del md5
    ssize_t bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1);
    if (bytes_read == -1) {
        perror("read");
        close(pipefd[0]);
        return NULL;
    }
    buffer[bytes_read] = '\0'; 

    waitpid(pid, NULL, 0); // esperem fill
    close(pipefd[0]);

    char *md5sum = (char *)malloc(33);
    if (sscanf(buffer, "%32s", md5sum) != 1) {
        perror("sscanf");
        free(md5sum);
        return NULL;
    }

    return md5sum;
}

int verifyMD5SUM(const char *file_path, const char *expected_md5) {
    char *actual_md5 = calculateMD5(file_path);
    if (actual_md5 == NULL) {
        return 1; 
    }

    int result = strcmp(actual_md5, expected_md5);
    free(actual_md5);

    return (result == 0) ? 0 : 1; //0 iguals, 1 diferents
}

