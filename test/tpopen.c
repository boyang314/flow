#include <stdio.h>
#include <string.h>
#include <unistd.h>

int requestResponse(const char* req, int ofd) {
    FILE *fd = popen(req, "r");
    if (!fd) {
        printf("popen failed for %s\n", req);
        return -1;
    }
    char buf[1024];
    while (fgets(buf, sizeof(buf), fd)) {
        write(ofd, buf, strlen(buf));
    }
    pclose(fd);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("%s command\n", argv[0]);
        return -1;
    }
    requestResponse(argv[1], STDOUT_FILENO);
}
