// linked_listA.cpp
// Complete implementation with Linear Search and QuickSort
// Compile with: clang++ -std=c++17 linked_listA.cpp -o linked_listA
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <cctype>
#include <cmath>
#include <unistd.h>
#include <limits>
#include <sys/resource.h>

#ifdef __APPLE__
#include <mach/mach.h>
#endif

using namespace std;
using namespace std::chrono;

// ----------------- Data Structures -----------------
struct SkillNode {
    string skillOriginal;
    string skillNorm;
    SkillNode* next;
    SkillNode(const string &orig, const string &norm) : skillOriginal(orig), skillNorm(norm), next(nullptr) {}
};

struct Resume {
    int id;
    SkillNode* skills;
    string skillsOriginal;
    string normKey;
    int skillCount;
    Resume* next;
    Resume(int i) : id(i), skills(nullptr), skillsOriginal(""), normKey(""), skillCount(0), next(nullptr) {}
};

struct Job {
    string titleOriginal;
    string titleSortKey;
    SkillNode* skills;
    string skillsOriginal;
    int totalMatched;
    Job* next;
    Job(const string &tOrig, const string &tSortKey, const string &sOrig) 
        : titleOriginal(tOrig), titleSortKey(tSortKey), skills(nullptr), skillsOriginal(sOrig), totalMatched(0), next(nullptr) {}
};

struct CandidateScore {
    int id;
    int score;
    Job* jobPtr;
    CandidateScore* next;
    CandidateScore(int i, int s) : id(i), score(s), jobPtr(nullptr), next(nullptr) {}
};

struct JobCount {
    Job* jobPtr;
    int count;
    JobCount* next;
    JobCount(Job* j, int c) : jobPtr(j), count(c), next(nullptr) {}
};

// ----------------- Utility Functions -----------------
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

string makeTitleSortKey(const string &title) {
    string t = title;
    size_t end = t.find_last_not_of(" \t\r\n");
    if (end != string::npos) t = t.substr(0, end+1);
    else t = "";

    int i = (int)t.size() - 1;
    while (i >= 0 && isspace((unsigned char)t[i])) --i;
    int endDigits = i;
    while (i >= 0 && isdigit((unsigned char)t[i])) --i;
    int startDigits = i + 1;
    string prefix = (startDigits <= endDigits) ? t.substr(0, startDigits) : t;
    string digits = (startDigits <= endDigits) ? t.substr(startDigits, endDigits - startDigits + 1) : "";

    string prefLower;
    bool prevSpace = false;
    for (char c : prefix) {
        if (isspace((unsigned char)c)) {
            if (!prevSpace) { prefLower.push_back(' '); prevSpace = true; }
        } else {
            prefLower.push_back((char)tolower((unsigned char)c));
            prevSpace = false;
        }
    }
    size_t a = prefLower.find_first_not_of(' ');
    size_t b = prefLower.find_last_not_of(' ');
    string prefTrim = (a==string::npos) ? string("") : prefLower.substr(a, b-a+1);

    string key;
    if (digits.empty()) {
        key = prefTrim + "|0";
    } else {
        const int W = 10;
        string padded(W - (int)digits.size(), '0');
        padded += digits;
        key = prefTrim + "|1" + padded;
    }
    return key;
}

double getMemoryUsageKB() {
#ifdef __linux__
    FILE* file = fopen("/proc/self/status", "r");
    if (file) {
        char line[128];
        while (fgets(line, 128, file)) {
            if (strncmp(line, "VmRSS:", 6) == 0) {
                long rss = 0;
                sscanf(line + 6, "%ld", &rss);
                fclose(file);
                return (double)rss;
            }
        }
        fclose(file);
    }
#elif defined(__APPLE__) && defined(__MACH__)
    struct mach_task_basic_info info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &count) == KERN_SUCCESS) {
        return info.resident_size / 1024.0;
    }
#endif
    
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
#if defined(__APPLE__) && defined(__MACH__)
        return usage.ru_maxrss / 1024.0;
#else
        return (double)usage.ru_maxrss;
#endif
    }
    return 0.0;
}

void printStepStatsSimple(long long stepMs, long long cumMs, double stepMemKB, double totalMemKB) {
    cout << "Step Time: " << stepMs << " ms | Cumulative Time: " << cumMs << " ms\n";
    
    // Convert KB to bytes for display
    long long stepBytes = (long long)(stepMemKB * 1024.0);
    long long totalBytes = (long long)(totalMemKB * 1024.0);
    
    cout << "Step Memory Change: " << stepBytes << " bytes | Current Total Memory: " << totalBytes << " bytes\n";
}

string trim(const string &s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

SkillNode* buildSkillListFromCSV(const string &skillsLine) {
    SkillNode* head = nullptr;
    SkillNode* tail = nullptr;
    size_t i = 0, n = skillsLine.size();
    string token;
    
    while (i <= n) {
        if (i == n || skillsLine[i] == ',') {
            string t = trim(token);
            if (!t.empty()) {
                string norm = normalizeKey(t);
                SkillNode* node = new SkillNode(t, norm);
                if (!head) { 
                    head = tail = node; 
                } else { 
                    tail->next = node; 
                    tail = node; 
                }
            }
            token.clear();
            i++;
        } else {
            token.push_back(skillsLine[i++]);
        }
    }
    
    // If no skills were added but skillsLine wasn't empty, treat whole string as one skill
    if (!head && !skillsLine.empty()) {
        string t = trim(skillsLine);
        if (!t.empty()) {
            string norm = normalizeKey(t);
            head = new SkillNode(t, norm);
        }
    }
    
    return head;
}

int countSkills(SkillNode* head) {
    int c=0;
    while (head) { c++; head=head->next; }
    return c;
}

int countMatchingSkills(SkillNode* jobSkills, SkillNode* resumeSkills) {
    int matches = 0;
    for (SkillNode* r = resumeSkills; r; r = r->next) {
        for (SkillNode* j = jobSkills; j; j = j->next) {
            if (!r->skillNorm.empty() && r->skillNorm == j->skillNorm) { matches++; break; }
        }
    }
    return matches;
}

void pushResume(Resume*& head, Resume* node) {
    node->next = head;
    head = node;
}

void pushJob(Job*& head, Job* node) {
    node->next = head;
    head = node;
}

void pushCandidateScore(CandidateScore*& head, int id, int score) {
    CandidateScore* node = new CandidateScore(id, score);
    node->next = head;
    head = node;
}

void pushJobCount(JobCount*& head, Job* job, int count) {
    JobCount* node = new JobCount(job, count);
    node->next = head;
    head = node;
}

// ----------------- Loaders -----------------
bool extractQuotedField(const string &line, size_t &pos, string &out) {
    out.clear();
    size_t n=line.size();
    while (pos<n && isspace((unsigned char)line[pos])) pos++;
    if (pos>=n) return false;
    if (line[pos]!='"'){
        size_t comma=line.find(',',pos);
        if (comma==string::npos){out=trim(line.substr(pos));pos=n;}
        else{out=trim(line.substr(pos,comma-pos));pos=comma+1;}
        return true;
    }
    pos++;
    while (pos<n){
        if (line[pos]=='"'){
            if (pos+1<n && line[pos+1]=='"'){out.push_back('"');pos+=2;}
            else{pos++;if(pos<n && line[pos]==',')pos++;break;}
        }else out.push_back(line[pos++]);
    }
    return true;
}

int loadJobsFromCSV(Job*& head,const string &fn){
    ifstream fin(fn);if(!fin.is_open()){cerr<<"Error: cannot open job file '"<<fn<<"'\n";return 0;}
    string line;getline(fin,line); // header
    int count=0;
    while(getline(fin,line)){
        if(trim(line).empty()) continue;
        size_t pos=0; string f1,f2;
        if(!extractQuotedField(line,pos,f1)) continue;
        extractQuotedField(line,pos,f2);
        Job* j=new Job(trim(f1), makeTitleSortKey(f1), trim(f2));
        j->skills = buildSkillListFromCSV(j->skillsOriginal);
        j->next = head; head = j; count++;
    }
    fin.close();
    return count;
}

int loadResumesFromCSV(Resume*& head,const string &fn){
    ifstream fin(fn);if(!fin.is_open()){cerr<<"Error: cannot open resume file '"<<fn<<"'\n";return 0;}
    string line; int id=1, count=0;
    // skip two header lines as in original
    if(getline(fin,line)) { /* skipped */ }
    if(getline(fin,line)) { /* skipped */ }
    while(getline(fin,line)){
        string skills; size_t pos=0;
        extractQuotedField(line,pos,skills);
        Resume* r=new Resume(id++);
        r->skillsOriginal = skills;
        r->skills = buildSkillListFromCSV(skills);
        r->normKey = normalizeKey(skills);
        r->skillCount = countSkills(r->skills);
        r->next = head; head = r; count++;
    }
    fin.close();
    return count;
}
// ----------------- QuickSort for Jobs -----------------
Job* getJobTail(Job* cur) { while (cur && cur->next) cur = cur->next; return cur; }
Job* partitionJob(Job* head, Job* end, Job** newHead, Job** newEnd) {
    Job* pivot = end;
    Job* prev = nullptr, *cur = head, *tail = pivot;
    while (cur != pivot) {
        if (cur->titleSortKey < pivot->titleSortKey) {
            if (!(*newHead)) *newHead = cur;
            prev = cur;
            cur = cur->next;
        } else {
            if (prev) prev->next = cur->next;
            Job* tmp = cur->next;
            cur->next = nullptr;
            tail->next = cur;
            tail = cur;
            cur = tmp;
        }
    }
    if (!(*newHead)) *newHead = pivot;
    *newEnd = tail;
    return pivot;
}
Job* quickSortJobRecur(Job* head, Job* end) {
    if (!head || head == end) return head;
    Job *newHead = nullptr, *newEnd = nullptr;
    Job* pivot = partitionJob(head, end, &newHead, &newEnd);
    if (newHead != pivot) {
        Job* tmp = newHead;
        while (tmp->next != pivot) tmp = tmp->next;
        tmp->next = nullptr;
        newHead = quickSortJobRecur(newHead, tmp);
        tmp = getJobTail(newHead);
        tmp->next = pivot;
    }
    pivot->next = quickSortJobRecur(pivot->next, newEnd);
    return newHead;
}

void quickSortJobs(Job** headRef) {
    if (!headRef || !(*headRef)) return;
    *headRef = quickSortJobRecur(*headRef, getJobTail(*headRef));
}

// ----------------- QuickSort for Resumes (by skill count, descending) -----------------
Resume* getResumeTail(Resume* cur) { while (cur && cur->next) cur = cur->next; return cur; }
Resume* partitionResume(Resume* head, Resume* end, Resume** newHead, Resume** newEnd) {
    Resume* pivot = end;
    Resume* prev = nullptr, *cur = head, *tail = pivot;
    while (cur != pivot) {
        // Sort by skillCount descending (greater skill count comes first)
        if (cur->skillCount > pivot->skillCount) {
            if (!(*newHead)) *newHead = cur;
            prev = cur;
            cur = cur->next;
        } else {
            if (prev) prev->next = cur->next;
            Resume* tmp = cur->next;
            cur->next = nullptr;
            tail->next = cur;
            tail = cur;
            cur = tmp;
        }
    }
    if (!(*newHead)) *newHead = pivot;
    *newEnd = tail;
    return pivot;
}

Resume* quickSortResumeRecur(Resume* head, Resume* end) {
    if (!head || head == end) return head;
    Resume *newHead = nullptr, *newEnd = nullptr;
    Resume* pivot = partitionResume(head, end, &newHead, &newEnd);
    if (newHead != pivot) {
        Resume* tmp = newHead;
        while (tmp->next != pivot) tmp = tmp->next;
        tmp->next = nullptr;
        newHead = quickSortResumeRecur(newHead, tmp);
        tmp = getResumeTail(newHead);
        tmp->next = pivot;
    }
    pivot->next = quickSortResumeRecur(pivot->next, newEnd);
    return newHead;
}

void quickSortResumes(Resume** headRef) {
    if (!headRef || !(*headRef)) return;
    *headRef = quickSortResumeRecur(*headRef, getResumeTail(*headRef));
}

// ----------------- QuickSort for CandidateScore -----------------
CandidateScore* getCandidateTail(CandidateScore* cur) {
    while (cur && cur->next) cur = cur->next;
    return cur;
}
CandidateScore* partitionCandidateScore(CandidateScore* head, CandidateScore* end, 
                                         CandidateScore** newHead, CandidateScore** newEnd) {
    CandidateScore* pivot = end;
    CandidateScore* prev = nullptr, *cur = head, *tail = pivot;
    while (cur != pivot) {
        if (cur->score > pivot->score) {
            if (!(*newHead)) *newHead = cur;
            prev = cur;
            cur = cur->next;
        } else {
            if (prev) prev->next = cur->next;
            CandidateScore* tmp = cur->next;
            cur->next = nullptr;
            tail->next = cur;
            tail = cur;
            cur = tmp;
        }
    }
    if (!(*newHead)) *newHead = pivot;
    *newEnd = tail;
    return pivot;
}

CandidateScore* quickSortCandidateRecur(CandidateScore* head, CandidateScore* end) {
    if (!head || head == end) return head;
    CandidateScore *newHead = nullptr, *newEnd = nullptr;
    CandidateScore* pivot = partitionCandidateScore(head, end, &newHead, &newEnd);
    if (newHead != pivot) {
        CandidateScore* tmp = newHead;
        while (tmp->next != pivot) tmp = tmp->next;
        tmp->next = nullptr;
        newHead = quickSortCandidateRecur(newHead, tmp);
        tmp = getCandidateTail(newHead);
        tmp->next = pivot;
    }
    pivot->next = quickSortCandidateRecur(pivot->next, newEnd);
    return newHead;
}

void quickSortCandidates(CandidateScore** headRef) {
    if (!headRef || !(*headRef)) return;
    *headRef = quickSortCandidateRecur(*headRef, getCandidateTail(*headRef));
}

// ----------------- QuickSort for JobCount -----------------
JobCount* getJobCountTail(JobCount* cur) {
    while (cur && cur->next) cur = cur->next;
    return cur;
}

JobCount* partitionJobCount(JobCount* head, JobCount* end, 
                             JobCount** newHead, JobCount** newEnd) {
    JobCount* pivot = end;
    JobCount* prev = nullptr, *cur = head, *tail = pivot;
    while (cur != pivot) {
        if (cur->count > pivot->count) {
            if (!(*newHead)) *newHead = cur;
            prev = cur;
            cur = cur->next;
        } else {
            if (prev) prev->next = cur->next;
            JobCount* tmp = cur->next;
            cur->next = nullptr;
            tail->next = cur;
            tail = cur;
            cur = tmp;
        }
    }
    if (!(*newHead)) *newHead = pivot;
    *newEnd = tail;
    return pivot;
}

JobCount* quickSortJobCountRecur(JobCount* head, JobCount* end) {
    if (!head || head == end) return head;
    JobCount *newHead = nullptr, *newEnd = nullptr;
    JobCount* pivot = partitionJobCount(head, end, &newHead, &newEnd);
    if (newHead != pivot) {
        JobCount* tmp = newHead;
        while (tmp->next != pivot) tmp = tmp->next;
        tmp->next = nullptr;
        newHead = quickSortJobCountRecur(newHead, tmp);
        tmp = getJobCountTail(newHead);
        tmp->next = pivot;
    }
    pivot->next = quickSortJobCountRecur(pivot->next, newEnd);
    return newHead;
}

void quickSortJobCounts(JobCount** headRef) {
    if (!headRef || !(*headRef)) return;
    *headRef = quickSortJobCountRecur(*headRef, getJobCountTail(*headRef));
}

// ----------------- Delete list helpers -----------------
void deleteCandidateList(CandidateScore* head) {
    while (head) {
        CandidateScore* tmp = head;
        head = head->next;
        delete tmp;
    }
}

void deleteJobCountList(JobCount* head) {
    while (head) {
        JobCount* tmp = head;
        head = head->next;
        delete tmp;
    }
}

// ----------------- Linear Search Functions -----------------
Job* linearSearchJobsExact(Job* jobHead, const string &qSortKey) {
    Job* results = nullptr;
    Job* tail = nullptr;
    for (Job* cur = jobHead; cur; cur = cur->next) {
        if (cur->titleSortKey == qSortKey) {
            Job* copy = new Job(cur->titleOriginal, cur->titleSortKey, cur->skillsOriginal);
            copy->skills = cur->skills;
            if (!results) {
                results = tail = copy;
            } else {
                tail->next = copy;
                tail = copy;
            }
        }
    }
    return results;
}

Job* linearSearchJobsPartial(Job* jobHead, const string &qNorm) {
    Job* results = nullptr;
    Job* tail = nullptr;
    for (Job* cur = jobHead; cur; cur = cur->next) {
        string titleNorm = normalizeKey(cur->titleOriginal);
        if (titleNorm.find(qNorm) != string::npos) {
            Job* copy = new Job(cur->titleOriginal, cur->titleSortKey, cur->skillsOriginal);
            copy->skills = cur->skills;
            if (!results) {
                results = tail = copy;
            } else {
                tail->next = copy;
                tail = copy;
            }
        }
    }
    return results;
}

Resume* linearSearchResume(Resume* head, int targetId) {
    for (Resume* r = head; r; r = r->next) {
        if (r->id == targetId) return r;
    }
    return nullptr;
}

// ----------------- Helper: print first N -----------------
void printFirstNJobs(Job* head, int N) {
    Job* cur = head;
    int shown = 0;
    while (cur && shown < N) {
        cout << (shown+1) << ". " << cur->titleOriginal << " | Skills: " << cur->skillsOriginal << "\n";
        cur = cur->next;
        shown++;
    }
    if (shown == 0) cout << "(none)\n";
    cout << "\n";
}

void printFirstNResumes(Resume* head, int N) {
    Resume* cur = head;
    int shown = 0;
    while (cur && shown < N) {
        cout << (shown+1) << ". Resume ID: " << cur->id << " | Skills count: " << cur->skillCount << "\n";
        cur = cur->next;
        shown++;
    }
    if (shown == 0) cout << "(none)\n";
    cout << "\n";
}

// ----------------- Matching logic -----------------
int computeWeightedScore(Job* job, Resume* resume) {
    if (!job->skills) return 0;
    int totalJob = countSkills(job->skills);
    if (totalJob == 0) return 0;
    int matches = countMatchingSkills(job->skills, resume->skills);
    double ratio = (double)matches / (double)totalJob;
    int score = (int)round(ratio * 100.0);
    return score;
}

CandidateScore* getTopCandidates(Resume* resumeHead, Job* job) {
    CandidateScore* head = nullptr;
    for (Resume* r = resumeHead; r; r = r->next) {
        int sc = computeWeightedScore(job, r);
        if (sc > 0) {
            pushCandidateScore(head, r->id, sc);
        }
    }
    quickSortCandidates(&head);
    return head;
}

int countList(CandidateScore* head) {
    int c = 0;
    while (head) { c++; head = head->next; }
    return c;
}

// ----------------- Interactive Search Functions -----------------
void searchByJobTitle(Job* jobHead, Resume* resumeHead, const string &queryRaw, 
                      const high_resolution_clock::time_point &globalStart, double globalMemStart) {
    auto stepStart = high_resolution_clock::now();
    double memStart = getMemoryUsageKB();

    string qNorm = normalizeKey(queryRaw);
    string qSortKey = makeTitleSortKey(queryRaw);

    // Original linear searches
    Job* results = linearSearchJobsExact(jobHead, qSortKey);
    if (!results && !qNorm.empty()) {
        results = linearSearchJobsPartial(jobHead, qNorm);
    }

    if (!results) {
        cout << "No jobs found matching '" << queryRaw << "'.\n\n";
    } else {
        const int MAX_DISPLAY = 5;
        const int TOPC = 50;

        // Step 1: build temporary JobCount list
        JobCount* jcHead = nullptr;
        JobCount* jcTail = nullptr;

        for (Job* j = results; j; j = j->next) {
            CandidateScore* candidates = getTopCandidates(resumeHead, j);
            int totalMatched = 0;
            for (CandidateScore* c = candidates; c; c = c->next) totalMatched++;

            JobCount* node = new JobCount(j, totalMatched);
            if (!jcHead) jcHead = jcTail = node;
            else { jcTail->next = node; jcTail = node; }

            deleteCandidateList(candidates);
        }

        // Step 2: sort by count descending (reuse your QuickSort/mergeSortJobCounts)
        quickSortJobCounts(&jcHead);  // or quickSortJobCounts(&jcHead); depending on your A version

        // Step 3: print top 5 jobs by matched candidates
        int displayed = 0;
        for (JobCount* jc = jcHead; jc && displayed < MAX_DISPLAY; jc = jc->next, displayed++) {
            Job* j = jc->jobPtr;
            cout << "Job: " << j->titleOriginal << "\n";
            cout << "Total matched candidates: " << jc->count << "\n";

            CandidateScore* candidates = getTopCandidates(resumeHead, j);
            cout << "Top " << TOPC << " candidates:\n";

            int shown = 0;
            for (CandidateScore* c = candidates; c && shown < TOPC; c = c->next, shown++) {
                cout << shown + 1 << ". candidate " << c->id << " : " << c->score << " score\n";
            }
            cout << "\n";

            deleteCandidateList(candidates);
        }

        deleteJobCountList(jcHead);

        // Step 4: free copied results
        while (results) {
            Job* tmp = results;
            results = results->next;
            delete tmp;
        }
    }

    // Step 5: time and memory stats
    auto stepEnd = high_resolution_clock::now();
    double memEnd = getMemoryUsageKB();
    long long stepMs = duration_cast<milliseconds>(stepEnd - stepStart).count();
    long long cumMs  = duration_cast<milliseconds>(stepEnd - globalStart).count();
    double stepMem = memEnd - memStart;
    printStepStatsSimple(stepMs, cumMs, stepMem, memEnd);
    cout << "\n";
}

void searchBySkill(Job* jobHead, Resume* resumeHead, const string &skillRaw,
                   const high_resolution_clock::time_point &globalStart, double globalMemStart) {
    auto stepStart = high_resolution_clock::now();
    double memStart = getMemoryUsageKB();

    string skillNorm = normalizeKey(skillRaw);
    JobCount* jobList = nullptr;

    // Count how many resumes have this searched skill
    int resumesWithSkill = 0;
    for (Resume* r = resumeHead; r; r = r->next) {
        for (SkillNode* s = r->skills; s; s = s->next) {
            if (!skillNorm.empty() && s->skillNorm == skillNorm) {
                resumesWithSkill++;
                break;
            }
        }
    }

    // For each job that contains the searched skill
    for (Job* j = jobHead; j; j = j->next) {
        bool jobHas = false;
        for (SkillNode* s = j->skills; s; s = s->next) {
            if (!skillNorm.empty() && s->skillNorm == skillNorm) { 
                jobHas = true; 
                break; 
            }
        }
        if (!jobHas) continue;

        // Count how many resumes match this job overall (by all job skills)
        int cnt = 0;
        for (Resume* r = resumeHead; r; r = r->next) {
            bool matched = false;
            for (SkillNode* js = j->skills; js; js = js->next) {
                for (SkillNode* rs = r->skills; rs; rs = rs->next) {
                    if (js->skillNorm == rs->skillNorm) {
                        matched = true;
                        break;
                    }
                }
                if (matched) break;
            }
            if (matched) cnt++;
        }

        // Push into job list
        pushJobCount(jobList, j, cnt);
    }

    // Sort jobs by total matched resumes
    quickSortJobCounts(&jobList);

    const int TOPJ = 1000;
    cout << "Top " << TOPJ << " jobs related to skill '" << skillRaw << "':\n";
    cout << "Total matched resumes with the skill: " << resumesWithSkill << "\n\n";

    if (!jobList) {
        cout << "No jobs found with that skill.\n\n";
    } else {
        int shown = 0;
        for (JobCount* jc = jobList; jc && shown < TOPJ; jc = jc->next, shown++) {
            Job* j = jc->jobPtr;
            cout << shown + 1 << ". " << j->titleOriginal << " | Total matched: " << jc->count;
            
            int bestId = 0, bestScore = -1;
            for (Resume* r = resumeHead; r; r = r->next) {
                int sc = computeWeightedScore(j, r);
                if (sc > bestScore) { 
                    bestScore = sc; 
                    bestId = r->id; 
                }
            }

            if (bestScore > 0) {
                cout << " | Best candidate: " << bestId << " | Score: " << bestScore;
            } else {
                cout << " | Best candidate: None";
            }
            cout << "\n";
        }
        cout << "\n";
    }

    deleteJobCountList(jobList);

    auto stepEnd = high_resolution_clock::now();
    double memEnd = getMemoryUsageKB();
    long long stepMs = duration_cast<milliseconds>(stepEnd - stepStart).count();
    long long cumMs  = duration_cast<milliseconds>(stepEnd - globalStart).count();
    double stepMem = memEnd - memStart;
    printStepStatsSimple(stepMs, cumMs, stepMem, memEnd);
    cout << "\n";
}

void searchByCandidateID(Job* jobHead, Resume* resumeHead, int candId,
                         const high_resolution_clock::time_point &globalStart, double globalMemStart) {
    auto stepStart = high_resolution_clock::now();
    double memStart = getMemoryUsageKB();

    Resume* target = linearSearchResume(resumeHead, candId);
    
    if (!target) {
        cout << "Candidate ID " << candId << " not found.\n\n";
        auto stepEnd = high_resolution_clock::now();
        double memEnd = getMemoryUsageKB();
        long long stepMs = duration_cast<milliseconds>(stepEnd - stepStart).count();
        long long cumMs  = duration_cast<milliseconds>(stepEnd - globalStart).count();
        double stepMem = memEnd - memStart;
        printStepStatsSimple(stepMs, cumMs, stepMem, memEnd);
        cout << "\n";
        return;
    }

    CandidateScore* jobMatches = nullptr;
    
    for (Job* j = jobHead; j; j = j->next) {
        int sc = computeWeightedScore(j, target);
        if (sc > 0) {
            CandidateScore* node = new CandidateScore(0, sc);
            node->jobPtr = j;
            node->next = jobMatches;
            jobMatches = node;
        }
    }
    
    quickSortCandidates(&jobMatches);

    const int TOPJ = 1000;  // Changed from 3 to 50
    cout << "Top " << TOPJ << " job matches for candidate " << candId << ":\n";
    
    if (!jobMatches) {
        cout << "(no matching jobs)\n\n";
    } else {
        int shown = 0;
        for (CandidateScore* jm = jobMatches; jm && shown < TOPJ; jm = jm->next, shown++) {
            Job* j = jm->jobPtr;
            cout << shown+1 << ". " << j->titleOriginal << " — Score: " << jm->score << "\n";
        }
        cout << "\n";
    }
    
    deleteCandidateList(jobMatches);

    auto stepEnd = high_resolution_clock::now();
    double memEnd = getMemoryUsageKB();
    long long stepMs = duration_cast<milliseconds>(stepEnd - stepStart).count();
    long long cumMs  = duration_cast<milliseconds>(stepEnd - globalStart).count();
    double stepMem = memEnd - memStart;
    printStepStatsSimple(stepMs, cumMs, stepMem, memEnd);
    cout << "\n";
}

// ----------------- Main flow -----------------
int main() {
    auto globalStart = high_resolution_clock::now();
    double globalMemStart = getMemoryUsageKB();

    cout << "[1/6] Loading jobs from job_grouped.csv...\n";
    auto s1 = high_resolution_clock::now();
    double m1s = getMemoryUsageKB();
    Job* jobHead = nullptr;
    int jobCount = loadJobsFromCSV(jobHead, "job_grouped.csv");
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
    Resume* resumeHead = nullptr;
    int resumeCount = loadResumesFromCSV(resumeHead, "resume_cleaned.csv");
    auto e2 = high_resolution_clock::now();
    double m2e = getMemoryUsageKB();
    cout << "Loaded " << resumeCount << " resumes.\n";
    printStepStatsSimple(duration_cast<milliseconds>(e2 - s2).count(),
                         duration_cast<milliseconds>(e2 - globalStart).count(),
                         m2e - m2s, m2e);
    cout << "\n";

    cout << "[3/6] Sorting jobs (natural order) ...\n";
    auto s3 = high_resolution_clock::now();
    double m3s = getMemoryUsageKB();
    quickSortJobs(&jobHead);
    auto e3 = high_resolution_clock::now();
    double m3e = getMemoryUsageKB();
    cout << "Sorted jobs. Displaying first 1000:\n";
    printFirstNJobs(jobHead, 1000);
    printStepStatsSimple(duration_cast<milliseconds>(e3 - s3).count(),
                         duration_cast<milliseconds>(e3 - globalStart).count(),
                         m3e - m3s, m3e);
    cout << "\n";

    cout << "[4/6] Sorting resumes ...\n";
    auto s4 = high_resolution_clock::now();
    double m4s = getMemoryUsageKB();
    quickSortResumes(&resumeHead);
    auto e4 = high_resolution_clock::now();
    double m4e = getMemoryUsageKB();
    cout << "Sorted resumes. Displaying first 1000:\n";
    printFirstNResumes(resumeHead, 1000);
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
    while (true) {
        cout << "================== MENU ==================\n";
        cout << "1. Search by Job Title\n2. Search by Skill\n3. Search by Candidate ID\n4. Exit\nEnter choice: ";
        int choice;
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Please enter a number.\n\n";
            continue;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        if (choice == 1) {
            cout << "Enter job title or keyword: ";
            string q;
            getline(cin, q);
            if (trim(q).empty()) {
                cout << "Empty input. Please type a job title or keyword.\n\n";
                continue;
            }
            searchByJobTitle(jobHead, resumeHead, q, globalStart, globalMemStart);
        } else if (choice == 2) {
            cout << "Enter skill: ";
            string sk;
            getline(cin, sk);
            if (trim(sk).empty()) {
                cout << "Empty input. Please type a skill.\n\n";
                continue;
            }
            searchBySkill(jobHead, resumeHead, sk, globalStart, globalMemStart);
        } else if (choice == 3) {
            cout << "Enter candidate ID (integer): ";
            int cid;
            if (!(cin >> cid)) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid candidate id.\n\n";
                continue;
            }
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            searchByCandidateID(jobHead, resumeHead, cid, globalStart, globalMemStart);
        } else if (choice == 4) {
            cout << "Exiting program.\n";
            break;
        } else {
            cout << "Invalid option.\n\n";
        }
    }

    return 0;
}