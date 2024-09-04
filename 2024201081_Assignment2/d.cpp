#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstring>
#include <pwd.h>
#include <limits.h>
#include <vector>
#include <sstream>

using namespace std;

// Function to print the custom shell prompt
void printPrompt() {
    char hostname[HOST_NAME_MAX];
    char cwd[PATH_MAX];
    gethostname(hostname, sizeof(hostname));
    getcwd(cwd, sizeof(cwd));

    struct passwd *pw = getpwuid(getuid());
    const char *user = pw->pw_name;

    cout << user << "@" << hostname << ":" << cwd << "> ";
}

// Function to parse commands separated by ';'
vector<string> parseCommands(char *input) {
    vector<string> commands;
    char *token = strtok(input, ";");
    while (token != nullptr) {
        commands.push_back(string(token));
        token = strtok(nullptr, ";");
    }
    return commands;
}

// Function to parse a command into tokens based on delimiters
vector<string> tokenize(const string &command, const char *delimiters) {
    vector<string> tokens;
    char *cmd = strdup(command.c_str());
    char *token = strtok(cmd, delimiters);
    while (token != nullptr) {
        tokens.push_back(string(token));
        token = strtok(nullptr, delimiters);
    }
    free(cmd);
    return tokens;
}

// Function to execute a single command with possible I/O redirection
void executeSingleCommand(vector<string> &args) {
    int input_fd = 0, output_fd = 1;

    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "<") {
            input_fd = open(args[i + 1].c_str(), O_RDONLY);
            if (input_fd < 0) {
                perror("Error opening input file");
                exit(EXIT_FAILURE);
            }
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
            args.erase(args.begin() + i, args.begin() + i + 2);
            i--;
        } else if (args[i] == ">") {
            output_fd = open(args[i + 1].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (output_fd < 0) {
                perror("Error opening output file");
                exit(EXIT_FAILURE);
            }
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
            args.erase(args.begin() + i, args.begin() + i + 2);
            i--;
        } else if (args[i] == ">>") {
            output_fd = open(args[i + 1].c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (output_fd < 0) {
                perror("Error opening output file");
                exit(EXIT_FAILURE);
            }
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
            args.erase(args.begin() + i, args.begin() + i + 2);
            i--;
        }
    }

    // Convert args to char* array for execvp
    vector<char *> exec_args;
    for (string &arg : args) {
        exec_args.push_back(&arg[0]);
    }
    exec_args.push_back(nullptr);

    execvp(exec_args[0], exec_args.data());
    perror("execvp failed");
    exit(EXIT_FAILURE);
}

// Function to execute a command with pipes and redirection
void executeWithPipesAndRedirection(vector<string> &commands) {
    int pipefd[2];
    int input_fd = 0;

    for (size_t i = 0; i < commands.size(); ++i) {
        pipe(pipefd);

        pid_t pid = fork();
        if (pid == 0) {  // Child process
            dup2(input_fd, 0);  // Redirect input from previous command
            if (i < commands.size() - 1) {
                dup2(pipefd[1], 1);  // Redirect output to the pipe
            }
            close(pipefd[0]);

            vector<string> args = tokenize(commands[i], " \t");
            executeSingleCommand(args);
        } else if (pid > 0) {  // Parent process
            wait(nullptr);  // Wait for the child to finish
            close(pipefd[1]);
            input_fd = pipefd[0];  // Set input for the next command
        } else {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }
    }
}

// Function to execute a full command (handle foreground, background, etc.)
void executeCommand(string command) {
    // Split the command by '|' to handle pipes
    vector<string> piped_commands = tokenize(command, "|");

    // Check for background execution ('&')
    bool is_background = false;
    if (!piped_commands.empty() && piped_commands.back().find('&') != string::npos) {
        is_background = true;
        piped_commands.back() = piped_commands.back().substr(0, piped_commands.back().size() - 1);
    }

    if (is_background) {
        pid_t pid = fork();
        if (pid == 0) {  // Child process
            setpgid(0, 0);  // Create a new process group
            executeWithPipesAndRedirection(piped_commands);
            exit(EXIT_SUCCESS);
        } else if (pid > 0) {  // Parent process
            cout << "Background process PID: " << pid << endl;
        } else {
            perror("fork failed");
        }
    } else {
        executeWithPipesAndRedirection(piped_commands);
    }
}

int main() {
    while (true) {
        printPrompt();

        string input;
        getline(cin, input);

        if (input.empty()) continue;

        char *cstr = new char[input.length() + 1];
        strcpy(cstr, input.c_str());

        vector<string> commands = parseCommands(cstr);
        for (string &command : commands) {
            executeCommand(command);
        }

        delete[] cstr;
    }
    return 0;
}
