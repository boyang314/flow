#include <stdio.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("%s command\n", argv[0]);
        return -1;
    }
    FILE *fd = popen(argv[1], "r");
    if (!fd) {
        printf("popen failed for %s\n", argv[1]);
        return -1;
    }
    char buf[1024];
    while (fgets(buf, sizeof(buf), fd)) {
        printf("%s", buf);
    }
    pclose(fd);
}
