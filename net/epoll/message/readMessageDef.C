#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stack>
#include <iterator>
#include <cstdint>
#include <cctype>
#include <cassert>

void split(const char *str, std::vector<std::string>& tokens, char c=' ') {
    do {
        const char *begin = str;
        while(*str != c && *str) str++;
        tokens.push_back(std::string(begin, str));
    } while (0 != *str++);
}
 
void preprocessing(const char* input, const char* output) {
    //read full file to buf, assume reasonable size
    std::ifstream ifile(input, std::ios::binary | std::ios::ate);
    std::streamsize size = ifile.tellg();
    ifile.seekg(0, std::ios::beg);
    std::vector<char> buf(size);
    if (!ifile.read(buf.data(), size))
    {
        std::cerr << "failed to read full file of " << input << '\n';
    }

    //first pass preprocessing
    std::ofstream ofile(output);
    for (size_t i=0; i<size; ++i) {
        if (std::isspace(buf[i])) continue;
        else if (buf[i] == '#') while (buf[i] != '\n') ++i;
        else if (buf[i] == '(') ofile << " ( ";
        else if (buf[i] == ')') ofile << " ) ";
        else { 
            while (!std::isspace(buf[i])) {
                if (buf[i] == ')')
                    ofile << " ) ";
                else
                    ofile << buf[i]; ++i; 
            }
            ofile << ' ';
        }
    }
    ofile.flush();
    ofile.close();
}

void pass1(const char* input, const char* output) {
    //read full file to string
    std::ifstream ifile(input);
    std::ostringstream oss;
    oss << ifile.rdbuf();
    std::string pass2 = oss.str();
    
    //top level expression
    std::ofstream ofile(output);
    std::stack<size_t> parserStack;
    for (size_t i=0; i<pass2.size(); ++i) {
        if (pass2[i] == '(') parserStack.push(i);
        if (pass2[i] == ')') {
            size_t top = parserStack.top();
            parserStack.pop();
            if (parserStack.empty()) {
                ofile << pass2.substr(top, i+1-top) << std::endl;
                //ofile << std::string(pass2.begin()+top, pass2.begin()+i+1) << std::endl; //buf case
            }
        }
    }
    ofile.flush();
    ofile.close();
}

void processPackage(const std::vector<std::string>& tokens) {
    //namespace and classname
    assert(tokens.size() == 5);
    std::ostream& ofile(std::cout);
    ofile << "namespace " << tokens[2] << "{\n";
    ofile << "\tclass " << tokens[3] << "{\n";
    ofile << "\t}\n";
    ofile << "}\n";
}

void processMessageFields(const std::vector<std::string>& tokens) {
    //vector of field
}

void processMessages(const std::vector<std::string>& tokens) {
    //vector of message
}

void processExpression(const std::vector<std::string>& tokens) {
    assert(tokens.size() > 2);
    if (tokens[1] == "package") processPackage(tokens);
    else if (tokens[1] == "messageFields") processMessageFields(tokens);
    else if (tokens[1] == "messages") processMessages(tokens);
    else std::cerr << "unrecognized top level token:" << tokens[1] << '\n';
}

void pass2(const char* input, const char* output) {
    std::ofstream ofile(output);
    std::ifstream ifile(input);
    std::string line;
    while(std::getline(ifile, line)) {
        std::vector<std::string> tokens;
		split(line.c_str(), tokens);
        processExpression(tokens);
        ofile << line << '\n';
	}
    ofile.flush();
    ofile.close();
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cerr << argv[0] << " messageDefFile\n";
        exit(1);
    }

    //first pass preprocessing
    preprocessing(argv[1], "pass1.txt");

    //second pass top level structure
    pass1("pass1.txt", "pass2.txt");
    
    //third pass handle expression
    pass2("pass2.txt", "pass3.txt");
}
