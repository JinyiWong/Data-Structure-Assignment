#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
using namespace std;

// ----------------- Helpers -----------------
string trim(const string &s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

string toLower(const string &s) {
    string out = s;
    transform(out.begin(), out.end(), out.begin(), ::tolower);
    return out;
}

// Case-insensitive find
size_t findCaseInsensitive(const string &hay, const string &needle) {
    string Lhay = toLower(hay);
    string Lneedle = toLower(needle);
    return Lhay.find(Lneedle);
}

string stripOuterQuotes(const string &s) {
    string t = trim(s);
    if (t.size() >= 2 && t.front() == '"' && t.back() == '"') {
        return t.substr(1, t.size() - 2);
    }
    return t;
}

// Split by delimiter
vector<string> split(const string &s, char delim) {
    vector<string> parts;
    string token;
    stringstream ss(s);
    while (getline(ss, token, delim)) {
        string t = trim(token);
        if (!t.empty()) parts.push_back(t);
    }
    return parts;
}

// Join vector with commas
string join(const vector<string>& v) {
    string out;
    for (size_t i = 0; i < v.size(); i++) {
        if (i) out += ", ";
        out += v[i];
    }
    return out;
}

// Quote CSV field if needed
string csvEscape(const string &field) {
    bool needQuote = false;
    for (char c : field) {
        if (c == ',' || c == '"' || c == '\n' || c == '\r') { needQuote = true; break; }
    }
    if (!needQuote) return field;
    string out = "\"";
    for (char c : field) {
        if (c == '"') out += "\"\"";
        else out += c;
    }
    out += "\"";
    return out;
}

// ----------------- Processing functions -----------------

// Clean skills: remove items starting with lowercase
string cleanSkills(const string &skillsLine) {
    vector<string> skills = split(skillsLine, ',');
    vector<string> cleaned;
    for (auto &s : skills) {
        if (!s.empty() && isupper((unsigned char)s[0])) {
            cleaned.push_back(s);
        }
    }
    return join(cleaned);
}

// Process job descriptions
void processJobs(const string &inFile, const string &outFile) {
    ifstream fin(inFile);
    ofstream fout(outFile);
    if (!fin.is_open() || !fout.is_open()) {
        cerr << "Error opening file!\n";
        return;
    }

    fout << "Job,Skills\n";

    string phrase = "needed with experience in";
    string line;
    while (getline(fin, line)) {
        string t = stripOuterQuotes(line);

        size_t pos = findCaseInsensitive(t, phrase);
        string job, remainder;
        if (pos != string::npos) {
            job = trim(t.substr(0, pos));
            remainder = trim(t.substr(pos + phrase.length()));
        } else {
            job = "";
            remainder = t;
        }

        // Keep only before first '.'
        size_t dotPos = remainder.find('.');
        if (dotPos != string::npos) {
            remainder = trim(remainder.substr(0, dotPos));
        }

        string cleanedSkills = cleanSkills(remainder);

        fout << csvEscape(job) << "," << csvEscape(cleanedSkills) << "\n";
    }

    fin.close();
    fout.close();
    cout << "Job descriptions cleaned -> " << outFile << endl;
}

// Process resumes
void processResumes(const string &inFile, const string &outFile) {
    ifstream fin(inFile);
    ofstream fout(outFile);
    if (!fin.is_open() || !fout.is_open()) {
        cerr << "Error opening file!\n";
        return;
    }

    fout << "Skills\n";

    string phrase = "Experienced professional skilled in";
    string line;
    while (getline(fin, line)) {
        string t = stripOuterQuotes(line);

        size_t pos = findCaseInsensitive(t, phrase);
        string remainder;
        if (pos != string::npos) {
            remainder = trim(t.substr(pos + phrase.length()));
        } else {
            remainder = t;
        }

        // Keep only before first '.'
        size_t dotPos = remainder.find('.');
        if (dotPos != string::npos) {
            remainder = trim(remainder.substr(0, dotPos));
        }

        string cleanedSkills = cleanSkills(remainder);

        fout << csvEscape(cleanedSkills) << "\n";
    }

    fin.close();
    fout.close();
    cout << "Resumes cleaned -> " << outFile << endl;
}

// ----------------- main -----------------
int main() {
    processJobs("job_description.csv", "job_description_cleaned.csv");
    processResumes("resume.csv", "resume_cleaned.csv");
    return 0;
}
