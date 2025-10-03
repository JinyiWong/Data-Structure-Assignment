#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>
#include <cstdlib>
using namespace std;

struct Job {
    string title;
    string skills;
};

// Load CSV into array
vector<Job> loadJobs(const string &filename) {
    vector<Job> jobs;
    ifstream file(filename);
    string line;
    getline(file, line); // skip header

    while (getline(file, line)) {
        stringstream ss(line);
        string jobTitle, skills;

        getline(ss, jobTitle, ',');
        getline(ss, skills);

        if (!jobTitle.empty() && !skills.empty()) {
            jobs.push_back({jobTitle, skills});
        }
    }
    return jobs;
}

// Linear search by keyword with runtime + memory
void linearSearch(const vector<Job> &jobs, const string &keyword) {
    auto start = chrono::high_resolution_clock::now();

    size_t memoryUsed = 0;
    bool found = false;
    cout << "\n[Linear Search Results for \"" << keyword << "\"]\n";
    for (const auto &job : jobs) {
        memoryUsed += sizeof(job) + job.title.capacity() + job.skills.capacity();
        if (job.skills.find(keyword) != string::npos) {
            cout << "Job: " << job.title << " | Skills: " << job.skills << endl;
            found = true;
        }
    }
    if (!found) cout << "No matches found.\n";

    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
    cout << "Search Runtime: " << duration.count() << " microseconds\n";
    cout << "Estimated Memory Used: " << memoryUsed << " bytes\n";
}

// Selection sort (by Job Title) with runtime + memory
void selectionSort(vector<Job> &jobs) {
    auto start = chrono::high_resolution_clock::now();

    size_t memoryUsed = 0;
    int n = jobs.size();
    for (int i = 0; i < n - 1; i++) {
        int minIndex = i;
        for (int j = i + 1; j < n; j++) {
            memoryUsed += sizeof(jobs[j]) + jobs[j].title.capacity() + jobs[j].skills.capacity();
            if (jobs[j].title < jobs[minIndex].title)
                minIndex = j;
        }
        swap(jobs[i], jobs[minIndex]);
    }

    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
    cout << "Sort Runtime: " << duration.count() << " microseconds\n";
    cout << "Estimated Memory Used: " << memoryUsed << " bytes\n";
}

void displayJobs(const vector<Job> &jobs) {
    for (const auto &job : jobs) {
        cout << "Job: " << job.title << " | Skills: " << job.skills << endl;
    }
}

int main() {
    vector<Job> jobs = loadJobs("job_description_cleaned.csv");

    if (jobs.empty()) {
        cout << "No jobs loaded. Please check the CSV file.\n";
        return 0;
    }

    int choice;
    do {
        cout << "\n--- Array Implementation (Set 1: Linear Search + Selection Sort) ---\n";
        cout << "1. Display Jobs\n2. Linear Search\n3. Selection Sort\n0. Exit\nChoice: ";
        cin >> choice;
        cin.ignore();

        if (choice == 1) {
            displayJobs(jobs);
        } else if (choice == 2) {
            string keyword;
            cout << "Enter skill to search: ";
            getline(cin, keyword);
            linearSearch(jobs, keyword);
        } else if (choice == 3) {
            cout << "Jobs sorted by title (Selection Sort).\n";
            displayJobs(jobs);
            selectionSort(jobs);
        }
    } while (choice != 0);

    return 0;
}
