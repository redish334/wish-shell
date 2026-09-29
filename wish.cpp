#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>

std::vector<std::string> search_paths = {"/bin"};

void print_error() {
    char error_message[30] = "An error has occurred\n";
    if (write(STDERR_FILENO, error_message, strlen(error_message)) < 0) {
    }
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

pid_t launch_command(std::string cmd_str) {
    std::string output_file = "";
    size_t redir_pos = cmd_str.find('>');

    if (redir_pos != std::string::npos) {
        std::string left = cmd_str.substr(0, redir_pos);
        std::string right = cmd_str.substr(redir_pos + 1);

        // Перевірка на множинний знак '>'
        if (right.find('>') != std::string::npos) {
            print_error();
            return -1;
        }

        std::vector<std::string> out_tokens = tokenize(right);
        if (out_tokens.size() != 1) {
            print_error();
            return -1;
        }
        output_file = out_tokens[0];
        cmd_str = left;
    }

    std::vector<std::string> args = tokenize(cmd_str);
    if (args.empty()) {
        if (redir_pos != std::string::npos) print_error();
        return -1;
    }

    if (execute_builtin(args)) {
        return -1;
    }

    std::string exec_path = "";
    for (const auto &p : search_paths) {
        std::string full = p + "/" + args[0];
        if (access(full.c_str(), X_OK) == 0) {
            exec_path = full;
            break;
        }
    }

    if (exec_path.empty()) {
        print_error();
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        print_error();
        return -1;
    }

    if (pid == 0) {
        if (!output_file.empty()) {
            int fd = open(output_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
            if (fd < 0) {
                print_error();
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);
        }

        std::vector<char*> c_args;
        for (const auto &arg : args) {
            c_args.push_back(const_cast<char*>(arg.c_str()));
        }
        c_args.push_back(nullptr);

        execv(exec_path.c_str(), c_args.data());
        print_error();
        exit(1);
    }

    return pid;
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

        // Розбиття на команди, розділені символом '&'
        std::vector<std::string> parallel_cmds;
        std::stringstream ss(line);
        std::string segment;
        while (std::getline(ss, segment, '&')) {
            parallel_cmds.push_back(segment);
        }

        std::vector<pid_t> pids;
        for (const auto &cmd : parallel_cmds) {
            pid_t pid = launch_command(cmd);
            if (pid > 0) {
                pids.push_back(pid);
            }
        }

        for (pid_t pid : pids) {
            waitpid(pid, nullptr, 0);
        }
    }

    return 0;
}
