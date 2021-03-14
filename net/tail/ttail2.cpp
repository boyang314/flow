#include <fstream>
#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/inotify.h>

int main(int argc, char** argv) {
    std::ifstream ifs(argv[1], std::ios::ate);
    if (!ifs) {
        std::cerr << "failed to open " << argv[1] << std::endl;
        exit(-1);
    }

    int fd = inotify_init();
    if (fd < 0) {
        std::cerr << "inotify_init failed: " << std::strerror(errno) << std::endl;
        exit(-1);
    }

    int wd = inotify_add_watch(fd, argv[1], IN_MODIFY);
    if (wd < 0) {
        std::cerr << "inotify_add_watch failed: " << std::strerror(errno) << std::endl;
        exit(-1);
    }

    std::string line;
    while (1) {
        inotify_event event;
        int len = read(fd, &event, sizeof(event));
        if (len < 0) {
            std::cerr << "read inotify fd failed: " << std::strerror(errno) << std::endl;
            exit(-1);
        }
        std::getline(ifs, line);
        std::cout << line << '\n';
        line.clear();
    }
}
