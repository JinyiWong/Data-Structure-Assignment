#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
using namespace std;
using namespace std::chrono;

// -------------------- Data Structures --------------------
struct Job {
    string title;
    vector<string> skills;
};

struct Resume {
    vector<string> skills;
};

// -------------------- Utility --------------------
vector<string> splitSkills(const string &line) {
    vector<string> skills;
    stringstream ss(line);
    string skill;
    while (getline(ss, skill, ',')) {
        // remove leading/trailing spaces
        while (!skill.empty() && skill.front() == ' ') skill.erase(skill.begin());
        while (!skill.empty() && skill.back() == ' ') skill.pop_back();
        skills.push_back(skill);
    }
    return skills;
}

// -------------------- Weighted Scoring --------------------
int calculateScore(const Resume &r, const Job &j) {
    int score = 0;
    for (auto &jobSkill : j.skills) {
        // Linear Search for matching skill
        for (auto &resumeSkill : r.skills) {
            if (resumeSkill == jobSkill) {
                score++;
                break;
            }
        }
    }
    return score;
}

// -------------------- Selection Sort --------------------
void selectionSort(vector<pair<int, string>> &arr) {
    int n = arr.size();
    for (int i = 0; i < n - 1; i++) {
        int maxIdx = i;
        for (int j = i + 1; j < n; j++) {
            if (arr[j].first > arr[maxIdx].first) {
                maxIdx = j;
            }
        }
        swap(arr[i], arr[maxIdx]);
    }
}

// -------------------- Main --------------------
int main() {
    vector<Job> jobs;
    vector<Resume> resumes;

    // Load job data
    ifstream jobFile("job_description_cleaned.csv");
    string line;
    getline(jobFile, line); // skip header
    while (getline(jobFile, line)) {
        stringstream ss(line);
        string jobTitle, skillStr;
        getline(ss, jobTitle, ',');
        getline(ss, skillStr, '\n');
        jobs.push_back({jobTitle, splitSkills(skillStr)});
    }
    jobFile.close();

    // Load resume data
    ifstream resumeFile("resume_cleaned.csv");
    getline(resumeFile, line); // skip header
    while (getline(resumeFile, line)) {
        resumes.push_back({splitSkills(line)});
    }
    resumeFile.close();

    // Start timer
    auto start = high_resolution_clock::now();

    // Calculate weighted scores (using Linear Search)
    vector<pair<int, string>> results; // (score, jobTitle)
    for (auto &resume : resumes) {
        for (auto &job : jobs) {
            int score = calculateScore(resume, job);
            results.push_back({score, job.title});
        }
    }

    // Sort results (using Selection Sort)
    selectionSort(results);

    // Stop timer
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);

    // Print results
    cout << "=== Results (Linear Search + Selection Sort) ===\n";
    for (auto &r : results) {
        cout << setw(15) << r.second << " | Score: " << r.first << "\n";
    }
    cout << "Execution Time: " << duration.count() << " ms\n";
    cout << "Approx Memory Used: " << results.size() * sizeof(pair<int,string>) << " bytes\n";

    return 0;
}
