#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <chrono>   // runtime measurement
using namespace std;
using namespace std::chrono;

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

// Quick Sort implementation (by Job Title)
int partition(vector<Job> &jobs, int low, int high) {
    string pivot = jobs[high].title;
    int i = low - 1;
    for (int j = low; j < high; j++) {
        if (jobs[j].title <= pivot) {
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

// Binary Search (by Job Title)
void binarySearch(const vector<Job> &jobs, const string &title) {
    auto start = high_resolution_clock::now();

    int low = 0, high = jobs.size() - 1;
    bool found = false;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (jobs[mid].title == title) {
            cout << "Job: " << jobs[mid].title << " | Skills: " << jobs[mid].skills << endl;
            found = true;
            break;
        } else if (jobs[mid].title < title) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }

    if (!found) cout << "No job found with title: " << title << endl;

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);

    cout << "Search runtime (Binary Search): " << duration.count() << " microseconds\n";
    cout << "Approx. memory usage: " << jobs.capacity() * sizeof(Job) << " bytes\n";
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
        cout << "\n--- Array Implementation (Set 2: Binary Search + Quick Sort) ---\n";
        cout << "1. Display Jobs\n2. Binary Search (by Job Title)\n3. Quick Sort\n0. Exit\nChoice: ";
        cin >> choice;
        cin.ignore();

        if (choice == 1) {
            displayJobs(jobs);
        } else if (choice == 2) {
            string title;
            cout << "Enter job title to search: ";
            getline(cin, title);
            binarySearch(jobs, title);
        } else if (choice == 3) {
            auto start = high_resolution_clock::now();
            quickSort(jobs, 0, jobs.size() - 1);
            auto stop = high_resolution_clock::now();
            auto duration = duration_cast<microseconds>(stop - start);

            cout << "Jobs sorted by title (Quick Sort).\n";
            displayJobs(jobs);

            cout << "Sorting runtime (Quick Sort): " << duration.count() << " microseconds\n";
            cout << "Approx. memory usage: " << jobs.capacity() * sizeof(Job) << " bytes\n";
        }
    } while (choice != 0);

    return 0;
}
