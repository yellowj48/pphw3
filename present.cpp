//g++ -std=c++14 present.cpp -o present -pthread

#include <iostream>
#include <memory>
#include <mutex>
#include <unordered_set>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <algorithm>
#include <random>
#include <climits>

#define THREAD_COUNT 4
#define NUM_GUESTS 500000

std::atomic<int> presentsAdded(0);
std::atomic<int> cardsWritten(0);
std::mutex giftBagMutex;
std::mutex cardsMutex;

class Node {
public:
    int data;
    Node* next;
    Node* prev;
    std::mutex mutex;

    Node(int n) : data(n), next(nullptr), prev(nullptr) {}
    ~Node() {}
};

class ConcurrentLinkedList {
private:
    Node* m_head;
    Node* m_tail;
    size_t m_size;
    std::mutex m_mutex;

public:
    ConcurrentLinkedList() : m_head(nullptr), m_tail(nullptr), m_size(0) {}
    ~ConcurrentLinkedList() {
        Node* current = m_head;
        while (current != nullptr) {
            Node* next = current->next;
            delete current;
            current = next;
        }
    }

    void insert(int data) {
        std::unique_lock<std::mutex> lock(m_mutex);

        Node* newNode = new Node(data);
        if (!m_head) {
            m_head = newNode;
            m_tail = newNode;
        } else {
            m_tail->next = newNode;
            newNode->prev = m_tail;
            m_tail = newNode;
        }
        ++m_size;
    }

    int removeHead() {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_head) {
            return INT_MIN;
        }

        Node* temp = m_head;
        int data = temp->data;
        m_head = m_head->next;
        if (m_head) {
            m_head->prev = nullptr;
        } else {
            m_tail = nullptr;
        }
        delete temp;
        --m_size;
        return data;
    }

    bool empty() {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_size == 0;
    }
};

// Generates a shuffled unordered set containing numbers from 0 to size - 1
std::unique_ptr<std::unordered_set<int>> generateUnorderedSet(int size) {
    std::unique_ptr<std::vector<int>> vec = std::make_unique<std::vector<int>>();
    for (int i = 0; i < size; i++) {
        vec->push_back(i);
    }

    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(vec->begin(), vec->end(), std::default_random_engine(seed));
    return std::make_unique<std::unordered_set<int>>(vec->begin(), vec->end());
}

// Util class for generating random numbers, replace with your Util implementation
class Util {
public:
    static int generateRandomNumber(int min, int max) {
        static std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(min, max);
        return dist(rng);
    }
};

void completeTask(ConcurrentLinkedList* list, std::unordered_set<int>* giftBag, std::unordered_set<int>* cards) {
    while (cardsWritten < NUM_GUESTS) {
        int task = Util::generateRandomNumber(0, 2);

        if (task == 0) { // TASK_ADD_PRESENT
            std::lock_guard<std::mutex> lock(giftBagMutex);
            if (!giftBag->empty()) {
                auto it = giftBag->begin();
                int present = *it;
                giftBag->erase(it);
                list->insert(present);
                int added = ++presentsAdded;
                // print statement
                if (added % 10000 == 0) {
                    std::cout << added << " presents added.\n";
                }
            }
        } else if (task == 1) { // TASK_WRITE_CARD
            int guest = list->removeHead();
            if (guest != INT_MIN) {
                std::lock_guard<std::mutex> lock(cardsMutex);
                cards->insert(guest);
                int written = ++cardsWritten;
                // print statement
                if (written % 10000 == 0) {
                    std::cout << written << " thank you cards written.\n";
                }
            }
        }
        // TASK_SEARCH_FOR_PRESENT can be implemented if needed
    }
}
int main() {
    auto giftBag = generateUnorderedSet(NUM_GUESTS);
    auto list = std::make_unique<ConcurrentLinkedList>();
    auto cards = std::make_unique<std::unordered_set<int>>();
    std::thread threads[THREAD_COUNT];

    auto start = std::chrono::high_resolution_clock::now(); // Start time measurement

    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads[i] = std::thread(completeTask, list.get(), giftBag.get(), cards.get());
    }

    for (auto& thread : threads) {
        thread.join();
    }

    auto end = std::chrono::high_resolution_clock::now(); // End time measurement
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "All " << NUM_GUESTS << " presents processed and thank you cards written.\n";
    std::cout << "Execution time: " << duration << " ms\n";
    return 0;
}
