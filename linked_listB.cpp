// linked_listB.cpp
// Complete implementation with Sentinel Search and Merge Sort
// Compile with: clang++ -std=c++17 linked_listB.cpp -o linked_listB
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <cctype>
#include <cmath>
#include <limits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined(_WIN32)
    #include <windows.h>
    #include <psapi.h>
#elif defined(__APPLE__) && defined(__MACH__)
    #include <mach/mach.h>
    #include <sys/resource.h>
#else
    #include <unistd.h>
    #include <sys/resource.h>
#endif

using namespace std;
using namespace std::chrono;

// ----------------- Data Structures -----------------
struct SkillNode {
    string skillOriginal;
    string skillNorm;
    SkillNode* next;
    SkillNode(const string &o, const string &n) : skillOriginal(o), skillNorm(n), next(nullptr) {}
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
    Job(const string &tO, const string &tS, const string &sO)
        : titleOriginal(tO), titleSortKey(tS), skills(nullptr), skillsOriginal(sO),
          totalMatched(0), next(nullptr) {}
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
    for (char c : s) if (!isspace((unsigned char)c)) out.push_back(c);
    return out;
}

string normalizeKey(const string &s) { return removeSpaces(toLowerCopy(s)); }

string trim(const string &s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

string makeTitleSortKey(const string &title) {
    string t = title;
    size_t end = t.find_last_not_of(" \t\r\n");
    if (end != string::npos) t = t.substr(0, end + 1);
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
    string prefTrim = (a == string::npos) ? string("") : prefLower.substr(a, b - a + 1);

    string key;
    if (digits.empty()) key = prefTrim + "|0";
    else {
        const int W = 10;
        string padded(W - (int)digits.size(), '0');
        padded += digits;
        key = prefTrim + "|1" + padded;
    }
    return key;
}

// ----------------- Memory Tracking -----------------
// double getMemoryUsageKB() {
// #ifdef __linux__
//     FILE* file = fopen("/proc/self/status", "r");
//     if (file) {
//         char line[128];
//         while (fgets(line, 128, file)) {
//             if (strncmp(line, "VmRSS:", 6) == 0) {
//                 long rss = 0;
//                 sscanf(line + 6, "%ld", &rss);
//                 fclose(file);
//                 return (double)rss;
//             }
//         }
//         fclose(file);
//     }
// #elif defined(__APPLE__) && defined(__MACH__)
//     struct mach_task_basic_info info;
//     mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
//     if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &count) == KERN_SUCCESS)
//         return info.resident_size / 1024.0;
// #endif
//     struct rusage usage;
//     if (getrusage(RUSAGE_SELF, &usage) == 0) {
// #if defined(__APPLE__) && defined(__MACH__)
//         return usage.ru_maxrss / 1024.0;
// #else
//         return (double)usage.ru_maxrss;
// #endif
//     }
//     return 0.0;
// }

double getMemoryUsageKB() {
#if defined(__linux__)
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
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &count) == KERN_SUCCESS)
        return info.resident_size / 1024.0;
#elif defined(_WIN32)
    PROCESS_MEMORY_COUNTERS info;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info)))
        return (double)info.WorkingSetSize / 1024.0;
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
    cout << "Step Memory Change: " << (long long)(stepMemKB * 1024) << " bytes | Current Total Memory: "
         << (long long)(totalMemKB * 1024) << " bytes\n";
}

// ----------------- Build Skill List -----------------
SkillNode* buildSkillListFromCSV(const string &line) {
    SkillNode *head = nullptr, *tail = nullptr;
    string token;
    for (size_t i = 0; i <= line.size(); ++i) {
        if (i == line.size() || line[i] == ',') {
            string t = trim(token);
            if (!t.empty()) {
                SkillNode* node = new SkillNode(t, normalizeKey(t));
                if (!head) head = tail = node;
                else { tail->next = node; tail = node; }
            }
            token.clear();
        } else token.push_back(line[i]);
    }
    // If nothing parsed but line not empty, create single skill
    if (!head && !trim(line).empty()) {
        string t = trim(line);
        head = new SkillNode(t, normalizeKey(t));
    }
    return head;
}

int countSkills(SkillNode* head) { int c=0; while (head){c++;head=head->next;} return c; }

int countMatchingSkills(SkillNode* job, SkillNode* resume) {
    int matches=0;
    for (SkillNode* r=resume;r;r=r->next){
        for (SkillNode* j=job;j;j=j->next){
            if (!r->skillNorm.empty() && r->skillNorm==j->skillNorm){matches++;break;}
        }
    }
    return matches;
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

// ----------------- Merge Sort (replaces QuickSort) -----------------
// Generic merge for linked lists (T must have 'next' pointer)
template<typename T, typename CMP>
T* mergeLists(T* a,T* b,CMP cmp){
    if(!a) return b;
    if(!b) return a;
    T* result = nullptr;
    if(cmp(a,b)){
        result = a;
        result->next = mergeLists(a->next,b,cmp);
    } else {
        result = b;
        result->next = mergeLists(a,b->next,cmp);
    }
    return result;
}

template<typename T>
void splitList(T* source,T** front,T** back){
    if(!source){ *front = *back = nullptr; return; }
    T* slow = source;
    T* fast = source->next;
    while(fast){
        fast = fast->next;
        if(fast){ slow = slow->next; fast = fast->next; }
    }
    *front = source;
    *back = slow->next;
    slow->next = nullptr;
}

// Merge sort for Jobs (sort by titleSortKey ascending)
void mergeSortJobs(Job** headRef){
    Job* head = *headRef;
    if(!head || !head->next) return;
    Job *a,*b;
    splitList(head,&a,&b);
    mergeSortJobs(&a);
    mergeSortJobs(&b);
    *headRef = mergeLists<Job>(a,b,[](Job* x, Job* y){ return x->titleSortKey < y->titleSortKey; });
}
void mergeSortResumes(Resume** headRef){
    Resume* head = *headRef;
    if(!head || !head->next) return;
    Resume *a,*b;
    splitList(head,&a,&b);
    mergeSortResumes(&a);
    mergeSortResumes(&b);
    *headRef = mergeLists<Resume>(a,b,[](Resume* x, Resume* y){ return x->skillCount > y->skillCount; });
}

// Merge sort for CandidateScore (sort by score descending)
void mergeSortCandidates(CandidateScore** headRef){
    CandidateScore* head = *headRef;
    if(!head || !head->next) return;
    CandidateScore *a,*b;
    splitList(head,&a,&b);
    mergeSortCandidates(&a);
    mergeSortCandidates(&b);
    *headRef = mergeLists<CandidateScore>(a,b,[](CandidateScore* x, CandidateScore* y){ return x->score > y->score; });
}

// Merge sort for JobCount (sort by count descending)
void mergeSortJobCounts(JobCount** headRef){
    JobCount* head = *headRef;
    if(!head || !head->next) return;
    JobCount *a,*b;
    splitList(head,&a,&b);
    mergeSortJobCounts(&a);
    mergeSortJobCounts(&b);
    *headRef = mergeLists<JobCount>(a,b,[](JobCount* x, JobCount* y){ return x->count > y->count; });
}

// ----------------- Sentinel Search (replaces Linear Search) -----------------
// Sentinel search for Resume by id.
// We place a sentinel value at tail->id = target and then iterate until found.
// If found at a real node, return it; otherwise restore and return nullptr.
// Note: to avoid breaking list elements we backup the tail node content.
Resume* sentinelSearchResume(Resume* head,int target){
    if(!head) return nullptr;
    Resume* tail = head;
    while(tail->next) tail = tail->next;
    Resume backup = *tail; 
    tail->id = target;
    Resume* cur = head;
    while(cur->id != target) cur = cur->next;
    bool foundAtTail = (cur == tail);
    *tail = backup;
    if(foundAtTail) return nullptr;
    return cur;
}

// Sentinel search for exact job titleSortKey.
// We use tail.titleSortKey = key as sentinel, then traverse and collect matches until sentinel.
// Collect copies of matching jobs into a new linked list (as original did).
// Fixed sentinel-style search for exact job titleSortKey.
// Uses tail sentinel but ensures correctness for string fields.
Job* sentinelSearchJobsExact(Job* head, const string &key) {
    if (!head) return nullptr;

    Job* tail = head;
    while (tail->next) tail = tail->next;

    string backupKey = tail->titleSortKey;

    tail->titleSortKey = key;

    Job* cur = head;
    Job* resultHead = nullptr;
    Job* resultTail = nullptr;

    while (cur) {
        if (cur->titleSortKey == key) {
            if (cur == tail && backupKey != key) break;
            Job* copy = new Job(cur->titleOriginal, cur->titleSortKey, cur->skillsOriginal);
            copy->skills = cur->skills;
            copy->next = nullptr;
            if (!resultHead) resultHead = resultTail = copy;
            else { resultTail->next = copy; resultTail = copy; }
        }

        if (cur == tail) break;
        cur = cur->next;
    }
    tail->titleSortKey = backupKey;

    return resultHead;
}

// Partial (substring) search replaced: sentinel doesn't help for substring easily;
// so we'll keep scanning but it's the only place substring-check is used.
Job* sentinelSearchJobsPartial(Job* head,const string &norm){
    Job* res = nullptr;
    Job* resTail = nullptr;
    for(Job* cur = head; cur; cur = cur->next){
        string titleNorm = normalizeKey(cur->titleOriginal);
        if(titleNorm.find(norm) != string::npos){
            Job* copy = new Job(cur->titleOriginal, cur->titleSortKey, cur->skillsOriginal);
            copy->skills = cur->skills;
            copy->next = nullptr;
            if(!res) res = resTail = copy;
            else { resTail->next = copy; resTail = copy; }
        }
    }
    return res;
}

// ----------------- Matching Logic -----------------
int computeWeightedScore(Job* j,Resume* r){
    if(!j->skills) return 0;
    int total = countSkills(j->skills);
    if(total == 0) return 0;
    int matches = countMatchingSkills(j->skills, r->skills);
    double ratio = (double)matches / (double)total;
    int score = (int)round(ratio * 100.0);
    return score;
}

CandidateScore* getTopCandidates(Resume* head,Job* j){
    CandidateScore* list = nullptr;
    for(Resume* r = head; r; r = r->next){
        int sc = computeWeightedScore(j, r);
        if(sc > 0){
            CandidateScore* node = new CandidateScore(r->id, sc);
            node->next = list;
            list = node;
        }
    }
    mergeSortCandidates(&list);
    return list;
}

void deleteCandidateList(CandidateScore* head){ while(head){ CandidateScore* t = head; head = head->next; delete t; } }
void deleteJobCountList(JobCount* head){ while(head){ JobCount* t = head; head = head->next; delete t; } }

// ----------------- Printing -----------------
void printFirstNJobs(Job* head,int N){
    Job* cur = head;
    int shown = 0;
    while(cur && shown < N){
        cout << (shown+1) << ". " << cur->titleOriginal << " | Skills: " << cur->skillsOriginal << "\n";
        cur = cur->next; shown++;
    }
    if(shown == 0) cout << "(none)\n";
    cout << "\n";
}

void printFirstNResumes(Resume* head,int N){
    Resume* cur = head;
    int shown = 0;
    while(cur && shown < N){
        cout << (shown+1) << ". Resume ID: " << cur->id << " | Skills count: " << cur->skillCount << "\n";
        cur = cur->next; shown++;
    }
    if(shown == 0) cout << "(none)\n";
    cout << "\n";
}

// ----------------- Search Operations -----------------
void searchByJobTitle(Job* jobHead, Resume* resumeHead, const string &queryRaw,
                      const high_resolution_clock::time_point &globalStart, double globalMemStart) {
    auto stepStart = high_resolution_clock::now();
    double memStart = getMemoryUsageKB();

    string qNorm = normalizeKey(queryRaw);
    string qSortKey = makeTitleSortKey(queryRaw);

    // Use sentinel searches (exact first, then partial)
    Job* results = sentinelSearchJobsExact(jobHead, qSortKey);
    if (!results && !qNorm.empty()) {
        results = sentinelSearchJobsPartial(jobHead, qNorm);
    }

    if (!results) {
        cout << "No jobs found matching '" << queryRaw << "'.\n\n";
    } else {
        const int MAX_DISPLAY = 5;
        const int TOPC = 50;

        // Step 1: build a temporary JobCount list for matched candidate totals
        JobCount* jcHead = nullptr;
        JobCount* jcTail = nullptr;

        for (Job* j = results; j; j = j->next) {
            CandidateScore* candidates = getTopCandidates(resumeHead, j);
            int totalMatched = 0;
            for (CandidateScore* c = candidates; c; c = c->next) totalMatched++;

            // Build JobCount node (job pointer + matched candidate count)
            JobCount* node = new JobCount(j, totalMatched);
            if (!jcHead) jcHead = jcTail = node;
            else { jcTail->next = node; jcTail = node; }

            deleteCandidateList(candidates);
        }

        // Step 2: sort JobCount list in descending order of count
        mergeSortJobCounts(&jcHead);

        // Step 3: display top 5 jobs by matched candidates
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

        // Step 4: free copied job results
        while (results) {
            Job* tmp = results;
            results = results->next;
            delete tmp;
        }
    }

    // Step 5: timing + memory stats
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

    // Count how many resumes have this skill
    int resumesWithSkill = 0;
    for (Resume* r = resumeHead; r; r = r->next) {
        for (SkillNode* s = r->skills; s; s = s->next) {
            if (!skillNorm.empty() && s->skillNorm == skillNorm) {
                resumesWithSkill++;
                break;
            }
        }
    }

    // For each job that has the searched skill
    for (Job* j = jobHead; j; j = j->next) {
        bool jobHas = false;
        for (SkillNode* s = j->skills; s; s = s->next) {
            if (!skillNorm.empty() && s->skillNorm == skillNorm) {
                jobHas = true;
                break;
            }
        }
        if (!jobHas) continue;

        // Count how many resumes match this job (based on all job skills)
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

        JobCount* node = new JobCount(j, cnt);
        node->next = jobList;
        jobList = node;
    }

    mergeSortJobCounts(&jobList);

    const int TOPJ = 1000;
    cout << "Top " << TOPJ << " jobs related to skill '" << skillRaw << "':\n";
    cout << "Total matched resumes with the skill: " << resumesWithSkill << "\n\n";

    if (!jobList) {
        cout << "No jobs found with that skill.\n\n";
    } else {
        int shown = 0;
        for (JobCount* jc = jobList; jc && shown < TOPJ; jc = jc->next, shown++) {
            Job* j = jc->jobPtr;
            cout << shown+1 << ". " << j->titleOriginal << " | Total matched: " << jc->count;

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

    Resume* target = sentinelSearchResume(resumeHead, candId);

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

    mergeSortCandidates(&jobMatches);

    const int TOPJ = 1000;
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
    mergeSortJobs(&jobHead);
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
    mergeSortResumes(&resumeHead);
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
