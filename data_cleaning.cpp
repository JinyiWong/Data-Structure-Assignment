#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cctype>
using namespace std;

// ----------------- Basic Helpers -----------------
string trim(const string &s) {
    size_t start = 0;
    while (start < s.size() && isspace((unsigned char)s[start])) start++;
    size_t end = s.size();
    while (end > start && isspace((unsigned char)s[end - 1])) end--;
    return s.substr(start, end - start);
}

string toLower(const string &s) {
    string out = s;
    for (size_t i = 0; i < out.size(); i++) out[i] = tolower((unsigned char)out[i]);
    return out;
}

size_t findCaseInsensitive(const string &hay, const string &needle) {
    string Lhay = toLower(hay);
    string Lneedle = toLower(needle);
    return Lhay.find(Lneedle);
}

string stripOuterQuotes(const string &s) {
    string t = trim(s);
    if (t.size() >= 2 && t.front() == '"' && t.back() == '"')
        return t.substr(1, t.size() - 2);
    return t;
}

// ----------------- Custom String Array -----------------
struct StringArray {
    string arr[200];
    int size;
};

void split(const string &s, char delim, StringArray &out) {
    out.size = 0;
    string token;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == delim) {
            string t = trim(token);
            if (!t.empty()) out.arr[out.size++] = t;
            token = "";
        } else {
            token += s[i];
        }
    }
    string t = trim(token);
    if (!t.empty()) out.arr[out.size++] = t;
}

string join(const StringArray &arr) {
    string out = "";
    for (int i = 0; i < arr.size; i++) {
        if (i > 0) out += ", ";
        out += arr.arr[i];
    }
    return out;
}

string csvEscape(const string &field) {
    string out = "\"";
    for (char c : field) {
        if (c == '"') out += "\"\"";
        else out += c;
    }
    out += "\"";
    return out;
}

// ----------------- Skill Cleaning -----------------
string cleanSkills(const string &skillsLine) {
    StringArray skills, cleaned;
    split(skillsLine, ',', skills);
    cleaned.size = 0;

    for (int i = 0; i < skills.size; i++) {
        if (!skills.arr[i].empty() && isupper((unsigned char)skills.arr[i][0])) {
            cleaned.arr[cleaned.size++] = skills.arr[i];
        }
    }
    return join(cleaned);
}

// ----------------- Process Jobs -----------------
void processJobs(const string &inFile, const string &outFile) {
    ifstream fin(inFile);
    ofstream fout(outFile);
    if (!fin.is_open() || !fout.is_open()) {
        cerr << "Error opening file!\n";
        return;
    }

    fout << "\"Job\",\"Skills\"\n";
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

        size_t dotPos = remainder.find('.');
        if (dotPos != string::npos)
            remainder = trim(remainder.substr(0, dotPos));

        string cleanedSkills = cleanSkills(remainder);
        fout << csvEscape(job) << "," << csvEscape(cleanedSkills) << "\n";
    }

    fin.close();
    fout.close();
    cout << "Job descriptions cleaned -> " << outFile << endl;
}

// ----------------- Process Resumes -----------------
void processResumes(const string &inFile, const string &outFile) {
    ifstream fin(inFile);
    ofstream fout(outFile);
    if (!fin.is_open() || !fout.is_open()) {
        cerr << "Error opening file!\n";
        return;
    }

    fout << "\"Skills\"\n";
    string phrase = "Experienced professional skilled in";
    string line;

    while (getline(fin, line)) {
        string t = stripOuterQuotes(line);
        size_t pos = findCaseInsensitive(t, phrase);
        string remainder;

        if (pos != string::npos)
            remainder = trim(t.substr(pos + phrase.length()));
        else
            remainder = t;

        size_t dotPos = remainder.find('.');
        if (dotPos != string::npos)
            remainder = trim(remainder.substr(0, dotPos));

        string cleanedSkills = cleanSkills(remainder);
        fout << csvEscape(cleanedSkills) << "\n";
    }

    fin.close();
    fout.close();
    cout << "Resumes cleaned -> " << outFile << endl;
}

// ----------------- Group Jobs (No STL Containers) -----------------
void groupJobs(const string &cleanedFile, const string &outFile) {
    ifstream fin(cleanedFile);
    ofstream fout(outFile);
    if (!fin.is_open() || !fout.is_open()) {
        cerr << "Error opening file for grouping!\n";
        return;
    }

    string header;
    getline(fin, header);

    struct Entry { string job; string skills; };
    const int MAX = 20000;
    Entry entries[MAX];
    int entryCount = 0;

    string line;
    while (getline(fin, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string jobField, skillField;
        getline(ss, jobField, ',');
        getline(ss, skillField);
        string job = stripOuterQuotes(jobField);
        string skills = stripOuterQuotes(skillField);
        if (entryCount < MAX) {
            entries[entryCount].job = job;
            entries[entryCount].skills = skills;
            entryCount++;
        }
    }
    fin.close();

    struct Group { string job; string skills; int groupNum; };
    Group groups[MAX];
    int groupCount = 0;

    // Remove duplicates manually
    for (int i = 0; i < entryCount; i++) {
        bool found = false;
        for (int j = 0; j < groupCount; j++) {
            if (groups[j].job == entries[i].job && groups[j].skills == entries[i].skills) {
                found = true;
                break;
            }
        }
        if (!found && groupCount < MAX) {
            groups[groupCount].job = entries[i].job;
            groups[groupCount].skills = entries[i].skills;
            groups[groupCount].groupNum = 0;
            groupCount++;
        }
    }

    // Assign group numbers per job
    for (int i = 0; i < groupCount; i++) {
        int countBefore = 0;
        for (int j = 0; j < i; j++) {
            if (groups[j].job == groups[i].job) countBefore++;
        }
        groups[i].groupNum = countBefore + 1;
    }

    fout << "\"Job\",\"Skills\"\n";
    for (int i = 0; i < groupCount; i++) {
        string labeledJob = groups[i].job + " " + to_string(groups[i].groupNum);
        fout << csvEscape(labeledJob) << "," << csvEscape(groups[i].skills) << "\n";
    }

    fout.close();
    cout << "Jobs grouped, duplicates removed, and labeled (no STL containers) -> " << outFile << endl;
}

// ----------------- main -----------------
int main() {
    processJobs("job_description.csv", "job_description_cleaned.csv");
    processResumes("resume.csv", "resume_cleaned.csv");
    groupJobs("job_description_cleaned.csv", "job_grouped.csv");
    return 0;
}