#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

int main(int argc, char** argv) {
    std::ifstream ifs(argv[1], std::ios::ate);
    if (!ifs) {
        std::cerr << "failed to open " << argv[1] << std::endl;
        exit(-1);
    }

    std::string line;
    while (1) {
        std::getline(ifs, line);
        if (!line.empty()) {
            std::cout << line << '\n';
            line.clear();
        } else ifs.clear();
        usleep(10000);
    }
}
