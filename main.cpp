#include <iostream>
#include <string>
#include <cctype>

using namespace std;

struct Entry {
    string en;
    string vi;
};

struct Node {
    Entry data;
    Node* next;
};

string toLowerStr(string s) {
    for (char &c : s) c = tolower(c);
    return s;
}

int hashFirstLetter(string en) {
    if (en.empty()) return 26;
    char c = tolower(en[0]);
    if (c >= 'a' && c <= 'z') return c - 'a';
    return 26;
}

Node* T[27] = {nullptr};

bool insertWord(string en, string vi) {
    if (en.empty() || vi.empty()) return false;

    string key = toLowerStr(en);
    int idx = hashFirstLetter(key);

    Node* cur = T[idx];

    while (cur != nullptr) {
        if (toLowerStr(cur->data.en) == key) {
            cur->data.vi = vi;
            return false;
        }
        cur = cur->next;
    }

    Node* newNode = new Node;
    newNode->data.en = en;
    newNode->data.vi = vi;
    newNode->next = T[idx];
    T[idx] = newNode;

    return true;
}

Node* searchWord(string en) {
    if (en.empty()) return nullptr;

    string key = toLowerStr(en);
    int idx = hashFirstLetter(key);

    Node* cur = T[idx];

    while (cur != nullptr) {
        if (toLowerStr(cur->data.en) == key) {
            return cur;
        }
        cur = cur->next;
    }

    return nullptr;
}