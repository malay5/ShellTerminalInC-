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
#include <dirent.h>
#include <fstream> //used for pinfo only
#include <sys/wait.h>
#include <grp.h>
#include <pwd.h>

#include <iostream>
#include <string>
#include <vector>
#include <termios.h>
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


using namespace std;



class LsCommand {
public:
    void execute(const vector<string>& args,string current_working_dir) {

        // cout<<"EXECUTE MALAY.txt"<<endl<<"\n\n";
        string directory = ".";
        bool show_all = false;
        bool long_format = false;

        // Parse arguments
        for (const auto& arg : args) {
            cout<<" "<<arg<<" ";
            if (arg == "-a") {
                show_all = true;
            } else if (arg == "-l") {
                long_format = true;
            } else if (arg == "-al" || arg == "-la") {
                show_all = true;
                long_format = true;
            } else {
                directory = arg;
            }
        }
        cout<<endl;
        if(directory=="~"){
            directory=current_working_dir;
        }
        list_directory(directory, show_all, long_format);
    }

private:
    void list_directory(const string& directory, bool show_all, bool long_format) {
        cout<<"U\n";
        cout<<directory.c_str()<<"\n\n";
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


bool search_directory(const string &current_dir, const string &target) {
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

// Function to trim whitespace from the beginning and end of a string
string trim(const string &str) {
    size_t start = str.find_first_not_of(" \t");
    size_t end = str.find_last_not_of(" \t");
    if (start == string::npos) {
        return "";  // No content
    }
    return str.substr(start, end - start + 1);
}

// Function to split a string based on a delimiter

// Custom split function that handles quoted substrings
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
void executeWithPipesAndRedirection(vector<string> &commands, const string& current_working_dir) {
    int pipefd[2];
    int input_fd = 0;  // The input file descriptor for the first command

    for (size_t i = 0; i < commands.size(); ++i) {
        // Create a pipe if this is not the last command
        if (i < commands.size() - 1) {
            if (pipe(pipefd) == -1) {
                perror("pipe failed");
                exit(EXIT_FAILURE);
            }
        }

        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            dup2(input_fd, 0);  // Redirect input
            if (i < commands.size() - 1) {
                dup2(pipefd[1], 1);  // Redirect output to the pipe
            }
            close(pipefd[0]);  // Close the unused read end of the pipe

            vector<string> args = custom_split(commands[i], ' ');

            // Handle specific commands or pass to execvp
            if (!args.empty() && args[0] == "ls") {
                LsCommand lsCmd;
                args.erase(args.begin());
                lsCmd.execute(args, current_working_dir);
            } else if (!args.empty() && args[0] == "search") {
                string target = args[1];
                bool found = search_directory(current_working_dir, target);
                cout << (found ? "True" : "False") << endl;
            } else {
                // Execute other commands normally
                executeSingleCommand(args);
            }

            exit(EXIT_SUCCESS);  // Terminate child process
        } else if (pid > 0) {
            // Parent process
            wait(nullptr);  // Wait for the child process to finish
            close(pipefd[1]);  // Close the write end of the pipe
            input_fd = pipefd[0];  // Save the read end for the next command
        } else {
            perror("fork failed");
            exit(EXIT_FAILURE);
        }
    }
}

// Function to execute a full command (handle foreground, background, etc.)
void executeCommand(const string &command, const string& current_working_dir) {
    vector<string> piped_commands = custom_split(command, '|');

    bool is_background = false;
    if (!piped_commands.empty() && piped_commands.back().find('&') != string::npos) {
        is_background = true;
        piped_commands.back() = piped_commands.back().substr(0, piped_commands.back().size() - 1);
    }

    if (is_background) {
        pid_t pid = fork();
        if (pid == 0) {
            setpgid(0, 0);
            executeWithPipesAndRedirection(piped_commands, current_working_dir);            exit(EXIT_SUCCESS);
        } else if (pid > 0) {
            cout << "Background process PID: " << pid << endl;
        } else {
            perror("fork failed");
        }
    } else {
            executeWithPipesAndRedirection(piped_commands, current_working_dir);    }
}

int main() {
    // printPrompt();

    // bool

    // Hardcoded input with multiple commands separated by semicolons
    // string input = " ls | grep .txt > out.txt; cat < in.txt | wc -l > lines.txt";
    string input = "ls | grep .*txt";
    // string input = "ls > somtung.txt";
    // cout<<input<<endl;
    // Split the input based on semicolons to handle multiple commands
    vector<string> commands = custom_split(input, ';');

        char cwd[PATH_MAX];
            getcwd(cwd, sizeof(cwd));
                string current_working_dir = cwd;

    // Execute each command individually
    for (string &command : commands) {
        command = trim(command);  // Trim whitespace from each command
        executeCommand(command,current_working_dir);
    }
    cout<<endl<<endl;

    return 0;
}
