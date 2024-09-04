#include <iostream>
#include <cstdlib> // For exit()
#include <string>

using namespace std;

void handle_eof() {
    cout << "CTRL-D pressed, exiting..." << endl;
    exit(0); // Exit the program
}

int main() {
    string line;
    
    cout << "Press CTRL-D to exit." << endl;

    while (true) {
        // Read input line by line
        if (!getline(cin, line)) {
            // If getline fails, it could be due to EOF (CTRL-D) or another error
            if (cin.eof()) {
                // Handle EOF
                handle_eof();
            } else {
                // Handle other errors if necessary
                cerr << "Error reading input" << endl;
                return 1;
            }
        }

        // Process input line if needed
        cout << "You entered: " << line << endl;
    }

    return 0;
}
