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

#ifndef _ALIGNMENT_HPP
#define _ALIGNMENT_HPP

#include <string>

/** @file */

namespace alzw {
    /**
     * Abstract alignment class.
     */
    class alignment {
    public:
        virtual ~alignment() { }
        
        /**
         * Get number of aligned sequences.
         * 
         * @returns number of aligned sequences
         */
        virtual size_t count() const = 0;
        
        /**
         * Get aligned sequence.
         *
         * @param index sequence index
         * @returns aligned sequence
         */
        virtual const std::string& operator[](size_t index) const = 0;
    };
}

#endif /* _ALIGNMENT_HPP */

