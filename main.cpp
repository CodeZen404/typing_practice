#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <termios.h>
#include <unistd.h>
#include <vector>
#include <sstream>
#include <cctype>
#include <cstdio>
#include <limits>

using namespace std;

void enableRawMode() {
    termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void disableRawMode() {
    termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

void setTextColor(const string &color) {
    if (color == "yellow") {
        cout << "\033[33m";
    } else if (color == "green") {
        cout << "\033[32m";
    } else if (color == "red") {
        cout << "\033[31m";
    } else {
        cout << "\033[0m";
    }
}

void replaceAll(string &s, const string &from, const string &to) {
    if (from.empty()) return;
    size_t start_pos = 0;
    while ((start_pos = s.find(from, start_pos)) != string::npos) {
        s.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

string processContent(const string &input) {
    string s = input;

    replaceAll(s, "—", "-"); // em dash
    replaceAll(s, "–", "-"); // en dash
    replaceAll(s, "“", "\""); // left smart double quote
    replaceAll(s, "”", "\""); // right smart double quote
    replaceAll(s, "‘", "'"); // left smart single quote
    replaceAll(s, "’", "'"); // right smart single quote / curly apostrophe
    replaceAll(s, "…", "..."); // ellipsis

    string out;
    out.reserve(s.size());
    for (unsigned char c : s) {
        if (c < 128) {
            out.push_back(static_cast<char>(c));
        } else {
        }
    }
    return out;
}

int main() {
    string fileName;
    system("clear");
    cout << "Enter the text file name: ";
    cin >> fileName;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    ifstream infile(fileName, ios::in | ios::binary);
    if (!infile) {
        cerr << "Error: Text file not found!\n";
        return 1;
    }

    stringstream buffer;
    buffer << infile.rdbuf();
    string rawContent = buffer.str();
    infile.close();

    string processedContent = processContent(rawContent);

    const string tempFileName = "temp_typing_file.txt";
    ofstream tempOut(tempFileName, ios::out | ios::binary);
    if (!tempOut) {
        cerr << "Error: Cannot create temp file!\n";
        return 1;
    }
    tempOut << processedContent;
    tempOut.close();

    ifstream file(tempFileName);
    if (!file) {
        cerr << "Error: Cannot open temp file for reading!\n";
        remove(tempFileName.c_str());
        return 1;
    }

    vector<string> lines;
    string line;
    while (getline(file, line)) {
        lines.push_back(line);
    }
    file.close();

    string flat;
    for (size_t i = 0; i < lines.size(); ++i) {
        flat += lines[i];
        if (i + 1 < lines.size()) flat += ' ';
    }

    struct Range { size_t start; size_t end; };
    vector<Range> wordRanges;
    size_t n = flat.size();
    size_t idx = 0;
    while (idx < n) {
        while (idx < n && isspace(static_cast<unsigned char>(flat[idx]))) ++idx;
        if (idx >= n) break;
        size_t start = idx;
        while (idx < n && !isspace(static_cast<unsigned char>(flat[idx]))) ++idx;
        size_t end = idx;
        if (end > start) wordRanges.push_back({start, end});
    }

    vector<bool> wordCorrect(wordRanges.size(), true);

    double totalCharsTyped = 0;
    for (const auto& l : lines) totalCharsTyped += l.size();

    enableRawMode();

    tcflush(STDIN_FILENO, TCIFLUSH);

    cout << "Start typing below:\n";
    setTextColor("yellow");

    size_t correctChars = 0;
    size_t flatPos = 0;
    size_t currentWordIndex = 0;

    auto startTime = chrono::steady_clock::now();

    for (size_t li = 0; li < lines.size(); ++li) {
        const string& currentLine = lines[li];

        cout << currentLine << endl;

        for (size_t i = 0; i < currentLine.size(); ++i) {
            char ch;
            cin.get(ch);

            char expected = currentLine[i];

            if (ch == expected) {
                setTextColor("green");
                correctChars++;
            } else {
                setTextColor("red");
            }
            cout << expected;
            cout.flush();

            while (currentWordIndex < wordRanges.size() && flatPos >= wordRanges[currentWordIndex].end) {
                ++currentWordIndex;
            }
            if (currentWordIndex < wordRanges.size()) {
                Range r = wordRanges[currentWordIndex];
                if (flatPos >= r.start && flatPos < r.end) {
                    if (ch != expected) {
                        wordCorrect[currentWordIndex] = false;
                    }
                }
            }

            flatPos++;
        }

        char c;
        while (true) {
            cin.get(c);
            if (c == '\n' || c == '\r') break;
        }
        cout << '\n';
        setTextColor("yellow");

        if (li + 1 < lines.size()) flatPos++;
    }

    auto endTime = chrono::steady_clock::now();
    disableRawMode();
    setTextColor("");

    double elapsedSeconds = chrono::duration<double>(endTime - startTime).count();
    if (elapsedSeconds < 1e-6) elapsedSeconds = 1e-6;

    size_t correctWords = 0;
    for (bool b : wordCorrect) if (b) ++correctWords;

    double minutes = elapsedSeconds / 60.0;
    double wpm = static_cast<double>(correctWords) / minutes;

    double accuracy = 0.0;
    if (totalCharsTyped > 0.0) {
        accuracy = (static_cast<double>(correctChars) / totalCharsTyped) * 100.0;
    }

    cout << "\nYou completed the typing!\n";
    cout << "Words Per Minute (WPM): " << wpm << "\n";
    cout << "Accuracy: " << accuracy << "%\n";

    remove(tempFileName.c_str());

    return 0;
}
