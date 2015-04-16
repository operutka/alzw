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

#ifndef _SEARCH_ENGINE_HPP
#define _SEARCH_ENGINE_HPP

#include <string>
#include <functional>
#include <unordered_map>
#include <deque>
#include <stdint.h>

#include "dictionary.hpp"
#include "decoder.hpp"
#include "fautomaton.hpp"

/** @file */

namespace alzw {
    // pre-declarations and types
    class signature;
    
    typedef std::reference_wrapper<const signature> signature_cref;
}

namespace std {
    // pre-declaration of hasher for signature reference
    template<>
    struct hash<alzw::signature_cref>;
    
    // pre-declaration of comparator for signature reference
    template<>
    struct equal_to<alzw::signature_cref>;
}

namespace alzw {
    /**
     * LM representative signature.
     */
    class signature {
        int* destinations;
        bool* finals;
        int scount;
        size_t hash;
    
        /**
         * Compute hash of this object.
         *
         * @returns hash
         */
        size_t compute_hash() const;
        
        friend class std::hash<signature_cref>;
        friend class std::equal_to<signature_cref>;
        
        /**
         * Signature initialization helper.
         *
         * @param sig signature of a prefix
         */
        void init(const signature& sig);
    
    public:
        /**
         * Create a new empty signature.
         */
        signature();
        
        /**
         * Copy constructor.
         *
         * @param sig other signature
         */
        signature(const signature& sig);
        
        /**
         * Create an empsilon signature.
         *
         * @param dfa DFA
         */
        signature(const df_automaton& dfa);
        
        /**
         * Create a new signature for a given automaton, prefix signature and 
         * suffix character.
         *
         * @param dfa    DFA
         * @param sig    prefix signature
         * @param suffix suffix symbol
         */
        signature(const df_automaton& dfa, 
            const signature& sig, uint8_t suffix);
        
        virtual ~signature();
        
        /**
         * Get destination state for a given initial state.
         *
         * @param sid initial state ID
         * @returns destination state ID
         */
        int destination(int sid) const { return destinations[sid]; }
        
        /**
         * Check if phrases with this signature go through a final state for 
         * a given initial state.
         *
         * @param sid initial state ID
         * @returns true if a final state is reached, false otherwise
         */
        bool is_final(int sid) const { return finals[sid]; }
        
        /**
         * Equality operator.
         *
         * @param other other signature
         * @returns true if this signature equals to the given signature, false 
         * otherwise
         */
        bool operator==(const signature& other) const;
        
        /**
         * Print the signature (used for debugging).
         */
        void print() const;
    };
}

namespace std {
    /**
     * Hasher for signature reference.
     */
    template<>
    struct hash<alzw::signature_cref> {
        size_t operator()(const alzw::signature_cref& sr) const {
            return sr.get().hash;
        }
    };
    
    /**
     * Comparator for signature reference.
     */
    template<>
    struct equal_to<alzw::signature_cref> {
        bool operator()(const alzw::signature_cref& x, 
            const alzw::signature_cref& y) const {
            return x.get() == y.get();
        }
    };
}

namespace alzw {
    /**
     * LM phrase representative.
     */
    class representative {
        const representative* transitions[DFA_ALPHABET_SIZE];
        const representative* prev;
        uint8_t sym;
        
        const df_automaton& dfa;
        signature sig;
    
    public:
        /**
         * Create a new epsilon representative for a given DFA.
         *
         * @param dfa DFA
         */
        representative(const df_automaton& dfa);
        
        /**
         * Create a new representative for given prefix representative and 
         * suffix symbol.
         *
         * @param prev prefix
         * @param sym  suffix
         */
        representative(const representative& prev, uint8_t sym);
        
        virtual ~representative() { }
        
        /**
         * Get prefix representative.
         *
         * @returns prefix representative
         */
        const representative * get_prev() const { return prev; }
        
        /**
         * Get last transition symbol.
         *
         * @returns last transition symbol
         */
        uint8_t symbol() const { return sym; }
        
        /**
         * Get current representative's signature.
         *
         * @returns signature
         */
        const signature& get_signature() const { return sig; }
        
        /**
         * Set next representative for a given transition symbol.
         *
         * @param sym transition symbol
         * @param r   next representative
         */
        void set_transition(uint8_t sym, const representative* r)
            { transitions[sym] = r; }
        
        /**
         * Get next representative for a given transition symbol.
         *
         * @param sym transition symbol
         * @returns next representative
         */
        const representative * get_transition(uint8_t sym) const
            { return transitions[sym]; }
    };
    
    /**
     * Table of representatives.
     */
    class representative_table {
        typedef std::unordered_map<signature_cref, representative*> signature_map;
        signature_map reps;
        representative* eps;
    
    public:
        /**
         * Create a new table of representatives for a given DFA.
         *
         * @param dfa DFA
         */
        representative_table(const df_automaton& dfa);
        
        virtual ~representative_table();
        
        /**
         * Get epsilon representative.
         *
         * @returns epsilon representative
         */
        const representative * epsilon() const { return eps; }
        
        /**
         * Print the table (used for debugging).
         */
        void print() const;
    };

// pattern-matching algorithms
#define SE_ALG_SIMPLE       0
#define SE_ALG_BMH          1
#define SE_ALG_DFA          2
#define SE_ALG_LM           3
    
    // pre-declaration
    class search_task;
    
    /**
     * Search engine class.
     */
    class search_engine {
    public:
        typedef void match_handler(size_t seq, size_t offset, void* misc);
    
    private:
        double construction_time;
        
        std::string alzw_file;
        std::string rseq;
        decoder dec;
        
        /**
         * Invoke a given search task.
         *
         * @param stask search task
         * @param h     match handler
         * @param misc  user data to be passed back to the handler
         */
        void search(search_task& stask, match_handler* h, void* misc);
    
    public:
        /**
         * Create a new search engine for given reference sequence and ALZW 
         * compressed file.
         *
         * @param rseq_file path to a file containing FASTA encoded reference
         * sequence
         * @param alzw_file path to an ALZW file archive
         */
        search_engine(const char* rseq_file, const char* alzw_file);
        
        virtual ~search_engine() { }
        
        /**
         * Search for a given pattern using a given pattern-matching algorithm.
         *
         * @param alg   algorithm
         * @param query pattern
         * @param h     match handler
         * @param misc  user data to be passed back to the handler
         */
        void search(int alg, const std::string& query, 
            match_handler* h, void* misc);
    };
    
    /**
     * Abstract stream search provider.
     */
    class stream_searcher {
        typedef std::unordered_map<uint64_t, const node*> node_map;
        
        std::deque<uint8_t> phrase;
        const decoder& dec;
        
        /**
         * Decode a given codeword and load the phrase into the internal stack.
         *
         * @param cw codeword
         */
        void load_phrase(uint64_t cw);
        
    protected:
        uint8_t* pattern;
        size_t plen;
        
        uint8_t* sbuffer;
        size_t sb_cap;
        size_t sb_size;
        size_t offset;
        size_t seq;
        
        /**
         * Search for the pattern inside the internal search buffer and discard 
         * used part of the buffer by incrementing offset and decrementing 
         * sb_size.
         *
         * @param h    match handler
         * @param misc user data to be passed back to the handler
         */
        virtual void search_step(
            search_engine::match_handler* h, void* misc) = 0;
        
    public:
        /**
         * Create a new stream search provider.
         *
         * @param dec   decoder
         * @param query pattern
         */
        stream_searcher(const decoder& dec, const std::string& query);
        
        virtual ~stream_searcher();
        
        /**
         * Reset the search provider. Discard all data inside the search 
         * buffer, drop current codeword and reset all counters.
         *
         * @param seq    new sequence number
         * @param offset initial offset
         */
        virtual void reset(size_t seq, size_t offset);
        
        /**
         * Process a given codeword.
         *
         * @param cw   codeword
         * @param h    match handler
         * @param misc user data to be passed back to the handler
         */
        size_t process_cw(uint64_t cw, 
            search_engine::match_handler* h, void* misc);
    };
    
    /**
     * Stream search provider using a naive pattern-matching algorithm.
     */
    class simple_stream_searcher : public stream_searcher {
    protected:
        virtual void search_step(search_engine::match_handler* h, void* misc);
        
    public:
        /**
         * Create a new simple stream searcher for a given pattern.
         *
         * @param dec   decoder
         * @param query pattern
         */
        simple_stream_searcher(const decoder& dec, const std::string& query);
        
        virtual ~simple_stream_searcher() { }
    };
    
    /**
     * Boyer-Moore-Horspool stream search provider.
     */
    class bmh_stream_searcher : public stream_searcher {
        size_t bcs[DFA_ALPHABET_SIZE];
        
    protected:
        virtual void search_step(search_engine::match_handler* h, void* misc);
        
    public:
        /**
         * Create a new BMH stream searcher for a given pattern.
         *
         * @param dec   decoder
         * @param query pattern
         */
        bmh_stream_searcher(const decoder& dec, const std::string& query);
        
        virtual ~bmh_stream_searcher() { }
    };
    
    /**
     * DFA stream search provider.
     */
    class dfa_stream_searcher : public stream_searcher {
        const df_automaton& dfa;
        int state, fstate;
    
    protected:
        virtual void search_step(search_engine::match_handler* h, void* misc);
        
    public:
        /**
         * Create a new BMH stream searcher for a given pattern. (note: query 
         * and dfa must equal)
         *
         * @param dec   decoder
         * @param query pattern
         * @param dfa   DFA
         */
        dfa_stream_searcher(const decoder& dec, const std::string& query, 
            const df_automaton& dfa);
        
        virtual ~dfa_stream_searcher() { }
        
        virtual void reset(size_t seq, size_t offset);
    };
    
    /**
     * Abstract search task.
     */
    class search_task {
        const std::string& alzw_file;
        const std::string& rseq;
        
        const dictionary& dict;
        
        int initial_pwidth;
        int pwidth;
        
        /**
         * Skip file table in a given ALZW stream.
         *
         * @param in ALZW stream
         * @returns number of encoded sequences
         */
        size_t skip_file_table(breader& in);
        
        /**
         * Read insertion from a given ALZW stream.
         *
         * @param in   ALZW stream
         * @param h    match handler
         * @param misc user data to be passed back to the handler
         */
        void read_insert(breader& in, 
            search_engine::match_handler* h, void* misc);
    
    protected:
        size_t rseq_offset;
        size_t seq_offset;
        size_t seq;
        
        /**
         * Initialize task.
         */
        virtual void init_search();
        
        /**
         * Enter a new sequence.
         */
        virtual void new_sequence();
        
        /**
         * Process a given codeword.
         *
         * @param cw   codeword
         * @param h    match handler
         * @param misc user data to be passed back to the handler
         */
        virtual size_t process_cw(uint64_t cw, 
            search_engine::match_handler* h, void* misc) = 0;
        
    public:
        /**
         * Create a new search task.
         * 
         * @param alzw_file ALZW encoded file
         * @param dec       decoder (must be already initialized and frozen)
         * @param rseq      reference sequence
         */
        search_task(const std::string& alzw_file, const decoder& dec, 
            const std::string& rseq);
        
        virtual ~search_task() { }
        
        /**
         * Invoke task.
         *
         * @param h    match handler
         * @param misc user data to be passed back to the handler
         */
        void search(search_engine::match_handler* h, void* misc);
    };
    
    /**
     * Sequence-search task (based on decoding).
     */
    class ss_task : public search_task {
        stream_searcher& ss;
    
    protected:
    
        virtual void init_search();
        
        virtual void new_sequence();
        
        virtual size_t process_cw(uint64_t cw, 
            search_engine::match_handler* h, void* misc);
        
    public:
        /**
         * Create a new search task for given search provider and ALZW stream.
         *
         * @param alzw_file ALZW encoded file
         * @param dec       decoder (must be already initialized and frozen)
         * @param rseq      reference sequence
         * @param ss        search provider
         */
        ss_task(const std::string& alzw_file, const decoder& dec, 
            const std::string& rseq, stream_searcher& ss);
        
        virtual ~ss_task() { }
    };
    
    /**
     * Lahoda-Melichar search task.
     */
    class lm_task : public search_task {
        typedef std::unordered_map<uint64_t, const representative*> representative_map;
        typedef std::unordered_map<uint64_t, const node*> node_map;
        
        /**
         * Match filter. Used to avoid multiple match handler invocations for 
         * a same match.
         */
        struct match_filter_context {
            search_engine::match_handler* handler;
            void* misc;
            ssize_t last_match;
            
            match_filter_context(search_engine::match_handler* h, void* misc,
                ssize_t last_match);
        };
        
        const decoder& dec;
        
        df_automaton dfa;
        int state;
        
        representative_table* rtable;
        representative_map rmap;
        
        std::deque<uint8_t> suffix_stack;
        
        dfa_stream_searcher ss;
        std::deque<std::pair<uint64_t, size_t>> cw_window;
        size_t window_offset;
        size_t window_size;
        
        ssize_t last_match;
        
        /**
         * Get signature of a given codeword.
         * 
         * @param cw codeword
         * @returns signature
         */
        const signature * get_signature(uint64_t cw);
        
        /**
         * Get representative of a given codeword.
         *
         * @param cw codeword
         * @returns representative
         */
        const representative * get_representative(uint64_t cw);
        
        /**
         * Get ALZW dictionary node with a given ID.
         *
         * @param id node ID
         * @returns node
         */
        const node * get_node(uint64_t id);
        
        /**
         * Get length of the phrase represented by a given node.
         *
         * @param id node ID (codeword)
         * @returns phrase length
         */
        size_t phrase_length(uint64_t id);
        
        /**
         * Match filter helper function.
         *
         * @param seq    sequence no.
         * @param offset match offset
         * @param misc   pointer to match_filter_context
         */
        static void match_filter(size_t seq, size_t offset, void* misc);
    
    protected:
        
        virtual void init_search();
        
        virtual void new_sequence();
        
        virtual size_t process_cw(uint64_t cw, 
            search_engine::match_handler* h, void* misc);
        
    public:
        /**
         * Create a new search task for given query and ALZW stream.
         *
         * @param alzw_file ALZW encoded file
         * @param dec       decoder (must be already initialized and frozen)
         * @param rseq      reference sequence
         * @param query     pattern
         */
        lm_task(const std::string& alzw_file, const decoder& dec, 
            const std::string& rseq, const std::string& query);
        
        virtual ~lm_task();
    };
}

#endif /* _SEARCH_ENGINE_HPP */

