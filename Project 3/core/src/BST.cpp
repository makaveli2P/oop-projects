#include "strset/BST.h"

#include <utility>

namespace strset {

namespace {

/// Calls a member of the observer if there is an observer.
template <class F>
void notify(BstObserver* observer, F&& call) {
    if (observer != nullptr) {
        std::forward<F>(call)(*observer);
    }
}

}  // namespace

BST::BST(std::initializer_list<String> values) {
    for (const String& value : values) {
        insert(value);
    }
}

BST::BST(const BST& other) : BST() {
    // The constructor delegates to BST(), so the object is complete before
    // assign_copy() starts. If an allocation throws, ~BST() runs and frees
    // the nodes that assign_copy() made, without recursion.
    assign_copy(other);
}

BST::BST(BST&& other) noexcept : root_(std::move(other.root_)), size_(std::exchange(other.size_, 0)) {}

BST& BST::operator=(const BST& other) {
    BST copy{other};
    swap(copy);
    return *this;
}

BST& BST::operator=(BST&& other) noexcept {
    if (this != &other) {
        clear();
        root_ = std::move(other.root_);
        size_ = std::exchange(other.size_, 0);
    }
    return *this;
}

BST::~BST() { clear(); }

void BST::swap(BST& other) noexcept {
    root_.swap(other.root_);
    std::swap(size_, other.size_);
}

void BST::clear(BstObserver* observer) {
    if (observer == nullptr) {
        // No observer: rotate the left subtrees to the right and free the
        // root each time it has no left child. This needs no extra memory, so
        // the destructor cannot throw, and it is O(n).
        while (root_) {
            if (root_->left_) {
                Link left = std::move(root_->left_);
                root_->left_ = std::move(left->right_);
                left->right_ = std::move(root_);
                root_ = std::move(left);
            } else {
                Link right = std::move(root_->right_);
                root_ = std::move(right);  // Frees the old root, which has no children now.
            }
        }
        size_ = 0;
        return;
    }
    // With an observer: free the nodes in postorder, so that the tree keeps
    // its shape and each freed node is a leaf. The stack holds the links from
    // the root to the current node.
    std::vector<Link*> path;
    if (root_) {
        path.push_back(&root_);
    }
    while (!path.empty()) {
        Link& link = *path.back();
        if (link->left_) {
            path.push_back(&link->left_);
        } else if (link->right_) {
            path.push_back(&link->right_);
        } else {
            observer->freeing(*this, *link);
            link.reset();
            --size_;
            path.pop_back();
        }
    }
}

std::size_t BST::height() const {
    std::size_t best = 0;
    std::vector<std::pair<const Node*, std::size_t>> stack;
    if (root_) {
        stack.emplace_back(root_.get(), 1);
    }
    while (!stack.empty()) {
        const auto [node, depth] = stack.back();
        stack.pop_back();
        best = depth > best ? depth : best;
        if (node->left_) stack.emplace_back(node->left_.get(), depth + 1);
        if (node->right_) stack.emplace_back(node->right_.get(), depth + 1);
    }
    return best;
}

BST::Link& BST::find_link(const String& value, const Node*& parent, BstObserver* observer) {
    parent = nullptr;
    Link* link = &root_;
    while (*link) {
        Node& node = **link;
        const int order = value.compare(node.value_);
        notify(observer, [&](BstObserver& o) { o.compared(*this, node, value, order); });
        if (order == 0) {
            break;
        }
        parent = &node;
        link = order < 0 ? &node.left_ : &node.right_;
    }
    return *link;
}

const Node* BST::find(const String& value, BstObserver* observer) const {
    const Node* node = root_.get();
    while (node != nullptr) {
        const int order = value.compare(node->value_);
        notify(observer, [&](BstObserver& o) { o.compared(*this, *node, value, order); });
        if (order == 0) {
            notify(observer, [&](BstObserver& o) { o.found(*this, *node); });
            return node;
        }
        node = order < 0 ? node->left_.get() : node->right_.get();
    }
    notify(observer, [&](BstObserver& o) { o.missing(*this, value); });
    return nullptr;
}

bool BST::insert(String value, BstObserver* observer) {
    const Node* parent = nullptr;
    Link& link = find_link(value, parent, observer);
    if (link) {
        notify(observer, [&](BstObserver& o) { o.duplicate(*this, *link); });
        return false;
    }
    link = std::make_unique<Node>(std::move(value));
    ++size_;
    const Side side = parent != nullptr && parent->left_.get() == link.get() ? Side::Left : Side::Right;
    notify(observer, [&](BstObserver& o) { o.linked(*this, *link, parent, side); });
    return true;
}

bool BST::contains(const String& value, BstObserver* observer) const { return find(value, observer) != nullptr; }

bool BST::remove(const String& value, BstObserver* observer) {
    const Node* parent = nullptr;
    Link& link = find_link(value, parent, observer);
    if (!link) {
        notify(observer, [&](BstObserver& o) { o.missing(*this, value); });
        return false;
    }
    remove_at(link, observer);
    return true;
}

void BST::remove_at(Link& link, BstObserver* observer) {
    Node& node = *link;
    if (node.left_ && node.right_) {
        // Two children: the in-order successor (the leftmost node of the
        // right subtree) takes the place of the node. The nodes are relinked;
        // no String is copied.
        Link* successor_link = &node.right_;
        while ((*successor_link)->left_) {
            successor_link = &(*successor_link)->left_;
        }
        notify(observer, [&](BstObserver& o) { o.removing(*this, node, successor_link->get()); });
        Link successor = std::move(*successor_link);
        *successor_link = std::move(successor->right_);
        successor->left_ = std::move(node.left_);
        successor->right_ = std::move(node.right_);
        link = std::move(successor);  // Frees the node, which has no children now.
    } else {
        notify(observer, [&](BstObserver& o) { o.removing(*this, node, nullptr); });
        Link child = node.left_ ? std::move(node.left_) : std::move(node.right_);
        link = std::move(child);
    }
    --size_;
}

void BST::assign_copy(const BST& source, BstObserver* observer) {
    if (&source == this) {
        return;
    }
    clear(observer);
    // Preorder with a stack: each job copies one node of the source into an
    // empty link of *this. The links are in heap nodes, so their addresses
    // stay valid while the copy grows.
    struct Job {
        const Node* original;
        Link* target;
    };
    std::vector<Job> stack;
    if (source.root_) {
        stack.push_back({source.root_.get(), &root_});
    }
    while (!stack.empty()) {
        const Job job = stack.back();
        stack.pop_back();
        *job.target = std::make_unique<Node>(job.original->value_);  // String's copy constructor.
        ++size_;
        Node& copy = **job.target;
        notify(observer, [&](BstObserver& o) { o.copied(*this, *job.original, copy); });
        if (job.original->right_) stack.push_back({job.original->right_.get(), &copy.right_});
        if (job.original->left_) stack.push_back({job.original->left_.get(), &copy.left_});
    }
}

std::size_t BST::unite(const BST& other, BstObserver* observer) {
    std::size_t added = 0;
    const std::vector<const Node*> nodes = other.nodes_in_preorder();
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        notify(observer, [&](BstObserver& o) { o.visited(other, *nodes[i], i); });
        if (insert(nodes[i]->value_, observer)) {
            ++added;
        }
    }
    return added;
}

std::size_t BST::intersect(const BST& other, BstObserver* observer) {
    if (&other == this) {
        return 0;
    }
    std::size_t removed = 0;
    // A removal frees only the removed node. The successor is relinked, not
    // copied, so the other pointers in `nodes` stay valid.
    const std::vector<const Node*> nodes = nodes_in_order();
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        const Node& node = *nodes[i];
        notify(observer, [&](BstObserver& o) { o.visited(*this, node, i); });
        if (other.find(node.value_, observer) == nullptr) {
            const Node* parent = nullptr;
            remove_at(find_link(node.value_, parent, nullptr), observer);
            ++removed;
        }
    }
    return removed;
}

bool BST::is_proper_subset_of(const BST& other, BstObserver* observer) const {
    if (&other == this) {
        return false;
    }
    const std::vector<const Node*> nodes = nodes_in_order();
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        notify(observer, [&](BstObserver& o) { o.visited(*this, *nodes[i], i); });
        if (other.find(nodes[i]->value_, observer) == nullptr) {
            return false;
        }
    }
    return size_ < other.size_;
}

void BST::walk(BstObserver* observer) const {
    const std::vector<const Node*> nodes = nodes_in_order();
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        notify(observer, [&](BstObserver& o) { o.visited(*this, *nodes[i], i); });
    }
}

std::vector<String> BST::values() const {
    std::vector<String> result;
    result.reserve(size_);
    for (const Node* node : nodes_in_order()) {
        result.push_back(node->value_);
    }
    return result;
}

std::vector<const Node*> BST::nodes_in_order() const {
    std::vector<const Node*> result;
    result.reserve(size_);
    std::vector<const Node*> stack;
    const Node* node = root_.get();
    while (node != nullptr || !stack.empty()) {
        while (node != nullptr) {
            stack.push_back(node);
            node = node->left_.get();
        }
        node = stack.back();
        stack.pop_back();
        result.push_back(node);
        node = node->right_.get();
    }
    return result;
}

std::vector<const Node*> BST::nodes_in_preorder() const {
    std::vector<const Node*> result;
    result.reserve(size_);
    std::vector<const Node*> stack;
    if (root_) {
        stack.push_back(root_.get());
    }
    while (!stack.empty()) {
        const Node* node = stack.back();
        stack.pop_back();
        result.push_back(node);
        if (node->right_) stack.push_back(node->right_.get());
        if (node->left_) stack.push_back(node->left_.get());
    }
    return result;
}

bool operator==(const BST& a, const BST& b) {
    if (a.size_ != b.size_) {
        return false;
    }
    const std::vector<const Node*> x = a.nodes_in_order();
    const std::vector<const Node*> y = b.nodes_in_order();
    for (std::size_t i = 0; i < x.size(); ++i) {
        if (x[i]->value() != y[i]->value()) {
            return false;
        }
    }
    return true;
}

}  // namespace strset
