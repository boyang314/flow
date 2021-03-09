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
#include <map>

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

namespace {
std::string ns;
std::string cn;
std::string fieldIdsEnum;
std::string fieldTypeIdsEnum{"MessageFieldTypeEnum"};
std::string messageTypeIdsEnum;
std::map<std::string, std::string> fieldNameToId;
std::map<std::string, std::string> fieldNameToType;
std::map<std::string, std::string> fieldNameToLength;
std::map<std::string, std::string> messageTypeToId;
typedef std::vector<std::string> ListOfFields;
std::map<std::string, ListOfFields> messageTypeToFields;
};

void processPackage(const std::vector<std::string>& tokens) {
    //namespace and classname
    assert(tokens.size() == 5);
    std::ostream& ofile(std::cout);
    ofile << "namespace " << tokens[2] << "{\n";
    ofile << "\tclass " << tokens[3] << "{\n";
    ofile << "\t}\n";
    ofile << "}\n";
    ns = tokens[2];
    cn = tokens[3];
}

void processMessageFields(const std::vector<std::string>& tokens) {
    //vector of fields
    assert(tokens.size() > 3);
    std::ostream& ofile(std::cout);
    fieldIdsEnum = cn + "FiledIdsEnum";
    ofile << "struct " << fieldIdsEnum << " {\n"; 
    ofile << "    enum TypeId {\n"; 
    for (size_t i=2; i<tokens.size(); ++i) {
        if (tokens[i] == "(") {
            std::string fname = tokens[++i];
            std::string fid = tokens[++i];
            std::string ftype = tokens[++i]; //ensure numOfEntries
            ofile << '\t' << fname << " = " << fid << ",\n";
            fieldNameToId[fname] = fid;
            fieldNameToType[fname] = ftype;
            if (ftype == "string") fieldNameToLength[fname] = tokens[++i];
            assert(tokens[++i] == ")");
        }
    }
    ofile << "    };\n"; 
    ofile << "};\n"; 
}

void processMessages(const std::vector<std::string>& tokens) {
    //vector of messages
    assert(tokens.size() > 3);
    std::ostream& ofile(std::cout);
    messageTypeIdsEnum = cn + "TypeIdsEnum";
    ofile << "struct " << messageTypeIdsEnum  << " {\n"; 
    ofile << "    enum TypeId {\n"; 
    for (size_t i=2; i<tokens.size(); ++i) {
        if (tokens[i] == "(") {
            std::string mname = tokens[++i];
            std::string mid = tokens[++i];
            messageTypeToId[mname] = mid;
            ofile << '\t' << mname << " = " << mid << ",\n";
            ListOfFields fields;
            while (tokens[++i] != ")") {
                fields.push_back(tokens[i]);
            }
            messageTypeToFields[mname] = fields;
        }
    }
    ofile << "    };\n"; 
    ofile << "};\n"; 
}

void processExpression(const std::vector<std::string>& tokens) {
    assert(tokens.size() > 2);
    if (tokens[1] == "package") processPackage(tokens);
    else if (tokens[1] == "messageFields") processMessageFields(tokens);
    else if (tokens[1] == "messages") processMessages(tokens);
    else std::cerr << "unrecognized top level token:" << tokens[1] << '\n';

    //construct header
    for (auto& entry : messageTypeToFields) {
        std::string messageType = entry.first;
        std::string msgHeader = messageType + "Header";
        std::cout << "struct " << msgHeader << " {\n";
        std::cout << "\t" << msgHeader << "(const uint32_t messageSize) : header(messageSize, " << messageTypeIdsEnum << "::" << messageType << ", " << messageTypeToFields[messageType].size() << ")";
        for (auto& field : entry.second) {
            std::cout << ", " << field << "(" << fieldIdsEnum << "::" << field << ", " << fieldTypeIdsEnum << "::_" << fieldNameToType[field];
            if (fieldNameToType[field] == "string") {
                std::cout << ", " << fieldNameToLength[field] << ")";
            } else {
                std::cout << ", sizeof(" << fieldNameToType[field] << "))";
            }
        }
        std::cout << " {}\n";
        std::cout << "\tbool operator==(const " << msgHeader << " &other) const { return memcmp(this, &other, sizeof(*this)) == 0; }\n";
        std::cout << "\tMessageHeader header;\n";
        for (auto& field : entry.second) std::cout << '\t' << "MessageFieldInfo " << field << ";\n";
        std::cout << "} __attribute__((packed));\n";
    }

    //construct class
    for (auto& entry : messageTypeToFields) {
        std::string messageType = entry.first;
        std::string msgHeader = messageType + "Header";

        std::cout << "struct " << messageType << " {\n";

        //first ctor
        std::cout << '\t' << messageType << "() : header_(sizeof(*this))";
        for (auto& field : entry.second) {
            std::string fieldName = field + "_";
            std::cout << ", " << fieldName << "()";
        }
        std::cout << " {}\n";

        //second ctor
        std::cout << '\t' << messageType << "(";
        int count = 0;
        for (auto& field : entry.second) {
            if (fieldNameToType[field] == "string") {
                std::cout << "const char* " << field;
            } else {
                std::cout << fieldNameToType[field] << " " << field;
            }
            if (++count != entry.second.size()) std::cout << ", ";
        }
        std::cout << ") : header_(sizeof(*this))";
        std::vector<std::string> strFields;
        for (auto& field : entry.second) {
            std::string fieldName = field + "_";
            std::cout << ", " << fieldName << "(";
            if (fieldNameToType[field] == "string") {
                std::cout << ")";
                strFields.push_back(field);
            } else {
                std::cout << field << ")";
            }
        }
        std::cout << " {";
        for (auto& field : strFields) {
            std::string fieldName = field + "_";
            int len = atoi(fieldNameToLength[field].c_str());
            std::cout << "\n\t" << "strncpy(" << fieldName << ", " << field << ", " << len-1 << ");";
            std::cout << "\n\t" << fieldName << "[" << len-1 << "] = '\\0'; ";
        }
        std::cout << "}\n";

        //third ctor
        std::cout << '\t' << messageType << "(MessageInputBuffer& messasgeBuffer) : header_(sizeof(*this))";
        for (auto& field : entry.second) {
            std::string fieldName = field + "_";
            std::cout << ", " << fieldName << "()";
        }
        std::cout << " { fromMessageBuffer(messageBuffer); }\n";

        //variable
        std::cout << '\t' <<  msgHeader << " header_;\n";
        for (auto& field : entry.second) {
            std::string fieldName = field + "_";
            if (fieldNameToType[field] == "string") {
                std::cout << '\t' << "char " << fieldName << "[" << fieldNameToLength[field] << "];\n";
            } else {
                std::cout << '\t' << fieldNameToType[field] << " " << fieldName << ";\n";
            }
        }
        std::cout << "} __attribute__((packed));\n";
    }
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
