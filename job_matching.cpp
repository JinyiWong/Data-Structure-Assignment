#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>

using namespace std;

// Structs
struct Job {
    string title;
    vector<string> skills;
};

struct Resume {
    int id;
    vector<string> skills;
};

// === Utility Functions ===
string toLowerStr(string s) {
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

// Split by comma and normalize
vector<string> split(const string &str, char delimiter = ',') {
    vector<string> tokens;
    string token;
    stringstream ss(str);

    while (getline(ss, token, delimiter)) {
        // Trim whitespace
        token.erase(remove_if(token.begin(), token.end(), ::isspace), token.end());
        // Remove quotes
        token.erase(remove(token.begin(), token.end(), '\"'), token.end());
        if (!token.empty()) {
            tokens.push_back(toLowerStr(token));
        }
    }
    return tokens;
}

// Weighted scoring
int calculateScore(const vector<string> &jobSkills, const vector<string> &resumeSkills) {
    int score = 0;
    for (auto &skill : resumeSkills) {
        for (auto &jobSkill : jobSkills) {
            if (skill == jobSkill) {
                score += 2; // weight for exact match
            }
        }
    }
    return score;
}

// === Load Functions ===
vector<Job> loadJobs(const string &filename) {
    vector<Job> jobs;
    ifstream file(filename);
    string line;

    if (!file.is_open()) {
        cerr << "Error: Could not open " << filename << endl;
        return jobs;
    }

    while (getline(file, line)) {
        if (line.empty()) continue;

        stringstream ss(line);
        string title, skillsStr;

        if (getline(ss, title, ',') && getline(ss, skillsStr)) {
            jobs.push_back({title, split(skillsStr, ',')});
        }
    }
    return jobs;
}

vector<Resume> loadResumes(const string &filename) {
    vector<Resume> resumes;
    ifstream file(filename);
    string line;
    int id = 1;

    if (!file.is_open()) {
        cerr << "Error: Could not open " << filename << endl;
        return resumes;
    }

    while (getline(file, line)) {
        if (line.empty()) continue;
        resumes.push_back({id++, split(line, ',')});
    }
    return resumes;
}

// === Main ===
int main() {
    auto jobs = loadJobs("D:/Degree-2502SE/Year 2/SEM 2/Data Structures (DSTR)/DSTR Assignment/Data-Structure-Assignment/job_description_cleaned.csv");
    auto resumes = loadResumes("D:/Degree-2502SE/Year 2/SEM 2/Data Structures (DSTR)/DSTR Assignment/Data-Structure-Assignment/resume_cleaned.csv");

    cout << "\n=== SUMMARY ===" << endl;
    cout << "Loaded " << jobs.size() << " jobs." << endl;
    cout << "Loaded " << resumes.size() << " resumes." << endl;

    // Matching process
    int totalMatches = 0;
    for (size_t j = 0; j < jobs.size(); j++) {
        for (auto &resume : resumes) {
            int score = calculateScore(jobs[j].skills, resume.skills);
            if (score > 0) {
                totalMatches++;
            }
        }
    }

    cout << "\nTotal matches across all jobs: " << totalMatches << endl;

    return 0;
}
