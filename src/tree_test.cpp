
#include "rb_tree.h"
#include "sb_tree.h"
#include "random.h"

#include <string>
#include <ctime>
#include <iostream>
#include <vector>

struct Node
{
    Node *rb_parent, *rb_left, *rb_right;
    Node *sb_parent, *sb_left, *sb_right;
    bool is_black;
    bool is_nil;
    size_t size;
    int value;
};

struct RBTreeInterface
{
    typedef int key_t;
    typedef Node node_t;
    typedef Node value_node_t;
    static key_t const &get_key(Node *node)
    {
        return node->value;
    }
    static bool is_nil(Node *node)
    {
        return node->is_nil;
    }
    static void set_nil(Node *node, bool nil)
    {
        node->is_nil = nil;
    }
    static Node *get_parent(Node *node)
    {
        return node->rb_parent;
    }
    static void set_parent(Node *node, Node *parent)
    {
        node->rb_parent = parent;
    }
    static Node *get_left(Node *node)
    {
        return node->rb_left;
    }
    static void set_left(Node *node, Node *left)
    {
        node->rb_left = left;
    }
    static Node *get_right(Node *node)
    {
        return node->rb_right;
    }
    static void set_right(Node *node, Node *right)
    {
        node->rb_right = right;
    }
    static bool is_black(Node *node)
    {
        return node->is_black;
    }
    static void set_black(Node *node, bool black)
    {
        node->is_black = black;
    }
    static bool predicate(key_t const &left, key_t const &right)
    {
        return left < right;
    }
};

struct SBTreeInterface
{
    typedef int key_t;
    typedef Node node_t;
    typedef Node value_node_t;
    static key_t const &get_key(Node *node)
    {
        return node->value;
    }
    static bool is_nil(Node *node)
    {
        return node->is_nil;
    }
    static void set_nil(Node *node, bool nil)
    {
        node->is_nil = nil;
    }
    static Node *get_parent(Node *node)
    {
        return node->sb_parent;
    }
    static void set_parent(Node *node, Node *parent)
    {
        node->sb_parent = parent;
    }
    static Node *get_left(Node *node)
    {
        return node->sb_left;
    }
    static void set_left(Node *node, Node *left)
    {
        node->sb_left = left;
    }
    static Node *get_right(Node *node)
    {
        return node->sb_right;
    }
    static void set_right(Node *node, Node *right)
    {
        node->sb_right = right;
    }
    static size_t get_size(Node *node)
    {
        return node->size;
    }
    static void set_size(Node *node, size_t size)
    {
        node->size = size;
    }
    static bool predicate(key_t const &left, key_t const &right)
    {
        return left < right;
    }
};

void tree_test()
{
    zzz::rb_tree<RBTreeInterface> rb;
    zzz::sb_tree<SBTreeInterface> sb;
    std::vector<Node *> data;
    std::vector<decltype(rb)> _unuse1;
    std::vector<decltype(sb)> _unuse2;
    ege::mtrandom r;

    int length = 2000;

    auto c = [&data](int v)
    {
        auto n = new Node();
        n->value = v;
        data.push_back(n);
        return n;
    };

    auto assert = [](bool no_error)
    {
        if(!no_error)
        {
            *static_cast<int *>(0) = 0;
        }
    };

    for(int i = 0; i < length / 2; ++i)
    {
        auto n = c(i);
        rb.insert(n);
        sb.insert(n);
        n = c(i);
        rb.insert(n);
        sb.insert(n);
    }
    assert(rb.find(0) == rb.begin());
    assert(sb.find(0) == sb.begin());
    assert(rb.find(length / 2 - 1) == ----rb.end());
    assert(sb.find(length / 2 - 1) == sb.end() - 2);
    assert(rb.count(1) == 2);
    assert(sb.count(1) == 2);
    assert(sb.count(1, 2) == 4);
    assert(sb.count(1, 3) == 6);
    assert(rb.range(1, 3) == std::make_pair(rb.find(1), rb.find(4)));
    assert(sb.range(0, 2) == std::make_pair(sb.begin(), sb.begin() + 6));
    assert(sb.range(2, 3) == sb.slice(4, 8));
    assert(sb.range(0, length) == sb.slice());
    assert(sb.front() == &*sb.begin());
    assert(sb.back() == &*--sb.end());
    assert(sb.rank(0) == 2);
    assert(sb.rank(1) == 4);
    assert(sb.rank(length) == sb.size());
    assert(sb.rank(length / 2) == sb.size());
    assert(sb.rank(length / 2 - 1) == sb.size());
    assert(sb.rank(length / 2 - 2) == sb.size() - 2);
    assert(rb.equal_range(2).first == rb.lower_bound(2));
    assert(sb.equal_range(2).second == sb.upper_bound(2));
    assert(sb.erase(3) == 2);
    assert(rb.erase(3) == 2);
    for(int i = 0; i < length / 2; ++i)
    {
        auto it_rb = rb.begin();
        auto it_sb = sb.begin();
        std::advance(it_rb, r.rand() % rb.size());
        std::advance(it_sb, r.rand() % sb.size());
        rb.erase(it_rb);
        sb.erase(it_sb);
    }
    for(int i = 0; i < length * 2 + 2; ++i)
    {
        auto n = c(r.rand());
        rb.insert(n);
        sb.insert(n);
    }
    for(int i = 0; i < length; ++i)
    {
        typedef decltype(sb.begin()) iter_t;
        int off = r.rand() % sb.size();
        iter_t it(sb.at(off));
        assert(it - sb.begin() == off);
        assert(it - off == sb.begin());
        assert(sb.begin() + off == it);
        assert(sb.begin() + off == sb.end() - (sb.size() - off));
        iter_t begin = sb.begin(), end = sb.end();
        for(int i = 0; i < off; ++i)
        {
            --it;
            ++begin;
            --end;
        }
        assert(sb.end() - end == off);
        assert(sb.begin() + off == begin);
        assert(sb.begin() == it);
        int part = sb.size() / 4;
        int a = part + r.rand() % (part * 2);
        int b = r.rand() % part;
        assert(iter_t(sb.at(a)) + b == iter_t(sb.at(a + b)));
        assert(sb.begin() + a == iter_t(sb.at(a + b)) - b);
        assert(iter_t(sb.at(a)) - iter_t(sb.at(b)) == a - b);
    }

    for(int i = 0; i < length * 2 + length / 2; ++i)
    {
        auto it_rb = rb.begin();
        auto it_sb = sb.begin();
        std::advance(it_rb, r.rand() % rb.size());
        std::advance(it_sb, r.rand() % sb.size());
        rb.erase(it_rb);
        sb.erase(it_sb);
    }
    
    length = 2000000;

    std::cout << "count = " << length << std::endl;

    for(int i = 0; i < length; ++i)
    {
        c(i);
    }

    time_t rb_t1 = clock();
    for(auto n : data)
    {
        rb.insert(n);
    }
    time_t rb_t2 = clock();

    std::cout << "rbt sorted insert " << rb_t2 - rb_t1 << std::endl;

    time_t sb_t1 = clock();
    for(auto n : data)
    {
        sb.insert(n);
    }
    time_t sb_t2 = clock();

    std::cout << "sbt sorted insert " << sb_t2 - sb_t1 << std::endl;

    for(auto n : data)
    {
        n->value = r.rand();
    }
    rb.clear();
    sb.clear();

    time_t rb_t3 = clock();
    for(auto n : data)
    {
        rb.insert(n);
    }
    time_t rb_t4 = clock();

    std::cout << "rbt random insert " << rb_t4 - rb_t3 << std::endl;

    time_t sb_t3 = clock();
    for(auto n : data)
    {
        sb.insert(n);
    }
    time_t sb_t4 = clock();

    std::cout << "sbt random insert " << sb_t4 - sb_t3 << std::endl;
    
    for(auto n : data)
    {
        delete n;
    }
    data.clear();
    system("pause");
}