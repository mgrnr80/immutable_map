/*
MIT License

Copyright (c) 2021 Massimo Guarnieri

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <array>
#include <functional>
#include <utility>

template <class K, class T>
class immutable_map
{
public:
    typedef typename std::pair<K, T> pair;

    immutable_map()
      : root_(nullptr),
        size_(0)
    {}

    immutable_map(const immutable_map& other)
      : root_(other.root_),
        size_(other.size_)
    {}

    immutable_map(immutable_map&& other)
    {
        std::swap(root_, other.root_);
        std::swap(size_, other.size_);
    }

    void operator = (const immutable_map& other)
    {
        root_ = other.root_;
        size_ = other.size_;
    }

    void operator = (immutable_map&& other)
    {
        std::swap(root_, other.root_);
        std::swap(size_, other.size_);
    }

    T& at(const K& key)
    {
        path p;
        auto match = find(p, key);
        if (!match) throw std::out_of_range();
        return p->get_node()->get_pair()->second;
    }

    const T& at(const K& key) const
    {
        path p;
        auto match = find(p, key);
        if (!match) throw std::out_of_range();
        return p->get_node()->get_pair()->second;
    }

    bool empty() const
    {
        return size_ == 0;
    }

    size_t size() const
    {
        return size_;
    }

    immutable_map insert(const pair& kvp) const
    {
        auto ptr = std::make_shared<pair>(kvp);
        return insert(ptr);
    }

    immutable_map insert(pair&& kvp) const
    {
        auto ptr = std::make_shared<pair>(std::move(kvp));
        return insert_imp(ptr);
    }

    immutable_map erase(const K& key) const
    {
        path p;
        auto match = find(p, key);
        if (!match) return *this;
        auto new_root = erase_imp(p);
        return immutable_map(std::move(new_root), size_ - 1);
    }

    bool contains(const K& key) const
    {
        path p;
        auto match = find(p, key);
        return match;
    }

    void foreach(std::function<void(const pair&)> f) const
    {
        if (root_) root_->foreach(f);
    }

    void validate() const
    {
        if (root_ && root_->is_red()) throw std::runtime_error("root is red");
        if (root_) root_->validate();
    }

private:

    enum color_t { BLACK = 0, RED = 1 };
    enum side_t { LEFT = 0, RIGHT = 1 };
    
    class node
    {
    public:
        node(std::shared_ptr<const pair> kvp, std::shared_ptr<const node>&& left_child, std::shared_ptr<const node>&& right_child, color_t color)
          : kvp_(kvp),
            children_{ left_child, right_child },
            color_(color)
        {}
        
        node(const node& other)
          : kvp_(other.kvp_),
            children_{ other.children_[0], other.children_[1] },
            color_(other.color_)
        {}
        
        const K& get_key() const
        {
            return kvp_->first;
        }

        std::shared_ptr<const pair> get_pair() const
        {
            return kvp_;
        }

        std::shared_ptr<const node> get_child(int side) const
        {
            return children_[side];
        }

        color_t get_color() const { return color_; }
        bool is_red() const { return color_ == RED; }
        bool is_black() const { return color_ == BLACK; }
        bool has_red_child() const
        {
            return (children_[0] && children_[0]->is_red()) || (children_[1] && children_[1]->is_red());
        }

        void set_pair(std::shared_ptr<const pair> kvp)
        {
            kvp_ = kvp;
        }

        void set_child(int side, std::shared_ptr<const node> child)
        {
            children_[side] = std::move(child);
        }

        void set_color(color_t color)
        {
            color_ = color;
        }

        std::shared_ptr<node> clone() const
        {
            auto new_node = std::make_shared<immutable_map::node>(
                kvp_,
                get_child(LEFT),
                get_child(RIGHT),
                get_color()
            );
            return new_node;      
        }

        void foreach(const std::function<void(const pair&)>& f) const
        {
            if (get_child(LEFT)) get_child(LEFT)->foreach(f);
            f(*kvp_);
            if (get_child(RIGHT)) get_child(RIGHT)->foreach(f);
        }

        int validate() const
        {
            int depth = 1, depth1, depth2;
            if (is_red())
            {
                depth = 0;
                if (get_child(LEFT) && get_child(LEFT)->is_red())
                    throw std::runtime_error("red node with red child");
                if (get_child(RIGHT) && get_child(RIGHT)->is_red())
                    throw std::runtime_error("red node with red child");
            }
            if (get_child(LEFT)) depth1 = get_child(LEFT)->validate();
            else depth1 = 0;
            if (get_child(RIGHT)) depth2 = get_child(RIGHT)->validate();
            else depth2 = 0;
            if (depth1 != depth2) throw std::runtime_error("invalid black depth");
            return depth1 + depth;
        }

        std::shared_ptr<const pair> kvp_;
        std::shared_ptr<const node> children_[2];
        color_t color_;
    };

    class path
    {
    public:
        path()
        {
            size_ = 0;
        }

        std::shared_ptr<const node> get_node()
        {
            if (size_ > 0) return path_[size_ - 1];
            return nullptr;
        }
        std::shared_ptr<const node> get_parent()
        {
            if (size_ > 1) return path_[size_ - 2];
            return nullptr;
        }
        std::shared_ptr<const node> get_grand_parent()
        {
            if (size_ > 2) return path_[size_ - 3];
            return nullptr;
        }
        std::shared_ptr<const node> operator[](size_t n)
        {
            return path_[n];
        }

        void push(std::shared_ptr<const node> node) { path_[size_++] = node; }
        void pop() { path_[--size_] = nullptr; }
        bool empty() const { return size_ == 0; }
        size_t size() const { return size_; }

    private:
        std::array<std::shared_ptr<const node>, sizeof(size_t) * 16> path_;
        size_t size_;
    };

    std::shared_ptr<const node> root_;
    size_t   size_;

    immutable_map(std::shared_ptr<const node>&& root, size_t size)
    {
        root_ = std::move(root);
        size_ = size;
    }

    immutable_map insert_imp(std::shared_ptr<const pair> kvp) const
    {
        path p;
        auto match = find(p, kvp->first);
        if (match)
        {
            auto new_node = p.get_node()->clone();
            new_node->set_pair(kvp);
            p.pop();
            return immutable_map(clone_path(p, new_node), size_);
        }
        auto new_root = insert_imp(kvp, p);
        return immutable_map(std::move(new_root), size_ + 1);
    }

    std::shared_ptr<node> insert_imp(std::shared_ptr<const pair> kvp, path& p) const
    {
        if (!root_)
        {
            return std::make_shared<immutable_map::node>(
                std::move(kvp), nullptr, nullptr, BLACK
            );
        }
        auto n = std::make_shared<immutable_map::node>(
            std::move(kvp), nullptr, nullptr, RED
        );
        return insert_fix(p, n);
    }

    bool find(path& p, const K& key) const
    {
        return find(root_, p, key);
    }

    static bool find(std::shared_ptr<const node> root, path& p, const K& key)
    {
        if (!root) return false;
        auto node = root;
        while (node)
        {
            p.push(node);
            if (node->get_key() == key) return true;
            if (node->get_key() > key) node = node->get_child(LEFT);
            else node = node->get_child(RIGHT);
        }
        return false;
    }

    // precondition: path is not empty
    std::shared_ptr<node> insert_fix(path& p, std::shared_ptr<node> n) const
    {
        if (p.empty())
        {
            n->set_color(BLACK);  
            return n;
        }
        auto parent = p.get_node();
        if (parent->is_black())
        {
            return clone_path(p, n);
        }
        else
        {
            auto grand_parent = p.get_parent();
            auto parent_side = grand_parent->get_key() > parent->get_key() ? LEFT : RIGHT;
            auto node_side = parent->get_key() > n->get_key() ? LEFT : RIGHT;
            auto uncle = grand_parent->get_child(1 - parent_side);
            if (uncle && uncle->is_red())
            {
                auto new_parent = parent->clone();
                new_parent->set_color(BLACK);
                new_parent->set_child(node_side, n);
                auto new_uncle = uncle->clone();
                new_uncle->set_color(BLACK);
                auto new_grand_parent = grand_parent->clone();
                new_grand_parent->set_color(RED);
                new_grand_parent->set_child(parent_side, new_parent);
                new_grand_parent->set_child(1 - parent_side, new_uncle);
                p.pop(); p.pop();
                return insert_fix(p, new_grand_parent);
            }
            else
            {                
                auto new_parent = parent->clone();
                auto new_grand_parent = grand_parent->clone();
                if (parent_side != node_side)
                {
                    new_parent->set_child(node_side, n->get_child(parent_side));
                    new_grand_parent->set_child(parent_side, n->get_child(node_side));
                    new_grand_parent->set_color(RED);
                    n->set_child(parent_side, new_parent),
                    n->set_child(node_side, new_grand_parent);
                    n->set_color(BLACK);
                    p.pop(); p.pop();
                    return clone_path(p, n);
                }
                else
                {
                    new_grand_parent->set_child(parent_side, parent->get_child(1 - parent_side));
                    new_grand_parent->set_color(RED);
                    new_parent->set_child(1 - parent_side, new_grand_parent);
                    new_parent->set_child(parent_side, n);
                    new_parent->set_color(BLACK);
                    p.pop(); p.pop();
                    return clone_path(p, new_parent);
                }
            }
        }
    }

    std::shared_ptr<node> clone_path(path& p, std::shared_ptr<node> n) const
    {
        while (auto parent = p.get_node())
        {
            auto new_parent = parent->clone();
            if (new_parent->get_key() > n->get_key())
                new_parent->set_child(LEFT, n);
            else
                new_parent->set_child(RIGHT, n);
            n = new_parent;
            p.pop();
        }
        return n;
    }

    std::shared_ptr<node> clone_path(path& p, std::shared_ptr<node> n, size_t depth) const
    {
        while (p.size() > depth)
        {
            auto parent = p.get_node();
            auto new_parent = parent->clone();
            if (new_parent->get_key() > n->get_key())
                new_parent->set_child(LEFT, n);
            else
                new_parent->set_child(RIGHT, n);
            n = new_parent;
            p.pop();
        }
        return n;
    }

    // precondition: depth > 0
    std::shared_ptr<node> clone_path(path& p, std::shared_ptr<node> n, int side, size_t depth) const
    {
        if (p.size() > depth)
        {
            auto parent = p.get_node();
            auto grand_parent = p.get_parent();
            auto new_parent = parent->clone();
            new_parent->set_child(side, n);
            p.pop();
            int parent_side = grand_parent->get_key() > parent->get_key() ? LEFT : RIGHT;
            return clone_path(p, new_parent, parent_side, depth);
        }
        return n;
    }

    void find_predecessor(path& p) const
    {
        auto node = p.get_node()->get_child(LEFT);
        while (node)
        {
            p.push(node);
            node = node->get_child(RIGHT);
        }
    }

    std::shared_ptr<node> erase_imp(path& p) const
    {
        auto n = p.get_node();
        bool has_left_child = (bool)n->get_child(LEFT);
        bool has_right_child = (bool)n->get_child(RIGHT);
        if (has_left_child && has_right_child)
        {
            return erase_intermediate_node(p);
        }
        else if (has_left_child)
        {
            auto new_child = n->get_child(LEFT)->clone();
            new_child->set_color(BLACK);
            p.pop();
            return clone_path(p, new_child);
        }
        else if (has_right_child)
        {
            auto new_child = n->get_child(RIGHT)->clone();
            new_child->set_color(BLACK);
            p.pop();
            return clone_path(p, new_child);
        }
        else if (n->is_red())
        {
            auto parent_side = p.get_parent()->get_key() > n->get_key() ? LEFT : RIGHT;
            auto new_parent = p.get_parent()->clone();
            new_parent->set_child(parent_side, nullptr);
            p.pop(); p.pop();
            return clone_path(p, new_parent);
        }
        else if (p.size() == 1) // deleting root
        {
            return nullptr;
        }
        else
        {
            auto parent_side = p.get_parent()->get_key() > n->get_key() ? LEFT : RIGHT;
            auto new_parent = p.get_parent()->clone();
            new_parent->set_child(parent_side, nullptr);
            p.pop(); p.pop();
            return delete_fixup(p, new_parent, parent_side);
        }
    }

    std::shared_ptr<node> erase_intermediate_node(path& p) const
    {
        auto erased_node = p.get_node();
        auto node_color = erased_node->get_color();
        auto depth = p.size();
        //auto kvp = erased_node->get_pair();
        find_predecessor(p); // predecessor has no right child
        auto predecessor = p.get_node();
        auto predecessor_depth = p.size();
        auto predecessor_side = (predecessor_depth - depth) > 1 ? RIGHT : LEFT;
        auto predecessor_color = predecessor->get_color();
        auto new_node = predecessor->clone();
        new_node->set_color(node_color);
        if (predecessor_color == RED) // removed node is red
        {
            p.pop();
            auto sub_tree = clone_path(p, nullptr, predecessor_side, depth);
            new_node->set_child(LEFT, sub_tree);
            new_node->set_child(RIGHT, erased_node->get_child(RIGHT));
            p.pop();
            return clone_path(p, new_node);
        }
        else
        {
            if (predecessor->get_child(LEFT)) // removed node has a left-child red
            {
                auto new_child = predecessor->get_child(LEFT)->clone();
                new_child->set_color(BLACK);
                p.pop();
                auto sub_tree = clone_path(p, new_child, depth);
                new_node->set_child(LEFT, sub_tree);
                new_node->set_child(RIGHT, erased_node->get_child(RIGHT));
                p.pop();
                return clone_path(p, new_node);
            }
            else // removed node is black with no children
            {
                p.pop();
                auto kvp = predecessor_side == LEFT ? predecessor->get_pair() : p.get_node()->get_pair();
                auto sub_tree = clone_path(p, nullptr, predecessor_side, depth);
                new_node->set_child(LEFT, sub_tree);
                new_node->set_child(RIGHT, erased_node->get_child(RIGHT));
                p.pop();
                auto temp_root = clone_path(p, new_node);
                find(temp_root, p, kvp->first);
                auto new_parent = p.get_node();
                p.pop();
                return delete_fixup(p, new_parent, predecessor_side);
            }
        }
    }

    std::shared_ptr<node> delete_fixup(path& p, std::shared_ptr<const node> parent, int side) const
    {
        if (has_black_sibling_with_red_child(parent, side))
        {
            auto new_parent = delete_fixup_1(parent, side);
            return clone_path(p, new_parent);
        }
        else if (has_black_sibling_with_black_children(parent, side))
        {
            auto parent_color = parent->get_color();
            auto new_parent = delete_fixup_2(parent, side);
            if (parent_color == BLACK && p.size() > 0) // parent is black and not root
            {
                auto grand_parent = p.get_node();
                auto parent_side = grand_parent->get_key() > parent->get_key() ? LEFT : RIGHT;
                auto new_grand_parent = grand_parent->clone();
                new_grand_parent->set_child(parent_side, new_parent);
                p.pop();
                return delete_fixup(p, new_grand_parent, parent_side);
            }
            else
            {
                return clone_path(p, new_parent);
            }
        }
        else
        {
            auto new_parent = delete_fixup_3(parent, side);
            return clone_path(p, new_parent);
        }
    }

    std::shared_ptr<node> delete_fixup_1(std::shared_ptr<const node> parent, int side) const
    {
        auto parent_color = parent->get_color();
        auto sibling = parent->get_child(1 - side);
        auto child_1 = sibling->get_child(side);
        if (child_1 && child_1->is_red())
        {
            auto new_child_1 = child_1->clone();
            auto new_parent = parent->clone();
            auto new_sibling = sibling->clone();
            new_child_1->set_color(parent_color);
            new_child_1->set_child(side, new_parent);
            new_child_1->set_child(1 - side, new_sibling);
            new_parent->set_color(BLACK);
            new_parent->set_child(1 - side, child_1->get_child(side));
            new_sibling->set_child(side, child_1->get_child(1 - side));
            return new_child_1;
        }
        else
        {
            auto child_2 = sibling->get_child(1 - side);
            auto new_child_2 = child_2->clone();
            auto new_parent = parent->clone();
            auto new_sibling = sibling->clone();
            new_sibling->set_color(parent_color);
            new_sibling->set_child(side, new_parent);
            new_sibling->set_child(1 - side, new_child_2);
            new_parent->set_color(BLACK);
            new_parent->set_child(1 - side, sibling->get_child(side));
            new_child_2->set_color(BLACK);
            return new_sibling;
        }
    }

    std::shared_ptr<node> delete_fixup_2(std::shared_ptr<const node> parent, int side)  const // recoloring
    {
        auto sibling = parent->get_child(1 - side);
        auto new_sibling = sibling->clone();
        auto new_parent = parent->clone();
        new_sibling->set_color(RED);
        new_parent->set_color(BLACK);
        new_parent->set_child(1 - side, new_sibling);
        return new_parent;
    }

    std::shared_ptr<node> delete_fixup_3(std::shared_ptr<const node> parent, int side)  const // adjustment
    {
        auto sibling = parent->get_child(1 - side);
        auto new_sibling = sibling->clone();
        auto new_parent = parent->clone();
        new_sibling->set_color(parent->get_color());
        new_sibling->set_child(side, new_parent);
        new_parent->set_color(RED);
        new_parent->set_child(1 - side, sibling->get_child(side));
        if (has_black_sibling_with_red_child(new_parent, side))
        {
            new_parent = delete_fixup_1(new_parent, side);
            new_sibling->set_child(side, new_parent);
            return new_sibling;
        }
        else if (has_black_sibling_with_black_children(new_parent, side))
        {
            new_parent = delete_fixup_2(new_parent, side);
            new_sibling->set_child(side, new_parent);
            return new_sibling;
        }
        else
        {
            return new_sibling;
        }
    }

    bool has_black_sibling_with_red_child(std::shared_ptr<const node> parent, int side) const
    {
        auto sibling = parent->get_child(1 - side);
        if (sibling->is_red()) return false;
        return sibling->has_red_child();
    }

    bool has_black_sibling_with_black_children(std::shared_ptr<const node> parent, int side) const
    {
        auto sibling = parent->get_child(1 - side);
        if (sibling->is_red()) return false;
        return !sibling->has_red_child();
    }

    bool has_red_sibling(std::shared_ptr<const node> parent, int side) const
    {
        auto sibling = parent->get_child(1 - side);
        return sibling && sibling->is_red();
    }
};
