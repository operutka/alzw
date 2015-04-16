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

#ifndef _SAM_ALIGNMENT_HPP
#define _SAM_ALIGNMENT_HPP

#include <vector>
#include <string>

#include "alignment.hpp"

/** @file */

namespace alzw {
    /**
     * SAM alignment.
     */
    class sam_alignment : public alignment {
        std::vector<std::string> seqs;
    
        /**
         * Create a new empty alignment.
         */
        sam_alignment() { }
        
    public:
        
        virtual ~sam_alignment() { }
        
        virtual size_t count() const { return seqs.size(); }
        
        virtual const std::string& operator[](size_t index) const
            { return seqs[index]; }
        
        /**
         * Read DNA alignment from a given binary SAM file. The only 
         * accepted base are A, C, G, T and N. All other bases will be 
         * converted to N. All lower-case symbols will be converted to 
         * upper-case.
         *
         * @param ref     reference sequence
         * @param samfile path to a file in binary SAM format
         * @returns alignemnt
         */
        static sam_alignment load(const std::string& ref, const char* samfile);
        
        /**
         * Read DNA alignment from a given binary SAM file. The only 
         * accepted base are A, C, G, T and N. All other bases will be 
         * converted to N. All lower-case symbols will be converted to 
         * upper-case.
         *
         * @param fastafile path to a file in FASTA format containing 
         * a reference sequence
         * @param samfile   path to a file in binary SAM format
         * @returns alignemnt
         */
        static sam_alignment load(const char* fastafile, const char* samfile);
    };
}

#endif /* _SAM_ALIGNMENT_HPP */

