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

#include <cmath>
#include <cstring>

#include "decoder.hpp"
#include "utils.hpp"

using namespace alzw;

// TODO: security checks for corrupted files

decoder::decoder(const std::string& rs, bool hash_index)
    : rseq(rs) {
    this->hash_index = hash_index;
    
    width = (int)ceil(log(dict.used_nodes()) / log(2));
    
    rbufferSize = 1024;
    rbuffer = new char[rbufferSize];
    
    offset = 0;
    
    ob_offset = 0;
}

decoder::~decoder() {
    delete [] rbuffer;
}

void decoder::output_char(char c, std::ostream* out) {
    if ((ob_offset + 1) == sizeof(obuffer))
        flush_output_buffer(out);
    
    obuffer[ob_offset++] = c;
}

void decoder::flush_output_buffer(std::ostream* out) {
    obuffer[ob_offset] = 0;
    *out << obuffer;
    ob_offset = 0;
}

size_t decoder::output_node(const node* n, uint32_t noffset, 
    size_t roffset, std::ostream* out) {
    size_t i = 0;
    
    if (hash_index)
        phrases[n->id() + noffset] = NULL;
    
    if (!out)
        return n->phrase_length() + noffset - n->length();
    
    while (n->parent()) {
        if (i >= rbufferSize) {
            rbufferSize = utils::crealloc((uint8_t**)&rbuffer, 
                rbufferSize, rbufferSize << 1);
        }
        
        if (noffset > 0)
            rbuffer[i++] = utils::base2char(n->get_base(--noffset));
        else {
            rbuffer[i++] = utils::base2char(n->symbol());
            n = n->parent();
            noffset = n->length();
        }
    }
    
    for (size_t j = 1; j <= i; j++) {
        output_char(rbuffer[i - j], out);
        if ((++offset % 60) == 0)
            output_char('\n', out);
    }
    
    return i;
}

size_t decoder::output_match(uint64_t id, size_t roffset, std::ostream* out) {
    size_t i = roffset;
    char c;
    
    dict.new_phrase();
    
    while (id > dict.get_id()) {
        c = rseq[i++];
        dict.add(c);
        if (out) {
            output_char(c, out);
            if ((++offset % 60) == 0)
                output_char('\n', out);
        }
    }
    
    dict.commit_phrase();
    
    if (hash_index)
        phrases[id] = NULL;
    
    return i - roffset;
}

size_t decoder::decode_mr(uint64_t cw, 
    size_t roffset, breader& in, std::ostream* out) {
    const node* n = dict.get(cw);
    if (n)
        return output_node(n, cw - n->id(), roffset, out);
    
    return output_match(cw, roffset, out);
}

void decoder::decode_ins(size_t roffset, breader& in, std::ostream* out) {
    size_t count = in.read_delta();
    const node* n;
    size_t cw;
    
    for (size_t i = 0; i < count; i++) {
        in.read(cw, width);
        n = dict.get(cw);
        output_node(n, cw - n->id(), roffset, out);
    }
}

void decoder::decode(breader& in, std::ostream* out) {
    const node* inode = dict.get_inode();
    const node* dnode = dict.get_dnode();
    const node* wnode = dict.get_wnode();
    uint64_t cw = 0;
    
    size_t roffset = 0;
    offset = 0;
    
    while (roffset < rseq.size()) {
        // drop the last read if it's too short
        if (width > in.read(cw, width))
            break;
        
        if (cw == inode->id())
            decode_ins(roffset, in, out);
        else if (cw == dnode->id())
            roffset += in.read_delta();
        else if (cw == wnode->id())
            width++;
        else
            roffset += decode_mr(cw, roffset, in, out);
    }
    
    if (out)
        flush_output_buffer(out);
}

void decoder::decode(breader& in) {
    decode(in, NULL);
}

void decoder::decode(breader& in, std::ostream& out) {
    decode(in, &out);
}

void decoder::freeze() {
    if (!hash_index)
        return;
    
    std::unordered_map<uint64_t, const node*>::iterator it = phrases.begin();
    for (; it != phrases.end(); ++it)
        it->second = dict.get(it->first);
}

