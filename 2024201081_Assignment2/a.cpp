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

using namespace std;




































class LsCommand {
public:
    void execute(const vector<string>& args,string current_working_dir) {
        string directory = ".";
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
                directory = arg;
            }
        }
        if(directory=="~"){
            directory=current_working_dir;
        }
        list_directory(directory, show_all, long_format);
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


string trim(const string &str) {
    size_t start = str.find_first_not_of(" \t");
    size_t end = str.find_last_not_of(" \t");
    if (start == string::npos) {
        return "";  // No content
    }
    return str.substr(start, end - start + 1);
}

vector<string> custom_split(const string& str, char delimiter = ' ') {
    vector<string> result;
    string token;
    
    // Loop through each character in the string
    for (char ch : str) {
        if (ch == delimiter) {
            result.push_back(token);  // Add the current token to the result
            token.clear();            // Clear the token for the next word
        } else {
            token += ch;  // Append character to token
        }
    }
    result.push_back(token);  // Add the last token

    return result;
}

string process_echo(string s) {
    bool escape_character_present = false;
    string res = "";
    for (auto c : s) {
        if (escape_character_present) {
            
            if(c=='\\'){
                res+='\\';
            }
            else if(c=='\''){
                res+='\'';
            }
            else if(c=='\"'){
                res+='\"';
            }
             else {
                res += c;
            }
            escape_character_present = false;
        } else if (c == '\\') {
            escape_character_present = true;
        }
        else if(c=='\'' || c=='\"'){
            escape_character_present=false;
        }
         else {
            res += c;
        }
    }
    return res;
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

    // Clear any remaining characters from the previous command
    size_t extra_spaces = prev_command_length > current_command.length() ? prev_command_length - current_command.length() : 0;;//100;
    cout << string(extra_spaces, ' ') << "\r" << prompt;

    fflush(stdout);
}

int main() {
    set_terminal_mode();
    atexit(reset_terminal_mode);

    LsCommand ls_command;
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        perror("gethostname");
        return 1;
    }

    bool inside_my_dir=true;

    char current_working_directory[8000];
    if (getcwd(current_working_directory, sizeof(current_working_directory)) != NULL) {
        std::cout << "Current working directory: " << current_working_directory << std::endl;
    } else {
        perror("getcwd() error");
        return 1;
    }

    string user = getenv("USER");
    size_t history_index = 0;
    vector<string> history;
    vector<string> path = {"~"};

    size_t prev_command_length = 0;

    char* last_dir=current_working_directory;
    vector<string> last_path={"~"};

    while (true) {
        string s;
        char ch;
        print_prompt(user, hostname, path, s, prev_command_length);

        while (true) {
            ssize_t bytesRead = read(STDIN_FILENO, &ch, 1);
            if (bytesRead <= 0) break;

            if (ch == 10) { // Enter key
                cout << endl;
                break;
            } 
            else if (ch == 127) { // Backspace
                if (!s.empty()) {
                    s.pop_back();
                }
                print_prompt(user, hostname, path, s, prev_command_length);
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
                            s = "";
                        }
                    }
                    print_prompt(user, hostname, path, s, prev_command_length);
                }
            } 
            else {
                s += ch;
                print_prompt(user, hostname, path, s, prev_command_length);
            }
        }

        trim(s);
        if (!s.empty()) {
            history.push_back(s);
            history_index = history.size();
        }

        prev_command_length = s.length(); // Update the length of the last command

        vector<string> commands = custom_split(s, ';');
        // Execute each command
        for (const auto& command : commands) {
            string s = trim(command);

        if (s == "exit") {
            cout << "Thank you for using the shell" << endl;
            return 0;
        }

        if (s.rfind("cd ", 0) == 0) {
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
        } else if (s.rfind("echo ", 0) == 0) {
            string echo_txt = s.substr(5);
            echo_txt = trim(echo_txt);
            string process_echo_txt = process_echo(echo_txt);
            cout << process_echo_txt << endl;
            continue;
        }
        else if (s.rfind("ls", 0) == 0) {

            // TODO: for directories it is still left
            vector<string> ls_args;
            istringstream iss(s);
            string word;
            while (iss >> word) {
                ls_args.push_back(word);
            }
            ls_args.erase(ls_args.begin());  // Remove the "ls" command part
            
            if (ls_args.empty()) {
                ls_command.execute({},current_working_directory);  // No args
            } else {
                ls_command.execute(ls_args,current_working_directory);  // Pass args
            }
            continue;
        }
        else if (s.rfind("search", 0) == 0) {
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
                bool found = search_directory(".", target);
                cout << (found ? "True" : "False") << endl;
            }
            continue;
        }
        cout << "Processing command: " << s << endl;
        }
    }
    return 0;
}