/**
* implement a container like std::map
*/
#ifndef SJTU_MAP_HPP
#define SJTU_MAP_HPP

// only for std::less<T>
#include <functional>
#include <cstddef>
#include "utility.hpp"
#include "exceptions.hpp"

namespace sjtu {

template<
   class Key,
   class T,
   class Compare = std::less<Key>
   > class map {
 public:
  typedef pair<const Key, T> value_type;

 private:
  struct Node {
    value_type data;
    int level;
    Node *left, *right, *parent;
    Node(const value_type &v, Node *p) : data(v), level(1), left(nullptr), right(nullptr), parent(p) {}
  };

  Node *root = nullptr;
  size_t _size = 0;
  Compare comp;

  static int lvl(Node *x) { return x ? x->level : 0; }

  Node *rotate_left(Node *x) {
    Node *r = x->right;
    x->right = r->left;
    if (r->left) r->left->parent = x;
    r->left = x;
    r->parent = x->parent;
    x->parent = r;
    return r;
  }

  Node *rotate_right(Node *x) {
    Node *l = x->left;
    x->left = l->right;
    if (l->right) l->right->parent = x;
    l->right = x;
    l->parent = x->parent;
    x->parent = l;
    return l;
  }

  Node *skew(Node *x) {
    if (!x || !x->left) return x;
    if (lvl(x->left) == x->level) {
      Node *n = rotate_right(x);
      return n;
    }
    return x;
  }

  Node *split(Node *x) {
    if (!x || !x->right) return x;
    if (x->right && lvl(x->right->right) == x->level) {
      Node *n = rotate_left(x);
      n->level++;
      return n;
    }
    return x;
  }

  Node *insert_rec(Node *x, Node *parent, const value_type &val, Node *&placed, bool &existed) {
    if (!x) {
      placed = new Node(val, parent);
      return placed;
    }
    if (comp(val.first, x->data.first)) {
      x->left = insert_rec(x->left, x, val, placed, existed);
    } else if (comp(x->data.first, val.first)) {
      x->right = insert_rec(x->right, x, val, placed, existed);
    } else {
      existed = true;
      placed = x;
      return x;
    }
    x = skew(x);
    x = split(x);
    return x;
  }

  Node *decrease_level(Node *x) {
    int l = lvl(x->left), r = lvl(x->right);
    int should = (l < r ? l : r) + 1;
    if (should < x->level) {
      x->level = should;
      if (x->right && lvl(x->right) > should) x->right->level = should;
    }
    return x;
  }

  Node *erase_rec(Node *x, const Key &key, bool &removed) {
    if (!x) return nullptr;
    if (comp(key, x->data.first)) {
      x->left = erase_rec(x->left, key, removed);
      if (x->left) x->left->parent = x;
    } else if (comp(x->data.first, key)) {
      x->right = erase_rec(x->right, key, removed);
      if (x->right) x->right->parent = x;
    } else {
      removed = true;
      if (!x->left && !x->right) {
        delete x;
        return nullptr;
      } else if (!x->left) {
        Node *r = x->right;
        r->parent = x->parent;
        delete x;
        return r;
      } else if (!x->right) {
        Node *l = x->left;
        l->parent = x->parent;
        delete x;
        return l;
      } else {
        // push target down using direction by subtree level
        if (lvl(x->left) >= lvl(x->right)) {
          x = rotate_right(x);
          if (x->right) x->right->parent = x;
          x->right = erase_rec(x->right, key, removed);
          if (x->right) x->right->parent = x;
        } else {
          x = rotate_left(x);
          if (x->left) x->left->parent = x;
          x->left = erase_rec(x->left, key, removed);
          if (x->left) x->left->parent = x;
        }
      }
    }
    x = decrease_level(x);
    x = skew(x);
    if (x->right) { x->right = skew(x->right); x->right->parent = x; }
    if (x->right && x->right->right) { x->right->right = skew(x->right->right); x->right->right->parent = x->right; }
    x = split(x);
    if (x->right) { x->right = split(x->right); x->right->parent = x; }
    return x;
  }

  void destroy(Node *x) {
    if (!x) return;
    destroy(x->left);
    destroy(x->right);
    delete x;
  }

  Node *min_node(Node *x) const {
    if (!x) return nullptr;
    while (x->left) x = x->left;
    return x;
  }
  Node *max_node(Node *x) const {
    if (!x) return nullptr;
    while (x->right) x = x->right;
    return x;
  }

  Node *find_node(const Key &key) const {
    Node *cur = root;
    while (cur) {
      if (comp(key, cur->data.first)) cur = cur->left;
      else if (comp(cur->data.first, key)) cur = cur->right;
      else return cur;
    }
    return nullptr;
  }

 public:
  class const_iterator;
  class iterator {
   private:
    Node *node = nullptr;
    const map *owner = nullptr;
   public:
    iterator() {}
    iterator(Node *n, const map *o) : node(n), owner(o) {}
    iterator(const iterator &other) : node(other.node), owner(other.owner) {}

    iterator operator++(int) {
      if (!owner || (!node && owner->root == nullptr)) throw invalid_iterator();
      if (!node) throw invalid_iterator();
      iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    iterator &operator++() {
      if (!owner || (!node && owner->root == nullptr)) throw invalid_iterator();
      if (!node) throw invalid_iterator();
      Node *cur = node;
      if (cur->right) {
        cur = cur->right;
        while (cur->left) cur = cur->left;
      } else {
        Node *p = cur->parent;
        while (p && cur == p->right) { cur = p; p = p->parent; }
        cur = p;
      }
      node = cur;
      return *this;
    }

    iterator operator--(int) {
      if (!owner) throw invalid_iterator();
      iterator tmp = *this;
      --(*this);
      return tmp;
    }

    iterator &operator--() {
      if (!owner) throw invalid_iterator();
      if (!node) {
        Node *m = owner->max_node(owner->root);
        if (!m) throw invalid_iterator();
        node = m;
        return *this;
      }
      Node *cur = node;
      if (cur->left) {
        cur = cur->left;
        while (cur->right) cur = cur->right;
      } else {
        Node *p = cur->parent;
        while (p && cur == p->left) { cur = p; p = p->parent; }
        cur = p;
      }
      if (!cur) throw invalid_iterator();
      node = cur;
      return *this;
    }

    value_type &operator*() const {
      if (!node) throw invalid_iterator();
      return const_cast<value_type &>(node->data);
    }

    bool operator==(const iterator &rhs) const { return node == rhs.node && owner == rhs.owner; }
    bool operator==(const const_iterator &rhs) const { return owner == rhs.owner && node == rhs.node; }

    bool operator!=(const iterator &rhs) const { return !(*this == rhs); }
    bool operator!=(const const_iterator &rhs) const { return !(*this == rhs); }

    value_type *operator->() const noexcept { return &const_cast<value_type &>(node->data); }

    friend class map;
    friend class const_iterator;
  };

  class const_iterator {
   private:
    const Node *node = nullptr;
    const map *owner = nullptr;
   public:
    const_iterator() {}
    const_iterator(const Node *n, const map *o) : node(n), owner(o) {}
    const_iterator(const const_iterator &other) : node(other.node), owner(other.owner) {}
    const_iterator(const iterator &other) : node(other.node), owner(other.owner) {}

    const_iterator operator++(int) {
      if (!owner || (!node && owner->root == nullptr)) throw invalid_iterator();
      if (!node) throw invalid_iterator();
      const_iterator tmp = *this;
      ++(*this);
      return tmp;
    }
    const_iterator &operator++() {
      if (!owner || (!node && owner->root == nullptr)) throw invalid_iterator();
      if (!node) throw invalid_iterator();
      const Node *cur = node;
      if (cur->right) {
        cur = cur->right;
        while (cur->left) cur = cur->left;
      } else {
        const Node *p = cur->parent;
        while (p && cur == p->right) { cur = p; p = p->parent; }
        cur = p;
      }
      node = cur;
      return *this;
    }
    const_iterator operator--(int) {
      if (!owner) throw invalid_iterator();
      const_iterator tmp = *this;
      --(*this);
      return tmp;
    }
    const_iterator &operator--() {
      if (!owner) throw invalid_iterator();
      if (!node) {
        const Node *m = owner->max_node(owner->root);
        if (!m) throw invalid_iterator();
        node = m;
        return *this;
      }
      const Node *cur = node;
      if (cur->left) {
        cur = cur->left;
        while (cur->right) cur = cur->right;
      } else {
        const Node *p = cur->parent;
        while (p && cur == p->left) { cur = p; p = p->parent; }
        cur = p;
      }
      if (!cur) throw invalid_iterator();
      node = cur;
      return *this;
    }
    const value_type &operator*() const {
      if (!node) throw invalid_iterator();
      return node->data;
    }
    const value_type *operator->() const noexcept { return &node->data; }

    bool operator==(const const_iterator &rhs) const { return node == rhs.node && owner == rhs.owner; }
    bool operator==(const iterator &rhs) const { return node == rhs.node && owner == rhs.owner; }
    bool operator!=(const const_iterator &rhs) const { return !(*this == rhs); }
    bool operator!=(const iterator &rhs) const { return !(*this == rhs); }

    friend class map;
    friend class iterator;
  };

 public:
  map() {}

  map(const map &other) : comp(other.comp) {
    copy_inorder(other.root);
  }

  map &operator=(const map &other) {
    if (this == &other) return *this;
    clear();
    comp = other.comp;
    copy_inorder(other.root);
    return *this;
  }

  ~map() { clear(); }

 private:
  void copy_inorder(const Node *x) {
    if (!x) return;
    copy_inorder(x->left);
    insert(value_type(x->data.first, x->data.second));
    copy_inorder(x->right);
  }

 public:
  T &at(const Key &key) {
    Node *n = find_node(key);
    if (!n) throw index_out_of_bound();
    return n->data.second;
  }

  const T &at(const Key &key) const {
    Node *n = find_node(key);
    if (!n) throw index_out_of_bound();
    return n->data.second;
  }

  T &operator[](const Key &key) {
    Node *n = find_node(key);
    if (n) return n->data.second;
    value_type v(key, T());
    Node *placed = nullptr; bool existed = false;
    root = insert_rec(root, nullptr, v, placed, existed);
    if (root) root->parent = nullptr;
    _size++;
    return placed->data.second;
  }

  const T &operator[](const Key &key) const {
    Node *n = find_node(key);
    if (!n) throw index_out_of_bound();
    return n->data.second;
  }

  iterator begin() { return iterator(min_node(root), this); }
  const_iterator cbegin() const { return const_iterator(min_node(root), this); }

  iterator end() { return iterator(nullptr, this); }
  const_iterator cend() const { return const_iterator(nullptr, this); }

  bool empty() const { return _size == 0; }
  size_t size() const { return _size; }

  void clear() {
    destroy(root);
    root = nullptr;
    _size = 0;
  }

  pair<iterator, bool> insert(const value_type &value) {
    Node *existing = find_node(value.first);
    if (existing) return pair<iterator, bool>(iterator(existing, this), false);
    Node *placed = nullptr; bool existed = false;
    root = insert_rec(root, nullptr, value, placed, existed);
    if (root) root->parent = nullptr;
    _size++;
    return pair<iterator, bool>(iterator(placed, this), true);
  }

  void erase(iterator pos) {
    if (pos.owner != this) throw invalid_iterator();
    if (pos == end()) throw invalid_iterator();
    bool removed = false;
    root = erase_rec(root, pos.node->data.first, removed);
    if (!removed) throw invalid_iterator();
    if (root) root->parent = nullptr;
    _size--;
  }

  size_t count(const Key &key) const { return find_node(key) ? 1 : 0; }

  iterator find(const Key &key) { return iterator(find_node(key), this); }
  const_iterator find(const Key &key) const { return const_iterator(find_node(key), this); }
};

}

#endif
