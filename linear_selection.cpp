#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>

using namespace std;
using namespace std::chrono;

// ================================
// STRUCTS
// ================================
struct Job {
    string title;
    vector<string> skills;
};

struct Resume {
    int id;
    vector<string> skills;
};

// ================================
// UTILITY FUNCTIONS
// ================================

// convert to lowercase
string toLowerStr(string s) {
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

// split string by comma and clean
vector<string> split(const string &str, char delimiter = ',') {
    vector<string> tokens;
    string token;
    stringstream ss(str);
    while (getline(ss, token, delimiter)) {
        token.erase(remove_if(token.begin(), token.end(), ::isspace), token.end());
        token.erase(remove(token.begin(), token.end(), '\"'), token.end());
        if (!token.empty())
            tokens.push_back(toLowerStr(token));
    }
    return tokens;
}

// ================================
// LOAD CSV DATA
// ================================
vector<Job> loadJobs(const string &filename) {
    vector<Job> jobs;
    ifstream file(filename);
    string line;

    if (!file.is_open()) {
        cerr << "Error: Could not open " << filename << endl;
        return jobs;
    }

    getline(file, line); // skip header if present
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

    getline(file, line); // skip header if present
    while (getline(file, line)) {
        if (line.empty()) continue;
        resumes.push_back({id++, split(line, ',')});
    }
    return resumes;
}

// ================================
// MATCHING & SCORING
// ================================

// weighted scoring: +2 points per exact skill match
int calculateScore(const vector<string> &jobSkills, const vector<string> &resumeSkills) {
    int score = 0;
    for (auto &rSkill : resumeSkills) {
        for (auto &jSkill : jobSkills) {
            if (rSkill == jSkill) {
                score += 2;
            }
        }
    }
    return score;
}

// ================================
// SEARCH AND MATCH (LINEAR SEARCH)
// ================================
int linearSearchJob(const vector<Job> &jobs, const string &jobTitle) {
    for (size_t i = 0; i < jobs.size(); i++) {
        if (toLowerStr(jobs[i].title) == toLowerStr(jobTitle)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// Search + match process with runtime + memory usage
vector<pair<int, int>> searchAndMatch(
    const vector<Job> &jobs,
    const vector<Resume> &resumes,
    const string &jobTitle
) {
    cout << "\n[Linear Search + Matching for \"" << jobTitle << "\"]\n";

    auto start = high_resolution_clock::now();
    size_t memoryUsed = 0;

    // Search job
    int jobIndex = linearSearchJob(jobs, jobTitle);

    if (jobIndex == -1) {
        cout << "Job not found.\n";
        return {};
    }

    // Matching resumes
    vector<pair<int, int>> scores; // resumeID, score
    for (auto &resume : resumes) {
        int score = calculateScore(jobs[jobIndex].skills, resume.skills);
        if (score > 0) {
            scores.push_back({resume.id, score});
        }
        // estimate memory used per resume
        memoryUsed += sizeof(resume) + sizeof(score);
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start);

    cout << "Runtime (Search + Match): " << duration.count() << " microseconds\n";
    cout << "Estimated Memory Used: " << memoryUsed << " bytes\n";

    return scores;
}

// ================================
// SELECTION SORT (SORT MATCH SCORES)
// ================================
void selectionSort(vector<pair<int, int>> &scores) {
    auto start = high_resolution_clock::now();
    size_t memoryUsed = 0;

    int n = scores.size();
    for (int i = 0; i < n - 1; i++) {
        int minIdx = i;
        for (int j = i + 1; j < n; j++) {
            memoryUsed += sizeof(scores[j]);
            if (scores[j].second < scores[minIdx].second)
                minIdx = j;
        }
        swap(scores[i], scores[minIdx]);
    }

    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start);

    cout << "Runtime (Selection Sort): " << duration.count() << " microseconds\n";
    cout << "Estimated Memory Used: " << memoryUsed << " bytes\n";
}

// ================================
// DISPLAY RESULTS
// ================================
void displayMatches(const vector<pair<int, int>> &scores) {
    if (scores.empty()) {
        cout << "No matching resumes found.\n";
        return;
    }

    cout << "\nTop Matching Resumes (Sorted by Score):\n";
    for (auto &s : scores) {
        cout << "Resume ID: " << s.first << " | Score: " << s.second << endl;
    }
}

// ================================
// MAIN PROGRAM (MENU)
// ================================
int main() {
    auto jobs = loadJobs("job_description_cleaned.csv");
    auto resumes = loadResumes("resume_cleaned.csv");

    if (jobs.empty() || resumes.empty()) {
        cout << "Error: Data not loaded properly.\n";
        return 0;
    }

    vector<pair<int, int>> scores;
    int choice;

    do {
        cout << "\n--- Set 1: Weighted Scoring + Linear Search + Selection Sort ---\n";
        cout << "1. Search Job and Match Candidates\n";
        cout << "2. Sort Matched Candidates by Score\n";
        cout << "0. Exit\n";
        cout << "Choice: ";
        cin >> choice;
        cin.ignore();

        if (choice == 1) {
            string jobTitle;
            cout << "Enter job title: ";
            getline(cin, jobTitle);
            scores = searchAndMatch(jobs, resumes, jobTitle);
        } else if (choice == 2) {
            if (scores.empty()) {
                cout << "No scores to sort. Please search and match first.\n";
            } else {
                displayMatches(scores);
                selectionSort(scores);
            }
        }

    } while (choice != 0);

    return 0;
}
