/*
 * ============================================================
 *  Pattern Matching Suite
 *  Algorithms: Naive | KMP | Rabin-Karp | Boyer-Moore
 *  Author: Semester Project – Spring 2026
 * ============================================================
 *
 *  HOW TO COMPILE:
 *    g++ -O2 -std=c++17 -o pattern_matching pattern_matching_suite.cpp
 *
 *  HOW TO RUN:
 *    ./pattern_matching                  (interactive menu)
 *    ./pattern_matching <textfile>       (load text from file)
 *
 * ============================================================
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <cmath>
#include <cassert>
#include <climits>
#include <set>

using namespace std;
using namespace chrono;

// ─────────────────────────────────────────────────────────────
//  RESULT STRUCTURE  (returned by every algorithm)
// ─────────────────────────────────────────────────────────────
struct SearchResult {
    string          algorithmName;
    string          pattern;
    vector<int>     occurrences;   // 0-based positions in text
    long long       comparisons;   // character comparisons made
    double          timeMs;        // wall-clock milliseconds
    size_t          memoryBytes;   // approximate extra memory used
};

// ─────────────────────────────────────────────────────────────
//  1. NAIVE (BRUTE FORCE) ALGORITHM
//  Time: O((n-m+1)*m)  |  Space: O(1)
// ─────────────────────────────────────────────────────────────
SearchResult naiveSearch(const string& text, const string& pattern) {
    SearchResult result;
    result.algorithmName = "Naive (Brute Force)";
    result.pattern       = pattern;
    result.comparisons   = 0;
    result.memoryBytes   = 0;          // no extra data structures

    int n = (int)text.size();
    int m = (int)pattern.size();

    auto start = high_resolution_clock::now();

    for (int i = 0; i <= n - m; i++) {
        int j;
        for (j = 0; j < m; j++) {
            result.comparisons++;
            if (text[i + j] != pattern[j]) break;
        }
        if (j == m) result.occurrences.push_back(i);
    }

    auto end = high_resolution_clock::now();
    result.timeMs = duration<double, milli>(end - start).count();
    return result;
}

// ─────────────────────────────────────────────────────────────
//  2. KNUTH-MORRIS-PRATT (KMP) ALGORITHM
//  Time: O(n+m)  |  Space: O(m)
// ─────────────────────────────────────────────────────────────
vector<int> buildLPS(const string& pattern, long long& comparisons) {
    int m = (int)pattern.size();
    vector<int> lps(m, 0);
    int len = 0, i = 1;
    while (i < m) {
        comparisons++;
        if (pattern[i] == pattern[len]) {
            lps[i++] = ++len;
        } else {
            if (len) len = lps[len - 1];
            else      lps[i++] = 0;
        }
    }
    return lps;
}

SearchResult kmpSearch(const string& text, const string& pattern) {
    SearchResult result;
    result.algorithmName = "KMP (Knuth-Morris-Pratt)";
    result.pattern       = pattern;
    result.comparisons   = 0;

    int n = (int)text.size();
    int m = (int)pattern.size();

    auto start = high_resolution_clock::now();

    vector<int> lps = buildLPS(pattern, result.comparisons);
    result.memoryBytes = m * sizeof(int);  // lps table

    int i = 0, j = 0;
    while (i < n) {
        result.comparisons++;
        if (text[i] == pattern[j]) {
            i++; j++;
            if (j == m) {
                result.occurrences.push_back(i - j);
                j = lps[j - 1];
            }
        } else {
            if (j) j = lps[j - 1];
            else   i++;
        }
    }

    auto end = high_resolution_clock::now();
    result.timeMs = duration<double, milli>(end - start).count();
    return result;
}

// ─────────────────────────────────────────────────────────────
//  3. RABIN-KARP ALGORITHM  (rolling hash)
//  Time: O(n+m) avg, O(nm) worst  |  Space: O(1)
// ─────────────────────────────────────────────────────────────
SearchResult rabinKarpSearch(const string& text, const string& pattern) {
    SearchResult result;
    result.algorithmName = "Rabin-Karp (Rolling Hash)";
    result.pattern       = pattern;
    result.comparisons   = 0;
    result.memoryBytes   = 2 * sizeof(long long);  // two hash values

    const long long BASE  = 256;
    const long long PRIME = 101;

    int n = (int)text.size();
    int m = (int)pattern.size();

    auto start = high_resolution_clock::now();

    long long patHash  = 0, winHash = 0;
    long long highPow  = 1;

    for (int i = 0; i < m - 1; i++) highPow = (highPow * BASE) % PRIME;

    for (int i = 0; i < m; i++) {
        patHash = (BASE * patHash + pattern[i]) % PRIME;
        winHash = (BASE * winHash + text[i])    % PRIME;
    }

    for (int i = 0; i <= n - m; i++) {
        if (patHash == winHash) {
            // verify character by character (spurious hit check)
            int j;
            for (j = 0; j < m; j++) {
                result.comparisons++;
                if (text[i + j] != pattern[j]) break;
            }
            if (j == m) result.occurrences.push_back(i);
        }
        if (i < n - m) {
            winHash = (BASE * (winHash - text[i] * highPow) + text[i + m]) % PRIME;
            if (winHash < 0) winHash += PRIME;
        }
    }

    auto end = high_resolution_clock::now();
    result.timeMs = duration<double, milli>(end - start).count();
    return result;
}

// ─────────────────────────────────────────────────────────────
//  4. BOYER-MOORE ALGORITHM  (bad character heuristic)
//  Time: O(n/m) best, O(nm) worst  |  Space: O(alphabet)
// ─────────────────────────────────────────────────────────────
SearchResult boyerMooreSearch(const string& text, const string& pattern) {
    SearchResult result;
    result.algorithmName = "Boyer-Moore (Bad Character)";
    result.pattern       = pattern;
    result.comparisons   = 0;

    int n = (int)text.size();
    int m = (int)pattern.size();

    // Build bad-character table  (256 ASCII)
    const int ALPHA = 256;
    vector<int> badChar(ALPHA, -1);
    for (int i = 0; i < m; i++) badChar[(unsigned char)pattern[i]] = i;
    result.memoryBytes = ALPHA * sizeof(int);

    auto start = high_resolution_clock::now();

    int shift = 0;
    while (shift <= n - m) {
        int j = m - 1;
        while (j >= 0) {
            result.comparisons++;
            if (pattern[j] == text[shift + j]) j--;
            else break;
        }
        if (j < 0) {
            result.occurrences.push_back(shift);
            shift += (shift + m < n) ? m - badChar[(unsigned char)text[shift + m]] : 1;
        } else {
            int bc = badChar[(unsigned char)text[shift + j]];
            shift += max(1, j - bc);
        }
    }

    auto end = high_resolution_clock::now();
    result.timeMs = duration<double, milli>(end - start).count();
    return result;
}

// ─────────────────────────────────────────────────────────────
//  DISPLAY HELPERS
// ─────────────────────────────────────────────────────────────
void printSeparator(char c = '=', int len = 70) {
    cout << string(len, c) << "\n";
}

void printResult(const SearchResult& r) {
    printSeparator('-');
    cout << "  Algorithm   : " << r.algorithmName << "\n";
    cout << "  Pattern     : \"" << r.pattern << "\"\n";
    cout << "  Occurrences : " << r.occurrences.size() << "\n";

    if (!r.occurrences.empty()) {
        cout << "  Positions   : ";
        int shown = 0;
        for (int pos : r.occurrences) {
            cout << pos;
            if (++shown < (int)r.occurrences.size()) cout << ", ";
            if (shown == 20 && r.occurrences.size() > 20) {
                cout << "... (" << r.occurrences.size() - 20 << " more)";
                break;
            }
        }
        cout << "\n";
    }

    cout << "  Comparisons : " << r.comparisons << "\n";
    cout << fixed << setprecision(4);
    cout << "  Time (ms)   : " << r.timeMs << "\n";
    cout << "  Memory (B)  : " << r.memoryBytes << "\n";
}

void printComparison(const vector<SearchResult>& results) {
    printSeparator('=');
    cout << "  COMPARATIVE ANALYSIS\n";
    printSeparator('=');

    // Header
    cout << left
         << setw(28) << "Algorithm"
         << setw(12) << "Matches"
         << setw(16) << "Comparisons"
         << setw(14) << "Time (ms)"
         << setw(14) << "Memory (B)"
         << "\n";
    printSeparator('-');

    for (const auto& r : results) {
        cout << left
             << setw(28) << r.algorithmName
             << setw(12) << r.occurrences.size()
             << setw(16) << r.comparisons
             << setw(14) << fixed << setprecision(4) << r.timeMs
             << setw(14) << r.memoryBytes
             << "\n";
    }

    // Find best
    auto bestTime = min_element(results.begin(), results.end(),
        [](const SearchResult& a, const SearchResult& b){ return a.timeMs < b.timeMs; });
    auto bestComp = min_element(results.begin(), results.end(),
        [](const SearchResult& a, const SearchResult& b){ return a.comparisons < b.comparisons; });

    printSeparator('-');
    cout << "  Fastest     : " << bestTime->algorithmName << " (" << bestTime->timeMs << " ms)\n";
    cout << "  Fewest cmp  : " << bestComp->algorithmName << " (" << bestComp->comparisons << ")\n";
}

// ─────────────────────────────────────────────────────────────
//  RECOMMENDATION SYSTEM
// ─────────────────────────────────────────────────────────────
void recommend(const string& text, const string& pattern) {
    printSeparator('=');
    cout << "  ALGORITHM RECOMMENDATION\n";
    printSeparator('-');

    int n = (int)text.size();
    int m = (int)pattern.size();

    // Heuristics
    bool shortPattern   = (m <= 5);
    bool longPattern    = (m >= 20);
    bool repetitiveText = false;
    bool dnaLike        = true;

    // Check if text looks like DNA (ACGT only)
    int dnaCount = 0;
    for (char c : text.substr(0, min(200, n)))
        if (c=='A'||c=='C'||c=='G'||c=='T') dnaCount++;
    if (dnaCount < (int)(min(200, n) * 0.8)) dnaLike = false;

    // Check repetitiveness (count unique chars / total in small window)
    if (n > 50) {
        string sample = text.substr(0, 100);
        int uniqueChars = (int)set<char>(sample.begin(), sample.end()).size();
        if (uniqueChars < 10) repetitiveText = true;
    }

    cout << "  Text length : " << n << " chars\n";
    cout << "  Pattern len : " << m << " chars\n";
    cout << "  DNA-like    : " << (dnaLike       ? "Yes" : "No") << "\n";
    cout << "  Repetitive  : " << (repetitiveText ? "Yes" : "No") << "\n\n";

    if (shortPattern) {
        cout << "  RECOMMENDATION: Naive or KMP\n";
        cout << "  REASON: Short patterns make preprocessing overhead (KMP/BM)\n";
        cout << "          not worth it. Both run fast on short patterns.\n";
    } else if (longPattern && !repetitiveText) {
        cout << "  RECOMMENDATION: Boyer-Moore\n";
        cout << "  REASON: Longer patterns allow bigger skips with bad-character\n";
        cout << "          heuristic, making BM sub-linear on average.\n";
    } else if (dnaLike || repetitiveText) {
        cout << "  RECOMMENDATION: KMP\n";
        cout << "  REASON: KMP guarantees O(n+m) even for repetitive text.\n";
        cout << "          Boyer-Moore degrades on small alphabets (DNA: A,C,G,T).\n";
    } else {
        cout << "  RECOMMENDATION: KMP or Boyer-Moore\n";
        cout << "  REASON: Natural language text with average-length patterns\n";
        cout << "          benefits from either. KMP is safer; BM is often faster.\n";
    }
    printSeparator('=');
}

// ─────────────────────────────────────────────────────────────
//  VISUAL OCCURRENCE MAP
// ─────────────────────────────────────────────────────────────
void visualOccurrenceMap(const string& text, const vector<int>& positions, int barWidth = 60) {
    if (positions.empty()) { cout << "  (no occurrences to display)\n"; return; }

    int n = (int)text.size();
    cout << "  Occurrence map (each cell ≈ " << (n / barWidth + 1) << " chars):\n  [";
    vector<bool> hit(barWidth, false);
    for (int p : positions) {
        int cell = (int)((long long)p * barWidth / n);
        if (cell < barWidth) hit[cell] = true;
    }
    for (bool h : hit) cout << (h ? '#' : '.');
    cout << "]\n";
    cout << "  0" << string(barWidth - 4, ' ') << "end\n";
}

// ─────────────────────────────────────────────────────────────
//  DEMO TEXT GENERATORS
// ─────────────────────────────────────────────────────────────
string generateDNA(int length) {
    string dna;
    const string bases = "ACGT";
    srand(42);
    for (int i = 0; i < length; i++) dna += bases[rand() % 4];
    return dna;
}

string generateNaturalLanguage() {
    return "The quick brown fox jumps over the lazy dog. "
           "A pattern matching algorithm searches for occurrences of a pattern "
           "within a larger text. The naive algorithm checks every position. "
           "The KMP algorithm uses a failure function to skip redundant comparisons. "
           "The Boyer-Moore algorithm skips characters using bad character heuristics. "
           "The Rabin-Karp algorithm uses rolling hash values for fast comparison. "
           "Pattern matching is widely used in text editors, compilers, bioinformatics, "
           "network intrusion detection, and search engines. "
           "The quick brown fox jumps over the lazy dog again and again.";
}

string generateRepetitive(int length) {
    string rep;
    while ((int)rep.size() < length) rep += "ABABCABABABD";
    return rep.substr(0, length);
}

// ─────────────────────────────────────────────────────────────
//  RUN ALL FOUR ALGORITHMS ON ONE TEXT+PATTERN
// ─────────────────────────────────────────────────────────────
vector<SearchResult> runAll(const string& text, const string& pattern) {
    return {
        naiveSearch(text, pattern),
        kmpSearch  (text, pattern),
        rabinKarpSearch(text, pattern),
        boyerMooreSearch(text, pattern)
    };
}

// ─────────────────────────────────────────────────────────────
//  MAIN  –  INTERACTIVE MENU
// ─────────────────────────────────────────────────────────────
int main(int argc, char* argv[]) {

    string text;

    // Optionally load text from file supplied as argument
    if (argc >= 2) {
        ifstream fin(argv[1]);
        if (!fin) { cerr << "Cannot open file: " << argv[1] << "\n"; return 1; }
        text.assign(istreambuf_iterator<char>(fin), istreambuf_iterator<char>());
        cout << "Loaded " << text.size() << " characters from " << argv[1] << "\n";
    }

    int choice;
    do {
        printSeparator('=');
        cout << "  PATTERN MATCHING SUITE\n";
        printSeparator('=');
        cout << "  1. Enter text manually\n";
        cout << "  2. Use DNA sequence (auto-generated, 500 chars)\n";
        cout << "  3. Use Natural Language sample\n";
        cout << "  4. Use Repetitive pattern text (500 chars)\n";
        if (!text.empty())
            cout << "  5. Use currently loaded text (" << text.size() << " chars)\n";
        cout << "  6. Search with single algorithm\n";
        cout << "  7. Compare ALL four algorithms\n";
        cout << "  8. Multiple pattern search (compare all)\n";
        cout << "  9. Get algorithm recommendation\n";
        cout << "  0. Exit\n";
        printSeparator('-');
        cout << "  Choice: ";
        cin >> choice;
        cin.ignore();

        if (choice == 1) {
            cout << "  Enter text: ";
            getline(cin, text);

        } else if (choice == 2) {
            text = generateDNA(500);
            cout << "  DNA text loaded.\n";

        } else if (choice == 3) {
            text = generateNaturalLanguage();
            cout << "  Natural language text loaded.\n";

        } else if (choice == 4) {
            text = generateRepetitive(500);
            cout << "  Repetitive text loaded.\n";

        } else if (choice == 6) {
            if (text.empty()) { cout << "  Please load text first (options 1-4).\n"; continue; }
            cout << "  Select algorithm:\n";
            cout << "    1. Naive  2. KMP  3. Rabin-Karp  4. Boyer-Moore\n";
            cout << "  Choice: ";
            int alg; cin >> alg; cin.ignore();
            cout << "  Enter pattern: ";
            string pat; getline(cin, pat);

            SearchResult r;
            if      (alg == 1) r = naiveSearch(text, pat);
            else if (alg == 2) r = kmpSearch(text, pat);
            else if (alg == 3) r = rabinKarpSearch(text, pat);
            else               r = boyerMooreSearch(text, pat);

            printResult(r);
            visualOccurrenceMap(text, r.occurrences);

        } else if (choice == 7) {
            if (text.empty()) { cout << "  Please load text first (options 1-4).\n"; continue; }
            cout << "  Enter pattern: ";
            string pat; getline(cin, pat);
            auto results = runAll(text, pat);
            for (auto& r : results) printResult(r);
            printComparison(results);
            cout << "\n  Occurrence map (KMP result):\n";
            visualOccurrenceMap(text, results[1].occurrences);

        } else if (choice == 8) {
            if (text.empty()) { cout << "  Please load text first (options 1-4).\n"; continue; }
            cout << "  How many patterns? ";
            int np; cin >> np; cin.ignore();
            for (int i = 0; i < np; i++) {
                cout << "  Pattern " << (i+1) << ": ";
                string pat; getline(cin, pat);
                auto results = runAll(text, pat);
                printComparison(results);
            }

        } else if (choice == 9) {
            if (text.empty()) { cout << "  Please load text first (options 1-4).\n"; continue; }
            cout << "  Enter pattern: ";
            string pat; getline(cin, pat);
            recommend(text, pat);
        }

    } while (choice != 0);

    cout << "  Goodbye!\n";
    return 0;
}
