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

#include "bit-io.hpp"
#include "utils.hpp"
#include "exception.hpp"

#include <cstring>

using namespace alzw;

void bwriter::write_str(const char* str) {
    write_buf((const uint8_t*)str, (strlen(str) + 1) << 3);
}

void bwriter::write_buf(const uint8_t* buffer, size_t bits) {
    size_t offset = 0;
    
    while (bits >= 8) {
        write(buffer[offset++], 8);
        bits -= 8;
    }
    
    if (bits > 0)
        write(buffer[offset], bits);
}

int bwriter::write_gamma(uint64_t n) {
    int bits  = utils::number_width(n);
    int width = (bits << 1) - 1;
    
    n = (1 << (bits - 1)) | n;
    
    write(n, width);
    
    return width;
}

int bwriter::write_delta(uint64_t n) {
    int bits   = utils::number_width(n);
    int gwidth = write_gamma(bits);
    int bwidth = bits - 1;
    
    write(n, bwidth);
    
    return gwidth + bwidth;
}

int breader::read_int() {
    uint64_t val = 0;
    
    read(val, sizeof(int) << 3);
    
    return (int)val;
}

uint64_t breader::read_gamma() {
    uint64_t bits = 0;
    uint64_t val  = 0;
        
    int len = read(val, 1);
    
    while (len && val == 0) {
        len = read(val, 1);
        bits++;
    }
    
    if (bits == 0)
        return 1;
    else if (bits > 63)
        throw runtime_exception("gamma code overflow");
    
    read(val, bits);
    
    return (1 << bits) | val;
}

uint64_t breader::read_delta() {
    uint64_t bits = read_gamma() - 1;
    uint64_t val  = 0;
    
    if (bits == 0)
        return 1;
    else if (bits > 63)
        throw runtime_exception("delta code overflow");
    
    read(val, bits);
    
    return (1 << bits) | val;
}

ssize_t breader::read_str(char* buffer, size_t size) {
    size_t offset = 0;
    uint64_t val = 0;
    
    int res = read(val, 8);
    while (res > 0 && val != 0 && (offset + 1) < size) {
        buffer[offset++] = (char)val;
        res = read(val, 8);
    }
    
    buffer[offset] = 0;
    
    if (val != 0)
        return -1;
    
    return offset;
}

stream_bwriter::stream_bwriter(FILE* stream) {
    this->stream     = stream;
    this->bit_offset = 0;
}

stream_bwriter::~stream_bwriter() {
    flush();
}

void stream_bwriter::write(uint64_t bits, int width) {
    if ((bit_offset + width) > (sizeof(buffer) << 3)) {
        fwrite(buffer, sizeof(uint8_t), bit_offset >> 3, stream);
        if (ferror(stream))
            throw io_exception("error while writing into a file");
        
        buffer[0] = buffer[bit_offset >> 3];
        bit_offset &= 0x7;
    }
    
    while (width > 0) {
        int wa = 8 - (bit_offset & 0x7);
        int wr = wa > width ? width : wa;
        uint8_t m = (1 << wr) - 1;
        uint8_t b = (bits >> (width - wr)) & m;
        buffer[bit_offset >> 3] &= ~m << (wa - wr);
        buffer[bit_offset >> 3] |=  b << (wa - wr);
        
        width -= wr;
        bit_offset += wr;
    }
}

void stream_bwriter::flush() {
    fwrite(buffer, sizeof(uint8_t), (bit_offset + 7) >> 3, stream);
    fflush(stream);
    
    bit_offset = 0;
}

file_bwriter::file_bwriter(const char* file)
    : stream_bwriter(fopen(file, "wb")) {
    if (!stream)
        throw io_exception("unable to open output file: %s", file);
}

file_bwriter::~file_bwriter() {
    fclose(stream);
}

stream_breader::stream_breader(FILE* stream) {
    this->stream     = stream;
    this->bit_offset = 0;
    this->available  = 0;
}

stream_breader::~stream_breader() {
}

int stream_breader::read(uint64_t& bits, int width) {
    if ((bit_offset + width) > available) {
        size_t a = (available >> 3) - (bit_offset >> 3);
        memcpy(buffer, buffer + (bit_offset >> 3), a);
        available = a << 3;
        bit_offset &= 0x7;
        
        a = fread(buffer + a, sizeof(uint8_t), sizeof(buffer) - a, stream);
        if (ferror(stream))
            throw io_exception("error while reading from a file");
        
        available += a << 3;
    }
    
    if (bit_offset >= available)
        return 0;
    if ((bit_offset + width) > available)
        width = available - bit_offset;
    
    int rest = width;
    bits = 0;
    
    while (rest > 0) {
        int wa = 8 - (bit_offset & 0x7);
        int wr = wa > rest ? rest : wa;
        uint8_t m = (1 << wa) - 1;
        uint8_t b = buffer[bit_offset >> 3] & m;
        bits <<= wr;
        bits |=  b >> (wa - wr);
        
        rest -= wr;
        bit_offset += wr;
    }
    
    return width;
}

file_breader::file_breader(const char* file)
    : stream_breader(fopen(file, "rb")) {
    if (!stream)
        throw io_exception("unable to open input file: %s", file);
}

file_breader::~file_breader() {
    fclose(stream);
}

