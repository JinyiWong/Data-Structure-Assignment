#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <windows.h>
#include <psapi.h>

using namespace std;
using namespace chrono;

// === Structs ===
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

vector<string> split(const string &str, char delimiter = ',') {
    vector<string> tokens;
    string token;
    stringstream ss(str);
    while (getline(ss, token, delimiter)) {
        token.erase(remove_if(token.begin(), token.end(), ::isspace), token.end());
        token.erase(remove(token.begin(), token.end(), '\"'), token.end());
        if (!token.empty()) tokens.push_back(toLowerStr(token));
    }
    return tokens;
}

int calculateScore(const vector<string> &jobSkills, const vector<string> &resumeSkills) {
    int score = 0;
    for (auto &skill : resumeSkills)
        for (auto &jobSkill : jobSkills)
            if (skill == jobSkill)
                score += 2; // weighted match
    return score;
}

// === Memory Usage ===
double getMemoryUsageMB() {
    PROCESS_MEMORY_COUNTERS_EX pmc;
    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
    SIZE_T memUsed = pmc.WorkingSetSize;
    return static_cast<double>(memUsed) / (1024.0 * 1024.0);
}

// === Load Functions ===
vector<Job> loadJobs(const string &filename) {
    vector<Job> jobs;
    ifstream file(filename);
    string line;
    getline(file, line); // skip header
    while (getline(file, line)) {
        if (line.empty()) continue;
        stringstream ss(line);
        string title, skillsStr;
        if (getline(ss, title, ',') && getline(ss, skillsStr))
            jobs.push_back({title, split(skillsStr, ',')});
    }
    return jobs;
}

vector<Resume> loadResumes(const string &filename) {
    vector<Resume> resumes;
    ifstream file(filename);
    string line;
    int id = 1;
    getline(file, line); // skip header
    while (getline(file, line)) {
        if (line.empty()) continue;
        resumes.push_back({id++, split(line, ',')});
    }
    return resumes;
}

// === Quick Sort (sort by title) ===
int partition(vector<Job> &jobs, int low, int high) {
    string pivot = jobs[high].title;
    int i = low - 1;
    for (int j = low; j < high; j++) {
        if (jobs[j].title < pivot) {
            i++;
            swap(jobs[i], jobs[j]);
        }
    }
    swap(jobs[i + 1], jobs[high]);
    return i + 1;
}

void quickSort(vector<Job> &jobs, int low, int high) {
    if (low < high) {
        int pi = partition(jobs, low, high);
        quickSort(jobs, low, pi - 1);
        quickSort(jobs, pi + 1, high);
    }
}

// === Binary Search ===
int binarySearch(const vector<Job> &jobs, const string &target) {
    int low = 0, high = jobs.size() - 1;
    while (low <= high) {
        int mid = (low + high) / 2;
        string titleLower = toLowerStr(jobs[mid].title);
        if (titleLower == target)
            return mid;
        else if (titleLower < target)
            low = mid + 1;
        else
            high = mid - 1;
    }
    return -1;
}

// === Job Search and Matching ===
void binarySearchAndMatch(const vector<Job> &jobs, const vector<Resume> &resumes) {
    string query;
    cout << "\nEnter job title to search (Set 2 - Binary Search): ";
    getline(cin, query);
    query = toLowerStr(query);

    auto start = high_resolution_clock::now();

    int index = binarySearch(jobs, query);
    int totalMatches = 0;
    bool found = (index != -1);

    if (found) {
        for (auto &resume : resumes) {
            int score = calculateScore(jobs[index].skills, resume.skills);
            if (score > 0) totalMatches++;
        }
    }

    auto end = high_resolution_clock::now();
    double runtime_ms = duration<double, milli>(end - start).count();
    double memoryMB = getMemoryUsageMB();

    if (found)
        cout << "\n[Set 2] Job found. Total matching resumes: " << totalMatches << endl;
    else
        cout << "\n[Set 2] No job found matching your search." << endl;

    cout << fixed << setprecision(3);
    cout << "Runtime: " << runtime_ms << " ms" << endl;
    cout << "Memory Usage: " << memoryMB << " MB\n" << endl;
}

// === Sorting Function ===
void quickSortJobs(vector<Job> &jobs) {
    auto start = high_resolution_clock::now();

    quickSort(jobs, 0, jobs.size() - 1);

    auto end = high_resolution_clock::now();
    double runtime_ms = duration<double, milli>(end - start).count();
    double memoryMB = getMemoryUsageMB();

    cout << fixed << setprecision(3);
    cout << "\n[Set 2] Jobs sorted using Quick Sort (by title)." << endl;
    cout << "Runtime: " << runtime_ms << " ms" << endl;
    cout << "Memory Usage: " << memoryMB << " MB\n" << endl;
}

// === Main ===
int main() {
    auto jobs = loadJobs("job_description_cleaned.csv");
    auto resumes = loadResumes("resume_cleaned.csv");

    cout << "\n=== SUMMARY (Set 2) ===" << endl;
    cout << "Loaded " << jobs.size() << " jobs." << endl;
    cout << "Loaded " << resumes.size() << " resumes." << endl;

    int choice;
    do {
        cout << "\n--- Array Implementation (Set 2: Binary Search + Quick Sort) ---" << endl;
        cout << "1. Search Job & Match Candidates" << endl;
        cout << "2. Sort Jobs (Quick Sort)" << endl;
        cout << "0. Exit" << endl;
        cout << "Choice: ";
        cin >> choice;
        cin.ignore();

        switch (choice) {
        case 1:
            binarySearchAndMatch(jobs, resumes);
            break;
        case 2:
            quickSortJobs(jobs);
            break;
        case 0:
            cout << "Exiting program..." << endl;
            break;
        default:
            cout << "Invalid choice. Try again." << endl;
        }
    } while (choice != 0);

    return 0;
}
