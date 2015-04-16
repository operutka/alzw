/*
Copyright (c) 2015 Perutka, Ondrej

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include <cstring>
#include <stdexcept>
#include <exception>
#include <deque>
#include <algorithm>

#include "dictionary.hpp"
#include "encoder.hpp"
#include "decoder.hpp"
#include "utils.hpp"

using namespace alzw;

// ############
// node methods
// ############

node::node() {
    this->nid = 0;
    this->sym = 0;
    this->par = NULL;
    this->deg = 0;
    this->plen = 0;
    this->children = NULL;

#ifdef NODE_COLLAPSING
    this->len = 0;
    this->seq = NULL;
#endif
}

node::node(uint64_t id, uint8_t sym, node* parent) {
    this->nid = id;
    this->sym = sym;
    this->par = parent;
    this->deg = 0;
    this->plen = 1;
    this->children = NULL;
    
    if (parent)
        this->plen += parent->plen;

#ifdef NODE_COLLAPSING
    this->len = 0;
    this->seq = NULL;
#endif
}

node::node(uint64_t id, uint8_t sym, uint32_t plen, node* parent) {
    this->nid = id;
    this->sym = sym;
    this->par = parent;
    this->deg = 0;
    this->plen = plen;
    this->children = NULL;

#ifdef NODE_COLLAPSING
    this->len = 0;
    this->seq = NULL;
#endif
}

#ifdef NODE_COLLAPSING
node::node(uint64_t id, uint8_t* phrase, uint32_t len, node* parent) {
    uint32_t size = len >> 1;
    
    this->nid = id;
    this->sym = phrase[0];
    this->par = parent;
    this->deg = 0;
    this->seq = new uint8_t[size];
    this->len = len - 1;
    this->plen = len;
    this->children = NULL;
    
    memset(seq, 0, sizeof(uint8_t) * size);
    
    for (uint32_t i = 1; i < len; i++)
        set_base(i - 1, phrase[i]);
    
    if (parent)
        this->plen += parent->plen;
}

node::node(node* n, uint32_t offset, uint32_t len, uint32_t plen, node* parent) {
    uint32_t size = (len + 1) >> 1;
    
    this->nid = n->id() + offset;
    this->par = parent;
    this->deg = 0;
    this->seq = new uint8_t[size];
    this->len = len;
    this->plen = plen;
    this->children = NULL;
    
    if (offset > 0)
        this->sym = n->get_base(offset - 1);
    else
        this->sym = n->symbol();
    
    memset(seq, 0, sizeof(uint8_t) * size);
    
    for (uint32_t i = 0; i < len; i++)
        set_base(i, n->get_base(offset + i));
}

node::~node() {
    delete [] seq;
}
#else
node::~node() {
}
#endif

void node::release_children(node_allocator& allocator) {
    if (deg > 1)
        allocator.free_children((node**)children, deg);
    
    deg = 0;
}

node * node::child(int base, uint32_t offset) {
    if (offset > length())
        throw std::overflow_error("offset out of range");
    else if (offset == length())
        return get(base);
    else if (get_base(offset) == base)
        return this;
    
    return NULL;
}

const node * node::child(int base, uint32_t offset) const {
    if (offset > length())
        throw std::overflow_error("offset out of range");
    else if (offset == length())
        return get(base);
    else if (get_base(offset) == base)
        return this;
    
    return NULL;
}

node * node::first_child(uint32_t offset) {
    if (offset > length())
        throw std::overflow_error("offset out of range");
    else if (offset < length())
        return this;
    else if (deg == 0)
        return NULL;
    else if (deg == 1)
        return (node*)children;
    
    return *(node**)children;
}

size_t node::size() const {
    size_t s = sizeof(node) + ((length() + 1) >> 1);
    if (deg > 1)
        s += deg * sizeof(node*);
    
    return s;
}

#ifdef NODE_COLLAPSING
void node::append(int base) {
    if (deg > 0)
        throw std::runtime_error("not a leaf node");
    
    size_t size = (++len + 1) >> 1;
    uint8_t* nseq = new uint8_t[size];
    
    if (seq) {
        memcpy(nseq, seq, len >> 1);
        delete [] seq;
    }
    
    seq = nseq;
    
    set_base(len - 1, base);
    
    plen++;
}

void node::append(uint8_t* phrase, uint32_t len) {
    if (deg > 0)
        throw std::runtime_error("not a leaf node");
    
    size_t size = (this->len + len + 1) >> 1;
    uint8_t* nseq = new uint8_t[size];
    
    if (seq) {
        memcpy(nseq, seq, (this->len + 1) >> 1);
        delete [] seq;
    }
    
    seq = nseq;
    
    for (uint32_t i = 0; i < len; i++)
        set_base(this->len + i, phrase[i]);
    
    this->len += len;
    this->plen += len;
}

void node::shrink(uint32_t len) {
    if (len > this->len)
        throw std::runtime_error("shrink size is greater than the current size");
    else if (len == this->len)
        return;
    
    size_t size = (len + 1) >> 1;
    uint8_t* nseq;
    if (len > 0)
        nseq = new uint8_t[size];
    else
        nseq = NULL;
    
    if (seq) {
        memcpy(nseq, seq, size);
        delete [] seq;
    }
    
    this->plen -= this->len - len;
    this->seq = nseq;
    this->len = len;
}
#endif

node * node::create(int base, node_allocator& allocator) {
    node* n = get(base);
    if (n)
        throw std::runtime_error("there is already a child node for the given base");
    
    if (deg == 0)
        children = n = allocator.alloc(base, this);
    else {
        node** tmp = allocator.alloc_children(deg + 1);
        if (deg == 1)
            tmp[0] = (node*)children;
        else {
            for (uint8_t i = 0; i < deg; i++)
                tmp[i] = ((node**)children)[i];
            allocator.free_children((node**)children, deg);
        }
        tmp[deg] = n = allocator.alloc(base, this);
        children = tmp;
    }
    
    deg++;
    
    return n;
}

node * node::get(int base) {
    if (deg == 0)
        return NULL;
    else if (deg == 1) {
        if (base == ((node*)children)->symbol())
            return (node*)children;
        else
            return NULL;
    }
    
    node** tmp = (node**)children;
    for (uint8_t i = 0; i < deg; i++) {
        if (tmp[i]->symbol() == base)
            return tmp[i];
    }
    
    return NULL;
}

const node * node::get(int base) const {
    if (deg == 0)
        return NULL;
    else if (deg == 1) {
        if (base == ((node*)children)->symbol())
            return (node*)children;
        else
            return NULL;
    }
    
    node** tmp = (node**)children;
    for (uint8_t i = 0; i < deg; i++) {
        if (tmp[i]->symbol() == base)
            return tmp[i];
    }
    
    return NULL;
}

void node::get_children(node** c) {
    if (deg < 2)
        c[0] = (node*)children;
    else {
        for (uint8_t i = 0; i < deg; i++)
            c[i] = ((node**)children)[i];
    }
}

void node::get_children(const node** c) const {
    if (deg < 2)
        c[0] = (const node*)children;
    else {
        for (uint8_t i = 0; i < deg; i++)
            c[i] = ((node**)children)[i];
    }
}

void node::set_children(node** c, uint8_t count, node_allocator& allocator) {
    if (count != deg) {
        if (deg > 1)
            allocator.free_children((node**)children, deg);
        if (count > 1)
            children = allocator.alloc_children(count);
        deg = count;
    }
    
    if (deg == 0)
        children = NULL;
    else if (deg == 1) {
        c[0]->par = this;
        children = c[0];
    } else {
        for (uint8_t i = 0; i < deg; i++) {
            c[i]->par = this;
            ((node**)children)[i] = c[i];
        }
    }
}

void node::set(int base, node* child, node_allocator& allocator) {
    if (child && base != child->symbol())
        throw std::runtime_error("given base does not match to the symbol of the given node");

    node* tmp[256];
    uint32_t d = deg;
    uint8_t i;
    
    get_children(tmp);
    
    for (i = 0; i < deg; i++) {
        if (tmp[i]->symbol() == base)
            break;
    }
    
    tmp[i] = child;
    
    if (i == deg && child)          // addition
        d++;
    else if (i < deg && !child) {   // deletion
        d--;
        for (; i < d; i++)
            tmp[i] = tmp[i + 1];
    }
    
    set_children(tmp, d, allocator);
}

void node::set_base(uint32_t index, int base) {
#ifdef NODE_COLLAPSING
    uint32_t i = index >> 1;
    uint32_t mask = 0xf << ((index & 1) << 2);
    seq[i] = (seq[i] & mask) | (base << ((~index & 1) << 2));
#endif
}

uint8_t node::get_base(uint32_t index) const {
#ifdef NODE_COLLAPSING
    uint32_t i = index >> 1;
    return (seq[i] >> ((~index & 1) << 2)) & 0xf;
#else
    return 0;
#endif
}

// ##################
// dictionary methods
// ##################

dictionary::dictionary(bool indexed) {
    if (indexed) {
        this->node_index = new indexed_node_allocator;
        this->allocator  = node_index;
    } else {
        this->allocator  = new simple_node_allocator;
        this->node_index = NULL;
    }
    
    this->root = new node;
    
    cur_node = root;
    cur_id = 0;
    offset = 0;
    
    addBufferSize = 4096;
    addBuffer = new uint8_t[addBufferSize];
    addLen = 0;
    
    root->create(0, *allocator);
    root->create(1, *allocator);
    root->create(2, *allocator);
    root->create(3, *allocator);
    root->create(4, *allocator);
    
    inode = allocator->alloc(0, NULL);
    dnode = allocator->alloc(0, NULL);
    wnode = allocator->alloc(0, NULL);
}

dictionary::~dictionary() {
    std::deque<node*> nodes;
    nodes.push_back(root);
    nodes.push_back(inode);
    nodes.push_back(dnode);
    nodes.push_back(wnode);
    node* n;
    
    node* children[256];
    
    while (!nodes.empty()) {
        n = nodes.back();
        nodes.pop_back();
        
        n->get_children(children);
        for (unsigned i = 0; i < n->degree(); i++)
            nodes.push_back(children[i]);
        
        n->release_children(*allocator);
        delete n;
    }
    
    delete allocator;
    
    delete [] addBuffer;
}

void dictionary::split_current() {
    if (addLen > 0)
        return;
    
    cur_node = allocator->split(cur_node, offset);
}

uint64_t dictionary::add(char c) {
    if (follow(c))
        return cur_id;

    int base = utils::char2base(c);
    
    // we need to split the current node first if the it is collapsed
    if (cur_node->collapsed())
        split_current();

#ifdef NODE_COLLAPSING
    if (((cur_id + 1) == allocator->next_id() && cur_node->degree() == 0) 
        || addLen > 0) {
        if (addLen >= addBufferSize)
            addBufferSize = utils::crealloc(&addBuffer, 
                addBufferSize, addBufferSize << 1);
        addBuffer[addLen++] = base;
    } else
#endif
        cur_node = cur_node->create(base, *allocator);
    
    offset = cur_node->length() + addLen;
    cur_id = cur_node->id() + offset;
    dpth++;
    
    return cur_id;
}

uint64_t dictionary::get_id() const {
    return cur_id;
}

uint64_t dictionary::next_id() const {
    if (addLen > 0)
        return cur_id + 1;
    
    return allocator->next_id();
}

const node * dictionary::get() const {
    return cur_node;
}

const node * dictionary::get(uint64_t id) const {
    if (node_index)
        return node_index->get(id);
    
    throw std::runtime_error("dictionary is not indexed");
}

bool dictionary::follow(char c) {
    if (addLen > 0)
        return false;

    int base = utils::char2base(c);
    node* child = cur_node->child(base, offset);
    if (child)
        dpth++;
    
    if (child == cur_node) {
        offset++;
        cur_id++;
    } else if (child) {
        cur_node = child;
        cur_id = child->id();
        offset = 0;
    }
    
    return child;
}

bool dictionary::can_follow(char c) {
    if (addLen > 0)
        return false;
    
    return cur_node->child(utils::char2base(c), offset);
}

void dictionary::commit_phrase() {
    if (addLen > 0)
        allocator->append(cur_node, addBuffer, addLen);
    
    addLen = 0;
}

void dictionary::new_phrase() {
    commit_phrase();
    reset_phrase();
}

void dictionary::reset_phrase() {
    cur_node = root;
    cur_id = 0;
    offset = 0;
    dpth = 0;
}

size_t dictionary::used_memory() const {
    return allocator->used_memory();
}

size_t dictionary::used_nodes() const {
    return allocator->used_nodes();
}

size_t dictionary::real_nodes() const {
    return allocator->real_nodes();
}

void dictionary::print() const {
    print(root, "");
}

#include <sstream>

void dictionary::print(const node* n, const std::string& prefix) const {
    if (n->collapsed())
        fprintf(stderr, "%s--> [%lu, %u] ", prefix.c_str(), n->id(), n->length());
    else
        fprintf(stderr, "%s--> [%lu]:\n", prefix.c_str(), n->id());
    
    for (uint32_t i = 0; i < n->length(); i++)
        fprintf(stderr, "%c", utils::base2char(n->get_base(i)));
    
    if (n->collapsed())
        fprintf(stderr, ":\n");
    
    const node* children[256];
    n->get_children(children);
    
    for (int i = 0; i < n->degree(); i++) {
        uint8_t sym = children[i]->symbol();
        std::stringstream p;
        for (size_t i = 6; i < prefix.length(); i++)
            p << " ";
        p << "    -- ";
        p << utils::base2char(sym) << " ";
        print(children[i], p.str());
    }
}

// #######################
// dictionary_view methods
// #######################

dictionary_view::dictionary_view(const dictionary& d)
    : dict(d) {
    reset_phrase();
}

bool dictionary_view::follow(char c) {
    const node* child = cur_node->child(utils::char2base(c), offset);
    if (child)
        dpth++;
    
    if (child == cur_node) {
        offset++;
        cur_id++;
    } else if (child) {
        cur_node = child;
        cur_id = child->id();
        offset = 0;
    }
    
    return child;
}

bool dictionary_view::can_follow(char c) {
    return cur_node->child(utils::char2base(c), offset);
}

void dictionary_view::reset_phrase() {
    cur_node = dict.get_root();
    cur_id = 0;
    offset = 0;
    dpth = 0;
}

// ##################
// node_index methods
// ##################

node_index::node_index() {
    root = NULL;
    count = 0;
}

node_index::~node_index() {
    free_tree(root);
}

node_index::rb_node::rb_node(node* n) {
    this->n = n;
    this->parent = NULL;
    this->left = NULL;
    this->right = NULL;
    this->black = false;
}

node_index::iterator::iterator(rb_node* root) {
    this->n = root;
    this->state = 0;
}

node * node_index::iterator::next() {
    while (n) {
        if (state == 0 && n->left)
            n = n->left;
        else if (state == 0) {
            state = 1;
            return n->n;
        } else if (state == 1 && n->right) {
            n = n->right;
            state = 0;
        } else if (state == 1)
            state = 2;
        else if (n->parent && n == n->parent->left) {
            state = 1;
            n = n->parent;
            return n->n;
        } else
            n = n->parent;
    }
    
    return NULL;
}

node_index::rb_node * node_index::alloc_node(node* n) {
    return new rb_node(n);
}

void node_index::free_node(rb_node* n) {
    delete n;
}

void node_index::free_tree(rb_node* n) {
    if (!n)
        return;
    
    free_tree(n->left);
    free_tree(n->right);
    free_node(n);
}

void node_index::clear() {
    free_tree(root);
    root = NULL;
    count = 0;
}

void node_index::insert(rb_node* n) {
    if (root)
        insert(root, n);
    else
        root = n;
    
    insert_normalize(n);
    
    count++;
}

void node_index::insert(rb_node* tree, rb_node* n) {
    uint64_t tid, nid = n->n->id();
    
    while (!n->parent) {
        tid = tree->n->id();
        if (nid == tid)
            throw std::runtime_error("duplicate id");
        else if (nid < tid && tree->left)
            tree = tree->left;
        else if (nid < tid) {
            tree->left = n;
            n->parent = tree;
        } else if (tree->right)
            tree = tree->right;
        else {
            tree->right = n;
            n->parent = tree;
        }
    }
}

void node_index::insert_normalize(rb_node* n) {
    rb_node* parent = n->parent;
    rb_node* gp = grandparent(n);
    rb_node* u = uncle(n);
    
    if (!parent)
        n->black = true;
    if (!parent || parent->black)
        return;
    
    if (u && !u->black) {
        parent->black = true;
        u->black = true;
        gp->black = false;
        insert_normalize(gp);
    } else {
        if (n == parent->right && parent == gp->left) {
            rotate_left(parent);
            n = n->left;
        } else if (n == parent->left && parent == gp->right) {
            rotate_right(parent);
            n = n->right;
        }
        
        parent = n->parent;
        parent->black = true;
        gp->black = false;
        
        if (n == parent->left)
            rotate_right(gp);
        else
            rotate_left(gp);
    }
}

node_index::rb_node * node_index::grandparent(rb_node* n) {
    if (n && n->parent)
        return n->parent->parent;
    
    return NULL;
}

node_index::rb_node * node_index::uncle(rb_node* n) {
    rb_node* gp = grandparent(n);
    if (!gp)
        return NULL;
    
    if (n->parent == gp->left)
        return gp->right;
    
    return gp->left;
}

void node_index::add(node* n) {
    insert(alloc_node(n));
}

void node_index::remove(rb_node* n) {
    // swap with predecessor if the node has both children
    if (n->left && n->right) {
        rb_node* tmp = pred(n);
        n->n = tmp->n;
        n = tmp;
    }
    
    rb_node* tmp = n->left ? n->left : n->right;
    rb_node* parent = n->parent;
    
    // set the child's parent
    if (tmp)    
        tmp->parent = parent;
    
    // move the only child (if any) to the parent
    if (!parent) {
        if (tmp)
            tmp->black = true;
        root = tmp;
    } else if (n == parent->left)
        parent->left = tmp;
    else
        parent->right = tmp;
    
    if (is_black(n)) {
        if (!is_black(tmp))
            tmp->black = true;
        else
            remove_normalize(parent, tmp);
    }
    
    free_node(n);
    count--;
}

void node_index::remove_normalize(rb_node* parent, rb_node* n) {
    if (!parent)
        return;
    
    rb_node* sibling = parent->left == n ? parent->right : parent->left;
    if (!is_black(sibling)) {
        parent->black = false;
        sibling->black = true;
        if (sibling == parent->left)
            rotate_right(parent);
        else
            rotate_left(parent);
    }
    
    remove_normalize2(parent, n);
}

void node_index::remove_normalize2(rb_node* parent, rb_node* n) {
    rb_node* sibling = parent->left == n ? parent->right : parent->left;
    
    if (is_black(parent)
        && is_black(sibling)
        && is_black(sibling->left) 
        && is_black(sibling->right)) {
        sibling->black = false;
        remove_normalize(parent->parent, parent);
    } else if (!is_black(parent)
        && is_black(sibling)
        && is_black(sibling->left) 
        && is_black(sibling->right)) {
        sibling->black = false;
        parent->black = true;
    } else
        remove_normalize3(parent, n);
}

void node_index::remove_normalize3(rb_node* parent, rb_node* n) {
    rb_node* sibling = parent->left == n ? parent->right : parent->left;
    
    if (n == parent->left && is_black(sibling->right)) {
        sibling->black = false;
        sibling->left->black = true;
        rotate_right(sibling);
        sibling = sibling->parent;
    } else if (n == parent->right && is_black(sibling->left)) {
        sibling->black = false;
        sibling->right->black = true;
        rotate_left(sibling);
        sibling = sibling->parent;
    }
    
    sibling->black = parent->black;
    parent->black = true;
    
    if (n == parent->left) {
        sibling->right->black = true;
        rotate_left(parent);
    } else {
        sibling->left->black = true;
        rotate_right(parent);
    }
}

void node_index::rotate_left(rb_node* n) {
    rb_node* parent = n->parent;
    rb_node* right = n->right;
    
    n->right = right->left;
    if (n->right)
        n->right->parent = n;
    
    right->left = n;
    right->parent = parent;
    n->parent = right;
    
    if (!parent)
        root = right;
    else if (n == parent->left)
        parent->left = right;
    else
        parent->right = right;
}

void node_index::rotate_right(rb_node* n) {
    rb_node* parent = n->parent;
    rb_node* left = n->left;
    
    n->left = left->right;
    if (n->left)
        n->left->parent = n;
    
    left->right = n;
    left->parent = parent;
    n->parent = left;
    
    if (!parent)
        root = left;
    else if (n == parent->left)
        parent->left = left;
    else
        parent->right = left;
}

bool node_index::is_black(rb_node* n) {
    return !n || n->black;
}

void node_index::remove(node* n) {
    uint64_t cid, nid = n->id();
    rb_node* current = root;
    
    while (current && nid != (cid = current->n->id())) {
        if (nid < cid)
            current = current->left;
        else
            current = current->right;
    }
    
    if (current)
        remove(current);
}

node_index::rb_node * node_index::pred(rb_node* n) {
    if (n->left)
        return max(n->left);
    
    rb_node* prev;
    
    while ((prev = n->parent)) {
        if (prev->right == n)
            return prev;
        n = prev;
    }
    
    return NULL;
}

node_index::rb_node * node_index::max(rb_node* tree) {
    if (!tree)
        return NULL;
    
    while (tree->right)
        tree = tree->right;
    
    return tree;
}

node * node_index::get(uint64_t id) {
    if (!root)
        return NULL;
    
    rb_node* prev = NULL;
    rb_node* current = root;
    uint64_t cid;
    
    while (current != prev) {
        cid = current->n->id();
        prev = current;
        if (id < cid && current->left)
            current = current->left;
        else if (id > cid && current->right)
            current = current->right;
    }
    
    if (cid <= id)
        return current->n;
    
    current = pred(current);
    if (current)
        return current->n;
    
    return NULL;
}

node_index::iterator node_index::begin() {
    return iterator(root);
}

void node_index::print() {
    print(root);
    printf("\n");
}

void node_index::print(rb_node* n) {
    if (n) {
        printf("(");
        print(n->left);
        printf(" %lu", n->n->id());
        if (n->black)
            printf("B ");
        else
            printf("R ");
        print(n->right);
        printf(")");
    } else
        printf("NULL");
}

void node_index::validate() {
    if (!root)
        return;
    if (!root->black)
        throw std::runtime_error("root is red");
    
    validate(root, -1, 0);
}

int node_index::validate(rb_node* n, int bdepth, int cdepth) {
    if (!n) {
        if (bdepth == -1 || bdepth == cdepth)
            return cdepth;
        else
            throw std::runtime_error("black depth violated");
    }
    
    if (is_black(n))
        cdepth++;
    else if (!is_black(n->left) || !is_black(n->right))
        throw std::runtime_error("red node has a red child");
    
    bdepth = validate(n->left, bdepth, cdepth);
    return validate(n->right, bdepth, cdepth);
}

// ######################
// node allocator methods
// ######################

node_allocator::node_allocator() {
    this->nodes = 0;
    this->mem = 0;
}

node * node_allocator::alloc(uint8_t sym, node* parent) {
    node* n = new node(nodes++, sym, parent);
    insert(n);
    
    mem += n->size();
    
    return n;
}

node * node_allocator::alloc(uint8_t* phrase, uint32_t len, node* parent) {
    node* n;
    if (len == 0)
        return NULL;
    else if (len == 1)
        n = new node(nodes++, phrase[0], parent);
    else
#ifdef NODE_COLLAPSING
        n = new node(nodes++, phrase, len, parent);
#else
        throw std::runtime_error("node collapsing is disabled");
#endif
    
    insert(n);
    
    mem += n->size();
    nodes += len - 1;
    
    return n;
}

node * node_allocator::split(node* n, uint32_t at) {
    uint32_t len = n->length();
    
    if (at == len)
        return n;

#ifdef NODE_COLLAPSING
    int base = n->get_base(at);
    size_t id = n->id();
    node* child;
    
    id += at + 1;
    
    if ((at + 1) == len)
        child = new node(id, base, n->phrase_length(), n);
    else
        child = new node(n, at + 1, len - at - 1, n->phrase_length(), n);
    
    mem -= n->size();
    n->shrink(at);
    mem += n->size();
    mem += child->size();
    
    node* children[256];
    n->get_children(children);
    child->set_children(children, n->degree(), *this);
    
    children[0] = child;
    n->set_children(children, 1, *this);
    
    insert(child);
    
    return n;
#else
    throw std::runtime_error("node collapsing is disabled");
#endif
}

void node_allocator::append(node* n, uint8_t sym) {
#ifdef NODE_COLLAPSING
    mem -= n->size();
    n->append(sym);
    mem += n->size();
    nodes++;
#else
    throw std::runtime_error("node collapsing is disabled");
#endif
}

void node_allocator::append(node* n, uint8_t* phrase, uint32_t len) {
#ifdef NODE_COLLAPSING
    mem -= n->size();
    n->append(phrase, len);
    mem += n->size();
    nodes += len;
#else
    throw std::runtime_error("node collapsing is disabled");
#endif
}

node ** node_allocator::alloc_children(uint8_t count) {
    node** result = new node*[count];
    mem += count * sizeof(node*);
    
    return result;
}

void node_allocator::free_children(node** children, uint8_t count) {
    delete [] children;
    mem -= count * sizeof(node*);
}

simple_node_allocator::simple_node_allocator() {
    rnodes = 0;
}

void indexed_node_allocator::insert(node* n) {
    index.add(n);
}

void indexed_node_allocator::remove(node* n) {
    index.remove(n);
}

node * indexed_node_allocator::get(uint64_t id) {
    node* n = index.get(id);
    if (id <= (n->id() + n->length()))
        return n;
    
    return NULL;
}

