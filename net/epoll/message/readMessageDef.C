#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdint>
#include <cctype>
 
int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cerr << argv[0] << " messageDefFile\n";
        exit(1);
    }

    std::ifstream file(argv[1], std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buf(size);
    if (!file.read(buf.data(), size))
    {
        std::cerr << "failed to read full file of " << argv[1] << '\n';
    }

    for (size_t i=0; i<size; ++i) {
        if (std::isspace(buf[i])) continue;
        else if (buf[i] == '#') while (buf[i] != '\n') ++i;
        else if (buf[i] == '(') std::cout << "( ";
        else if (buf[i] == ')') std::cout << " )";
        else { 
            while (!std::isspace(buf[i])) {
                if (buf[i] == ')')
                    std::cout << " )";
                else
                    std::cout << buf[i]; ++i; 
            }
            std::cout << ' ';
        }
    }
/* 
    // prepare file for next snippet
    std::ofstream("test.txt", std::ios::binary) << "abcd1\nabcd2\nabcd3";
*/ 
}
