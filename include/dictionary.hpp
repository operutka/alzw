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

#ifndef _DICTIONARY_HPP
#define _DICTIONARY_HPP

#include <unordered_map>
#include <cstdio>
#include <stdint.h>

/** @file */

#define NODE_COLLAPSING

namespace alzw {
    // pre-declarations:
    class node_allocator;
    class indexed_node_allocator;
    class simple_node_allocator;
    
    class encoder;
    class decoder;
    
    /**
     * ALZW dictionary node. (Do NOT use any virtual methods in order to save 
     * some space that would be used by vtable.)
     */
    class node {
        uint64_t nid;       // node ID
        node* par;          // parent node
#ifdef NODE_COLLAPSING
        uint8_t* seq;       // collapsed sequence
        void*  children;    // children
        uint32_t plen;      // phrase length
        uint32_t len;       // collapsed sequence length
        uint8_t deg;        // number of children
        uint8_t sym;        // symbol for transition from a parent node to this node
#else
        void*  children;    // children
        uint32_t plen;      // phrase length
        uint8_t deg;        // number of children
        uint8_t sym;        // symbol for transition from a parent node to this node
#endif
        
        /**
         * Set symbol in the collapsed sequence.
         *
         * @param index zero-based offset
         * @param base  symbol
         */
        void set_base(uint32_t index, int base);
        
    public:
        /**
         * Create a new node.
         */
        node();
        
        /**
         * Create a new node with a given ID (phrase length will be set to 
         * parent phrase length + 1).
         *
         * @param id     node ID (codeword)
         * @param sym    transition symbol for the parent --> this transition
         * @param parent parent node
         */
        node(uint64_t id, uint8_t sym, node* parent);
        
        /**
         * Create a new node with a given ID.
         * 
         * @param id     node ID (codeword)
         * @param sym    transition symbol for the parent --> this transition
         * @param plen   forced phrase length
         * @param parent parent node
         */
        node(uint64_t id, uint8_t sym, uint32_t plen, node* parent);
        
#ifdef NODE_COLLAPSING
        /**
         * Create a new collapsed node with a given ID (phrase length will be 
         * set to parent phrase length + len).
         *
         * @param id     node ID (codeword)
         * @param phrase collapsed phrase including the transition symbol for 
         * the parent --> this transition
         * @param len    collapsed phrase length
         * @param parent parent node
         */
        node(uint64_t id, uint8_t* phrase, uint32_t len, node* parent);
        
        /**
         * Create a new collapsed node based on a given node (usefull for 
         * splitting of collapsed nodes).
         * 
         * @param n      template
         * @param offset zero-based offset within the template
         * @param len    collapsed sequence length
         * @param plen   forced phrase length
         * @param parent parent node
         */
        node(node* n, uint32_t offset, uint32_t len, uint32_t plen, node* parent);
#endif
        
        /**
         * Destructor (do NOT use virtual destructor in order to avoid using 
         * vtable).
         */
        ~node();
        
        /**
         * Release allocated links for children nodes.
         *
         * @param allocator node allocator
         */
        void release_children(node_allocator& allocator);
        
        /**
         * Get node ID (codeword).
         *
         * @returns node ID
         */
        uint64_t id() const { return nid; }
        
        /**
         * Set node ID.
         *
         * @param id new ID
         */
        void set_id(uint64_t id) { nid = id; }
        
        /**
         * Get transition symbol for the parent --> this transition
         *
         * @returns transition symbol
         */
        uint8_t symbol() const { return sym; }
        
        /**
         * Get parent node.
         *
         * @returns parent node or NULL
         */
        node * parent() { return par; }
        
        /**
         * Get parent node.
         *
         * @returns parent node or NULL
         */
        const node * parent() const { return par; }
        
        /**
         * Get node out degree (number of children).
         *
         * @returns number of children
         */
        uint8_t degree() const { return deg; }
        
        /**
         * Get child for a given symbol and offset.
         *
         * @param base   transition symbol
         * @param offset zero-based offset within a collapsed node
         * @returns child node, this or NULL
         */
        node * child(int base, uint32_t offset);
        
        /**
         * Get child for a given symbol and offset.
         *
         * @param base   transition symbol
         * @param offset zero-based offset within a collapsed node
         * @returns child node, this or NULL
         */
        const node * child(int base, uint32_t offset) const;
        
        /**
         * Get first child for a given offset.
         *
         * @param offset zero-based offset within a collapsed node
         * @returns child node, this or NULL
         */
        node * first_child(uint32_t offset);
        
        /**
         * Get children.
         *
         * @param c output buffer, the buffer should have at least degree() 
         * fields
         */
        void get_children(node** c);
        
        /**
         * Set children.
         *
         * @param c         children
         * @param count     number of children
         * @param allocator node allocator
         */
        void set_children(node** c, uint8_t count, node_allocator& allocator);
        
        /**
         * Get children.
         *
         * @param c output buffer, the buffer should have at least degree() 
         * fields
         */
        void get_children(const node** c) const;
        
        /**
         * Get phrase length (number of symbols between the root and the end of 
         * this node).
         *
         * @returns phrase length
         */
        uint32_t phrase_length() const { return plen; }

#ifdef NODE_COLLAPSING
        /**
         * Is this a collapsed node?
         *
         * @returns true if this node is collapsed, false otherwise
         */
        bool collapsed() const { return len > 0; }
        
        /**
         * Get length of the collapse sequence.
         *
         * @returns length of the collapse sequence
         */
        uint32_t length() const { return len; }
        
        /**
         * Append a given symbol to the collapsed sequence.
         *
         * @param base symbol
         */
        void append(int base);
        
        /**
         * Append a given phrase to the collapsed sequence.
         *
         * @param phrase phrase to be appended
         * @param len    phrase length
         */
        void append(uint8_t* phrase, uint32_t len);
        
        /**
         * Shrink the collapsed sequence to a given length.
         *
         * @param len target length
         */
        void shrink(uint32_t len);
#else
        /**
         * Is this a collapsed node?
         *
         * @returns true if this node is collapsed, false otherwise
         */
        bool collapsed() const { return false; }
        
        /**
         * Get length of the collapse sequence.
         *
         * @param length of the collapse sequence
         */
        uint32_t length() const { return 0; }
#endif

        /**
         * Get size (in bytes) of this node.
         *
         * @returns node size
         */
        size_t size() const;
        
        /**
         * Create a new child of this node for a given transition symbol.
         *
         * @param base      transition symbol
         * @param allocator node allocator
         * @returns created node
         */
        node * create(int base, node_allocator& allocator);
        
        /**
         * Get child node for a given symbol.
         *
         * @param base transition symbol
         * @returns child node or NULL
         */
        node * get(int base);
        
        /**
         * Get child node for a given symbol.
         *
         * @param base transition symbol
         * @returns child node or NULL
         */
        const node * get(int base) const;
        
        /**
         * Set child node for a given symbol.
         *
         * @param base      transition symbol
         * @param child     child node
         * @param allocator node allocator
         */
        void set(int base, node* child, node_allocator& allocator);
        
        /**
         * Get symbol from the collapsed sequence.
         *
         * @param index zero-based offset
         * @returns symbol
         */
        uint8_t get_base(uint32_t index) const;
    };
    
    /**
     * ALZW dictionary.
     */
    class dictionary {
        indexed_node_allocator* node_index;
        node_allocator* allocator;
        
        node* root;
        node* inode;
        node* dnode;
        node* wnode;
        
        node* cur_node;
        size_t cur_id;
        uint32_t offset;
        uint32_t dpth;
        
        uint8_t* addBuffer;
        size_t addBufferSize;
        uint32_t addLen;
        
        /**
         * Split current node.
         */
        void split_current();
        
        /**
         * Print subtree (for debugging).
         *
         * @param n      node
         * @param prefix prefix spacing
         */
        void print(const node* n, const std::string& prefix) const;
        
    public:
        /**
         * Create a new dictionary.
         *
         * @param indexed if true, the codewords will be indexed using RB tree
         */
        dictionary(bool indexed = true);
        
        virtual ~dictionary();
        
        /**
         * Follow transition from the current node for a given transition 
         * symbol. Add a new node if there is no such transition. All added 
         * nodes must be later commited using commit_phrase() method.
         *
         * @param c transition character
         * @returns new node ID (codeword)
         */
        uint64_t add(char c);
        
        /**
         * Get ID of the current node.
         *
         * @returns current node ID (current codeword)
         */
        uint64_t get_id() const;
        
        /**
         * Get ID of a newly added node.
         *
         * @returns ID of a newly added node
         */
        uint64_t next_id() const;
        
        /**
         * Get current node.
         *
         * @returns current node
         */
        const node * get() const;
        
        /**
         * Get node with a given ID.
         *
         * @param id node ID (codeword)
         * @returns node
         */
        const node * get(uint64_t id) const;
        
        /**
         * Get root node.
         *
         * @returns root node
         */
        const node * get_root() const { return root; }
        
        /**
         * Get insertion node.
         *
         * @returns insertion node
         */
        const node * get_inode() const { return inode; }
        
        /**
         * Get deletion node.
         *
         * @returns deletion node
         */
        const node * get_dnode() const { return dnode; }
        
        /**
         * Get width-increment node.
         *
         * @returns width-increment node
         */
        const node * get_wnode() const { return wnode; }
        
        /**
         * Follow transition from the current node for a given transition 
         * symbol.
         *
         * @param c transition symbol
         * @returns false if there is no such transition, true otherwise
         */
        bool follow(char c);
        
        /**
         * Check if there is transition for a given symbol from the current 
         * node.
         *
         * @param c transition symbol
         * @returns false if there is no such transition, true otherwise
         */
        bool can_follow(char c);
        
        /**
         * Commit the current phrase (all added symbols).
         */
        void commit_phrase();
        
        /**
         * Commit the current phrase and start a new one (set the root node as 
         * the current node).
         */
        void new_phrase();
        
        /**
         * Discard the current phrase and start a new one (set the root node as 
         * the current node).
         */
        void reset_phrase();
        
        /**
         * Get number of bytes used by dictionary nodes.
         * 
         * @returns used memory
         */
        size_t used_memory() const;
        
        /**
         * Get number of used virtual nodes (codewords).
         *
         * @returns number of codewords
         */
        size_t used_nodes() const;
        
        /**
         * Get number of real nodes.
         *
         * @returns number of real nodes.
         */
        size_t real_nodes() const;
        
        /**
         * Get dictionary depth.
         * 
         * @returns dictionary depth
         */
        uint32_t depth() const { return dpth; }
        
        /**
         * Print the dictionary (used for debugging).
         */
        void print() const;
    };
    
    /**
     * Dictionary view.
     */
    class dictionary_view {
        const dictionary& dict;
        const node* cur_node;
        size_t cur_id;
        uint32_t offset;
        uint32_t dpth;
    
    public:
        /**
         * Create a new dictionary view for a given dictionary.
         *
         * @param dict dictionary
         */
        dictionary_view(const dictionary& dict);
        
        /**
         * Get ID of the current node.
         *
         * @returns current node ID (current codeword)
         */
        uint64_t get_id() const { return cur_id; }
        
        /**
         * Get current node.
         *
         * @returns current node
         */
        const node * get() const { return cur_node; }
        
        /**
         * Get node with a given ID.
         *
         * @param id node ID (codeword)
         * @returns node
         */
        const node * get(uint64_t id) const { return dict.get(id); }
        
        /**
         * Get root node.
         *
         * @returns root node
         */
        const node * get_root() const { return dict.get_root(); }
        
        /**
         * Get insertion node.
         *
         * @returns insertion node
         */
        const node * get_inode() const { return dict.get_inode(); }
        
        /**
         * Get deletion node.
         *
         * @returns deletion node
         */
        const node * get_dnode() const { return dict.get_dnode(); }
        
        /**
         * Get width-increment node.
         *
         * @returns width-increment node
         */
        const node * get_wnode() const { return dict.get_wnode(); }
        
        /**
         * Follow transition from the current node for a given transition 
         * symbol.
         *
         * @param c transition symbol
         * @returns false if there is no such transition, true otherwise
         */
        bool follow(char c);
        
        /**
         * Check if there is transition for a given symbol from the current 
         * node.
         *
         * @param c transition symbol
         * @returns false if there is no such transition, true otherwise
         */
        bool can_follow(char c);
        
        /**
         * Start a new phrase (set the root node as the current node).
         */
        void reset_phrase();
        
        /**
         * Get number of bytes used by dictionary nodes.
         * 
         * @returns used memory
         */
        size_t used_memory() const { return dict.used_memory(); }
        
        /**
         * Get number of used virtual nodes (codewords).
         *
         * @returns number of codewords
         */
        size_t used_nodes() const { return dict.used_nodes(); }
        
        /**
         * Get number of real nodes.
         *
         * @returns number of real nodes.
         */
        size_t real_nodes() const { return dict.real_nodes(); }
        
        /**
         * Get dictionary depth.
         * 
         * @returns dictionary depth
         */
        uint32_t depth() const { return dpth; }
    };
    
    /**
     * Node index (based on RB tree).
     */
    class node_index {
    private:
        /**
         * RB node.
         */
        struct rb_node {
            node* n;
            rb_node* parent;
            rb_node* left;
            rb_node* right;
            bool black;
            
            rb_node(node* n);
        };
        
    public:
        /**
         * RB tree iterator.
         */
        class iterator {
            int state;
            rb_node* n;
        
        public:
            iterator(rb_node* root);
            node * next();
        };
    
    private:
        rb_node* root;
        size_t count;
        
        /**
         * Allocate a new RB node for a given dictionary node.
         *
         * @param n dictionary node
         * @returns RB node
         */
        rb_node * alloc_node(node* n);
        
        /**
         * Free a given RB node.
         *
         * @param n RB node
         */
        void free_node(rb_node* n);
        
        /**
         * Free a given RB (sub)tree.
         *
         * @param n RB (sub)tree
         */
        void free_tree(rb_node* n);
        
        /**
         * Insert a given RB node.
         *
         * @param n RB node
         */
        void insert(rb_node* n);
        
        /**
         * Insert a given RB node into a given RB tree.
         *
         * @param tree RB tree
         * @param n    RB node
         */
        void insert(rb_node* tree, rb_node* n);
        
        /**
         * Normalize RB tree after insertion.
         *
         * @param n inserted node
         */
        void insert_normalize(rb_node* n);
        
        /**
         * Remove a given node from RB tree.
         *
         * @param n RB node to be removed
         */
        void remove(rb_node* n);
        
        /**
         * Normalize RB tree after deletion.
         *
         * @param parent parent of the deleted node
         * @param n      child of the deleted node
         */
        void remove_normalize(rb_node* parent, rb_node* n);
        
        /**
         * Normalize RB tree after deletion (part 2).
         *
         * @param parent parent of the deleted node
         * @param n      child of the deleted node
         */
        void remove_normalize2(rb_node* parent, rb_node* n);
        
        /**
         * Normalize RB tree after deletion (part 3).
         *
         * @param parent parent of the deleted node
         * @param n      child of the deleted node
         */
        void remove_normalize3(rb_node* parent, rb_node* n);
        
        /**
         * Get predecessor of a given RB node.
         *
         * @param n RB node
         * @returns predecessor or NULL
         */
        rb_node * pred(rb_node* n);
        
        /**
         * Get maximum in a given RB tree.
         *
         * @param tree RB tree
         * @returns max RB node
         */
        rb_node * max(rb_node* tree);
        
        /**
         * Get grandparent of a given RB node.
         *
         * @param n RB node
         * @returns node's grandparent or NULL
         */
        rb_node * grandparent(rb_node* n);
        
        /**
         * Get uncle of a given RB node.
         *
         * @param n RB node
         * @returns node's uncle or NULL
         */
        rb_node * uncle(rb_node* n);
        
        /**
         * Rotate left around a given node.
         *
         * @param n RB node
         */
        void rotate_left(rb_node* n);
        
        /**
         * Rotate right around a given node.
         *
         * @param n RB node
         */
        void rotate_right(rb_node* n);
        
        /**
         * Is a given node black?
         *
         * @param n RB node or NULL
         * @returns true if the node is black, false otherwise
         */
        bool is_black(rb_node* n);
        
        /**
         * Print a given RB (sub)tree (used for debugging).
         *
         * @param n RB (sub)tree
         */
        void print(rb_node* n);
        
        /**
         * Validate a given RB (sub)tree.
         *
         * @param n RB (sub)tree
         * @param bdepth left-subtree black depth
         * @param cdepth right-subtree black depth
         * @returns black depth
         */
        int validate(rb_node* n, int bdepth, int cdepth);
        
    public:
        /**
         * Create a new ALZW node index.
         */
        node_index();
        
        virtual ~node_index();
        
        /**
         * Insert a given node.
         *
         * @param n ALZW node
         */
        void add(node* n);
        
        /**
         * Remove a given node.
         *
         * @param n ALZW node
         */
        void remove(node* n);
        
        /**
         * Remove all nodes.
         */
        void clear();
        
        /**
         * Get node with a given ID or the nearest lower node.
         *
         * @param id node ID
         * @returns node
         */
        node * get(uint64_t id);
        
        /**
         * Get number of indexed nodes.
         *
         * @returns number of nodes
         */
        size_t size() const { return count; }
        
        /**
         * Get number of bytes used by this index.
         *
         * @returns number of bytes used by this index
         */
        size_t used_memory() const { return count * sizeof(rb_node); }
        
        /**
         * Get index iterator.
         *
         * @returns index iterator
         */
        iterator begin();
        
        /**
         * Print the index (used for debugging).
         */
        void print();
        
        /**
         * Validate the RB tree.
         */
        void validate();
    };
    
    /**
     * Abstract node allocator.
     */
    class node_allocator {
    protected:
        size_t nodes;
        size_t mem;
        
        /**
         * Insert a given node into an index (if any).
         *
         * @param n node
         */
        virtual void insert(node* n) = 0;
        
        /**
         * Remove a given node from index (if any).
         *
         * @param n node
         */
        virtual void remove(node* n) = 0;
        
    public:
        /**
         * Create a new node allocator.
         */
        node_allocator();
        
        virtual ~node_allocator() { }
        
        /**
         * Get number of bytes used by nodes allocated using this allocator 
         * including size of a used index (if any).
         * 
         * @returns used memory
         */
        virtual size_t used_memory() const { return mem; }
        
        /**
         * Get number of used virtual nodes.
         *
         * @returns number of virtual nodes
         */
        virtual size_t used_nodes() const { return nodes; }
        
        /**
         * Get ID that will be assigned to the next newly allocated node.
         *
         * @returns next node ID
         */
        virtual uint64_t next_id() const { return nodes; }
        
        /**
         * Get number of real nodes.
         *
         * @returns number of real nodes.
         */
        virtual size_t real_nodes() const = 0;
        
        /**
         * Allocate a new node for a given transition parent -- sym --> node.
         * 
         * @param sym    transition symbol
         * @param parent parent node
         * @returns new node
         */
        virtual node * alloc(uint8_t sym, node* parent);
        
        /**
         * Allocate a new collapsed node for transitions according to a given 
         * phrase and parent.
         *
         * @param phrase collapsed phrase
         * @param len    collapsed phrase length
         * @param parent parent node
         * @returns new node
         */
        virtual node * alloc(uint8_t* phrase, uint32_t len, node* parent);
        
        /**
         * Split a given collapsed node at a given index.
         *
         * @param n  node to be split
         * @param at zero-based split index
         * @returns left part
         */
        virtual node * split(node* n, uint32_t at);
        
        /**
         * Append symbol into a given collapsed node.
         *
         * @param n   node
         * @param sym transition symbol
         */
        virtual void append(node* n, uint8_t sym);
        
        /**
         * Append phrase to a given collapsed node.
         *
         * @param n      node
         * @param phrase phrase
         * @param len    phrase length
         */
        virtual void append(node* n, uint8_t* phrase, uint32_t len);
        
        /**
         * Allocate array for storing children node pointers.
         *
         * @param count array width
         * @returns array
         */
        virtual node ** alloc_children(uint8_t count);
        
        /**
         * Free a given array for storing children node pointers.
         *
         * @param children array
         * @param count    array width
         */
        virtual void free_children(node** children, uint8_t count);
    };
    
    /** 
     * Simple node allocator (contains no index).
     */
    class simple_node_allocator : public node_allocator {
        size_t rnodes;
        
    protected:
        
        virtual void insert(node* n) { rnodes++; }
        
        virtual void remove(node* n) { rnodes--; }
        
    public:
        /**
         * Create a new simple node allocator.
         */
        simple_node_allocator();
        
        virtual ~simple_node_allocator() { }
        
        virtual size_t real_nodes() const { return rnodes; }
    };
    
    /**
     * RB tree indexed node allocator.
     */
    class indexed_node_allocator : public node_allocator {
        node_index index;
        
    protected:
        
        virtual void insert(node* n);
        
        virtual void remove(node* n);
        
    public:
        
        virtual ~indexed_node_allocator() { }
        
        virtual size_t used_memory() const { return mem + index.used_memory(); }
        
        virtual size_t real_nodes() const { return index.size(); }
        
        /**
         * Get node with a given ID or a collapsed node containing the given 
         * codeword.
         *
         * @param id node ID (codeword)
         */
        virtual node * get(uint64_t id);
    };
}

#endif /* _DICTIONARY_HPP */

