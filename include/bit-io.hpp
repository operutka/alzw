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

#ifndef _BIT_IO_HPP
#define _BIT_IO_HPP

#include <stdint.h>
#include <cstdio>

/** @file */

namespace alzw {
    /**
     * Abstract bit-writer.
     */
    class bwriter {
    public:
        virtual ~bwriter() { }
        
        /**
         * Write given number of least significant bits of a given number in 
         * big endian order.
         *
         * @param bits  bits
         * @param width number of bits
         */
        virtual void write(uint64_t bits, int width) = 0;
        
        /**
         * Write a given null-terminated string.
         *
         * @param str string
         */
        virtual void write_str(const char* str);
        
        /**
         * Write first n bits from a given buffer.
         *
         * @param buffer buffer
         * @param bits   number of bits to write
         */
        virtual void write_buf(const uint8_t* buffer, size_t bits);
        
        /**
         * Write a gamma-encoded number.
         *
         * @param n number
         * @returns number of written bits
         */
        virtual int write_gamma(uint64_t n);
        
        /**
         * Write a delta-encoded number.
         *
         * @param n number
         * @returns number of written bits
         */
        virtual int write_delta(uint64_t n);
        
        /**
         * Flush buffered data. All buffered bits will be written. Note that 
         * complete bytes must be written, this may cause to write more bits 
         * in case the last bits does not form a complete byte.
         */
        virtual void flush() = 0;
    };
    
    /**
     * Abstract bit-reader.
     */
    class breader {
    public:
        virtual ~breader() { }
        
        /**
         * Read a given number of bits. Bits will be read in big endian order 
         * and put into a given integer variable as its least significant bits. 
         * Note that up to 64 bits can be read at once.
         *
         * @param bits  output variable
         * @param width number of bits to be read
         * @returns number of bits actually read (0 means EOF)
         */
        virtual int read(uint64_t& bits, int width) = 0;
        
        /**
         * Read integer in big endian order.
         *
         * @returns read integer
         */
        virtual int read_int();
        
        /**
         * Read a gamma-encoded number.
         *
         * @returns number
         */
        virtual uint64_t read_gamma();
        
        /**
         * Read a delta-encoded number.
         * 
         * @returns number
         */
        virtual uint64_t read_delta();
        
        /**
         * Read a null-terminated string (max size - 1 characters will be 
         * read).
         *
         * @param buffer output buffer
         * @param size   buffer size
         * @returns number of read bytes or -1 if the buffer is full and the 
         * '\0' character was not read yet
         */
        virtual ssize_t read_str(char* buffer, size_t size);
    };
    
    /**
     * Stream bit-writer.
     */
    class stream_bwriter : public bwriter {
        uint8_t buffer[4096];
        size_t bit_offset;
    
    protected:
        FILE* stream;
    
    public:
        /**
         * Create a new bit-writer for a given stream.
         *
         * @param stream stream
         */
        stream_bwriter(FILE* stream);
        
        virtual ~stream_bwriter();
        
        virtual void write(uint64_t bits, int width);
        virtual void flush();
    };
    
    /**
     * Stream bit-reader.
     */
    class stream_breader : public breader {
        uint8_t buffer[4096];
        size_t bit_offset;
        size_t available;
    
    protected:
        FILE* stream;
    
    public:
        /**
         * Create a new bit-reader for a given stream.
         *
         * @param stream stream
         */
        stream_breader(FILE* stream);
        
        virtual ~stream_breader();
        
        virtual int read(uint64_t& bits, int width);
    };
    
    /**
     * File bit-writer.
     */
    class file_bwriter : public stream_bwriter {
    public:
        /**
         * Create a new bit-writer for a given file.
         *
         * @param file path to a file
         */
        file_bwriter(const char* file);
        
        virtual ~file_bwriter();
    };
    
    /**
     * File bit-reader.
     */
    class file_breader : public stream_breader {
    public:
        /**
         * Create a new bit-reader for a given file.
         *
         * @param file path to a file
         */
        file_breader(const char* file);
        
        virtual ~file_breader();
    };
}

#endif /* _BIT_IO_HPP */

