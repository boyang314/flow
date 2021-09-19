#include <stdio.h>
#include <string.h>
#include <unistd.h>
void reqResp(const char* req, int ofd) {
    FILE *fd = popen(req, "r");
    char buf[512];
    while (fgets(buf, sizeof(buf), fd)) {
        write(ofd, buf, strlen(buf));
    }
    pclose(fd);
}
