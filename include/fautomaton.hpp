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

#ifndef _FAUTOMATON_HPP
#define _FAUTOMATON_HPP

#include <unordered_map>
#include <string>

/** @file */

namespace alzw {
    /**
     * Deterministic finite automaton.
     */
    class df_automaton {
    public:

// size of the used alphabet
#define DFA_ALPHABET_SIZE   5
        
        /**
         * Automaton state.
         */
        class state {
            int transitions[DFA_ALPHABET_SIZE];
            int sid;
            
            friend class df_automaton;
            
        public:
            /**
             * Create a new state.
             */
            state();
            
            /**
             * Copy constructor.
             * 
             * @param other other state
             */
            state(const state& other);
            
            virtual ~state() { }
            
            /** 
             * Assignment operator.
             *
             * @param other other state
             * @returns this
             */
            state& operator=(const state& other);
            
            /**
             * Get state ID.
             *
             * @returns state ID
             */
            int id() const { return sid; }
            
            /**
             * Set transition.
             *
             * @param sym transition symbol
             * @param sid destination
             */
            void set(uint8_t sym, int sid) { transitions[sym] = sid; }
            
            /**
             * Get destination for a given transition symbol.
             *
             * @param sym transition symbol
             * @returns destination state or NULL
             */
            int get(uint8_t sym) const { return transitions[sym]; }
            
            /**
             * Print the state (used for debugging).
             */
            void print() const;
        };
        
    private:
        state* states;
        int scount;
        
    public:
        /**
         * Create a new empty automaton.
         */
        df_automaton();
        
        /**
         * Create a new automaton with a given number of states.
         * 
         * @param states number of states
         */
        df_automaton(int states);
        
        /**
         * Copy constructor.
         *
         * @param other other automaton
         */
        df_automaton(const df_automaton& other);
        
        virtual ~df_automaton();
        
        /**
         * Assignment operator.
         *
         * @param other other automaton
         * @returns this
         */
        df_automaton& operator=(const df_automaton& other);
        
        /**
         * Get state with a given ID.
         *
         * @param sid state ID
         * @returns state
         */
        state * get(int sid);
        
        /**
         * Get state with a given ID.
         *
         * @param sid state ID
         * @returns state
         */
        const state * get(int sid) const;
        
        /**
         * Get number of states.
         *
         * @returns number of states
         */
        int state_count() const { return scount; }
        
        /**
         * Get next state ID for a given transition.
         *
         * @param sid state ID
         * @param sym transition symbol
         * @returns destination ID
         */
        int next(int sid, uint8_t sym) const;
        
        /**
         * Print the automaton (used for debugging).
         */
        void print() const;
    };
    
    /**
     * Pattern matching DFA builder.
     */
    class pattern_matching_dfa_builder {
        /**
         * Make border array for a given string.
         *
         * @param s string
         * @returns border array
         */
        size_t * make_border_array(const std::string& s);
        
    public:
        /**
         * Build pattern matching DFA for a given string.
         *
         * @param dfa     output DFA
         * @param pattern pattern
         */
        void build(df_automaton& dfa, const std::string& pattern);
    };
}

#endif /* _FAUTOMATON_HPP */

