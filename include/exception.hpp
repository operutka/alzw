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

#ifndef _EXCEPTION_HPP
#define _EXCEPTION_HPP

#include <cstdarg>
#include <stdexcept>

/** @file */

namespace alzw {
    /**
     * ALZW exception class.
     */
    class exception : public std::exception {
    protected:
        std::string msg;
        
    public:
        /**
         * Create an empty exception.
         */
        exception() throw() { }
        
        virtual ~exception() throw() { }
        
        /**
         * Get error message.
         * 
         * @returns error message
         */
        virtual const char * what() const throw() { return msg.c_str(); }
    };
    
    /**
     * IO exception class.
     */
    class io_exception : public exception {
    public:
        /**
         * Create a new IO exception with formatted message.
         *
         * @param fmt message format string
         */
        io_exception(const char* fmt, ...) throw();
        
        virtual ~io_exception() throw() { }
    };
    
    /**
     * Parse exception class.
     */
    class parse_exception : public exception {
    public:
        /**
         * Create a new IO exception with formatted message.
         *
         * @param fmt message format string
         */
        parse_exception(const char* fmt, ...) throw();
        
        virtual ~parse_exception() throw() { }
    };
    
    /**
     * Runtime exception class.
     */
    class runtime_exception : public exception {
    public:
        /**
         * Create a new IO exception with formatted message.
         *
         * @param fmt message format string
         */
        runtime_exception(const char* fmt, ...) throw();
        
        virtual ~runtime_exception() throw() { }
    };
}

#endif /* _EXCEPTION_HPP */

