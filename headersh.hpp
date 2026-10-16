#pragma once
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

using namespace std;

string getPrompt();
string input();