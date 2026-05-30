/**
 * implement a container like std::map
 */
#ifndef SJTU_MAP_HPP
#define SJTU_MAP_HPP

// only for std::less<T>
#include <functional>
#include <cstddef>
#include "../map/utility.h"
#include "../map/exceptions.h"

namespace sjtu {

template<
    class Key,
    class T,
    class Compare = std::less <Key>
> class map {
 public:
  /**
   * the internal type of data.
   * it should have a default constructor, a copy constructor.
   * You can use sjtu::map as value_type by typedef.
   */
  typedef pair<const Key, T> value_type;

private:
  struct Node {
    Node* parent, *left, *right;
    value_type* value;
    int height;

    Node(const value_type &v, Node* parent_ = nullptr) : parent(parent_), height(1), left(nullptr), right(nullptr) {
      value = new value_type(v);
    }
    ~Node() {
      delete value;
    }
  };

  Node* root;
  size_t sz;
  Compare comp;

  int height(Node* p) const {
    if (!p) return 0;
    else return p->height;
  }
  void update_height(Node* p) const {
    if (p) {
      p->height = 1 + std::max(height(p->left), height(p->right));
    }
  }
  int bf(Node* p) const {
    if (!p) return 0;
    else return height(p->left) - height(p->right);
  }

  // 右旋
  Node* rotate_right(Node* p) {
    Node* node = p->left;
    Node* node2 = node->right;
    p->left = node2;
    if (node2) node2->parent = p;
    node->right = p;
    node->parent = p->parent;
    p->parent = node;

    update_height (p);
    update_height (node);
    return node;
  }
  // 左旋
  Node* rotate_left(Node* p) {
    Node* node = p->right;
    Node* node2 = node->left;
    p->right = node2;
    if (node2) node2->parent = p;
    node->left = p;
    node->parent = p->parent;
    p->parent = node;

    update_height (p);
    update_height (node);
    return node;
  }
  // 调整平衡，返回新根
  Node* rebalance(Node* p) {
    update_height (p);
    int bf_ = bf(p);

    if (bf_ == 2) {
      // LL
      if (bf(p->left) >= 0) {
        return rotate_right(p);
      }
      // LR
      else if (bf(p->left) == -1) {
        p->left = rotate_left(p->left);
        p->left->parent = p;
        return rotate_right(p);
      }
    } else if (bf_ == -2) {
      // RR
      if (bf(p->right) <= 0) {
        return rotate_left(p);
      }
      // RL
      else if (bf(p->right) == 1) {
        p->right = rotate_right(p->right);
        p->right->parent = p;
        return rotate_left(p);
      }
    }
    return p;
  }
  // 复制一棵树
  Node* copy(Node* node, Node* parent) {
    if (!node) return nullptr;
    Node* new_node = new Node (*node->value, parent);
    new_node->height = node->height;
    new_node->left = copy(node->left, new_node);
    new_node->right = copy(node->right, new_node);
    return new_node;
  }
  // 删除一棵树
  void destroy(Node* p) {
    if (!p) return;
    destroy(p->left);
    destroy(p->right);
    delete p;
  }

  // insert辅助函数
  Node* insert_node(Node* node,Node* parent, const value_type &v, Node*& inserted) {
    if (!node) {
      node = new Node(v, parent);
      inserted = node;
      ++sz;
      return node;
    }
    if (comp(v.first, node->value->first)) {
      node->left = insert_node(node->left, node, v, inserted);
    } else if (comp(node->value->first, v.first)) {
      node->right = insert_node(node->right, node, v, inserted);
    } else {
      // 相等，不插入
      inserted = node;
      return node;
    }
    return rebalance(node);
  }

  // erase辅助函数
  Node* erase_node(Node* node, const Key& key) {
    if (!node) return nullptr;

    if (comp(key,node->value->first)) {
      node->left = erase_node(node->left, key);
      if (node->left) node->left->parent = node;
    } else if (comp(node->value->first, key)) {
      node->right = erase_node(node->right, key);
      if (node->right) node->right->parent = node;
    } else {
      // 找到了
      if (!node->left || !node->right) {
        // 无子节点或只有一个子节点
        Node* child = node->left ? node->left : node->right;
        if (child) child->parent = node->parent;
        delete node;
        --sz;
        return child;
      } else {
        // 找右子树最左节点
        Node* temp = node->right;
        while (temp->left) temp = temp->left;

        if (temp->parent != node) {
          temp->parent->left = temp->right;
          if (temp->right) temp->right->parent = temp->parent;
          temp->right = node->right;
          temp->right->parent = temp;
        }
        temp->left = node->left;
        if (temp->left) temp->left->parent = temp;
        temp->parent = node->parent;

        delete node;
        --sz;
        return rebalance(temp);
      }
    }

    return rebalance(node);
  }

  // 中序遍历辅助函数
  Node* left(Node* p) const {
    if (!p) return nullptr;
    while (p->left) p = p->left;
    return p;
  }
  Node* right(Node* p) const {
    if (!p) return nullptr;
    while (p->right) p = p->right;
    return p;
  }
  // 中序后继（++it）
  Node* next(Node* p) const {
    if (!p) return nullptr;
    if (p->right) return left(p->right);
    // 向上找祖先
    Node* par = p->parent;
    while (par && p == par->right) {
      p = par;
      par = par->parent;
    }
    return par;
  }
  // 中序前驱（--it）
  Node* previous(Node* p) const {
    if (!p) return nullptr;
    if (p->left) return right(p->left);
    // 向上
    Node* par = p->parent;
    while (par && p == par->left) {
      p = par;
      par = par->parent;
    }
    return par;
  }

  // 查找辅助函数
  Node* find_node(Node* p, const Key& key) const {
    while (p) {
      if (comp(key, p->value->first)) {
        p = p->left;
      } else if (comp(p->value->first, key)) {
        p = p->right;
      } else {
        return p;
      }
    }
    return nullptr;
  }
  /**
   * see BidirectionalIterator at CppReference for help.
   *
   * if there is anything wrong throw invalid_iterator.
   *     like it = map.begin(); --it;
   *       or it = map.end(); ++end();
   */
public:
  class const_iterator;
  class iterator {
    friend class const_iterator;
    friend class map;
   private:
    /**
     * TODO add data members
     *   just add whatever you want.
     */
    const map* map_;
    Node* current;
   public:
    iterator() : map_(nullptr), current(nullptr) {
      // TODO
    }

    iterator(const iterator &other) : map_(other.map_), current(other.current) {
      // TODO
    }

    iterator(Node* node, const map* m) : map_(m), current(node) {}

    /**
     * TODO iter++
     */
    iterator operator++(int) {
      iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    /**
     * TODO ++iter
     */
    iterator &operator++() {
      if (!current || !map_) throw invalid_iterator();
      current = map_->next(current);
      return *this;
    }

    /**
     * TODO iter--
     */
    iterator operator--(int) {
      iterator tmp = *this;
      --(*this);
      return tmp;
    }

    /**
     * TODO --iter
     */
    iterator &operator--() {
      if (!map_) throw invalid_iterator();
      if (!current) {
        // 如果是end()--
        current = map_->right(map_->root);
        if (!current) throw invalid_iterator();
        return *this;
      }
      Node* p = map_->previous(current);
      if (!p) throw invalid_iterator(); // 如果是begin()--
      current = p;
      return *this;
    }

    /**
     * a operator to check whether two iterators are same (pointing to the same memory).
     */
    value_type &operator*() const {
      if (!current) throw invalid_iterator();
      return *current->value;
    }

    bool operator==(const iterator &rhs) const {
      return map_ == rhs.map_ && current == rhs.current;
    }

    bool operator==(const const_iterator &rhs) const {
      return map_ == rhs.map_ && current == rhs.current;
    }

    /**
     * some other operator for iterator.
     */
    bool operator!=(const iterator &rhs) const {
      return !(*this == rhs);
    }

    bool operator!=(const const_iterator &rhs) const {
      return !(*this == rhs);
    }

    /**
     * for the support of it->first.
     * See <http://kelvinh.github.io/blog/2013/11/20/overloading-of-member-access-operator-dash-greater-than-symbol-in-cpp/> for help.
     */
    value_type *operator->() const
    noexcept {
      if (!current) return nullptr;
      return current->value;
    }
  };
  class const_iterator {
    // it should has similar member method as iterator.
    //  and it should be able to construct from an iterator.
    friend class map;
    friend class iterator;
   private:
    // data members.
    const map* map_;
    Node* current;
   public:
    const_iterator() : map_(nullptr), current(nullptr) {
      // TODO
    }

    const_iterator(const const_iterator &other) : map_(other.map_), current(other.current) {
      // TODO
    }

    const_iterator(const iterator &other) : map_(other.map_), current(other.current) {
      // TODO
    }

    const_iterator(Node* node, const map* m) : map_(m), current(node) {}
    // And other methods in iterator.
    // And other methods in iterator.
    // And other methods in iterator.
    const_iterator operator++(int) {
      const_iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    /**
     * TODO ++iter
     */
    const_iterator &operator++() {
      if (!current || !map_) throw invalid_iterator();
      current = map_->next(current);
      return *this;
    }

    /**
     * TODO iter--
     */
    const_iterator operator--(int) {
      const_iterator tmp = *this;
      --(*this);
      return tmp;
    }

    /**
     * TODO --iter
     */
    const_iterator &operator--() {
      if (!map_) throw invalid_iterator();
      if (!current) {
        // 如果是end()--
        current = map_->right(map_->root);
        if (!current) throw invalid_iterator();
        return *this;
      }
      Node* p = map_->previous(current);
      if (!p) throw invalid_iterator(); // 如果是begin()--
      current = p;
      return *this;
    }

    /**
     * a operator to check whether two iterators are same (pointing to the same memory).
     */
    const value_type &operator*() const {
      if (!current) throw invalid_iterator();
      return *current->value;
    }

    bool operator==(const iterator &rhs) const {
      return map_ == rhs.map_ && current == rhs.current;
    }

    bool operator==(const const_iterator &rhs) const {
      return map_ == rhs.map_ && current == rhs.current;
    }

    /**
     * some other operator for iterator.
     */
    bool operator!=(const iterator &rhs) const {
      return !(*this == rhs);
    }

    bool operator!=(const const_iterator &rhs) const {
      return !(*this == rhs);
    }

    /**
     * for the support of it->first.
     * See <http://kelvinh.github.io/blog/2013/11/20/overloading-of-member-access-operator-dash-greater-than-symbol-in-cpp/> for help.
     */
    const value_type *operator->() const
    noexcept {
      if (!current) return nullptr;
      return current->value;
    }
  };

  /**
   * TODO two constructors
   */
public:

  map() : root(nullptr), sz(0) {}

  map(const map &other) : sz(other.sz), comp(other.comp) {
    root = copy(other.root, nullptr);
  }

  /**
   * TODO assignment operator
   */
  map &operator=(const map &other) {
    if (this == &other) {
      return *this;
    }
    destroy(root);
    root = copy(other.root, nullptr);
    sz = other.sz;
    comp = other.comp;
    return *this;
  }

  /**
   * TODO Destructors
   */
  ~map() {
    destroy(root);
  }

  /**
   * TODO
   * access specified element with bounds checking
   * Returns a reference to the mapped value of the element with key equivalent to key.
   * If no such element exists, an exception of type `index_out_of_bound'
   */
  T &at(const Key &key) {
    Node* p = find_node(root, key);
    if (!p) throw index_out_of_bound();
    return p->value->second;
  }

  const T &at(const Key &key) const {
    Node* p = find_node(root, key);
    if (!p) throw index_out_of_bound();
    return p->value->second;
  }

  /**
   * TODO
   * access specified element
   * Returns a reference to the value that is mapped to a key equivalent to key,
   *   performing an insertion if such key does not already exist.
   */
  T &operator[](const Key &key) {
    Node* inserted = nullptr;
    root = insert_node(root, nullptr, value_type(key, T()), inserted);
    if (root) root->parent = nullptr;
    return inserted->value->second;
  }

  /**
   * behave like at() throw index_out_of_bound if such key does not exist.
   */
  const T &operator[](const Key &key) const {
    Node* p = find_node(root, key);
    if (!p) throw index_out_of_bound();
    return p->value->second;
  }

  /**
   * return a iterator to the beginning
   */
  iterator begin() {
    Node* p = left(root);
    return iterator(p, this);
  }

  const_iterator cbegin() const {
    Node* p = left(root);
    return const_iterator(p, this);
  }

  /**
   * return a iterator to the end
   * in fact, it returns past-the-end.
   */
  iterator end() {
    return iterator(nullptr, this);
  }

  const_iterator cend() const {
    return const_iterator(nullptr, this);
  }

  /**
   * checks whether the container is empty
   * return true if empty, otherwise false.
   */
  bool empty() const {
    return size() == 0;
  }

  /**
   * returns the number of elements.
   */
  size_t size() const {
    return sz;
  }

  /**
   * clears the contents
   */
  void clear() {
    destroy(root);
    root = nullptr;
    sz = 0;
  }

  /**
   * insert an element.
   * return a pair, the first of the pair is
   *   the iterator to the new element (or the element that prevented the insertion),
   *   the second one is true if insert successfully, or false.
   */
  pair<iterator, bool> insert(const value_type &value) {
    size_t old_size = sz;
    Node* inserted = nullptr;
    root = insert_node(root, nullptr, value, inserted);
    if (root) root->parent = nullptr;
    bool success = false;
    if (sz != old_size) success = true;
    return pair<iterator, bool>(iterator(inserted, this), success);
  }

  /**
   * erase the element at pos.
   *
   * throw if pos pointed to a bad element (pos == this->end() || pos points an element out of this)
   */
  void erase(iterator pos) {
    if (pos == end() || pos.map_ != this) {
      throw invalid_iterator();
    }
    root = erase_node(root, pos.current->value->first);
    if (root) root->parent = nullptr;
  }

  /**
   * Returns the number of elements with key
   *   that compares equivalent to the specified argument,
   *   which is either 1 or 0
   *     since this container does not allow duplicates.
   * The default method of check the equivalence is !(a < b || b > a)
   */
  size_t count(const Key &key) const {
    if (find_node(root, key)) return 1;
    else return 0;
  }

  /**
   * Finds an element with key equivalent to key.
   * key value of the element to search for.
   * Iterator to an element with key equivalent to key.
   *   If no such element is found, past-the-end (see end()) iterator is returned.
   */
  iterator find(const Key &key) {
    Node* p = find_node(root, key);
    return iterator(p, this);
  }

  const_iterator find(const Key &key) const {
    Node* p = find_node(root, key);
    return const_iterator(p, this);
  }
};

}

#endif