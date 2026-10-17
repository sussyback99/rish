#include "assets/headersh.hpp"

vector<string> history;
string histFile = string(getenv("HOME") ? getenv("HOME") : ".") + "/.rish_history";

struct ParsedCommand {
    string name;
    vector<string> args;
    vector<string> flags;
};

ParsedCommand parseLine(const string& line) {
    ParsedCommand cmd;
    stringstream ss(line);
    string token;

    if (ss >> cmd.name) {
        while (ss >> token) {
            if (!token.empty() && token[0] == '@') {
                cmd.flags.push_back(token.substr(1));
            } else {
                cmd.args.push_back(token);
            }
        }
    }
    return cmd;
}

void loadHistory() {
    ifstream f(histFile);
    string line;
    while (getline(f, line)) {
        if (!line.empty()) history.push_back(line);
    }
}

void saveHistory() {
    ofstream f(histFile);
    for (auto& h : history) f << h << "\n";
}

string getPrompt() {
    string user = getenv("USER") ? getenv("USER") : "user";

    char host[256] = {0};
    gethostname(host, 255);
    string h = host;
    size_t dot = h.find('.');
    if (dot != string::npos) h = h.substr(0, dot);

    char cwd[4096] = {0};
    getcwd(cwd, 4095);
    string path = cwd;

    string home = getenv("HOME") ? getenv("HOME") : "";
    if (!home.empty() && path.find(home) == 0) {
        path = "~" + path.substr(home.size());
    }

    char sym = (geteuid() == 0) ? '#' : '$';

    string p;
    p += "\033[33m(";
    p += "\033[31m";
    p += user + "@" + h;
    p += "\033[0m ";
    p += "\033[32m";
    p += path;
    p += "\033[0m";
    p += "\033[33m)";
    p += "\033[0m";
    p += sym;
    p += " ";
    return p;
}

string findSuggestion(const string& prefix) {
    if (prefix.empty()) return "";
    for (int i = (int)history.size() - 1; i >= 0; --i) {
        if (history[i].size() > prefix.size() &&
            history[i].compare(0, prefix.size(), prefix) == 0) {
            return history[i].substr(prefix.size());
        }
    }
    return "";
}

string input() {
    termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    string line;
    string suggestion;

    while (true) {
        int c = fgetc(stdin);
        if (c == EOF) break;

        if (c == '\n') {
            printf("\033[0m\n");
            break;
        }
        else if (c == 127 || c == 8) {
            if (!line.empty()) {
                line.pop_back();
                printf("\b \b");
            }
        }
        else if (c == '\t') {
            if (!suggestion.empty()) {
                line += suggestion;
                printf("%s", suggestion.c_str());
            }
        }
        else if (c == 27) {
            int c1 = fgetc(stdin);
            int c2 = fgetc(stdin);
            if (c1 == '[' && c2 == 'A') {
                if (!history.empty()) {
                    for (size_t i = 0; i < line.size(); ++i) printf("\b \b");
                    line = history.back();
                    printf("%s", line.c_str());
                }
            }
        }
        else if (c >= 32 && c < 127) {
            line += (char)c;
            printf("%c", (char)c);
        }

        string newSug = findSuggestion(line);
        if (newSug != suggestion) {
            if (!suggestion.empty()) {
                for (size_t i = 0; i < suggestion.size(); ++i) printf(" \b");
            }
            suggestion = newSug;
            if (!suggestion.empty()) {
                printf("\033[2;31m%s\033[0m", suggestion.c_str());
                for (size_t i = 0; i < suggestion.size(); ++i) printf("\b");
            }
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return line;
}

void getCwd() {
    char temp[1024];
    if (getcwd(temp, sizeof(temp)) != nullptr) {
        printf("%s\n", temp);
    }
    else {
        fprintf(stderr, "rish FSError! Unknown cwd!\n");
    }
}

int main() {
    loadHistory();

    while (true) {
        printf("%s", getPrompt().c_str());
        fflush(stdout);

        string line = input();
        if (line.empty()) continue;

        history.push_back(line);

        ParsedCommand cmd = parseLine(line);

        if (cmd.name == "exit") {
            break;
        }
        else if (cmd.name == "cd") {
            if (!cmd.args.empty()) {
                string dir = cmd.args[0];
                if (dir == "~" && getenv("HOME")) {
                    dir = getenv("HOME");
                }
                if (chdir(dir.c_str()) != 0) {
                    perror("rish: cd error");
                }
            } else {
                if (getenv("HOME")) chdir(getenv("HOME"));
            }
        }
        else if (cmd.name == "print") {
            bool isCwd = false;
            string Newline = "\n";
            string text = "";

            for (const auto& f : cmd.flags) {
                if (f == "cwd") isCwd = true;
                else if (f == "n" || f == "nonewline") Newline = "";
            }

            for (size_t i = 0; i < cmd.args.size(); ++i) {
                if (i > 0) text += " ";
                text += cmd.args[i];
            }

            if (isCwd) {
                getCwd();
            } else {
                printf("%s%s", text.c_str(), Newline.c_str());
            }
        }
        else if (cmd.name == "who") {
            bool myuid = false;
            for (const auto& f : cmd.flags) {
                if (f == "myuid") myuid = true;
            }
            if (myuid) {
                printf("%d\n", getuid());
            }
            else {
                fprintf(stderr, "who: unknown arguments");
            }
        }
        
        else {
            printf("rish: %s: Unknown Command\n", cmd.name.c_str());
        }
    }

    saveHistory();
    return 0;
}