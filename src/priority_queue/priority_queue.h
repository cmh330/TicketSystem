#ifndef SJTU_PRIORITY_QUEUE_HPP
#define SJTU_PRIORITY_QUEUE_HPP

#include <cmath>       // in case you need it
#include <cstddef>     // for size_t
#include <functional>  // for std::less

#include "exceptions.hpp"

namespace sjtu {

/**
 * @brief A container automatically sorting its contents, similar to
 * std::priority_queue but with extra functionalities.
 *
 * The extra functionalities are:
 * - Merge two priority queues into one (with good time complexity).
 * - Clear all elements in the queue.
 * - Limited exception safety for some operations (e.g. push, pop, top, merge)
 * when the comparator throws exceptions from `Compare` only.
 *
 * This @priority_queue does not support passing an underlying container as a template parameter.
 * Also, it does not support passing a comparator object as a constructor argument.
 *
 */
template <class T, class Compare = std::less<T>>
class priority_queue {
    private:
    struct Node {
        Node* left = nullptr;
        Node* right = nullptr;
        T data;
        Node(const T& data_) : data(data_) {}
    };

    Node* root = nullptr;
    Compare comp;
    int sz = 0;

    void swap(Node*& a, Node*& b) {
        Node* temp = a;
        a = b;
        b = temp;
    }
    // total length of the right side of a and b, used in merge
    int total_length(Node*a, Node* b) {
        int total = 0;
        for (Node* temp = a; temp != nullptr; temp = temp->right) ++total;
        for (Node* temp = b; temp != nullptr; temp = temp->right) ++total;
        return total;
    }

    Node* Merge(Node* a, Node* b) {
        if (a == nullptr) return b;
        if (b == nullptr) return a;

        int length = total_length(a, b);
        Node** compare_result = new Node*[length];
        int n = 0;

        try {
            while (a != nullptr && b != nullptr) {
                if (comp(a->data, b->data)) swap(a, b);
                compare_result[n++] = a;
                a = a->right;
            }
        } catch (...) {
            delete[] compare_result;
            throw;
        }

        while (a != nullptr) {
            compare_result[n++] = a;
            a = a->right;
        }
        while (b != nullptr) {
            compare_result[n++] = b;
            b = b->right;
        }

        Node* temp = nullptr;
        for (int i = length - 1; i >= 0; --i) {
            compare_result[i]->right = temp;
            swap(compare_result[i]->left, compare_result[i]->right);
            temp = compare_result[i];
        }
        delete[] compare_result;
        return temp;
    }

    void destroy(Node* a) {
        if (a == nullptr) return;
        destroy(a->right);
        destroy(a->left);
        delete a;
    }

    Node* copy(Node* a) {
        if (a == nullptr) return nullptr;
        Node* new_ = new Node(a->data);
        new_->left = copy(a->left);
        new_->right = copy(a->right);
        return new_;
    }


   public:
    priority_queue() : root(nullptr), sz(0) {}
    priority_queue(const priority_queue&other) {
        root = copy(other.root);
        sz = other.sz;
    }
    ~priority_queue() {
        destroy(root);
    }

    priority_queue& operator=(const priority_queue&other) {
        if (this == &other) return *this;
        clear();
        root = copy(other.root);
        sz = other.sz;
        return *this;
    }

    /** Adds one element to the queue. */
    void push(const T&d) {
        Node* new_ = new Node(d);
        try {
            root = Merge(root, new_);
        } catch (...) {
            delete new_;
            throw;
        }
        ++sz;
    }

    /**
     * Returns a read-only reference of the first element in the queue.
     *
     * @throws container_is_empty when the first element does not exist.
     */
    const T& top() const {
        if (sz == 0) throw container_is_empty();
        return root->data;
    }

    /**
     * Removes the first element in the queue.
     *
     * @throws container_is_empty when the first element does not exist.
     */
    void pop() {
        if (sz == 0) throw container_is_empty();
        Node* old_root = root;
        root = Merge(root->right, root->left);
        delete old_root;
        --sz;
    }

    /** Returns the number of elements in the queue. */
    size_t size() const {
        return sz;
    }

    /** Returns whether there is any element in the queue. */
    bool empty() const {
        return sz == 0;
    }

    /** Clears all elements in the queue. */
    void clear() {
        destroy(root);
        root = nullptr;
        sz = 0;
    }

    /**
     * @brief Merges two priority queues into one.
     *
     * The merged data shall be stored in the current priority queue and the
     * other priority queue shall be cleared after merging.
     *
     * The time complexity shall be O(log n) or better.
     */
    void merge(priority_queue&other) {
        if (this == &other) return;
        root = Merge(root, other.root);
        sz += other.sz;
        other.sz = 0;
        other.root = nullptr;
    }
};

}  // namespace sjtu

#endif