#include ".assets/headersh.hpp"

vector<string> history;
string histFile = string(getenv("HOME") ? getenv("HOME") : ".") + "/.rish_history";

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

int main() {
    loadHistory();

    while (true) {
        printf("%s", getPrompt().c_str());
        fflush(stdout);

        string line = input();

        if (line.empty()) continue;

        history.push_back(line);

        if (line == "exit") {
            break;
        } else if (line.starts_with("cd ")) {
            string dir = line.substr(3);
            if (dir == "~") dir = getenv("HOME");
            chdir(dir.c_str());
            continue;
        } else {
            system(line.c_str());
        }
    }

    saveHistory();
    return 0;
}