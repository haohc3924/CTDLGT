// Course project: English-Vietnamese dictionary using hash table + linked list chaining.
// Build: g++ -std=c++17 -O2 -Wall -Wextra -pedantic src/main.cpp -o dict

#include <algorithm>
#include <chrono>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

struct Entry {
  string en;
  string vi;
};

// =========================
// Tiện ích xử lý chuỗi
// =========================
static string trim(const string& s) {
  size_t i = 0;
  while (i < s.size() && isspace(static_cast<unsigned char>(s[i]))) i++;
  size_t j = s.size();
  while (j > i && isspace(static_cast<unsigned char>(s[j - 1]))) j--;
  return s.substr(i, j - i);
}

static string toLowerAscii(string s) {
  for (char& c : s) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
  return s;
}

static bool startsWithAlpha(const string& s) {
  return !s.empty() && isalpha(static_cast<unsigned char>(s[0]));
}

static bool containsPipe(const string& s) { return s.find('|') != string::npos; }

// =========================
// Phân nhóm hiển thị (A-Z + non-alpha)
// Lưu ý: đây chỉ dùng cho "Display by group",
// KHÔNG dùng làm chỉ số hash table nữa.
// =========================
static int groupIndexByFirstLetter(const string& en) {
  if (en.empty()) return 26;
  unsigned char c = static_cast<unsigned char>(en[0]);
  c = static_cast<unsigned char>(tolower(c));
  if (c >= 'a' && c <= 'z') return static_cast<int>(c - 'a');
  return 26; // nhóm ký tự đầu không phải chữ cái
}

struct Node {
  Entry data;
  Node* next = nullptr;
};

struct LinkedList {
  Node* head = nullptr;

  ~LinkedList() { clear(); }

  LinkedList() = default;
  LinkedList(const LinkedList&) = delete;
  LinkedList& operator=(const LinkedList&) = delete;

  bool empty() const { return head == nullptr; }

  void clear() {
    Node* cur = head;
    while (cur) {
      Node* nxt = cur->next;
      delete cur;
      cur = nxt;
    }
    head = nullptr;
  }

  static int compareEntry(const Entry& a, const Entry& b) {
    const string al = toLowerAscii(a.en);
    const string bl = toLowerAscii(b.en);
    if (al < bl) return -1;
    if (al > bl) return 1;
    return 0;
  }

  Node* findByEnglishExact(const string& enLower) const {
    Node* cur = head;
    while (cur) {
      if (toLowerAscii(cur->data.en) == enLower) return cur;
      cur = cur->next;
    }
    return nullptr;
  }

  bool removeByEnglishExact(const string& enLower) {
    Node* cur = head;
    Node* prev = nullptr;
    while (cur) {
      if (toLowerAscii(cur->data.en) == enLower) {
        if (prev) prev->next = cur->next;
        else head = cur->next;
        delete cur;
        return true;
      }
      prev = cur;
      cur = cur->next;
    }
    return false;
  }

  // Insert new entry or update existing if same English key (case-insensitive).
  // We insert at head for O(1); optional sorting is done separately.
  bool upsert(const Entry& e) {
    const string key = toLowerAscii(e.en);
    if (Node* found = findByEnglishExact(key)) {
      found->data.vi = e.vi;
      found->data.en = e.en;
      return false; // updated
    }
    Node* n = new Node;
    n->data = e;
    n->next = head;
    head = n;
    return true; // inserted
  }

  // Merge sort for linked list, sorting by English word (case-insensitive).
  static Node* mergeSorted(Node* a, Node* b) {
    Node dummy;
    Node* tail = &dummy;
    dummy.next = nullptr;
    while (a && b) {
      if (compareEntry(a->data, b->data) <= 0) {
        tail->next = a;
        a = a->next;
      } else {
        tail->next = b;
        b = b->next;
      }
      tail = tail->next;
    }
    tail->next = a ? a : b;
    return dummy.next;
  }

  static Node* getMiddle(Node* h) {
    if (!h) return h;
    Node* slow = h;
    Node* fast = h->next;
    while (fast && fast->next) {
      slow = slow->next;
      fast = fast->next->next;
    }
    return slow;
  }

  static Node* mergeSort(Node* h) {
    if (!h || !h->next) return h;
    Node* mid = getMiddle(h);
    Node* right = mid->next;
    mid->next = nullptr;
    Node* leftSorted = mergeSort(h);
    Node* rightSorted = mergeSort(right);
    return mergeSorted(leftSorted, rightSorted);
  }

  void sortByEnglish() { head = mergeSort(head); }

  vector<Entry> toVector() const {
    vector<Entry> out;
    Node* cur = head;
    while (cur) {
      out.push_back(cur->data);
      cur = cur->next;
    }
    return out;
  }

  size_t size() const {
    size_t n = 0;
    for (Node* cur = head; cur; cur = cur->next) n++;
    return n;
  }
};

struct HashDictionary {
  // Hash table "thực sự": băm trên TOÀN BỘ chuỗi, số bucket là số nguyên tố lớn
  // để giảm hiện tượng dồn bucket (giảm thoái hóa danh sách liên kết quá dài).
  static constexpr int BUCKETS = 10007;
  LinkedList table[BUCKETS];
  size_t count = 0;

  // Hàm băm kiểu polynomial rolling hash (đơn giản nhưng hiệu quả cho chuỗi).
  // - Chuyển về lowercase để lookup case-insensitive.
  // - Dùng modulo BUCKETS để ra chỉ số hợp lệ.
  static int hashEnglishKey(const string& enLower) {
    // BASE chọn số lẻ tương đối lớn để trộn tốt hơn.
    constexpr uint64_t BASE = 131;
    uint64_t h = 0;
    for (unsigned char ch : enLower) {
      h = (h * BASE + ch) % static_cast<uint64_t>(BUCKETS);
    }
    return static_cast<int>(h);
  }

  void clear() {
    for (int i = 0; i < BUCKETS; i++) table[i].clear();
    count = 0;
  }

  bool addOrUpdate(const string& en, const string& vi) {
    Entry e{trim(en), trim(vi)};
    if (e.en.empty() || e.vi.empty()) return false;
    // Vì file dùng '|' làm delimiter, ta chặn để tránh parse sai về sau.
    if (containsPipe(e.en) || containsPipe(e.vi)) return false;
    const string keyLower = toLowerAscii(e.en);
    int idx = hashEnglishKey(keyLower);
    bool inserted = table[idx].upsert(e);
    if (inserted) count++;
    return true;
  }

  const Entry* lookup(const string& en) const {
    const string key = toLowerAscii(trim(en));
    if (key.empty()) return nullptr;
    int idx = hashEnglishKey(key);
    Node* found = table[idx].findByEnglishExact(key);
    if (!found) return nullptr;
    return &found->data;
  }

  bool remove(const string& en) {
    const string key = toLowerAscii(trim(en));
    if (key.empty()) return false;
    int idx = hashEnglishKey(key);
    bool ok = table[idx].removeByEnglishExact(key);
    if (ok) count--;
    return ok;
  }

  bool updateMeaning(const string& en, const string& newVi) {
    const string key = toLowerAscii(trim(en));
    if (key.empty()) return false;
    const string viTrim = trim(newVi);
    if (containsPipe(viTrim)) return false;
    int idx = hashEnglishKey(key);
    Node* found = table[idx].findByEnglishExact(key);
    if (!found) return false;
    found->data.vi = viTrim;
    return true;
  }

  vector<Entry> getAllEntries() const {
    vector<Entry> all;
    all.reserve(count);
    for (int i = 0; i < BUCKETS; i++) {
      auto v = table[i].toVector();
      all.insert(all.end(), v.begin(), v.end());
    }
    return all;
  }

  vector<Entry> getEntriesByGroup(int groupIdx) const {
    vector<Entry> out;
    for (int i = 0; i < BUCKETS; i++) {
      Node* cur = table[i].head;
      while (cur) {
        if (groupIndexByFirstLetter(cur->data.en) == groupIdx) out.push_back(cur->data);
        cur = cur->next;
      }
    }
    return out;
  }

  void printGroup(int groupIdx, bool sorted) const {
    if (groupIdx < 0 || groupIdx > 26) return;
    auto entries = getEntriesByGroup(groupIdx);
    // Tối ưu: chỉ sort 1 lần (không vừa sort linked-list rồi lại sort vector).
    if (sorted) {
      stable_sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
        return toLowerAscii(a.en) < toLowerAscii(b.en);
      });
    }

    if (groupIdx == 26)
      cout << "== Group: (non-alpha) ==\n";
    else
      cout << "== Group: " << static_cast<char>('A' + groupIdx) << " ==\n";
    for (const auto& e : entries) cout << left << setw(24) << e.en << " : " << e.vi << "\n";
    cout << "Total in group: " << entries.size() << "\n";
  }

  void printAll(bool sorted) const {
    vector<Entry> all = getAllEntries();
    if (sorted) {
      stable_sort(all.begin(), all.end(), [](const Entry& a, const Entry& b) {
        return toLowerAscii(a.en) < toLowerAscii(b.en);
      });
    }
    cout << "== Dictionary (all entries) ==\n";
    for (const auto& e : all) cout << left << setw(24) << e.en << " : " << e.vi << "\n";
    cout << "Total entries: " << all.size() << "\n";
  }
};

static bool parseLineEntry(const string& line, Entry& out) {
  // Format: english|vietnamese
  const size_t pos = line.find('|');
  if (pos == string::npos) return false;
  string en = trim(line.substr(0, pos));
  string vi = trim(line.substr(pos + 1));
  if (en.empty() || vi.empty()) return false;
  // Không cho phép '|' xuất hiện trong dữ liệu vì sẽ phá cấu trúc file.
  // (Nếu muốn nâng cấp: có thể escape như \| hoặc dùng CSV có quote.)
  if (containsPipe(en) || containsPipe(vi)) return false;
  out = Entry{en, vi};
  return true;
}

static bool loadFromFile(HashDictionary& dict, const string& path) {
  ifstream in(path);
  if (!in) return false;
  string line;
  size_t loaded = 0;
  while (getline(in, line)) {
    line = trim(line);
    if (line.empty()) continue;
    if (!line.empty() && line[0] == '#') continue;
    Entry e;
    if (!parseLineEntry(line, e)) continue;
    if (dict.addOrUpdate(e.en, e.vi)) loaded++;
  }
  cout << "Loaded/updated: " << loaded << " entries.\n";
  return true;
}

static bool saveToFile(const HashDictionary& dict, const string& path) {
  ofstream out(path);
  if (!out) return false;
  vector<Entry> all = dict.getAllEntries();
  stable_sort(all.begin(), all.end(), [](const Entry& a, const Entry& b) {
    return toLowerAscii(a.en) < toLowerAscii(b.en);
  });
  out << "# Format: english|vietnamese\n";
  for (const auto& e : all) out << e.en << " | " << e.vi << "\n";
  return true;
}

static string promptLine(const string& label) {
  cout << label;
  string s;
  getline(cin, s);
  return s;
}

static int promptInt(const string& label, int lo, int hi) {
  while (true) {
    cout << label;
    string s;
    getline(cin, s);
    stringstream ss(s);
    int x;
    if (ss >> x && x >= lo && x <= hi) return x;
    cout << "Invalid. Please enter integer in [" << lo << ", " << hi << "].\n";
  }
}

static string randomWord(mt19937& rng, int len) {
  uniform_int_distribution<int> d('a', 'z');
  string s;
  s.reserve(static_cast<size_t>(len));
  for (int i = 0; i < len; i++) s.push_back(static_cast<char>(d(rng)));
  return s;
}

static void runPerformanceTest() {
  cout << "== Performance test (hash+chaining) ==\n";
  cout << "This test generates random keys and measures insertion/lookup time.\n";
  vector<int> sizes = {1000, 5000, 10000, 20000, 50000};

  random_device rd;
  mt19937 rng(rd());
  uniform_int_distribution<int> lenDist(4, 10);

  for (int n : sizes) {
    HashDictionary dict;
    vector<string> keys;
    keys.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; i++) keys.push_back(randomWord(rng, lenDist(rng)));

    auto t1 = chrono::high_resolution_clock::now();
    for (int i = 0; i < n; i++) dict.addOrUpdate(keys[i], "nghia");
    auto t2 = chrono::high_resolution_clock::now();

    // Lookup half existing + half random miss
    int q = n;
    int hits = 0;
    auto t3 = chrono::high_resolution_clock::now();
    for (int i = 0; i < q / 2; i++) {
      if (dict.lookup(keys[i])) hits++;
    }
    for (int i = 0; i < q / 2; i++) {
      if (dict.lookup(keys[i] + "x")) hits++;
    }
    auto t4 = chrono::high_resolution_clock::now();

    auto insMs = chrono::duration_cast<chrono::milliseconds>(t2 - t1).count();
    auto lookMs = chrono::duration_cast<chrono::milliseconds>(t4 - t3).count();
    cout << "n=" << setw(6) << n << " | insert(ms)=" << setw(6) << insMs
         << " | lookup(ms)=" << setw(6) << lookMs << " | hits=" << hits << "\n";
  }
}

static void printMenu() {
  cout << "\n===== EN-VI Dictionary (Hash Table + Linked List) =====\n";
  cout << "1) Load from file\n";
  cout << "2) Save to file\n";
  cout << "3) Add new word\n";
  cout << "4) Lookup word\n";
  cout << "5) Update meaning\n";
  cout << "6) Delete word\n";
  cout << "7) Display by group (first letter)\n";
  cout << "8) Display all\n";
  cout << "9) Performance test\n";
  cout << "0) Exit\n";
}

int main() {
  ios::sync_with_stdio(false);
  cin.tie(nullptr);

  HashDictionary dict;

  while (true) {
    printMenu();
    int choice = promptInt("Choose: ", 0, 9);
    if (choice == 0) {
      cout << "Bye.\n";
      return 0;
    }

    if (choice == 1) {
      string path = promptLine("File path to load: ");
      if (!loadFromFile(dict, trim(path))) cout << "Cannot load file.\n";
    } else if (choice == 2) {
      string path = promptLine("File path to save: ");
      if (!saveToFile(dict, trim(path))) cout << "Cannot save file.\n";
      else cout << "Saved.\n";
    } else if (choice == 3) {
      string en = promptLine("English: ");
      string vi = promptLine("Vietnamese meaning: ");
      if (!startsWithAlpha(trim(en))) {
        cout << "Note: English word does not start with A-Z; it will go to non-alpha group.\n";
      }
      if (!dict.addOrUpdate(en, vi)) {
        cout << "Invalid input. Note: character '|' is not allowed.\n";
      }
      else cout << "Added/updated.\n";
    } else if (choice == 4) {
      string en = promptLine("English to lookup: ");
      const Entry* e = dict.lookup(en);
      if (!e) cout << "Not found.\n";
      else cout << "Found: " << e->en << " = " << e->vi << "\n";
    } else if (choice == 5) {
      string en = promptLine("English to update: ");
      string vi = promptLine("New Vietnamese meaning: ");
      if (!dict.updateMeaning(en, vi)) {
        cout << "Update failed (not found or meaning contains '|').\n";
      }
      else cout << "Updated.\n";
    } else if (choice == 6) {
      string en = promptLine("English to delete: ");
      if (!dict.remove(en)) cout << "Not found.\n";
      else cout << "Deleted.\n";
    } else if (choice == 7) {
      cout << "Enter group letter A-Z or '*' for non-alpha.\n";
      string g = promptLine("Group: ");
      g = trim(g);
      int groupIdx = -1;
      if (g == "*") groupIdx = 26;
      else if (!g.empty()) {
        char c = static_cast<char>(toupper(static_cast<unsigned char>(g[0])));
        if (c >= 'A' && c <= 'Z') groupIdx = c - 'A';
      }
      if (groupIdx < 0) {
        cout << "Invalid group.\n";
      } else {
        int sorted = promptInt("Sort within group? (0/1): ", 0, 1);
        dict.printGroup(groupIdx, sorted == 1);
      }
    } else if (choice == 8) {
      int sorted = promptInt("Sort output? (0/1): ", 0, 1);
      dict.printAll(sorted == 1);
    } else if (choice == 9) {
      runPerformanceTest();
    }
  }
}

