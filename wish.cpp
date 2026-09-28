#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>

std::vector<std::string> search_paths = {"/bin"};

void print_error() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

std::vector<std::string> tokenize(const std::string &str) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (ss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

bool execute_builtin(const std::vector<std::string> &args) {
    if (args.empty()) return true;

    if (args[0] == "exit") {
        if (args.size() != 1) {
            print_error();
        } else {
            exit(0);
        }
        return true;
    }

    if (args[0] == "cd") {
        if (args.size() != 2) {
            print_error();
        } else {
            if (chdir(args[1].c_str()) != 0) {
                print_error();
            }
        }
        return true;
    }

    if (args[0] == "path") {
        search_paths.clear();
        for (size_t i = 1; i < args.size(); ++i) {
            search_paths.push_back(args[i]);
        }
        return true;
    }

    return false;
}

int main(int argc, char *argv[]) {
    std::istream *input_stream = &std::cin;
    std::ifstream file_stream;
    bool is_interactive = true;

    if (argc == 2) {
        file_stream.open(argv[1]);
        if (!file_stream.is_open()) {
            print_error();
            return 1;
        }
        input_stream = &file_stream;
        is_interactive = false;
    } else if (argc > 2) {
        print_error();
        return 1;
    }

    std::string line;
    while (true) {
        if (is_interactive) {
            std::cout << "wish> ";
            std::cout.flush();
        }

        if (!std::getline(*input_stream, line)) {
            break;
        }

        std::vector<std::string> args = tokenize(line);
        if (!args.empty()) {
            execute_builtin(args);
        }
    }

    return 0;
}
