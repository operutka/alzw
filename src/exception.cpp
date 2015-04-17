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

#include <cstdio>

#include "exception.hpp"

using namespace alzw;

#define FORMAT_EXCEPTION(fmt) \
    char buffer[4096]; \
    va_list args; \
    va_start(args, fmt); \
    vsnprintf(buffer, sizeof(buffer), fmt, args); \
    va_end(args); \
    msg = buffer;

io_exception::io_exception(const char* fmt, ...) throw() {
    FORMAT_EXCEPTION(fmt)
}

parse_exception::parse_exception(const char* fmt, ...) throw() {
    FORMAT_EXCEPTION(fmt)
}

runtime_exception::runtime_exception(const char* fmt, ...) throw() {
    FORMAT_EXCEPTION(fmt)
}

