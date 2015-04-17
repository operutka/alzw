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

#include <deque>
#include <cstring>
#include <iostream>
#include <vector>
#include <algorithm>

#include "search-engine.hpp"
#include "utils.hpp"
#include "exception.hpp"

using namespace alzw;

// #################
// signature methods
// #################

signature::signature() {
    destinations = NULL;
    finals       = NULL;
    
    scount = 0;
    
    hash = compute_hash();
}

signature::signature(const signature& sig) {
    delete [] destinations;
    delete [] finals;
    
    scount = sig.scount;
    hash   = sig.hash;
    
    if (scount > 0) {
        destinations = new int[scount];
        finals       = new bool[scount];
        memcpy(destinations, sig.destinations, scount * sizeof(int));
        memcpy(finals, sig.finals, scount * sizeof(bool));
    } else {
        destinations = NULL;
        finals       = NULL;
    }
}

signature::signature(const df_automaton& dfa) {
    scount = dfa.state_count();
    
    destinations = new int[scount];
    finals       = new bool[scount];
    
    for (int i = 0; i < scount; i++) {
        destinations[i] = i;
        finals[i] = false;
    }
    
    hash = compute_hash();
}

signature::signature(const df_automaton& dfa, 
    const signature& sig, uint8_t suffix) {
    scount       = dfa.state_count();
    destinations = new int[scount];
    finals       = new bool[scount];
    
    init(sig);
    
    int fsid = scount - 1;
    int dsid;
    
    for (int sid = 0; sid < scount; sid++) {
        dsid = destinations[sid];
        dsid = dfa.next(dsid, suffix);
        finals[sid] |= dsid == fsid;
        destinations[sid] = dsid;
    }
    
    hash = compute_hash();
}

signature::~signature() {
    delete [] destinations;
    delete [] finals;
}

void signature::init(const signature& sig) {
    if (sig.scount == 0) {
        for (int i = 0; i < scount; i++) {
            destinations[i] = i;
            finals[i] = false;
        }
    } else {
        memcpy(destinations, sig.destinations, scount * sizeof(int));
        memcpy(finals, sig.finals, scount * sizeof(bool));
    }
}

size_t signature::compute_hash() const {
    std::hash<int> ihasher;
    std::hash<bool> bhasher;
    
    size_t hash = 13;
    hash = hash * 19 + ihasher(scount);
    for (int i = 0; i < scount; i++) {
        hash = hash * 11 + ihasher(destinations[i]);
        hash = hash * 17 + bhasher(finals[i]);
    }
    
    return hash;
}

bool signature::operator==(const signature& other) const {
    if (scount != other.scount)
        return false;
    
    for (int i = 0; i < scount; i++) {
        if (destinations[i] != other.destinations[i]
            || finals[i] != other.finals[i])
            return false;
    }
    
    return true;
}

void signature::print() const {
    for (int i = 0; i < scount; i++)
        fprintf(stderr, "(%d, %d) ", destinations[i], (int)finals[i]);
}

// ######################
// representative methods
// ######################

representative::representative(const df_automaton& d)
    : dfa(d)
    , sig(dfa) {
    this->prev = NULL;
    this->sym  = 0;
    
    memset(transitions, 0, sizeof(transitions));
}

representative::representative(const representative& prev, uint8_t sym)
    : dfa(prev.dfa)
    , sig(dfa, prev.sig, sym) {
    this->prev = &prev;
    this->sym  = sym;
    
    memset(transitions, 0, sizeof(transitions));
}

// ############################
// representative_table methods
// ############################

representative_table::representative_table(const df_automaton& dfa) {
    std::deque<representative*> repr_queue;
    representative* p;
    representative* r;
    uint8_t sym;
    
    eps = new representative(dfa);
    
    repr_queue.push_back(eps);
    while (!repr_queue.empty()) {
        r = repr_queue.front();
        repr_queue.pop_front();
        p = (representative*)r->get_prev();
        sym = r->symbol();
        
        const signature& sig = r->get_signature();
        signature_cref sig_cref(sig);
        
        signature_map::iterator it = reps.find(sig_cref);
        if (it == reps.end()) {
            if (p) p->set_transition(sym, r);
            reps[sig_cref] = r;
            for (int i = 0; i < DFA_ALPHABET_SIZE; i++)
                repr_queue.push_back(new representative(*r, i));
        } else {
            if (p) p->set_transition(sym, reps[sig_cref]);
            delete r;
        }
    }
}

representative_table::~representative_table() {
    signature_map::iterator it = reps.begin();
    for (; it != reps.end(); ++it)
        delete it->second;
}

void representative_table::print() const {
    fprintf(stderr, "table of representants (size: %lu, eps: %016lx):\n", (unsigned long)reps.size(), (unsigned long)eps);
    signature_map::const_iterator it = reps.begin();
    for (; it != reps.end(); ++it) {
        representative* r = it->second;
        const signature& sig = r->get_signature();
        fprintf(stderr, "%016lx: %016lx %02x ", (unsigned long)r, (unsigned long)r->get_prev(), r->symbol());
        sig.print();
        fprintf(stderr, "transitions:\n");
        for (int i = 0; i < DFA_ALPHABET_SIZE; i++) {
            const representative* t = r->get_transition(i);
            if (!t)
                fprintf(stderr, "    %3d -> ERROR\n", i);
            else if (t != eps)
                fprintf(stderr, "    %3d -> %016lx\n", i, (unsigned long)t);
        }
    }
}

// #######################
// stream_searcher methods
// #######################

stream_searcher::stream_searcher(const decoder& d, const std::string& query)
    : dec(d) {
    this->plen     = query.length();
    this->pattern  = new uint8_t[plen];
    
    this->sb_cap   = ((plen << 1) + 4095) & ~4095;
    this->sbuffer  = new uint8_t[sb_cap];
    this->sb_size  = 0;
    this->offset   = 0;
    
    this->seq      = 0;
    
    for (size_t i = 0; i < plen; i++)
        pattern[i] = utils::char2base(query[i]);
}

stream_searcher::~stream_searcher() {
    delete [] pattern;
    delete [] sbuffer;
}

void stream_searcher::load_phrase(uint64_t cw) {
    const node_map& nmap = dec.get_phrases();
    node_map::const_iterator it = nmap.find(cw);
    if (it == nmap.end())
        throw runtime_exception("unknown codeword: 0x%016lx", (unsigned long)cw);
    
    const node* n = it->second;
    
    while (n->parent()) {
        if (cw > n->id())
            phrase.push_back(n->get_base(--cw - n->id()));
        else {
            phrase.push_back(n->symbol());
            n = n->parent();
            cw = n->id() + n->length();
        }
    }
}

void stream_searcher::reset(size_t seq, size_t offset) {
    this->sb_size = 0;
    this->offset  = offset;
    this->seq     = seq;
    
    phrase.clear();
}

size_t stream_searcher::process_cw(uint64_t cw, 
    search_engine::match_handler* h, void* misc) {
    load_phrase(cw);
    
    size_t res = phrase.size();
    size_t i;
    
    while (!phrase.empty()) {
        if (sb_size >= sb_cap)
            search_step(h, misc);
        
        i = offset + sb_size++;
        sbuffer[i % sb_cap] = phrase.back();
        phrase.pop_back();
    }
    
    search_step(h, misc);
    
    return res;
}

// ##############################
// simple_stream_searcher methods
// ##############################

simple_stream_searcher::simple_stream_searcher(
    const decoder& dec, const std::string& query)
    : stream_searcher(dec, query) {
}

void simple_stream_searcher::search_step(
    search_engine::match_handler* h, void* misc) {
    while (sb_size >= plen) {
        bool match = true;
        for (size_t i = 0; i < plen && match; i++)
            match &= sbuffer[(offset + i) % sb_cap] == pattern[i];
        if (match && h)
            (*h)(seq, offset, misc);
        
        offset++;
        sb_size--;
    }
}

// ###########################
// bmh_stream_searcher methods
// ###########################

bmh_stream_searcher::bmh_stream_searcher(
    const decoder& dec, const std::string& query)
    : stream_searcher(dec, query) {
    size_t end = plen - 1;
    
    for (size_t i = 0; i < DFA_ALPHABET_SIZE; i++)
        bcs[i] = plen;
    
    for (size_t i = 0; i < end; i++)
        bcs[pattern[i]] = end - i;
}

void bmh_stream_searcher::search_step(
    search_engine::match_handler* h, void* misc) {
    size_t end = plen - 1;
    size_t shift;
    
    while (sb_size >= plen) {
        bool match = true;
        for (ssize_t i = end; i >= 0 && match; i--)
            match &= sbuffer[(offset + i) % sb_cap] == pattern[i];
        if (match && h)
            (*h)(seq, offset, misc);
        
        shift    = bcs[sbuffer[(offset + end) % sb_cap]];
        offset  += shift;
        sb_size -= shift;
    }
}

// ###########################
// dfa_stream_searcher methods
// ###########################

dfa_stream_searcher::dfa_stream_searcher(
    const decoder& dec, const std::string& query, const df_automaton& fa)
    : stream_searcher(dec, query)
    , dfa(fa) {
    state  = 0;
    fstate = query.length();
}

void dfa_stream_searcher::search_step(
    search_engine::match_handler* h, void* misc) {
    while (sb_size > 0) {
        state = dfa.next(state, sbuffer[offset++ % sb_cap]);
        if (state == fstate && h)
            (*h)(seq, offset - plen, misc);
        
        sb_size--;
    }
}

void dfa_stream_searcher::reset(size_t seq, size_t offset) {
    stream_searcher::reset(seq, offset);
    state = 0;
}

// ###################
// search_task methods
// ###################

search_task::search_task(const std::string& alzwf, const decoder& dec, 
    const std::string& rs)
    : alzw_file(alzwf)
    , rseq(rs)
    , dict(dec.get_dictionary()) {
    dictionary tmpd(false);
    initial_pwidth = (int)ceil(log(tmpd.used_nodes()) / log(2));
}

void search_task::search(search_engine::match_handler* h, void* misc) {
    fprintf(stderr, "searching...\n");
    
    file_breader in(alzw_file.c_str());
    size_t seqc = skip_file_table(in);
    init_search();
    
    const node* inode = dict.get_inode();
    const node* dnode = dict.get_dnode();
    const node* wnode = dict.get_wnode();
    uint64_t cw;
    size_t plen;
    size_t i = 0;
    
    while (i < seqc) {
        if (pwidth > in.read(cw, pwidth))
            throw runtime_exception("unexpected EOF in ALZW stream");
        
        if (cw == dnode->id())
            rseq_offset += in.read_delta();
        else if (cw == inode->id())
            read_insert(in, h, misc);
        else if (cw == wnode->id()) {
            if (pwidth == (sizeof(cw) << 3))
                throw runtime_exception("codeword width overflow");
            pwidth++;
        } else {
            plen = process_cw(cw, h, misc);
            seq_offset  += plen;
            rseq_offset += plen;
        }
        
        if (rseq_offset >= rseq.length()) {
            new_sequence();
            i++;
        }
    }
}

void search_task::init_search() {
    rseq_offset = 0;
    seq_offset  = 0;
    seq    = 1;
    pwidth = initial_pwidth;
}

void search_task::new_sequence() {
    rseq_offset = 0;
    seq_offset  = 0;
    seq++;
}

size_t search_task::skip_file_table(breader& in) {
    char buffer[4096];
    
    int seqc = in.read_int();
    for (int i = 0; i < seqc; i++) {
        if (in.read_str(buffer, sizeof(buffer)) < 0)
            throw runtime_exception("ALZW sequence file name is too long, maximum supported length is 4095 characters");
    }
    
    if (seqc < 0)
        throw runtime_exception("negative number of ALZW sequences");
    else if (seqc == 0)
        return 1;
    
    return seqc;
}

void search_task::read_insert(breader& in, 
    search_engine::match_handler* h, void* misc) {
    size_t icount = in.read_delta();
    uint64_t cw   = 0;
    
    for (size_t i = 0; i < icount; i++) {
        if (pwidth > in.read(cw, pwidth))
            throw runtime_exception("unexpected EOF in ALZW stream");
        
        seq_offset += process_cw(cw, h, misc);
    }
}

// ###############
// ss_task methods
// ###############

ss_task::ss_task(const std::string& alzw_file, const decoder& dec, 
    const std::string& rseq, stream_searcher& s)
    : search_task(alzw_file, dec, rseq)
    , ss(s) {
}

void ss_task::init_search() {
    search_task::init_search();
    ss.reset(seq, 0);
}

void ss_task::new_sequence() {
    search_task::new_sequence();
    ss.reset(seq, 0);
}

size_t ss_task::process_cw(uint64_t cw, 
    search_engine::match_handler* h, void* misc) {
    return ss.process_cw(cw, h, misc);
}

// ###############
// lm_task methods
// ###############

lm_task::lm_task(const std::string& alzw_file, const decoder& d, 
    const std::string& rseq, const std::string& query)
    : search_task(alzw_file, d, rseq)
    , dec(d)
    , ss(dec, query, dfa) {
    pattern_matching_dfa_builder bldr;
    bldr.build(dfa, query);
    rtable = new representative_table(dfa);
}

lm_task::~lm_task() {
    delete rtable;
}

void lm_task::init_search() {
    search_task::init_search();
    
    state = 0;
    
    window_offset = 0;
    window_size   = 0;
    cw_window.clear();
    
    last_match = -1;
}

void lm_task::new_sequence() {
    search_task::new_sequence();
    
    state  = 0;
    
    window_offset = 0;
    window_size   = 0;
    cw_window.clear();
    
    last_match = -1;
}

size_t lm_task::process_cw(uint64_t cw, 
    search_engine::match_handler* h, void* misc) {
    const signature* sig = get_signature(cw);
    
    if (sig->is_final(state)) {
        match_filter_context mfc(h, misc, last_match);
        
        ss.reset(seq, window_offset);
        for (size_t i = 0; i < cw_window.size(); i++)
            ss.process_cw(cw_window[i].first, match_filter, &mfc);
        ss.process_cw(cw, match_filter, &mfc);
        
        last_match = mfc.last_match;
    }
    
    state = sig->destination(state);
    
    size_t plen = phrase_length(cw);
    cw_window.push_back(std::make_pair(cw, plen));
    window_size += plen;
    
    while (!cw_window.empty()) {
        size_t tmp = cw_window.front().second;
        if ((window_size - tmp) < (size_t)dfa.state_count())
            break;
        
        cw_window.pop_front();
        window_size   -= tmp;
        window_offset += tmp;
    }
    
    return plen;
}

const signature * lm_task::get_signature(uint64_t cw) {
    const representative* r = get_representative(cw);
    return &r->get_signature();
}

const representative * lm_task::get_representative(uint64_t cw) {
    representative_map::iterator it = rmap.find(cw);
    if (it != rmap.end())
        return it->second;
    
    const node* n = get_node(cw);
    uint64_t orig_cw = cw;
    
    if (!n)
        throw runtime_exception("unknown codeword: 0x%016lx", (unsigned long)cw);
    
    while (!rmap.count(cw) && n->parent()) {
        if (cw > n->id())
            suffix_stack.push_back(n->get_base(--cw - n->id()));
        else {
            suffix_stack.push_back(n->symbol());
            n = n->parent();
            cw = n->id() + n->length();
        }
    }
    
    const representative* r = n->parent()
        ? rmap[cw]
        : rtable->epsilon();
    
    while (!suffix_stack.empty()) {
        r = r->get_transition(suffix_stack.back());
        suffix_stack.pop_back();
    }
    
    return rmap[orig_cw] = r;
}

const node * lm_task::get_node(uint64_t id) {
    const node_map& nmap = dec.get_phrases();
    node_map::const_iterator it = nmap.find(id);
    if (it == nmap.end())
        return NULL;
    
    return it->second;
}

size_t lm_task::phrase_length(uint64_t id) {
    const node* n = get_node(id);
    if (!n)
        return 0;
    
    size_t plen = n->phrase_length();
    size_t roffset = n->id() + n->length() - id;
    
    return plen - roffset;
}

lm_task::match_filter_context::match_filter_context(
    search_engine::match_handler* h, void* misc, ssize_t last_match) {
    this->handler    = h;
    this->misc       = misc;
    this->last_match = last_match;
}

void lm_task::match_filter(size_t seq, size_t offset, void* misc) {
    match_filter_context* mfc = (match_filter_context*)misc;
    if (mfc->last_match >= (ssize_t)offset)
        return;
    
    mfc->last_match = offset;
    
    (*mfc->handler)(seq, offset, mfc->misc);
}

// #####################
// search_engine methods
// #####################

search_engine::search_engine(const char* rseqf, const char* alzwf)
    : construction_time(utils::time())
    , alzw_file(alzwf)
    , rseq(utils::load_fasta(rseqf))
    , dec(rseq) {
    file_breader br(alzw_file.c_str());
    char buffer[4096];
    
    int seqc = br.read_int();
    for (int i = 0; i < seqc; i++) {
        if (br.read_str(buffer, sizeof(buffer)) < 0)
            throw runtime_exception("ALZW sequence file name is too long, maximum supported length is 4095 characters");
    }
    
    if (seqc < 0)
        throw runtime_exception("negative number of ALZW sequences");
    else if (seqc == 0)
        seqc = 1;
    
    for (int i = 0; i < seqc; i++)
        dec.decode(br);
    
    dec.freeze();
    
    double t = utils::time() - construction_time;
    fprintf(stderr, "index loaded in [s]: %.6f\n", t);
}

void search_engine::search(search_task& stask, match_handler* h, void* misc) {
    double t = utils::time();
    stask.search(h, misc);
    t = utils::time() - t;
    fprintf(stderr, "search time [s]: %.6f\n", t);
}

void search_engine::search(int alg, const std::string& query, 
    match_handler* h, void* misc) {
    double t = utils::time();
    if (alg == SE_ALG_SIMPLE) {
        simple_stream_searcher sss(dec, query);
        ss_task stask(alzw_file, dec, rseq, sss);
        
        double t2 = utils::time() - t;
        fprintf(stderr, "preprocessing time [s]: %.9f\n", t2);
        search(stask, h, misc);
    } else if (alg == SE_ALG_BMH) {
        bmh_stream_searcher bmh(dec, query);
        ss_task stask(alzw_file, dec, rseq, bmh);
        
        double t2 = utils::time() - t;
        fprintf(stderr, "preprocessing time [s]: %.9f\n", t2);
        search(stask, h, misc);
    } else if (alg == SE_ALG_DFA) {
        pattern_matching_dfa_builder bldr;
        df_automaton dfa;
        
        bldr.build(dfa, query);
        
        dfa_stream_searcher dfa_ss(dec, query, dfa);
        ss_task stask(alzw_file, dec, rseq, dfa_ss);
        
        double t2 = utils::time() - t;
        fprintf(stderr, "preprocessing time [s]: %.9f\n", t2);
        search(stask, h, misc);
    } else if (alg == SE_ALG_LM) {
        lm_task stask(alzw_file, dec, rseq, query);
        
        double t2 = utils::time() - t;
        fprintf(stderr, "preprocessing time [s]: %.9f\n", t2);
        search(stask, h, misc);
    }
    
    t = utils::time() - t;
    fprintf(stderr, "total time [s]: %.6f\n", t);
}

