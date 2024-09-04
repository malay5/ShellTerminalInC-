#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <sstream>
#include <string>

using namespace std;


string trim(const string &str) {
    size_t start = str.find_first_not_of(" \t");
    size_t end = str.find_last_not_of(" \t");
    if (start == string::npos) {
        return "";  // No content
    }
    return str.substr(start, end - start + 1);
}

vector<string> custom_split(const string &s, char delimiter) {
    vector<string> result;
    stringstream ss(s);
    string item;
    while (getline(ss, item, delimiter)) {
        result.push_back(item);
    }
    return result;
}

bool search_directory(const string &dir, const string &target) {
    // Implement your directory search logic here
    // Return true if the file or folder is found, otherwise false
    return false;  // Placeholder return
}

void execute_command(const string &cmd, int in_fd, int out_fd) {
    vector<string> args;
    istringstream iss(cmd);
    string arg;
    while (iss >> arg) {
        args.push_back(arg);
    }

    // Check for custom commands
    if (args.empty()) {
        cerr << "Empty command" << endl;
        exit(EXIT_FAILURE);
    }

    string command = args[0];
    if (command == "exit") {
        if (in_fd != -1) close(in_fd);
        if (out_fd != -1) close(out_fd);
        cout << "Thank you for using the shell" << endl;
        exit(0);
    }
    else if (command == "search") {
        if (args.size() != 2) {
            cerr << "Usage: search <file_or_folder_name>" << endl;
            exit(EXIT_FAILURE);
        }
        string target = args[1];
        bool found = search_directory(".", target);
        cout << (found ? "True" : "False") << endl;
        exit(0);
    }

    // Redirect file descriptors
    if (in_fd != -1) {
        dup2(in_fd, STDIN_FILENO);
        close(in_fd);
    }
    if (out_fd != -1) {
        dup2(out_fd, STDOUT_FILENO);
        close(out_fd);
    }

    // Convert args to C-style array
    vector<char*> c_args;
    for (auto& arg : args) {
        c_args.push_back(const_cast<char*>(arg.c_str()));
    }
    c_args.push_back(nullptr);

    // Execute the command
    execvp(c_args[0], c_args.data());
    perror("execvp");
    exit(EXIT_FAILURE);
}

void execute_pipeline(const vector<string> &commands) {
    int num_pipes = commands.size() - 1;
    vector<int> pipe_fds(num_pipes * 2);

    // Create pipes
    for (int i = 0; i < num_pipes; ++i) {
        if (pipe(pipe_fds.data() + i * 2) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    for (size_t i = 0; i < commands.size(); ++i) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if (pid == 0) {
            // Child process

            int in_fd = (i == 0) ? -1 : pipe_fds[(i - 1) * 2];
            int out_fd = (i == num_pipes) ? -1 : pipe_fds[i * 2 + 1];

            execute_command(commands[i], in_fd, out_fd);

            // Close file descriptors
            if (in_fd != -1) close(in_fd);
            if (out_fd != -1) close(out_fd);

            exit(EXIT_SUCCESS);
        }
    }

    // Parent process
    for (size_t i = 0; i < commands.size(); ++i) {
        wait(nullptr);
    }

    // Close all pipe file descriptors
    for (auto fd : pipe_fds) {
        close(fd);
    }
}

int main() {
    // Example command with pipes
    string s = "cat shell.cpp | wc";

    // Split the commands
    vector<string> commands = custom_split(s, '|');
    for (auto& cmd : commands) {
        cmd = trim(cmd);  // Make sure to implement or use a trim function
    }

    // Execute the pipeline
    execute_pipeline(commands);

    return 0;
}
