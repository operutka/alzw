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
#include <deque>

#include "fautomaton.hpp"
#include "utils.hpp"

using namespace alzw;

// ###########################
// df_automaton::state methods
// ###########################

df_automaton::state::state() {
    this->sid = 0;
    
    for (int i = 0; i < DFA_ALPHABET_SIZE; i++)
        transitions[i] = -1;
}

df_automaton::state::state(const state& other) {
    sid = other.sid;
    
    memcpy(transitions, other.transitions, sizeof(transitions));
}

df_automaton::state& df_automaton::state::operator=(const state& other) {
    sid = other.sid;
    
    memcpy(transitions, other.transitions, sizeof(transitions));
    
    return *this;
}

void df_automaton::state::print() const {
    for (int i = 0; i < DFA_ALPHABET_SIZE; i++) {
        if (transitions[i] >= 0)
            fprintf(stderr, "    %3d -> %d\n", i, transitions[i]);
        else if (transitions[i] < 0)
            fprintf(stderr, "    %3d -> ERROR\n", i);
    }
}

// ####################
// df_automaton methods
// ####################

df_automaton::df_automaton() {
    this->states = NULL;
    this->scount = 0;
}

df_automaton::df_automaton(int states) {
    this->states = new state[states];
    this->scount = states;
    for (int i = 0; i < states; i++)
        this->states[i].sid = i;
}

df_automaton::df_automaton(const df_automaton& other) {
    states = new state[other.scount];
    scount = other.scount;
    for (int i = 0; i < scount; i++)
        states[i] = other.states[i];
}

df_automaton::~df_automaton() {
    delete [] states;
}

df_automaton& df_automaton::operator=(const df_automaton& other) {
    delete [] states;
    
    states = new state[other.scount];
    scount = other.scount;
    for (int i = 0; i < scount; i++)
        states[i] = other.states[i];
    
    return *this;
}

df_automaton::state * df_automaton::get(int sid) {
    if (sid < 0 || sid >= scount)
        throw std::runtime_error("sid out of range");
    
    return states + sid;
}

const df_automaton::state * df_automaton::get(int sid) const {
    if (sid < 0 || sid >= scount)
        throw std::runtime_error("sid out of range");
    
    return states + sid;
}

int df_automaton::next(int sid, uint8_t sym) const {
    const state* s = get(sid);
    return s->get(sym);
}

void df_automaton::print() const {
    for (int i = 0; i < scount; i++) {
        fprintf(stderr, "state %8d transitions:\n", i);
        states[i].print();
    }
}

// ####################################
// pattern_matching_dfa_builder methods
// ####################################

void pattern_matching_dfa_builder::build(
    df_automaton& dfa, const std::string& pattern) {
    dfa = df_automaton(pattern.length() + 1);
    
    df_automaton::state* state = dfa.get(0);
    for (int a = 0; a < DFA_ALPHABET_SIZE; a++)
        state->set(a, 0);
    
    for (size_t i = 0; i < pattern.length(); i++) {
        state = dfa.get(i);
        state->set(utils::char2base(pattern[i]), i + 1);
    }
    
    size_t* ba = make_border_array(pattern);
    for (size_t i = 1; i <= pattern.length(); i++) {
        state = dfa.get(i);
        for (int a = 0; a < DFA_ALPHABET_SIZE; a++) {
            if (i == pattern.length() || a != utils::char2base(pattern[i])) {
                df_automaton::state* tmp = dfa.get(ba[i - 1]);
                state->set(a, tmp->get(a));
            }
        }
    }
    delete [] ba;
}

size_t * pattern_matching_dfa_builder::make_border_array(const std::string& s) {
    size_t* ba = new size_t[s.length()];
    
    ba[0] = 0;
    
    for (size_t i = 1; i < s.length(); i++) {
        size_t j = ba[i - 1];
        while (j > 0 && s[i] != s[j])
            j = ba[j - 1];
        
        if (s[i] == s[j])
            ba[i] = j + 1;
        else
            ba[i] = 0;
    }
    
    return ba;
}

