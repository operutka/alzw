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

#ifndef _ENCODER_HPP
#define _ENCODER_HPP

#include <string>
#include <vector>
#include <deque>

#include "dictionary.hpp"
#include "bit-io.hpp"

/** @file */

namespace alzw {
    /**
     * ALZW encoder.
     */
    class encoder {
        dictionary dict;
        std::deque<uint64_t> ins_queue;
        int sync_period;
        
        // encoding stats:
        size_t ndel;
        size_t nins;
        size_t nmm;
        
        size_t nmatches;
        size_t nmismatches;
        size_t ninserts;
        size_t ndeletes;
        
        size_t nmmseqs;
        size_t nmseqs;
        size_t niseqs;
        size_t ndseqs;
        
        size_t nmmouts;
        size_t niouts;
        size_t ndouts;
        
        size_t nmmbits;
        size_t nibits;
        size_t ndbits;
        
        size_t width;

#define OP_MATCH    0
#define OP_MISMATCH 1
#define OP_INS      2
#define OP_DEL      3

        int8_t last_op;

        // flags:
        bool fmismatch;
        bool fnew_node;
        bool fwidth_inc;
        
        /**
         * Encode match character.
         *
         * @param c   character
         * @param out output
         */
        void match(char c, bwriter& out);
        
        /**
         * Encode mismatch character.
         *
         * @param c   character
         * @param out output
         */
        void mismatch(char c, bwriter& out);
        
        /**
         * Encode insertion character.
         *
         * @param c   character
         * @param out output
         */
        void ins(char c, bwriter& out);
        
        /**
         * Encode deletion.
         *
         * @param out output
         */
        void del(bwriter& out);
        
        /**
         * Output match/mismatch codeword.
         *
         * @param id  codeword
         * @param out output
         */
        void out_mm(size_t id, bwriter& out);
        
        /**
         * Output insertion codeword.
         *
         * @param id  codeword
         * @param out output
         */
        void out_ins(size_t id, bwriter& out);
        
        /**
         * Output deletion.
         *
         * @param size number of deleted characters
         * @param out  output
         */
        void out_del(size_t size, bwriter& out);
        
        /**
         * Flush match/mismatch subsequence.
         *
         * @param out output
         */
        void flush_mm(bwriter& out);
        
        /**
         * Flush insertion subsequence.
         * 
         * @param out output
         */
        void flush_ins(bwriter& out);
        
        /**
         * Flush deletion subsequence.
         * 
         * @param out output
         */
        void flush_del(bwriter& out);
        
        /**
         * Flush all.
         * 
         * @param out output
         */
        void flush(bwriter& out);
        
        /**
         * Enter synchronization point.
         * 
         * @param out output
         */
        void sync(bwriter& out);
        
    public:
        /**
         * Create a new ALZW encoder.
         *
         * @param sync_period synchronization period (or minimum phrase length
         * in case of adaptive synchronization)
         */
        encoder(int sync_period);
        
        /**
         * Encode a given pairwise alignment.
         *
         * @param rseq     reference sequence
         * @param aseq     aligned sequence
         * @param out      output
         * @param sync_map synchronization map for adaptive synchronization
         */
        void encode(const std::string& rseq, const std::string& aseq, 
            bwriter& out, std::vector<uint32_t>* sync_map = NULL);
        
        /**
         * Get dictionary.
         *
         * @returns dictionary
         */
        const dictionary & get_dictionary() const { return dict; }
        
        /**
         * Get size of encoded data.
         * 
         * @returns size of encoded data
         */
        size_t size() const { return (nmmbits + nibits + ndbits) >> 3; }
        
        /**
         * Get total number of encoded bits in match/mismatch subsequences.
         * 
         * @returns number of encoded match/mismatch bits
         */
        size_t mmbits() const { return nmmbits; }
        
        /**
         * Get total number of encoded bits in insertion subsequences.
         * 
         * @returns number of encoded insertion bits
         */
        size_t ibits() const { return nibits; }
        
        /**
         * Get total number of encoded bits in deletion subsequences.
         * 
         * @returns number of encoded deletion bits
         */
        size_t dbits() const { return ndbits; }
        
        /**
         * Get total number of matched symbols.
         *
         * @returns total number of matched symbols
         */
        size_t matches() const { return nmatches; }
        
        /**
         * Get total number of mismatched symbols.
         *
         * @returns total number of mismatched symbols
         */
        size_t mismatches() const { return nmismatches; }
        
        /**
         * Get total number of inserted symbols.
         *
         * @returns total number of inserted symbols
         */
        size_t inserts() const { return ninserts; }
        
        /**
         * Get total number of deleted symbols.
         *
         * @returns total number of deleted symbols
         */
        size_t deletes() const { return ndeletes; }
        
        /**
         * Get total number of match/mismatch subsequences read.
         *
         * @returns number of match/mismatch subsequences
         */
        size_t mmseqs() const { return nmmseqs; }
        
        /**
         * Get total number of match subsequences read.
         *
         * @returns number of match subsequences
         */
        size_t mseqs() const { return nmseqs; }
        
        /**
         * Get total number of insertion subsequences read.
         *
         * @returns number of insertion subsequences
         */
        size_t iseqs() const { return niseqs; }
        
        /**
         * Get total number of deletion subsequences read.
         *
         * @returns number of deletion subsequences
         */
        size_t dseqs() const { return ndseqs; }
        
        /**
         * Get total number of written match/mismatch subsequences.
         *
         * @returns number of written match/mismatch subsequences
         */
        size_t mmouts() const { return nmmouts; }
        
        /**
         * Get total number of written insertion subsequences.
         *
         * @returns number of written insertion subsequences
         */
        size_t iouts() const { return niouts; }
        
        /**
         * Get total number of written deletion subsequences.
         *
         * @returns number of written deletion subsequences
         */
        size_t douts() const { return ndouts; }
        
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
    };
}

#endif /* _ENCODER_HPP */

