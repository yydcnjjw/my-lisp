#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

ssize_t file_read_all(char *filename, u_int8_t **s_file) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        printf("file open failure: %d\n", errno);
        return -1;
    }

    struct stat s;
    if (-1 == stat(filename, &s)) {
        printf("file parse failure: %d\n", errno);
        return -1;
    }
    size_t file_size;
    file_size = s.st_size;

    u_char* s_file_p;
    *s_file = s_file_p = malloc(file_size * sizeof(u_char));
    bzero(s_file_p, file_size * sizeof(char));

#define BUF_SIZE 4 * 1024
    size_t size_tmp = file_size;
    while (size_tmp > 0) {
        ssize_t size = read(fd, s_file_p, BUF_SIZE);
        if (size == -1) {
            printf("error\n", errno);
            return -1;
        }

        size_tmp -= size;
        s_file_p += size;
    }

    printf("%s", *s_file);
    return file_size;
}
