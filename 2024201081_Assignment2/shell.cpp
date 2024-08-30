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

int main(){

    set_terminal_mode();
    atexit(reset_terminal_mode);

    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        perror("gethostname"); // Error handling
        return 1;
    }
    // cout<<hostname<<endl;
    string user = getenv("USER");
    size_t history_index=0;
    vector<string> history;
        // char *host = getenv("HOSTNAME");
    // cout<<user<<" "<<endl;
    // cout<<"WELCOME TO THE FUTURISTIC SHELL\n";
    vector <string> path;
    path={"~"};
    cout<<"LOOP STARTS"<<endl;
    while(1){

        cout<<user<<"@"<<hostname<<":";
        bool is_root_dir=true;
        for(auto i:path){
            if(is_root_dir){
                cout<<i;
                is_root_dir=false;    
            }
            else{
                cout<<"/"<<i;
            }
        }
        cout<<">";
        string s;
        char ch;
        int f=0;
        cout<<endl;
        while (f<1) {

        ssize_t bytesRead = read(STDIN_FILENO, &ch, 1);
            if (bytesRead <= 0) break;

            if (ch == 10) { // Enter key
                cout << endl;
                break;
            } else if (ch == 127) { // Backspace
                if (!s.empty()) {
                    cout << "\b \b"; // Erase the last character
                    s.pop_back();
                }
            } else if (ch == 27) { // Escape sequence (arrow keys)
                read(STDIN_FILENO, &ch, 1); // Skip [
                read(STDIN_FILENO, &ch, 1); // Get the final character

                if (ch == 'A') { // Up arrow
                    if (history_index > 0) {
                        history_index--;
                        s = history[history_index];
                        cout << "\r" << user << "@" << hostname << ":";
                        for (auto &i : path) {
                            if (&i == &path[0]) {
                                cout << i;
                            } else {
                                cout << "/" << i;
                            }
                        }
                        cout << "> " << s;
                    }
                } else if (ch == 'B') { // Down arrow
                    if (history_index < history.size() - 1) {
                        history_index++;
                        s = history[history_index];
                        cout << "\r" << user << "@" << hostname << ":";
                        for (auto &i : path) {
                            if (&i == &path[0]) {
                                cout << i;
                            } else {
                                cout << "/" << i;
                            }
                        }
                        cout << "> " << s;
                    }
                }
            } else {
                cout << ch;
                s += ch;
            }
        }

        // getline(cin,s);
        trim(s);
        if(s=="exit"){
            cout<<"Thank you for using the shell"<<endl;
            return 0;
            break;
        }
        if(s.rfind("cd ",0)==0){
            string move_to = s.substr(3);
            if(move_to=="~"){
                path={"~"};
                continue;
            }
            trim(move_to);
            
            if (chdir(move_to.c_str()) == 0) {
                // Successfully changed directory
                cout<<"RUNNING"<<endl;
                path.push_back(move_to);
                // cout<<"Changing directory to "<<move_to<<endl;
            } else {
                // Failed to change directory
                // perror("cd error");
                cout<<"ERROR\n";

            }
            
            
            continue;
        }
        
        else if(s.rfind("echo ",0)==0){
            string echo_txt=s.substr(5);
            echo_txt=trim(echo_txt);
            string process_echo_txt=process_echo(echo_txt);
            cout<<process_echo_txt<<endl;
            continue;
        }
        cout<<"Processing command:"<<s<<endl;
    }
    return 0;
}