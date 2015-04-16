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

#ifndef _DECODER_HPP
#define _DECODER_HPP

#include <ostream>
#include <string>
#include <unordered_map>

#include "dictionary.hpp"
#include "bit-io.hpp"

/** @file */

namespace alzw {
    /**
     * ALZW decoder.
     */
    class decoder {
        std::unordered_map<uint64_t, const node*> phrases;
        bool hash_index;
        
        const std::string& rseq;
        dictionary dict;
        
        size_t offset;
        int width;
        
        // output buffer
        char obuffer[4096];
        size_t ob_offset;
        
        // buffer for reversed phrases
        char* rbuffer;
        size_t rbufferSize;
        
        /**
         * Output a given character. Use the internal buffer first. In case 
         * the buffer is full, flush it.
         *
         * @param c   character
         * @param out output stream pointer (may be NULL)
         */
        void output_char(char c, std::ostream* out);
        
        /**
         * Flush the internal output buffer.
         *
         * @param out output stream pointer (may be NULL)
         */
        void flush_output_buffer(std::ostream* out);
        
        /**
         * Decode a single sequence from a given input.
         *
         * @param in  input
         * @param out output stream pointer (may be NULL)
         */
        void decode(breader& in, std::ostream* out);
        
        /**
         * Decode insertion.
         *
         * @param roffset reference sequence offset
         * @param in      input
         * @param out     output stream pointer (may be NULL)
         */
        void decode_ins(size_t roffset, breader& in, std::ostream* out);
        
        /**
         * Decode a single math/replace subsequence.
         *
         * @param cw      codeword
         * @param roffset reference sequence offset
         * @param in      input
         * @param out     output stream pointer (may be NULL)
         * @returns phrase width
         */
        size_t decode_mr(uint64_t cw, 
            size_t roffset, breader& in, std::ostream* out);
        
        /**
         * Output the phrase represented by a given node.
         *
         * @param n       node
         * @param noffset offset within a collapsed node
         * @param roffset reference sequence offset
         * @param out     output stream pointer (may be NULL)
         * @returns phrase width
         */
        size_t output_node(const node* n, uint32_t noffset, 
            size_t roffset, std::ostream* out);
        
        /**
         * Copy symbols from the reference sequence until there is a given 
         * codeword in the dictionary.
         *
         * @param cw      codeword
         * @param roffset reference sequence offset
         * @param out     output stream pointer (may be NULL)
         * @returns phrase width
         */
        size_t output_match(uint64_t cw, size_t roffset, std::ostream* out);
        
    public:
        /**
         * Create a new decoder using a given reference sequence.
         *
         * @param rseq       reference sequence
         * @param hash_index whether or not to create a hash-index for decoded 
         * codewords (note: freeze() method must be called after decoding all 
         * sequences in order to use the index)
         */
        decoder(const std::string& rseq, bool hash_index = true);
        
        virtual ~decoder();
        
        /**
         * Decode a next sequence and drop it (the sequence will be used only 
         * to update the dictionary).
         *
         * @param in input
         */
        void decode(breader& in);
    
        /**
         * Decode a next sequence.
         *
         * @param in  input
         * @param out output
         */
        void decode(breader& in, std::ostream& out);
        
        /**
         * Freeze the dictionary and build the hash-index. No sequences should
         * be decoded after invoking this method!
         */
        void freeze();
        
        /**
         * Get dictionary.
         *
         * @returns dictionary
         */
        const dictionary & get_dictionary() const { return dict; }
        
        /**
         * Get hash-index.
         *
         * @returns codeword -> phrase map
         */
        const std::unordered_map<uint64_t, const node*>& get_phrases() const 
            { return phrases; }
        
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

#endif /* _DECODER_HPP */

