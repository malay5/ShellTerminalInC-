#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pwd.h>
#include <limits.h>
#include <vector>
#include <cstring>
#include <termios.h>

#include <iostream>

#include <regex>

#include <string>
#include <vector>
#include <termios.h>
#include <cstdlib>  // For exit()
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <grp.h>
#include <pwd.h>
#include <sstream> //Check what tf is this
#include <iomanip>
#include <fstream> //used for pinfo only
#include <sys/wait.h>
#include <map>

using namespace std;

const int MAX_PATH_LENGTH = 1024;  // Adjust size if needed
char main_wd[MAX_PATH_LENGTH];

char history_path[MAX_PATH_LENGTH];

map<pid_t, string> backgroundProcesses;  // To store background process PIDs and commands

void setWorkingDirectory() {
    if (getcwd(main_wd, sizeof(main_wd)) == nullptr) {
        std::cerr << "Error retrieving current working directory" << std::endl;
    }
    // Ensure there is a trailing slash in main_wd
    if (main_wd[strlen(main_wd) - 1] != '/') {
        strcat(main_wd, "/");
    }

    // Concatenate the file name to the path
    strcpy(history_path, main_wd);
    strcat(history_path, "history.logs");
}


// Function to read the file into a vector of strings
std::vector<std::string> load_history_from_file(const std::string& filename) {
    std::vector<std::string> history;
    int fd = open(filename.c_str(), O_RDONLY);
    if (fd == -1) {
        perror("Error opening file for reading");
        return history;
    }

    const size_t bufferSize = 1024;
    char buffer[bufferSize];
    ssize_t bytesRead;
    std::string currentLine;

    while ((bytesRead = read(fd, buffer, bufferSize)) > 0) {
        for (ssize_t i = 0; i < bytesRead; ++i) {
            if (buffer[i] == '\n') {
                history.push_back(currentLine);
                currentLine.clear();
            } else {
                currentLine += buffer[i];
            }
        }
    }
    if (!currentLine.empty()) {
        history.push_back(currentLine); // Add the last line if it wasn't followed by a newline
    }

    if (bytesRead == -1) {
        perror("Error reading file");
    }

    close(fd);
    return history;
}

class LsCommand {
public:
    void execute(const vector<string>& args, const string& current_working_dir) {
        vector<string> directories;
        bool show_all = false;
        bool long_format = false;

        // Parse arguments
        for (const auto& arg : args) {
            if (arg == "-a") {
                show_all = true;
            } else if (arg == "-l") {
                long_format = true;
            } else if (arg == "-al" || arg == "-la") {
                show_all = true;
                long_format = true;
            } else {
                directories.push_back(arg);
            }
        }

        // Use current directory if no directories are specified
        if (directories.empty()) {
            directories.push_back(".");
        }

        // Process each directory
        if(directories.size()==1){
         string directory = (directories[0] == "~") ? current_working_dir : directories[0];
            list_directory(directory, show_all, long_format);   
        }
        else{
        for (const auto& dir : directories) {
            string directory = (dir == "~") ? current_working_dir : dir;
            cout<<directory<<":\n";
            list_directory(directory, show_all, long_format);
        }}
    }

private:
    void list_directory(const string& directory, bool show_all, bool long_format) {
        DIR* dir = opendir(directory.c_str());
        if (!dir) {
            perror("opendir");
            return;
        }

        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (!show_all && entry->d_name[0] == '.') {
                continue;
            }

            if (long_format) {
                struct stat file_stat;
                string full_path = directory + "/" + entry->d_name;
                if (stat(full_path.c_str(), &file_stat) == 0) {
                    print_long_format(file_stat, entry->d_name);
                } else {
                    perror("stat");
                }
            } else {
                cout << entry->d_name <<"    ";
            }
        }
        cout<<endl;

        closedir(dir);
    }

    void print_long_format(const struct stat& file_stat, const string& name) {
        // Print file type and permissions
        cout << (S_ISDIR(file_stat.st_mode) ? 'd' : '-') 
             << ((file_stat.st_mode & S_IRUSR) ? 'r' : '-')
             << ((file_stat.st_mode & S_IWUSR) ? 'w' : '-')
             << ((file_stat.st_mode & S_IXUSR) ? 'x' : '-')
             << ((file_stat.st_mode & S_IRGRP) ? 'r' : '-')
             << ((file_stat.st_mode & S_IWGRP) ? 'w' : '-')
             << ((file_stat.st_mode & S_IXGRP) ? 'x' : '-')
             << ((file_stat.st_mode & S_IROTH) ? 'r' : '-')
             << ((file_stat.st_mode & S_IWOTH) ? 'w' : '-')
             << ((file_stat.st_mode & S_IXOTH) ? 'x' : '-') << ' ';

        // Print the number of hard links
        cout << file_stat.st_nlink << ' ';

        // Print the owner and group names
        struct passwd* pw = getpwuid(file_stat.st_uid);
        struct group* grp = getgrgid(file_stat.st_gid);
        cout << (pw ? pw->pw_name : "unknown") << ' '
             << (grp ? grp->gr_name : "unknown") << ' ';

        // Print the size of the file
        cout << setw(8) << file_stat.st_size << ' ';

        // Print the last modification time
        char timebuf[80];
        struct tm* timeinfo = localtime(&file_stat.st_mtime);
        strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", timeinfo);
        cout << timebuf << ' ';

        // Print the file name
        cout << name << endl;
    }
};

;

bool search_directory(const string &current_dir, const string &target) {
    // cout<<"I"<<endl;
    // cout<<current_dir.c_str();
    DIR *dir = opendir(current_dir.c_str());
    if (dir == nullptr) {
        perror("opendir");
        return false;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        string entry_name = entry->d_name;

        // Skip "." and ".."
        if (entry_name == "." || entry_name == "..") continue;

        string full_path = current_dir + "/" + entry_name;

        // Check if the entry matches the target
        if (entry_name == target) {
            closedir(dir);
            return true;
        }

        // If it's a directory, search recursively
        if (entry->d_type == DT_DIR) {
            if (search_directory(full_path, target)) {
                closedir(dir);
                return true;
            }
        }
    }

    closedir(dir);
    return false;
}


// Function to trim whitespace from the beginning and end of a string
string trim(const string &str) {
    size_t start = str.find_first_not_of(" \t");
    size_t end = str.find_last_not_of(" \t");
    if (start == string::npos) {
        return "";  // No content
    }
    return str.substr(start, end - start + 1);
}

class CdCommand {
private:
    vector<string> path;
    string last_dir;
    vector<string> last_path;
    bool inside_my_dir;

    void updateWorkingDirectory() {
        char prev_working_directory[8000];
        if (getcwd(prev_working_directory, sizeof(prev_working_directory)) != NULL) {
            last_dir = prev_working_directory;
        } else {
            perror("getcwd() error");
        }
    }

public:
    CdCommand() : inside_my_dir(true) {
        path.push_back("~");
    }

    void execute(const string& s) {
        if (s.rfind("cd ", 0) != 0) return;

        string move_to = s.substr(3);
        trim(move_to);

        if (move_to == "~") {
            updateWorkingDirectory();
            if (chdir(getenv("HOME")) == 0) {
                last_path = path;
                path = {"~"};
            } else {
                cout << "ERROR: Could not change directory" << endl;
            }
        } else if (move_to == ".") {
            updateWorkingDirectory();
            last_path = path;
        } else if (move_to == "..") {
            if (inside_my_dir) {
                if (path.size() == 1) {
                    inside_my_dir = false;
                } else {
                    updateWorkingDirectory();
                    if (chdir("..") == 0) {
                        path.pop_back();
                        last_path = path;
                    } else {
                        cout << "ERROR: Could not change directory" << endl;
                    }
                }
            }
        } else if (move_to == "-") {
            if (inside_my_dir) {
                swap(path, last_path);
                string temp = last_dir;
                if (chdir(last_dir.c_str()) == 0) {
                    last_dir = temp;
                } else {
                    cout << "ERROR: Could not change directory" << endl;
                }
            }
        } else {
            if (chdir(move_to.c_str()) == 0) {
                path.push_back(move_to);
            } else {
                cout << "ERROR: Could not change directory" << endl;
            }
        }
    }
};
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



// Function to split a string based on a delimiter
vector<string> custom_split_basic(const string &str, char delimiter = ' ') {
    vector<string> result;
    string token;
    
    // Loop through each character in the string
    for (char ch : str) {
        if (ch == delimiter) {
            if (!token.empty()) {
                result.push_back(token);  // Add the current token to the result
                token.clear();            // Clear the token for the next word
            }
        } else {
            token += ch;  // Append character to token
        }
    }
    if (!token.empty()) {
        result.push_back(token);  // Add the last token
    }

    return result;
}

// Advanced Custom split function that handles quoted substrings
vector<string> custom_split(const string& str, char delimiter = ' ') {
    vector<string> result;
    string token;
    bool inside_quotes = false;

    for (char ch : str) {
        if (ch == '"') {
            inside_quotes = !inside_quotes;
            token += ch;
        } else if (ch == delimiter) {
            if (inside_quotes) {
                token += ch;
            } else {
                if (!token.empty()) {
                    result.push_back(token);
                    token.clear();
                }
            }
        } else {
            token += ch;
        }
    }
    if (!token.empty()) {
        result.push_back(token);
    }

    return result;
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

            vector<string> args = custom_split(commands[i], ' ');
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
void executeCommand(const string &command) {
    // Split the command by '|' to handle pipes
    vector<string> piped_commands = custom_split(command, '|');

    // Check for background execution ('&')
    bool is_background = false;
    if (!piped_commands.empty() && piped_commands.back().find('&') != string::npos) {
        is_background = true;
        piped_commands.back() = piped_commands.back().substr(0, piped_commands.back().size() - 1);
    }

    if (is_background) {
        pid_t pid = fork();
        // cout<<pid;
        if (pid == 0) {  // Child process
            setpgid(0, 0);  // Create a new process group
            executeWithPipesAndRedirection(piped_commands);
            exit(EXIT_SUCCESS);
        } else if (pid > 0) {  // Parent process
            cout << "Background process PID: " << pid+1 << endl;
            backgroundProcesses[pid+1] = command;  // Store the background process
        } else {
            perror("fork failed");
        }
    } else {
        executeWithPipesAndRedirection(piped_commands);
    }
}


// Function to bring a background process to the foreground
void bringToForeground(pid_t pid) {
    if (backgroundProcesses.find(pid) != backgroundProcesses.end()) {
        int status;
        tcsetpgrp(STDIN_FILENO, pid);  // Set terminal control to the background process
        waitpid(pid, &status, WUNTRACED);  // Wait for the process to finish or stop
        tcsetpgrp(STDIN_FILENO, getpgid(0));  // Return terminal control to the shell
        if (WIFSTOPPED(status)) {
            kill(pid, SIGCONT);  // Continue the process if it was stopped
        }
        backgroundProcesses.erase(pid);  // Remove from background list
    } else {
        cout << "No such background process with PID: " << pid << endl;
    }
}

void reset_terminal_mode() {
    struct termios tty;
    if (tcgetattr(STDIN_FILENO, &tty) != 0) {
        perror("tcgetattr");
        exit(EXIT_FAILURE);
    }
    tty.c_lflag |= (ICANON | ECHO); // Enable canonical mode and echo
    if (tcsetattr(STDIN_FILENO, TCSANOW, &tty) != 0) {
        perror("tcsetattr");
        exit(EXIT_FAILURE);
    }
}



void set_terminal_mode() {
    struct termios tty;
    if (tcgetattr(STDIN_FILENO, &tty) != 0) {
        perror("tcgetattr");
        exit(EXIT_FAILURE);
    }
    tty.c_lflag &= ~(ICANON | ECHO); // Disable canonical mode and echo
    if (tcsetattr(STDIN_FILENO, TCSANOW, &tty) != 0) {
        perror("tcsetattr");
        exit(EXIT_FAILURE);
    }
}


string process_echo(const string& s) {
    string result;
    bool inside_single_quote = false;
    bool inside_double_quote = false;
    bool escape_character_present = false;

    for (char c : s) {
        if (escape_character_present) {
            // Handle escape sequences
            if (c == '\\' || c == '\'' || c == '\"') {
                result += c;  // Add escaped character
            } else {
                result += '\\';  // Add backslash if not an escape sequence
                result += c;     // Add the current character
            }
            escape_character_present = false;
        } else if (c == '\\') {
            // Set escape character flag
            escape_character_present = true;
        } else if (c == '\'') {
            // Toggle single quote status if not escaped
            if (!escape_character_present) {
                inside_single_quote = !inside_single_quote;
            } else {
                result += c;  // Include escaped single quote
            }
        } else if (c == '\"') {
            // Toggle double quote status if not escaped
            if (!escape_character_present) {
                inside_double_quote = !inside_double_quote;
            } else {
                result += c;  // Include escaped double quote
            }
        } else {
            if (c == ' ' && !inside_single_quote && !inside_double_quote) {
                // Replace multiple spaces with a single space when outside quotes
                if (!result.empty() && result.back() != ' ') {
                    result += c;
                }
            } else {
                result += c;
            }
        }
    }

    // Handle cases where quotes are mismatched or unclosed
    if (inside_single_quote || inside_double_quote) {
        cerr << "Warning: Unmatched quotes in input." << endl;
        return "";
    }

    return result;
}
void print_prompt(const string &user, const string &hostname, const vector<string> &path, const string &current_command, size_t prev_command_length) {
    string prompt = user + "@" + hostname + ":";
    bool is_root_dir = true;
    for (const auto &dir : path) {
        if (is_root_dir) {
            prompt += dir;
            is_root_dir = false;
        } else {
            prompt += "/" + dir;
        }
    }
    prompt += "> " + current_command;

    // Move cursor to the beginning of the line
    cout << "\r" << prompt;

        size_t max_length = max(prev_command_length, current_command.length());
            // cout << string(max_length - current_command.length(), ' ');

    // Clear any remaining characters from the previous command
    // size_t extra_spaces =100;// prev_command_length;//prev_command_length > current_command.length() ? prev_command_length - current_command.length() : 0;;
    // for(int i=0;i<100;i++){
    //     cout<<" ";
    // }

        size_t extra_spaces = prev_command_length > current_command.length() ? prev_command_length - current_command.length() : 0;;//100;

    cout << string(extra_spaces, ' ') ;
    cout<< "\r" << prompt;

    fflush(stdout);
}

void add_line(const std::string& line) {
    const char* filename = history_path;
    const size_t maxLines = 20;

    // Open the file with append mode
    int fd = open(filename, O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (fd == -1) {
        perror("Error opening file for appending");
        return;
    }

    // Write the new line to the file
    std::string formattedLine = line + "\n";
    if (write(fd, formattedLine.c_str(), formattedLine.size()) == -1) {
        perror("Error writing to file");
        close(fd);
        return;
    }

    // Close the file descriptor
    close(fd);

    // Reopen the file for reading and writing
    fd = open(filename, O_RDWR);
    if (fd == -1) {
        perror("Error reopening file for reading");
        return;
    }

    // Read all lines from the file
    std::vector<std::string> lines;
    char buffer[1024];
    ssize_t bytesRead;
    std::string currentLine;

    while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0) {
        for (ssize_t i = 0; i < bytesRead; ++i) {
            if (buffer[i] == '\n') {
                lines.push_back(currentLine);
                currentLine.clear();
            } else {
                currentLine += buffer[i];
            }
        }
    }
    if (!currentLine.empty()) {
        lines.push_back(currentLine); // Add the last line if it wasn't followed by a newline
    }

    if (bytesRead == -1) {
        perror("Error reading file");
        close(fd);
        return;
    }

    // Keep only the last `maxLines` lines
    if (lines.size() > maxLines) {
        lines.erase(lines.begin(), lines.begin() + (lines.size() - maxLines));
    }

    // Truncate the file and write the remaining lines
    if (ftruncate(fd, 0) == -1) {
        perror("Error truncating file");
        close(fd);
        return;
    }

    lseek(fd, 0, SEEK_SET); // Move the file offset to the beginning

    for (const auto& line : lines) {
        std::string lineWithNewline = line + "\n";
        if (write(fd, lineWithNewline.c_str(), lineWithNewline.size()) == -1) {
            perror("Error writing to file");
            close(fd);
            return;
        }
    }

    // Close the file descriptor
    close(fd);
}


string display_process_info(const string& pid) {
    string proc_dir = "/proc/" + pid;  // e.g., "/proc/1234"
    string status_file = proc_dir + "/status";  // e.g., "/proc/1234/status"
    string cmdline_file = proc_dir + "/cmdline";  // e.g., "/proc/1234/cmdline"
    string exe_link = proc_dir+"/exe";

    ifstream status_stream(status_file);
    ifstream cmdline_stream(cmdline_file);

    if (!status_stream.is_open() || !cmdline_stream.is_open()) {
        std::cerr << "ERROR: Could not open the process information files for PID: " << pid << std::endl;
        return "ERROR: Could not open the process information files for PID: ";
    }

    // Print process status
    string line;
    string process_status;
    string memory;
    string output="";

    while (getline(status_stream, line)) {
        if (line.find("State:") == 0) {
            process_status = line.substr(line.find_first_of(" \t") + 1, 1);  // e.g., "R" for running
        } else if (line.find("VmSize:") == 0) {
            memory = line.substr(line.find_first_of(" \t") + 1);  // e.g., "67854 kB"
        }
    }
    char exe_path[1024];
    ssize_t len = readlink(exe_link.c_str(), exe_path, sizeof(exe_path) - 1);

    if (len != -1) {
        exe_path[len] = '\0';  // Ensure null-terminated string
    } else {
        std::cerr << "ERROR: Could not determine the executable path for PID: " << pid << std::endl;
        output = "ERROR: Could not determine the executable path for PID: " + pid + "\n";
        return output;
    }

    

        // Display information
    std::cout << "pid -- " << pid << std::endl;
    std::cout << "Process Status -- " << process_status << std::endl;
    std::cout << "memory -- " << memory << std::endl;
    std::cout << "Executable Path -- " << exe_path << std::endl;

     output += "pid -- " + pid + "\n";
        output += "Process Status -- " + process_status + "\n";
        output += "memory -- " + memory + "\n";
        output += "Executable Path -- " + std::string(exe_path) + "\n";
    // // Print command line
    // cout << "Command: ";
    // while (getline(cmdline_stream, line, '\0')) {
    //     cout << line << " ";  // Output the command that started the process
    // }
    // cout << endl;

    status_stream.close();
    cmdline_stream.close();

    return output;
}

void print_history(int num_lines = -1) {
    const char* filename = history_path;
    
    // Open the file with read-only permission
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return;
    }

    const size_t bufferSize = 1024;
    char buffer[bufferSize];
    ssize_t bytesRead;

// Use a vector to store all lines
    std::vector<std::string> lines;
    std::string currentLine;

    // Read from the file and store lines
    while ((bytesRead = read(fd, buffer, bufferSize)) > 0) {
        for (ssize_t i = 0; i < bytesRead; ++i) {
            if (buffer[i] == '\n') {
                lines.push_back(currentLine);
                currentLine.clear();
            } else {
                currentLine += buffer[i];
            }
        }
    }

    // Add the last line if there's no newline at the end
    if (!currentLine.empty()) {
        lines.push_back(currentLine);
    }

    if (bytesRead == -1) {
        perror("Error reading file");
    }

    // Close the file descriptor
    close(fd);

    // Print the last 'num_lines' lines if specified, else print all lines
    if (num_lines < 0 || num_lines > lines.size()) {
        num_lines = lines.size();
    }

    for (int i = lines.size() - num_lines; i < lines.size(); ++i) {
        std::cout << lines[i] << std::endl;
    }
}


void sighup_handler(int signum) {
    // Handle SIGHUP
    cout<<"J";
    printf("Received SIGHUP\n");

}

void sigint_handler(int signum) {
    // Handle SIGHUP
    cout<<"J";
    printf("Received SIGINT\n");
}

void sigtstp_handler(int signum) {
    // Handle SIGHUP
    cout<<"J\n";
    printf("Received SssssssssssssssssssssssssIGTSTP\n");
}

void setup_signal_handlers() {
    // signal(SIGTSTP, sigtstp_handler); // Handle CTRL-Z
    // signal(SIGINT, sigint_handler);   // Handle CTRL-C
    // signal(SIGHUP, sighup_handler);   // Handle CTRL-D
}

vector<string> autocompleteCommands = {"ls", "cd", "mkdir", "rm", "cp", "mv", "touch", "cat", "echo", "exit","search","pinfo","history"};

// Function to get autocomplete suggestions for commands
vector<string> get_command_suggestions(const string &prefix) {
    vector<string> suggestions;
    for (const string &cmd : autocompleteCommands) {
        if (cmd.find(prefix) == 0) {
            suggestions.push_back(cmd);
        }
    }
    return suggestions;
}

// Function to get autocomplete suggestions for files
vector<string> get_file_suggestions(const string &prefix) {
    // cout<<"PREFIX:"<<prefix<<endl;
    vector<string> suggestions;
    DIR *dir;
    struct dirent *entry;

    if ((dir = opendir(".")) != nullptr) {
        while ((entry = readdir(dir)) != nullptr) {
            string filename = entry->d_name;
            if(filename=="." || filename==".."){
                continue;
            }
            // Check if the file name starts with the prefix
            if (filename.find(prefix) == 0) {
                struct stat statbuf;
                if (stat(filename.c_str(), &statbuf) == 0) {
                    // Check if it's a directory
                    if (S_ISDIR(statbuf.st_mode)) {
                        filename += "/"; // Append / for directories
                    }
                }
                suggestions.push_back(filename);
            }
        }
        closedir(dir);
    } else {
        perror("opendir");
    }
    
    return suggestions;
}


// Function to get the last segment before the last delimiter
string get_last_segment(const string &input) {
    size_t pos = input.find_last_of(" ;|");
    if (pos == string::npos) return input;
    return input.substr(pos + 1);
}


// Function to split the input string by delimiters
vector<string> split_by_delimiters(const string &input) {
    vector<string> parts;
    size_t start = 0;
    size_t end = input.find_first_of(" ;|");

    while (end != string::npos) {
        if (end > start) {
            parts.push_back(input.substr(start, end - start));
        }
        parts.push_back(string(1, input[end]));  // Add delimiter
        start = end + 1;
        end = input.find_first_of(" ;|", start);
    }
    if (start < input.length()) {
        parts.push_back(input.substr(start));
    }

    return parts;
}

// Function to check if a substring is the first word in the input
bool is_first_part(const string &input) {
    size_t start = 0;
    size_t end = input.find(' ');

    // Extract the first part of the input before the first space
    string first_part = (end == string::npos) ? input : input.substr(start, end - start);

    // Check if the input string matches the first part or is a prefix of the first part
    return (input == first_part || first_part.find(input) == 0);
}


int main() {
    signal(SIGHUP, sighup_handler);
    set_terminal_mode();
    atexit(reset_terminal_mode);

    setWorkingDirectory();

    // setup_signal_handlers(); // Set up signal handlers

        CdCommand cdCommand;

        LsCommand lsCommand;
        vector<string> last_path={"~"};

        char current_working_directory[8000];

if (getcwd(current_working_directory, sizeof(current_working_directory)) != NULL) {
            // std::cout << "Current working directory: " << current_working_directory << std::endl;
        } else {
            cerr << "getcwd() error";
            return 1;
        }

        const char* filename = history_path;
        int fd = open(filename, O_CREAT | O_WRONLY | O_APPEND, 0644);
    if (fd == -1) {
        perror("Error creating/opening file");
        return 1;
    }

    // Close the file descriptor immediately, as we just needed to create the file
    close(fd);
    
        vector<string> history=load_history_from_file(filename);;
        size_t history_index = history.size();
            bool inside_my_dir=true;
                

                    size_t prev_command_length = 0;


char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        perror("gethostname");
        return 1;
    }
             string user = getlogin();
        vector<string> path = {"~"};


    char* last_dir=current_working_directory;




    while (true) {
                string s;
        char ch;
        


        print_prompt(user, hostname, path, s, prev_command_length);

        while (true) {
            
            // signal(SIGINT, sigint_handler);   // Handle CTRL-C
            // signal(SIGTSTP, sigtstp_handler); // Handle CTRsignal(SIGHUP, sighup_handler);L-Z

            ssize_t bytesRead = read(STDIN_FILENO, &ch, 1);
            if (bytesRead <= 0) break;

            if (ch == 10) { // Enter key
                cout << endl;
                break;
            }   
            else if (ch == 127) { // Backspace
                if (!s.empty()) {
                    s.pop_back();
                    print_prompt(user, hostname, path, s, s.size()+1);    
                }
                
                
            }
             else if (ch == 27) { // Escape sequence (arrow keys)
                read(STDIN_FILENO, &ch, 1); // Skip [
                if (ch == '[') {
                    read(STDIN_FILENO, &ch, 1); // Get the final character

                    if (ch == 'A') { // Up arrow
                        if (history_index > 0) {
                            history_index--;
                            prev_command_length=s.length();
                            s = history[history_index];
                        }
                    } else if (ch == 'B') { // Down arrow
                        if (history_index < history.size() - 1) {
                            history_index++;
                            prev_command_length=s.length();

                            s = history[history_index];
                        } else if (history_index == history.size() - 1) {
                            history_index++;
                            prev_command_length=s.length();
                            s = "";
                        }
                    }
                    print_prompt(user, hostname, path, s, prev_command_length);
                }
            } 
            else if (ch == 9) { // Tab key
                vector<string> parts = split_by_delimiters(s);
                string last_segment = get_last_segment(s);
                vector<string> suggestions;
                // cout<<"\n";
                // cout<<last_segment<<"\n\n";
                

                // if (parts.empty() || parts.back().empty() || parts.back() == " " || parts.back() == ";" || parts.back() == "|") {
                    // Context is command or file after delimiter
                    // if (parts.empty() || parts.back() == " " || parts.back() == ";" || parts.back() == "|") {
                    if(is_first_part(s)){
                        suggestions = get_command_suggestions(last_segment);

                        if (suggestions.size() == 1) {
                            s = s.substr(0, s.length() - last_segment.length()) + suggestions[0];
                        } else {
                            // Display suggestions (if multiple)
                            // For simplicity, just add the first suggestion
                            // if (!suggestions.empty()) {
                            //     s = s.substr(0, s.length() - last_segment.length()) + suggestions[0];
                            // }
                            cout<<"\n";
                            for (const string& suggestion : suggestions) {
                                cout << suggestion << "\t";
                            }
                            cout<<"\n";
                        }
                    } else {//if (parts.back() == " " || parts.back() == ";" || parts.back() == "|") {
                        // Context is file
                        // cout<<"\n"<<"X"<<"\n";
                        suggestions = get_file_suggestions(last_segment);
                        if (suggestions.size() == 1) {
                            s = s.substr(0, s.length() - last_segment.length()) + suggestions[0];
                        } else {
                            // Display suggestions (if multiple)
                            // For simplicity, just add the first suggestion
                            cout<<"\n";
                            for (const string& suggestion : suggestions) {
                                cout << suggestion << "\t";
                            }
                            cout<<"\n";
                        }
                    }
                // }
                
                print_prompt(user, hostname, path, s, prev_command_length);
            }
            else if(ch==4){  //ctrl + d implemented
                
                exit(0);
            }
            else {
                s += ch;
                
                print_prompt(user, hostname, path, s, prev_command_length);
            }
        }

        s = trim(s);
        if (s.empty()) {
        continue;}
        else{
                add_line(s);
        }
        s = trim(s);

        // string s = input;
            std::smatch matches;

        if (!s.empty()) {
            history.push_back(s);
            history_index = history.size();
        }
        else{
            // executeCommand(s);
        }

        prev_command_length = s.length(); // Update the length of the last command
                vector<string> s_commands = custom_split(s, ';');
        for (const auto& command : s_commands) {
            string s = trim(command);
            if (std::regex_search(s, std::regex("[|><]"))) {
                executeCommand(s);
            }
            else if (s == "exit") {
                cout << "Thank you for using the shell" << endl;
                return 0;
            }
            else if (std::regex_match(s, std::regex("^cd\\s*(.*)?$"))) {
                if(s=="cd"){
                    char prev_working_directory[8000];
                        if (getcwd(prev_working_directory, sizeof(prev_working_directory)) != NULL) {
                                // std::cout << "Current working directory: " << current_working_directory << std::endl;
                            } else {
                                perror("getcwd() error");
                                return 1;
                            }
                        last_dir=prev_working_directory;
                
                if (chdir(current_working_directory)==0){
                    last_path=path;
                    path = {"~"};
                    
                }
                else{
                    cout << "ERROR: Could not change directory" << endl;
                }
            
                }
                string move_to = s.substr(3);
                if (move_to == "~") {

                char prev_working_directory[8000];
                        if (getcwd(prev_working_directory, sizeof(prev_working_directory)) != NULL) {
                                // std::cout << "Current working directory: " << current_working_directory << std::endl;
                            } else {
                                perror("getcwd() error");
                                return 1;
                            }
                        last_dir=prev_working_directory;
                
                if (chdir(current_working_directory)==0){
                    last_path=path;
                    path = {"~"};
                    
                }
                else{
                    cout << "ERROR: Could not change directory" << endl;
                }
            }
                else if(move_to == "."){
                // Do nothing
                char prev_working_directory[8000];
                        if (getcwd(prev_working_directory, sizeof(prev_working_directory)) != NULL) {
                                // std::cout << "Current working directory: " << current_working_directory << std::endl;
                            } else {
                                perror("getcwd() error");
                                return 1;
                            }
                        last_dir=prev_working_directory;
                        last_path=path;
                
            }
            else if(move_to == ".."){
                if(inside_my_dir){
                    if(path.size()==1){
                        inside_my_dir=false;
                    }
                    else{
                        char prev_working_directory[8000];
                        if (getcwd(prev_working_directory, sizeof(prev_working_directory)) != NULL) {
                                // std::cout << "Current working directory: " << current_working_directory << std::endl;
                            } else {
                                perror("getcwd() error");
                                return 1;
                            }
                        if (chdir("..")==0){
                            path.pop_back();
                            
                            
                            last_dir=prev_working_directory;
                            last_path=path;
                        }
                        else{
                            cout << "ERROR: Could not change directory" << endl;
                        }

                        


                    }
                }else{

                }
            }
            else if(move_to == "-"){
                if(inside_my_dir){
                    last_path,path=path,last_path;//To check, as this doesn't work

                    char prev_working_directory[8000];
    if (getcwd(prev_working_directory, sizeof(prev_working_directory)) != NULL) {
        // std::cout << "Current working directory: " << current_working_directory << std::endl;
    } else {
        perror("getcwd() error");
        return 1;
    }

                    char* temp=prev_working_directory;
                    if (chdir(last_dir)==0){
                        // path = {"~"}
                        last_dir=temp;
                    }
                    else{
                        cout << "ERROR: Could not change directory" << endl;
                    }
                }
                else{

                }
            }
            else {
                trim(move_to);
                if (chdir(move_to.c_str()) == 0) {
                    path.push_back(move_to);
                } else {
                    cout << "ERROR: Could not change directory" << endl;
                }
            }
            continue;
            }
            else if (std::regex_match(s, std::regex("^echo\\s*(.*)?$"))) {
                string echo_txt = s.substr(5);
                echo_txt = trim(echo_txt);
                string process_echo_txt = process_echo(echo_txt);
                cout << process_echo_txt << endl;
                continue;
            }
            else if (std::regex_match(s, std::regex("^pinfo\\s*(.*)?$"))) {
                istringstream iss(s);
            vector<string> tokens;
            string token;

            while (iss>>token){
                tokens.push_back(token);
            }

            if (tokens.size() == 1) {
                string pid = std::to_string(getpid());  // Current process PID, e.g., "5678"
                display_process_info(pid);
            } else if (tokens.size() == 2) {
                string pid = tokens[1];  // pid = "1234"
                display_process_info(pid);
            } else {
                cerr << "ERROR: Invalid arguments for pinfo command" << std::endl;
            }
            // handle_pinfo_command(command);
            continue;
            }
            else if (std::regex_match(s, std::regex("^pwd\\s*(.*)?$"))) {
        cout << "PWD\n";
        char current_working_directory[8000];
        if (getcwd(current_working_directory, sizeof(current_working_directory)) != NULL) {
            // std::cout << "Current working directory: " << current_working_directory << std::endl;
        } else {
            cerr << "getcwd() error";
            return 1;
        }
    }
            else if (std::regex_match(s, std::regex("^ls\\s*(.*)?$"))) {
                // std::vector<std::string> ls_args;
                std::istringstream iss(s);
                std::string word;

                vector<string> ls_args = custom_split(s, ' ');
                // cout<<"LS\n\n";
                // for(auto x:ls_args){
                //     cout<<x<<"\n";
                // }
                // cout<<"\n\n";
                ls_args.erase(ls_args.begin());  // Remove the "ls" command part
                lsCommand.execute(ls_args,current_working_directory);
                // cout<<"XXXXXXXXXx";
                continue;
                
            }
            else if (std::regex_match(s, std::regex("^search\\s*(.*)?$"))) {
                
                // cout<<"SEARCH";
                vector<string> search_args;
            istringstream iss(s);
            string word;
            while (iss >> word) {
                search_args.push_back(word);
            }

            if (search_args.size() != 2) {
                cout << "Usage: search <file_or_folder_name>" << endl;
            } else {
                string target = search_args[1];
                char current_working_directory[8000];
                if (getcwd(current_working_directory, sizeof(current_working_directory)) != NULL) {
                    // std::cout << "Current working directory: " << current_working_directory << std::endl;
                } else {
                    cerr << "getcwd() error";
                    return 1;
                }
                bool found = search_directory(current_working_directory, target);
                cout << (found ? "True" : "False") << endl;
            }
            }
            else if (std::regex_match(s,matches, std::regex("^history\\s*(\\d*)?$"))) {
                int num_lines = -1;
                if (matches[1].length() > 0) {
                    std::istringstream iss(matches[1]);
                    iss >> num_lines;
                }
                print_history(num_lines);            }
            else{
                executeCommand(s);
            }


        }


        // Execute the hardcoded command
        // executeCommand(input);
    }
    return 0;
}
