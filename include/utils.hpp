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

#ifndef _UTILS_HPP
#define _UTILS_HPP

#include <string>
#include <cstdio>

/** @file */

namespace alzw {
    namespace utils {
        /**
         * Read FASTA encoded DNA sequence from a given stream. The only 
         * accepted symbols are A, C, G, T and N. All lower-case symbols will 
         * be converted to upper-case. The file can contain only a single 
         * sequence.
         *
         * @param file stream
         * @returns sequence of symbols A, C, G, T and N
         */
        std::string load_fasta(FILE* file);
        
        /**
         * Read FASTA encoded DNA sequence from a given file. The only 
         * accepted symbols are A, C, G, T and N. All lower-case symbols will 
         * be converted to upper-case. The file can contain only a single 
         * sequence.
         *
         * @param file path to a file
         * @returns sequence of symbols A, C, G, T and N
         */
        std::string load_fasta(const char* file);
        
        /**
         * Realloc a given buffer if needed.
         *
         * @param buffer pointer to a buffer (it may point to NULL)
         * @param size   current size (may be 0)
         * @param nsize  required size
         * @returns new size
         */
        size_t realloc(uint8_t** buffer, size_t size, size_t nsize);
        
        /**
         * Realloc a given buffer if needed and copy all data from the old 
         * buffer to the new one.
         *
         * @param buffer pointer to a buffer (it may point to NULL)
         * @param size   current size (may be 0)
         * @param nsize  required size
         * @returns new size
         */
        size_t crealloc(uint8_t** buffer, size_t size, size_t nsize);

// used alphabet
#define ALPHABET        "ACGTN"

        /**
         * Convert a given character to a base (index in the used alphabet).
         *
         * @param c character
         * @returns base
         */
        uint8_t char2base(char c);
        
        /**
         * Convert a given base to a character (representative from the used 
         * alphabet).
         *
         * @param base base
         * @returns character
         */
        char base2char(uint8_t base);
        
        /**
         * Get bit-width of a given number.
         *
         * @param n number
         * @returns width of the given number
         */
        int number_width(uint64_t n);
        
        /**
         * Get current timestamp.
         *
         * @returns timestamp in seconds
         */
        double time();
    }
}

#endif /* _UTILS_HPP */

