#include <iostream>
#include <string>
#include <vector>
#include <termios.h>
#include <unistd.h>

using namespace std;

string trim(const string &str) {
    size_t start = str.find_first_not_of(" \t");
    size_t end = str.find_last_not_of(" \t");
    if (start == string::npos) {
        return "";  // No content
    }
    return str.substr(start, end - start + 1);
}

string process_echo(string s){
    bool escape_character_present=false;
    string res="";
    for(auto c:s){
        if(escape_character_present){
            if(c=='n'){
                res+="\n";
            }else{
                res+='\\'<<c;
            }
            escape_character_present=false;
        }
        else if(c=='\\'){
            escape_character_present=true;
        }
        else{
            res+=c;
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
    size_t extra_spaces = prev_command_length > current_command.length() ? prev_command_length - current_command.length() : 0;
    cout << string(extra_spaces, ' ') << "\r" << prompt;

    fflush(stdout);
}

int main(){
    set_terminal_mode();
    atexit(reset_terminal_mode);

    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        perror("gethostname");
        return 1;
    }
    string user = getenv("USER");
    size_t history_index = 0;
    vector<string> history;
    vector<string> path = {"~"};

    size_t prev_command_length = 0;

    while(1) {
        string s;
        char ch;
        print_prompt(user, hostname, path, s, prev_command_length);

        while (true) {
            ssize_t bytesRead = read(STDIN_FILENO, &ch, 1);
            if (bytesRead <= 0) break;

            if (ch == 10) { // Enter key
                cout << endl;
                break;
            } else if (ch == 127) { // Backspace
                if (!s.empty()) {
                    s.pop_back();
                }
                print_prompt(user, hostname, path, s, prev_command_length);
            } else if (ch == 27) { // Escape sequence (arrow keys)
                read(STDIN_FILENO, &ch, 1); // Skip [
                if (ch == '[') {
                    read(STDIN_FILENO, &ch, 1); // Get the final character

                    if (ch == 'A') { // Up arrow
                        if (history_index > 0) {
                            history_index--;
                            s = history[history_index];
                        }
                    } else if (ch == 'B') { // Down arrow
                        if (history_index < history.size() - 1) {
                            history_index++;
                            s = history[history_index];
                        }
                        else{
                            s="";
                            history_index++;
                            print_prompt(user, hostname, path, s, prev_command_length);

                        }
                    }
                    print_prompt(user, hostname, path, s, prev_command_length);
                }
            } else {
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

        if (s == "exit") {
            cout << "Thank you for using the shell" << endl;
            return 0;
        }

        if (s.rfind("cd ", 0) == 0) {
            string move_to = s.substr(3);
            if (move_to == "~") {
                path = {"~"};
            } else {
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
        cout << "Processing command: " << s << endl;
    }
    return 0;
}
