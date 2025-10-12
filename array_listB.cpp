// array_listB.cpp (binary search, selection sort)
#define NOMINMAX
#include <windows.h>
#include <psapi.h>

#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <cctype>
#include <cmath>
#include <limits>
#include <sstream>

using namespace std;
using namespace std::chrono;

// ------------------- CONFIG -------------------
const int MAX_JOBS = 7000;
const int MAX_RESUMES = 11000;
const int MAX_SKILLS = 200;
const int MAX_RESULTS = 7000;

// PERFORMANCE LIMITS - adjust these if queries are too slow
const int MAX_JOBS_TO_ANALYZE = 100;  // Limit jobs processed per query
const int MAX_JOBS_TO_DISPLAY = 5;     // Show top 5 jobs only

// Hash table config for inverted index
const int HASH_TABLE_SIZE = 10007; // Prime number for better distribution
const int MAX_RESUMES_PER_SKILL = 11000; // Worst case: all resumes have same skill

// ------------------- STRUCTS -------------------
struct Skill {
    string original;
    string norm;
};

struct Job {
    string titleOriginal;
    string titleSortKey;
    string skillsOriginal;
    Skill skills[MAX_SKILLS];
    int skillCount;
};

struct Resume {
    int id;
    string skillsOriginal;
    Skill skills[MAX_SKILLS];
    int skillCount;
};

struct CandidateScore {
    int id;
    int score;
    int jobIndex;
};

struct JobCount {
    int jobIndex;
    int count;
    int bestCandidateId;
    int bestCandidateScore;
};

// ------------------- MANUAL DYNAMIC ARRAY -------------------
struct IntArray {
    int* data;
    int size;
    int capacity;
    
    void init() {
        capacity = 16;
        size = 0;
        data = new int[capacity];
    }
    
    void push(int value) {
        if (size >= capacity) {
            int newCap = capacity * 2;
            int* newData = new int[newCap];
            for (int i = 0; i < size; ++i) {
                newData[i] = data[i];
            }
            delete[] data;
            data = newData;
            capacity = newCap;
        }
        data[size++] = value;
    }
    
    void clear() {
        size = 0;
    }
    
    void destroy() {
        if (data) {
            delete[] data;
            data = nullptr;
        }
        size = 0;
        capacity = 0;
    }
};

// ------------------- MANUAL HASH SET (for resume skills) -------------------
struct HashSetNode {
    string key;
    HashSetNode* next;
};

struct HashSet {
    HashSetNode* buckets[503]; // Small prime for individual sets
    
    void init() {
        for (int i = 0; i < 503; ++i) {
            buckets[i] = nullptr;
        }
    }
    
    unsigned int hash(const string& s) {
        unsigned int h = 0;
        for (char c : s) {
            h = h * 31 + (unsigned char)c;
        }
        return h % 503;
    }
    
    void insert(const string& key) {
        unsigned int idx = hash(key);
        
        // Check if already exists
        HashSetNode* curr = buckets[idx];
        while (curr) {
            if (curr->key == key) return;
            curr = curr->next;
        }
        
        // Insert new
        HashSetNode* newNode = new HashSetNode;
        newNode->key = key;
        newNode->next = buckets[idx];
        buckets[idx] = newNode;
    }
    
    bool contains(const string& key) {
        unsigned int idx = hash(key);
        HashSetNode* curr = buckets[idx];
        while (curr) {
            if (curr->key == key) return true;
            curr = curr->next;
        }
        return false;
    }
    
    bool empty() {
        for (int i = 0; i < 503; ++i) {
            if (buckets[i]) return false;
        }
        return true;
    }
    
    void destroy() {
        for (int i = 0; i < 503; ++i) {
            HashSetNode* curr = buckets[i];
            while (curr) {
                HashSetNode* tmp = curr;
                curr = curr->next;
                delete tmp;
            }
            buckets[i] = nullptr;
        }
    }
};

// ------------------- MANUAL HASH MAP (Inverted Index: skill -> resume indices) -------------------
struct HashMapNode {
    string key;
    IntArray value;
    HashMapNode* next;
};

struct HashMap {
    HashMapNode* buckets[HASH_TABLE_SIZE];
    
    void init() {
        for (int i = 0; i < HASH_TABLE_SIZE; ++i) {
            buckets[i] = nullptr;
        }
    }
    
    unsigned int hash(const string& s) {
        unsigned int h = 0;
        for (char c : s) {
            h = h * 31 + (unsigned char)c;
        }
        return h % HASH_TABLE_SIZE;
    }
    
    IntArray* get(const string& key) {
        unsigned int idx = hash(key);
        HashMapNode* curr = buckets[idx];
        while (curr) {
            if (curr->key == key) {
                return &curr->value;
            }
            curr = curr->next;
        }
        return nullptr;
    }
    
    IntArray* getOrCreate(const string& key) {
        unsigned int idx = hash(key);
        HashMapNode* curr = buckets[idx];
        
        while (curr) {
            if (curr->key == key) {
                return &curr->value;
            }
            curr = curr->next;
        }
        
        // Create new node
        HashMapNode* newNode = new HashMapNode;
        newNode->key = key;
        newNode->value.init();
        newNode->next = buckets[idx];
        buckets[idx] = newNode;
        return &newNode->value;
    }
    
    void destroy() {
        for (int i = 0; i < HASH_TABLE_SIZE; ++i) {
            HashMapNode* curr = buckets[i];
            while (curr) {
                HashMapNode* tmp = curr;
                curr = curr->next;
                tmp->value.destroy();
                delete tmp;
            }
            buckets[i] = nullptr;
        }
    }
};

// ------------------- GLOBAL BUFFERS -------------------
static CandidateScore globalCandArr[MAX_RESUMES];
static JobCount globalJobCountArr[MAX_JOBS];
static int globalIdxArr[MAX_RESULTS];
static CandidateScore globalJMArr[MAX_JOBS];

// Manual hash set for each resume
static HashSet resumeSkillSets[MAX_RESUMES];

// Manual inverted index
static HashMap skillToResumes;

// ------------------- UTILITIES -------------------
string toLowerCopy(const string &s) {
    string out;
    out.reserve(s.size());
    for (char c : s) out.push_back((char)tolower((unsigned char)c));
    return out;
}

string removeSpaces(const string &s) {
    string out;
    out.reserve(s.size());
    for (char c : s) if (!isspace((unsigned char)c)) out.push_back(c);
    return out;
}

string normalizeKey(const string &s) {
    return removeSpaces(toLowerCopy(s));
}

string trim(const string &s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

string makeTitleSortKey(const string &title) {
    string t = trim(title);
    int i = (int)t.size() - 1;
    while (i >= 0 && isspace((unsigned char)t[i])) --i;
    int endDigits = i;
    while (i >= 0 && isdigit((unsigned char)t[i])) --i;
    int startDigits = i + 1;

    string prefix = (startDigits <= endDigits) ? t.substr(0, startDigits) : t;
    string digits = (startDigits <= endDigits) ? t.substr(startDigits, endDigits - startDigits + 1) : "";

    string prefLower = toLowerCopy(trim(prefix));
    if (digits.empty()) return prefLower + "|0";
    const int W = 10;
    string padded(W - (int)digits.size(), '0');
    padded += digits;
    return prefLower + "|1" + padded;
}

double getMemoryUsageKB() {
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        return (double)pmc.WorkingSetSize / 1024.0;
    }
    return 0.0;
}

void printStepStatsSimple(long long stepMs, long long cumMs, double stepMemKB, double totalMemKB) {
    cout << "Step Time: " << stepMs << " ms | Cumulative Time: " << cumMs << " ms\n";
    long long stepBytes = (long long)(stepMemKB * 1024.0);
    long long totalBytes = (long long)(totalMemKB * 1024.0);
    cout << "Step Memory Change: " << stepBytes << " bytes | Current Total Memory: " << totalBytes << " bytes\n";
}

// ------------------- CSV FIELD EXTRACTION -------------------
bool extractQuotedField(const string &line, size_t &pos, string &outField) {
    outField.clear();
    size_t n = line.size();
    while (pos < n && isspace((unsigned char)line[pos])) pos++;
    if (pos >= n) return false;

    if (line[pos] != '"') {
        size_t comma = line.find(',', pos);
        if (comma == string::npos) {
            outField = trim(line.substr(pos));
            pos = n;
        } else {
            outField = trim(line.substr(pos, comma - pos));
            pos = comma + 1;
        }
        return true;
    }

    pos++;
    while (pos < n) {
        if (line[pos] == '"') {
            if (pos + 1 < n && line[pos + 1] == '"') {
                outField.push_back('"');
                pos += 2;
            } else {
                pos++;
                if (pos < n && line[pos] == ',') pos++;
                break;
            }
        } else {
            outField.push_back(line[pos++]);
        }
    }
    return true;
}

// ------------------- SKILL PARSING -------------------
int buildSkillArray(const string &skillsLine, Skill arr[], int maxSkills) {
    int count = 0;
    string token;
    size_t i = 0, n = skillsLine.size();
    while (i <= n) {
        if (i == n || skillsLine[i] == ',') {
            string t = trim(token);
            if (!t.empty() && count < maxSkills) {
                arr[count].original = t;
                arr[count].norm = normalizeKey(t);
                ++count;
            }
            token.clear();
            ++i;
        } else {
            token.push_back(skillsLine[i++]);
        }
    }
    if (count == 0 && !trim(skillsLine).empty()) {
        string t = trim(skillsLine);
        if (!t.empty() && count < maxSkills) {
            arr[count].original = t;
            arr[count].norm = normalizeKey(t);
            ++count;
        }
    }
    return count;
}

// ------------------- LOADING CSVs -------------------
int loadJobsFromCSV(Job jobs[], int maxJobs, const string &filename) {
    ifstream fin(filename);
    if (!fin.is_open()) {
        cerr << "Error: cannot open job file '" << filename << "'\n";
        return 0;
    }
    string line;
    if (!getline(fin, line)) {
        fin.close();
        return 0;
    }
    int count = 0;
    while (getline(fin, line) && count < maxJobs) {
        if (trim(line).empty()) continue;
        size_t pos = 0;
        string field1, field2;
        if (!extractQuotedField(line, pos, field1)) continue;
        extractQuotedField(line, pos, field2);
        string jobOrig = trim(field1);
        string skillsOrig = trim(field2);
        jobs[count].titleOriginal = jobOrig;
        jobs[count].titleSortKey = makeTitleSortKey(jobOrig);
        jobs[count].skillsOriginal = skillsOrig;
        jobs[count].skillCount = buildSkillArray(skillsOrig, jobs[count].skills, MAX_SKILLS);
        ++count;
    }
    fin.close();
    return count;
}

int loadResumesFromCSV(Resume resumes[], int maxResumes, const string &filename) {
    ifstream fin(filename);
    if (!fin.is_open()) {
        cerr << "Error: cannot open resume file '" << filename << "'\n";
        return 0;
    }
    string line;
    getline(fin, line);
    getline(fin, line);
    int id = 1;
    int count = 0;
    while (getline(fin, line) && count < maxResumes) {
        size_t pos = 0;
        string skills;
        extractQuotedField(line, pos, skills);
        resumes[count].id = id++;
        resumes[count].skillsOriginal = trim(skills);
        resumes[count].skillCount = buildSkillArray(skills, resumes[count].skills, MAX_SKILLS);
        ++count;
    }
    fin.close();
    return count;
}

// ------------------- SELECTION SORT IMPLEMENTATIONS -------------------
void selectionSortJobs(Job arr[], int n) {
    for (int i = 0; i < n - 1; ++i) {
        int minIdx = i;
        for (int j = i + 1; j < n; ++j) {
            if (arr[j].titleSortKey < arr[minIdx].titleSortKey) {
                minIdx = j;
            }
        }
        if (minIdx != i) {
            Job tmp = arr[i];
            arr[i] = arr[minIdx];
            arr[minIdx] = tmp;
        }
    }
}

void selectionSortResumes(Resume arr[], int n) {
    for (int i = 0; i < n - 1; ++i) {
        int maxIdx = i;
        for (int j = i + 1; j < n; ++j) {
            if (arr[j].skillCount > arr[maxIdx].skillCount) {
                maxIdx = j;
            }
        }
        if (maxIdx != i) {
            Resume tmp = arr[i];
            arr[i] = arr[maxIdx];
            arr[maxIdx] = tmp;
        }
    }
}

void selectionSortResumesByID(Resume arr[], int n) {
    for (int i = 0; i < n - 1; ++i) {
        int minIdx = i;
        for (int j = i + 1; j < n; ++j) {
            if (arr[j].id < arr[minIdx].id) {  // Sort by ID ascending
                minIdx = j;
            }
        }
        if (minIdx != i) {
            Resume tmp = arr[i];
            arr[i] = arr[minIdx];
            arr[minIdx] = tmp;
        }
    }
}

void selectionSortCandidateScores(CandidateScore arr[], int n) {
    for (int i = 0; i < n - 1; ++i) {
        int maxIdx = i;
        for (int j = i + 1; j < n; ++j) {
            if (arr[j].score > arr[maxIdx].score) {
                maxIdx = j;
            }
        }
        if (maxIdx != i) {
            CandidateScore tmp = arr[i];
            arr[i] = arr[maxIdx];
            arr[maxIdx] = tmp;
        }
    }
}

void selectionSortJobCounts(JobCount arr[], int n) {
    for (int i = 0; i < n - 1; ++i) {
        int maxIdx = i;
        for (int j = i + 1; j < n; ++j) {
            if (arr[j].count > arr[maxIdx].count) {
                maxIdx = j;
            }
        }
        if (maxIdx != i) {
            JobCount tmp = arr[i];
            arr[i] = arr[maxIdx];
            arr[maxIdx] = tmp;
        }
    }
}

// ------------------- BINARY SEARCH IMPLEMENTATIONS -------------------
// Binary search for exact job title match (returns first occurrence)
int binarySearchJobExact(Job arr[], int n, const string &qSortKey) {
    int left = 0, right = n - 1;
    int result = -1;
    
    while (left <= right) {
        int mid = left + (right - left) / 2;
        
        if (arr[mid].titleSortKey == qSortKey) {
            result = mid;
            right = mid - 1; // Continue searching left for first occurrence
        } else if (arr[mid].titleSortKey < qSortKey) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    
    return result;
}

// Collect all jobs with exact match
int binarySearchJobsExact(Job arr[], int n, const string &qSortKey, int outIdxs[], int maxOut) {
    int firstIdx = binarySearchJobExact(arr, n, qSortKey);
    if (firstIdx == -1) return 0;
    
    int count = 0;
    // Collect all matches starting from first occurrence
    for (int i = firstIdx; i < n && count < maxOut; ++i) {
        if (arr[i].titleSortKey == qSortKey) {
            outIdxs[count++] = i;
        } else {
            break; // No more matches
        }
    }
    
    return count;
}

int binarySearchJobsPartial(Job arr[], int n, const string &qNorm, int outIdxs[], int maxOut) {
    if (n == 0 || qNorm.empty()) return 0;
    
    string qLower = toLowerCopy(qNorm);
    int count = 0;

    int left = 0, right = n - 1;
    int startIdx = 0;
    
    while (left <= right) {
        int mid = left + (right - left) / 2;
        string titleNorm = normalizeKey(arr[mid].titleOriginal);
        
        // Compare prefixes
        int cmp = titleNorm.compare(0, qLower.length(), qLower);
        
        if (cmp < 0) {
            left = mid + 1;
        } else {
            startIdx = mid;
            right = mid - 1;
        }
    }

    for (int i = startIdx; i < n && count < maxOut; ++i) {
        string titleNorm = normalizeKey(arr[i].titleOriginal);
        if (titleNorm.find(qLower) != string::npos) {
            outIdxs[count++] = i;
        }
        // Early termination: if title starts with a letter much greater than query, stop
        if (!titleNorm.empty() && !qLower.empty() && titleNorm[0] > qLower[qLower.length()-1] + 5) {
            break;
        }
    }
    
    // Scan backward from startIdx
    for (int i = startIdx - 1; i >= 0 && count < maxOut; --i) {
        string titleNorm = normalizeKey(arr[i].titleOriginal);
        if (titleNorm.find(qLower) != string::npos) {
            outIdxs[count++] = i;
        }
        // Early termination
        if (!titleNorm.empty() && !qLower.empty() && titleNorm[0] < qLower[0] - 5) {
            break;
        }
    }
    
    return count;
}

int binarySearchResumeById(Resume arr[], int n, int targetId) {
    int left = 0;
    int right = n - 1;
    
    while (left <= right) {
        int mid = left + (right - left) / 2;
        
        if (arr[mid].id == targetId) {
            return mid;  // Found at index mid
        } else if (arr[mid].id < targetId) {
            left = mid + 1;  // Search right half
        } else {
            right = mid - 1;  // Search left half
        }
    }
    
    return -1;  // Not found
}

// ------------------- MATCH / SCORE -------------------
int countMatchingSkillsWithSet(const Job &job, HashSet &resSkillSet) {
    if (job.skillCount == 0 || resSkillSet.empty()) return 0;
    int matches = 0;
    for (int i = 0; i < job.skillCount; ++i) {
        const string &jn = job.skills[i].norm;
        if (jn.empty()) continue;
        if (resSkillSet.contains(jn)) ++matches;
    }
    return matches;
}

int computeWeightedScoreWithSet(const Job &job, HashSet &resSkillSet) {
    if (job.skillCount == 0) return 0;
    int totalJob = job.skillCount;
    int matches = countMatchingSkillsWithSet(job, resSkillSet);
    double ratio = (double)matches / (double)totalJob;
    return (int)round(ratio * 100.0);
}

// Get candidate resumes using inverted index
void getCandidateResumesForJob(const Job &job, IntArray &candidateIndices) {
    candidateIndices.clear();
    
    // Use a temporary set to avoid duplicates
    static bool seen[MAX_RESUMES];
    for (int i = 0; i < MAX_RESUMES; ++i) seen[i] = false;
    
    for (int i = 0; i < job.skillCount; ++i) {
        const string &skillNorm = job.skills[i].norm;
        if (skillNorm.empty()) continue;
        
        IntArray* resumeList = skillToResumes.get(skillNorm);
        if (resumeList) {
            for (int j = 0; j < resumeList->size; ++j) {
                int ridx = resumeList->data[j];
                if (!seen[ridx]) {
                    seen[ridx] = true;
                    candidateIndices.push(ridx);
                }
            }
        }
    }
}

// ------------------- PRINT FIRST N -------------------
void printFirstNJobs(Job jobs[], int nJobs, int N) {
    int shown = 0;
    for (int i = 0; i < nJobs && shown < N; ++i, ++shown) {
        cout << (shown + 1) << ". " << jobs[i].titleOriginal << " | Skills: " << jobs[i].skillsOriginal << "\n";
    }
    if (shown == 0) cout << "(none)\n";
    cout << "\n";
}

void printFirstNResumes(Resume resumes[], int nResumes, int N) {
    int shown = 0;
    for (int i = 0; i < nResumes && shown < N; ++i, ++shown) {
        cout << (shown + 1) << ". Resume ID: " << resumes[i].id << " | Skills count: " << resumes[i].skillCount << "\n";
    }
    if (shown == 0) cout << "(none)\n";
    cout << "\n";
}

// ------------------- SEARCH FUNCTIONS -------------------
void searchByJobTitle(Job jobs[], int nJobs, Resume resumes[], int nResumes,
                      const string &queryRaw, const high_resolution_clock::time_point &globalStart, double globalMemStart) {
    cout << "[DEBUG] searchByJobTitle called with query: '" << queryRaw << "'\n";
    cout << "[DEBUG] nJobs=" << nJobs << ", nResumes=" << nResumes << "\n";
    cout << flush;
    
    auto stepStart = high_resolution_clock::now();
    double memStart = getMemoryUsageKB();

    cout << "Searching for jobs matching '" << queryRaw << "'...\n";
    cout << flush;
    
    string qNorm = normalizeKey(queryRaw);
    string qSortKey = makeTitleSortKey(queryRaw);
    int *resultsIdx = globalIdxArr;
    
    // Try binary search for exact match first
    int rcount = binarySearchJobsExact(jobs, nJobs, qSortKey, resultsIdx, MAX_RESULTS);
    
    // If no exact match, fall back to partial scan
    if (rcount == 0 && !qNorm.empty()) {
        rcount = binarySearchJobsPartial(jobs, nJobs, qNorm, resultsIdx, MAX_RESULTS);
    }

    cout << "Found " << rcount << " matching jobs.\n";

    if (rcount == 0) {
        cout << "No jobs found matching '" << queryRaw << "'.\n\n";
    } else {
        const int TOPC = 1000;

        // CRITICAL: Limit how many jobs we analyze to avoid timeout
        if (rcount > MAX_JOBS_TO_ANALYZE) {
            cout << "NOTE: Found " << rcount << " jobs, analyzing first " << MAX_JOBS_TO_ANALYZE << " for performance.\n";
            rcount = MAX_JOBS_TO_ANALYZE;
        }

        JobCount *jcArr = globalJobCountArr;
        int jcN = 0;

        cout << "Analyzing candidates for " << min(rcount, MAX_JOBS_TO_DISPLAY) << " jobs...\n";
        
        // For each matched job, get candidates using inverted index
        // LIMIT to first MAX_JOBS_TO_DISPLAY to avoid processing too many
        int jobsToProcess = min(rcount, MAX_JOBS_TO_DISPLAY);
        for (int ri = 0; ri < jobsToProcess && jcN < MAX_RESULTS; ++ri) {
            int jidx = resultsIdx[ri];
            
            if (ri % 10 == 0 && ri > 0) {
                cout << "  Processing job " << ri << "/" << jobsToProcess << "...\r" << flush;
            }
            
            IntArray candidateIndices;
            candidateIndices.init();
            getCandidateResumesForJob(jobs[jidx], candidateIndices);
            
            int totalMatched = 0;
            int bestId = 0;
            int bestScore = -1;
            
            for (int ci = 0; ci < candidateIndices.size; ++ci) {
                int r = candidateIndices.data[ci];
                if (resumes[r].skillCount == 0) continue;
                int sc = computeWeightedScoreWithSet(jobs[jidx], resumeSkillSets[r]);
                if (sc > 0) {
                    ++totalMatched;
                    if (sc > bestScore) { bestScore = sc; bestId = resumes[r].id; }
                }
            }
            
            jcArr[jcN].jobIndex = jidx;
            jcArr[jcN].count = totalMatched;
            jcArr[jcN].bestCandidateId = bestId;
            jcArr[jcN].bestCandidateScore = bestScore;
            ++jcN;
            
            candidateIndices.destroy();
        }

        cout << "\nSorting results...\n";
        if (jcN > 1) selectionSortJobCounts(jcArr, jcN);

        int displayed = 0;
        for (int k = 0; k < jcN && displayed < MAX_JOBS_TO_DISPLAY; ++k, ++displayed) {
            int jidx = jcArr[k].jobIndex;
            cout << "\nJob: " << jobs[jidx].titleOriginal << "\n";
            cout << "Total matched candidates: " << jcArr[k].count << "\n";

            cout << "  Gathering candidate scores...\n";
            
            IntArray candidateIndices;
            candidateIndices.init();
            getCandidateResumesForJob(jobs[jidx], candidateIndices);
            
            CandidateScore *candArr = globalCandArr;
            int candN = 0;
            
            for (int ci = 0; ci < candidateIndices.size && candN < MAX_RESUMES; ++ci) {
                int r = candidateIndices.data[ci];
                if (resumes[r].skillCount == 0) continue;
                int sc = computeWeightedScoreWithSet(jobs[jidx], resumeSkillSets[r]);
                if (sc > 0) {
                    candArr[candN].id = resumes[r].id;
                    candArr[candN].score = sc;
                    candArr[candN].jobIndex = jidx;
                    ++candN;
                }
            }

            if (candN > 1) selectionSortCandidateScores(candArr, candN);

            cout << "Top " << TOPC << " candidates:\n";
            for (int c = 0; c < candN && c < TOPC; ++c) {
                cout << c + 1 << ". candidate " << candArr[c].id << " : " << candArr[c].score << " score\n";
            }
            
            candidateIndices.destroy();
        }
        cout << "\n";
    }

    auto stepEnd = high_resolution_clock::now();
    double memEnd = getMemoryUsageKB();
    long long stepMs = duration_cast<milliseconds>(stepEnd - stepStart).count();
    long long cumMs = duration_cast<milliseconds>(stepEnd - globalStart).count();
    double stepMem = memEnd - memStart;
    printStepStatsSimple(stepMs, cumMs, stepMem, memEnd);
    cout << "\n";
}

void searchBySkill(Job jobs[], int nJobs, Resume resumes[], int nResumes,
                   const string &skillRaw, const high_resolution_clock::time_point &globalStart, double globalMemStart) {
    auto stepStart = high_resolution_clock::now();
    double memStart = getMemoryUsageKB();

    cout << "Searching for skill '" << skillRaw << "'...\n";

    string skillNorm = normalizeKey(skillRaw);
    JobCount *jcArr = globalJobCountArr;
    int jcN = 0;

    // Get resumes with this skill from inverted index
    IntArray* resumesWithSkill = skillToResumes.get(skillNorm);
    
    if (!resumesWithSkill || resumesWithSkill->size == 0) {
        cout << "No resumes found with that skill.\n\n";
        auto stepEnd = high_resolution_clock::now();
        double memEnd = getMemoryUsageKB();
        long long stepMs = duration_cast<milliseconds>(stepEnd - stepStart).count();
        long long cumMs = duration_cast<milliseconds>(stepEnd - globalStart).count();
        double stepMem = memEnd - memStart;
        printStepStatsSimple(stepMs, cumMs, stepMem, memEnd);
        cout << "\n";
        return;
    }

    cout << "Found " << resumesWithSkill->size << " resumes with this skill.\n";
    cout << "Matching with jobs...\n";

    for (int j = 0; j < nJobs && jcN < MAX_JOBS; ++j) {
        if (j % 1000 == 0 && j > 0) {
            cout << "  Processed " << j << "/" << nJobs << " jobs...\r" << flush;
        }
        
        bool jobHas = false;
        for (int s = 0; s < jobs[j].skillCount; ++s) {
            if (!skillNorm.empty() && jobs[j].skills[s].norm == skillNorm) { jobHas = true; break; }
        }
        if (!jobHas) continue;

        int cnt = 0;
        int bestId = 0;
        int bestScore = -1;
        
        for (int ri = 0; ri < resumesWithSkill->size; ++ri) {
            int r = resumesWithSkill->data[ri];
            ++cnt;
            int sc = computeWeightedScoreWithSet(jobs[j], resumeSkillSets[r]);
            if (sc > bestScore) { bestScore = sc; bestId = resumes[r].id; }
        }

        jcArr[jcN].jobIndex = j;
        jcArr[jcN].count = cnt;
        jcArr[jcN].bestCandidateId = bestId;
        jcArr[jcN].bestCandidateScore = bestScore;
        ++jcN;
    }

    cout << "\nFound " << jcN << " jobs with this skill.\n";
    cout << "Sorting results...\n";
    
    if (jcN > 1) selectionSortJobCounts(jcArr, jcN);

    const int TOPJ = 1000;
    cout << "Top " << TOPJ << " jobs related to skill '" << skillRaw << "':\n";
    if (jcN == 0) {
        cout << "No jobs found with that skill.\n\n";
    } else {
        int shown = 0;
        for (int i = 0; i < jcN && shown < TOPJ; ++i, ++shown) {
            int jid = jcArr[i].jobIndex;
            cout << shown + 1 << ". " << jobs[jid].titleOriginal << " | Total matched: " << jcArr[i].count;
            if (jcArr[i].bestCandidateScore > 0) {
                cout << " | Best candidate: " << jcArr[i].bestCandidateId << " | Score: " << jcArr[i].bestCandidateScore;
            } else {
                cout << " | Best candidate: None";
            }
            cout << "\n";
        }
        cout << "\n";
    }

    auto stepEnd = high_resolution_clock::now();
    double memEnd = getMemoryUsageKB();
    long long stepMs = duration_cast<milliseconds>(stepEnd - stepStart).count();
    long long cumMs = duration_cast<milliseconds>(stepEnd - globalStart).count();
    double stepMem = memEnd - memStart;
    printStepStatsSimple(stepMs, cumMs, stepMem, memEnd);
    cout << "\n";
}

void searchByCandidateID(Job jobs[], int nJobs, Resume resumes[], int nResumes,
                         Resume resumesByID[], // ADD THIS PARAMETER
                         int candId, const high_resolution_clock::time_point &globalStart, double globalMemStart) {
    auto stepStart = high_resolution_clock::now();
    double memStart = getMemoryUsageKB();

    // Use binary search on ID-sorted array
    int ridxByID = binarySearchResumeById(resumesByID, nResumes, candId);
    
    if (ridxByID == -1) {
        cout << "Candidate ID " << candId << " not found.\n\n";
        auto stepEnd = high_resolution_clock::now();
        double memEnd = getMemoryUsageKB();
        long long stepMs = duration_cast<milliseconds>(stepEnd - stepStart).count();
        long long cumMs = duration_cast<milliseconds>(stepEnd - globalStart).count();
        double stepMem = memEnd - memStart;
        printStepStatsSimple(stepMs, cumMs, stepMem, memEnd);
        cout << "\n";
        return;
    }

    // Find the same resume in the skill-sorted array (for accessing resumeSkillSets)
    int ridx = -1;
    for (int i = 0; i < nResumes; ++i) {
        if (resumes[i].id == candId) {
            ridx = i;
            break;
        }
    }

    CandidateScore *jmArr = globalJMArr;
    int jmN = 0;
    for (int j = 0; j < nJobs && jmN < MAX_JOBS; ++j) {
        int sc = computeWeightedScoreWithSet(jobs[j], resumeSkillSets[ridx]);
        if (sc > 0) {
            jmArr[jmN].id = 0;
            jmArr[jmN].score = sc;
            jmArr[jmN].jobIndex = j;
            ++jmN;
        }
    }

    if (jmN > 1) selectionSortCandidateScores(jmArr, jmN);

    const int TOPJ = 1000;
    cout << "Top " << TOPJ << " job matches for candidate " << candId << ":\n";
    if (jmN == 0) {
        cout << "(no matching jobs)\n\n";
    } else {
        int shown = 0;
        for (int i = 0; i < jmN && shown < TOPJ; ++i, ++shown) {
            int jidx = jmArr[i].jobIndex;
            cout << shown + 1 << ". " << jobs[jidx].titleOriginal << " | Score: " << jmArr[i].score << "\n";
        }
        cout << "\n";
    }

    auto stepEnd = high_resolution_clock::now();
    double memEnd = getMemoryUsageKB();
    long long stepMs = duration_cast<milliseconds>(stepEnd - stepStart).count();
    long long cumMs = duration_cast<milliseconds>(stepEnd - globalStart).count();
    double stepMem = memEnd - memStart;
    printStepStatsSimple(stepMs, cumMs, stepMem, memEnd);
    cout << "\n";
}

// ------------------- MAIN -------------------
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    auto globalStart = high_resolution_clock::now();
    double globalMemStart = getMemoryUsageKB();

    cout << "[1/6] Loading jobs from job_grouped.csv...\n";
    auto s1 = high_resolution_clock::now();
    double m1s = getMemoryUsageKB();
    static Job jobs[MAX_JOBS];
    int jobCount = loadJobsFromCSV(jobs, MAX_JOBS, "job_grouped.csv");
    auto e1 = high_resolution_clock::now();
    double m1e = getMemoryUsageKB();
    cout << "Loaded " << jobCount << " jobs.\n";
    printStepStatsSimple(duration_cast<milliseconds>(e1 - s1).count(),
                         duration_cast<milliseconds>(e1 - globalStart).count(),
                         m1e - m1s, m1e);
    cout << "\n";

    cout << "[2/6] Loading resumes from resume_cleaned.csv...\n";
    auto s2 = high_resolution_clock::now();
    double m2s = getMemoryUsageKB();
    static Resume resumes[MAX_RESUMES];
    int resumeCount = loadResumesFromCSV(resumes, MAX_RESUMES, "resume_cleaned.csv");
    auto e2 = high_resolution_clock::now();
    double m2e = getMemoryUsageKB();
    cout << "Loaded " << resumeCount << " resumes.\n";
    printStepStatsSimple(duration_cast<milliseconds>(e2 - s2).count(),
                         duration_cast<milliseconds>(e2 - globalStart).count(),
                         m2e - m2s, m2e);
    cout << "\n";

    cout << "[2b/6] Creating ID-sorted copy of resumes for binary search...\n";
    auto s2b = high_resolution_clock::now();
    double m2bs = getMemoryUsageKB();
    
    static Resume resumesByID[MAX_RESUMES];  // Separate copy sorted by ID
    for (int i = 0; i < resumeCount; ++i) {
        resumesByID[i] = resumes[i];
    }
    
    if (resumeCount > 1) {
        selectionSortResumesByID(resumesByID, resumeCount);
    }
    
    auto e2b = high_resolution_clock::now();
    double m2be = getMemoryUsageKB();
    cout << "Created and sorted ID-indexed copy with " << resumeCount << " resumes.\n";
    printStepStatsSimple(duration_cast<milliseconds>(e2b - s2b).count(),
                         duration_cast<milliseconds>(e2b - globalStart).count(),
                         m2be - m2bs, m2be);
    cout << "\n";

    // Build resume skill sets AND inverted index (fully manual)
    {
        auto srs = high_resolution_clock::now();
        cout << "[Indexing] Building resume skill sets and inverted index...\n";
        
        skillToResumes.init();
        
        int totalSkillsIndexed = 0;
        for (int i = 0; i < resumeCount; ++i) {
            if (i % 1000 == 0) {
                cout << "  Indexing resume " << i << "/" << resumeCount << "...\r" << flush;
            }
            resumeSkillSets[i].init();
            for (int s = 0; s < resumes[i].skillCount; ++s) {
                const string &k = resumes[i].skills[s].norm;
                if (!k.empty()) {
                    resumeSkillSets[i].insert(k);
                    // Build inverted index
                    IntArray* arr = skillToResumes.getOrCreate(k);
                    arr->push(i);
                    totalSkillsIndexed++;
                }
            }
        }
        
        auto ers = high_resolution_clock::now();
        long long dt = duration_cast<milliseconds>(ers - srs).count();
        cout << "\n[Indexing] Built indexes in " << dt << " ms.\n";
        cout << "[Indexing] Total skill entries indexed: " << totalSkillsIndexed << "\n";
    }
    cout << "\n";

    cout << "[3/6] Sorting jobs (title asc) using selection sort...\n";
    auto s3 = high_resolution_clock::now();
    double m3s = getMemoryUsageKB();
    if (jobCount > 1) selectionSortJobs(jobs, jobCount);
    auto e3 = high_resolution_clock::now();
    double m3e = getMemoryUsageKB();
    cout << "Sorted jobs. Displaying first 1000:\n";
    printFirstNJobs(jobs, jobCount, 1000);
    printStepStatsSimple(duration_cast<milliseconds>(e3 - s3).count(),
                         duration_cast<milliseconds>(e3 - globalStart).count(),
                         m3e - m3s, m3e);
    cout << "\n";

    cout << "[4/6] Sorting resumes (skill count desc) using selection sort...\n";
    auto s4 = high_resolution_clock::now();
    double m4s = getMemoryUsageKB();
    if (resumeCount > 1) selectionSortResumes(resumes, resumeCount);
    auto e4 = high_resolution_clock::now();
    double m4e = getMemoryUsageKB();
    cout << "Sorted resumes. Displaying first 1000:\n";
    printFirstNResumes(resumes, resumeCount, 1000);
    printStepStatsSimple(duration_cast<milliseconds>(e4 - s4).count(),
                         duration_cast<milliseconds>(e4 - globalStart).count(),
                         m4e - m4s, m4e);
    cout << "\n";

    cout << "[5/6] Ready. Matching occurs at search time.\n";
    auto s5 = high_resolution_clock::now();
    double m5s = getMemoryUsageKB();
    auto e5 = high_resolution_clock::now();
    double m5e = getMemoryUsageKB();
    printStepStatsSimple(duration_cast<milliseconds>(e5 - s5).count(),
                         duration_cast<milliseconds>(e5 - globalStart).count(),
                         m5e - m5s, m5e);
    cout << "\n";

    cout << "[6/6] Entering interactive menu.\n\n";
    
    // Add a small delay and flush to ensure all output is visible
    cout << flush;
    
    while (true) {
        cout << "================== MENU ==================\n";
        cout << "===== BINARY SEARCH & SELECTION SORT =====\n";
        cout << "1. Search by Job Title\n2. Search by Skill\n3. Search by Candidate ID\n4. Exit\nEnter choice: ";
        cout << flush; // Force output
        
        int choice;
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Please enter a number.\n\n";
            continue;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        cout << flush;

        if (choice == 1) {
            cout << "Enter job title or keyword: ";
            cout << flush; // Force output before waiting for input
            
            string q;
            getline(cin, q);
                        
            if (trim(q).empty()) {
                cout << "Empty input. Please type a job title or keyword.\n\n";
                continue;
            }
            searchByJobTitle(jobs, jobCount, resumes, resumeCount, q, globalStart, globalMemStart);
        } else if (choice == 2) {
            cout << "Enter skill: ";
            cout << flush;
            
            string sk;
            getline(cin, sk);
                        
            if (trim(sk).empty()) {
                cout << "Empty input. Please type a skill.\n\n";
                continue;
            }
            searchBySkill(jobs, jobCount, resumes, resumeCount, sk, globalStart, globalMemStart);
        } else if (choice == 3) {
            cout << "Enter candidate ID (integer): ";
            cout << flush;
            
            int cid;
            if (!(cin >> cid)) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid candidate id.\n\n";
                continue;
            }
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            
            // Pass resumesByID array for binary search
            searchByCandidateID(jobs, jobCount, resumes, resumeCount, resumesByID, cid, globalStart, globalMemStart);
        } else if (choice == 4) {
            cout << "Exiting program.\n";
            break;
        } else {
            cout << "Invalid option.\n\n";
        }
    }

    // Cleanup
    cout << "Cleaning up memory...\n";
    for (int i = 0; i < resumeCount; ++i) {
        resumeSkillSets[i].destroy();
    }
    skillToResumes.destroy();

    return 0;
}