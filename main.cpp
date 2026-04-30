#include <iostream>
#include <string>

using namespace std;

// ======================
// Struct lưu dữ liệu từ
// ======================
struct Entry {
    string english;
    string vietnamese;
};

// ======================
// Struct Node (Linked List)
// ======================
struct Node {
    Entry data;
    Node* next;

    Node(string en, string vi) {
        data.english = en;
        data.vietnamese = vi;
        next = nullptr;
    }
};

// ======================
// Hash Table
// ======================
class HashTable {
private:
    static const int SIZE = 101; // Sử dụng số nguyên tố để giảm xung đột
    Node* table[SIZE];

public:
    // Constructor: Khởi tạo bảng băm rỗng
    HashTable() {
        for (int i = 0; i < SIZE; i++) {
            table[i] = nullptr;
        }
    }

    // Destructor: Giải phóng bộ nhớ khi chương trình kết thúc (Rất quan trọng)
    ~HashTable() {
        for (int i = 0; i < SIZE; i++) {
            Node* current = table[i];
            while (current != nullptr) {
                Node* temp = current;
                current = current->next;
                delete temp; // Xóa từng node trong bucket
            }
            table[i] = nullptr;
        }
    }

    // ======================
    // Hàm hash (Polynomial Rolling Hash)
    // ======================
    int hashFunction(string key) {
        unsigned int hash = 0; // Dùng unsigned để tránh số âm
        for (char c : key) {
            hash = hash * 31 + c;
        }
        return hash % SIZE;
    }

    // ======================
    // Thêm từ mới hoặc cập nhật
    // ======================
    void insert(string en, string vi) {
        int index = hashFunction(en);
        Node* current = table[index];

        // Kiểm tra xem từ đã tồn tại chưa, nếu có thì cập nhật nghĩa
        while (current != nullptr) {
            if (current->data.english == en) {
                current->data.vietnamese = vi; // Cập nhật nghĩa mới
                return;
            }
            current = current->next;
        }

        // Nếu chưa tồn tại, thêm vào đầu danh sách (Chaining)
        Node* newNode = new Node(en, vi);
        newNode->next = table[index];
        table[index] = newNode;
    }

    // ======================
    // Tìm kiếm từ
    // ======================
    void search(string en) {
        int index = hashFunction(en);
        Node* current = table[index];

        while (current != nullptr) {
            if (current->data.english == en) {
                cout << " [OK] " << current->data.english 
                     << " có nghĩa là: " << current->data.vietnamese << endl;
                return;
            }
            current = current->next;
        }

        cout << " [!] Không tìm thấy từ: " << en << endl;
    }

    // ======================
    // Hiển thị cấu trúc hash table
    // ======================
    void display() {
        cout << "\n--- CAU TRUC BANG BAM ---" << endl;
        for (int i = 0; i < SIZE; i++) {
            if (table[i] != nullptr) {
                cout << "Bucket " << i << ": ";
                Node* current = table[i];
                while (current != nullptr) {
                    cout << "[" << current->data.english << ":" << current->data.vietnamese << "] -> ";
                    current = current->next;
                }
                cout << "NULL" << endl;
            }
        }
        cout << "-------------------------\n" << endl;
    }
    // ======================
// Xóa từ
// ======================
void deleteWord(string en) {
    if (en.empty()) {
        cout << "Tu khong hop le!\n";
        return;
    }

    int index = hashFunction(en);
    Node* current = table[index];
    Node* prev = nullptr;

    while (current != nullptr) {
        if (current->data.english == en) {

            // Xóa node đầu
            if (prev == nullptr) {
                table[index] = current->next;
            }
            // Xóa giữa / cuối
            else {
                prev->next = current->next;
            }

            delete current;
            cout << " [OK] Da xoa tu: " << en << endl;
            return;
        }

        prev = current;
        current = current->next;
    }

    cout << " [!] Khong tim thay tu: " << en << endl;
}
// ======================
// Cập nhật từ
// ======================
void updateWord(string en) {
    if (en.empty()) {
        cout << "Tu khong hop le!\n";
        return;
    }

    int index = hashFunction(en);
    Node* current = table[index];

    while (current != nullptr) {
        if (current->data.english == en) {

            cout << "Tim thay tu!\n";
            cout << "1. Sua nghia\n";
            cout << "2. Sua ca tu\n";
            cout << "Chon: ";

            int choice;
            cin >> choice;
            cin.ignore();

            // ===== CASE 1: SỬA NGHĨA =====
            if (choice == 1) {
                string newMeaning;
                cout << "Nhap nghia moi: ";
                getline(cin, newMeaning);

                current->data.vietnamese = newMeaning;
                cout << " [OK] Cap nhat nghia thanh cong!\n";
            }

            // ===== CASE 2: SỬA CẢ TỪ =====
            else if (choice == 2) {
                string newWord, newMeaning;

                cout << "Nhap tu moi: ";
                getline(cin, newWord);
                cout << "Nhap nghia moi: ";
                getline(cin, newMeaning);

                // kiểm tra trùng
                int newIndex = hashFunction(newWord);
                Node* check = table[newIndex];

                while (check != nullptr) {
                    if (check->data.english == newWord) {
                        cout << " [!] Tu moi da ton tai!\n";
                        return;
                    }
                    check = check->next;
                }

                // xóa cũ + thêm mới
                deleteWord(en);
                insert(newWord, newMeaning);

                cout << " [OK] Cap nhat tu thanh cong!\n";
            }

            return;
        }

        current = current->next;
    }

    cout << " [!] Khong tim thay tu!\n";
}
};

// ======================
// Chương trình chính
// ======================
int main() {
    HashTable dict;

    // Thêm dữ liệu
    dict.insert("apple", "qua tao");
    dict.insert("cat", "con meo");
    dict.insert("dog", "con cho");
    dict.insert("apple", "trai tao"); // Test cập nhật nghĩa cho từ trùng

    // Hiển thị bảng băm
    dict.display();

    // Tìm kiếm
    cout << "Ket qua tim kiem:" << endl;
    dict.search("cat");
    dict.search("apple");
    dict.search("banana");

    return 0;
}
