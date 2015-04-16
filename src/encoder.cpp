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

#include "encoder.hpp"

using namespace alzw;

encoder::encoder(int sync_period)
    : dict(false) {
    this->sync_period = sync_period;
    
    ndel = 0;
    nins = 0;
    nmm = 0;
    
    nmatches = 0;
    nmismatches = 0;
    ninserts = 0;
    ndeletes = 0;
    
    nmmseqs = 0;
    nmseqs = 0;
    niseqs = 0;
    ndseqs = 0;
    
    nmmouts = 0;
    niouts = 0;
    ndouts = 0;
    
    nmmbits = 0;
    nibits = 0;
    ndbits = 0;
    
    width = (int)ceil(log(dict.used_nodes()) / log(2));
    
    last_op = -1;
    
    fmismatch = false;
    fnew_node = false;
    fwidth_inc = false;
}

static void next_sync_point(size_t& current, 
    size_t& index, std::vector<uint32_t>* sync_map, size_t sync_period) {
    if (sync_map && sync_period) {
        size_t soffset = 0;
        while (soffset < sync_period && index < sync_map->size())
            soffset += (*sync_map)[index++];
        current += soffset;
    } else
        current += sync_period;
}

void encoder::encode(const std::string& rseq, const std::string& aseq, 
    bwriter& out, std::vector<uint32_t>* sync_map) {
    size_t alen = aseq.size();
    size_t roffset = 0;
    size_t next_sp = 0;
    size_t smi = 0;
    char c1, c2;
    
    next_sync_point(next_sp, smi, sync_map, sync_period);
    
    for (size_t i = 0; i < alen; i++) {
        c1 = rseq[i];
        c2 = aseq[i];
        
        if (c1 != '-') {
            if (next_sp > 0 && next_sp == roffset) {
                next_sync_point(next_sp, smi, sync_map, sync_period);
                sync(out);
            }
            roffset++;
        }
        
        if (c1 == '-')
            ins(c2, out);
        else if (c2 == '-')
            del(out);
        else if (c1 == c2)
            match(c2, out);
        else
            mismatch(c2, out);
    }
    
    flush(out);
}

void encoder::match(char c, bwriter& out) {
    const node* wnode = dict.get_wnode();
    size_t id, next;
    bool can_follow;
    
    flush_ins(out);
    flush_del(out);
    
    if (last_op != OP_MATCH)
        nmseqs++;
    if (last_op != OP_MATCH && last_op != OP_MISMATCH)
        nmmseqs++;
    last_op = OP_MATCH;
    
    if (!fmismatch) {
        id = dict.get_id();
        can_follow = dict.can_follow(c);
        next = dict.next_id();
        if ((next & (next - 1)) != 0) {
            dict.add(c);
            fnew_node = !can_follow;
        } else if (can_follow) {
            dict.follow(c);
        } else if (fwidth_inc) {
            dict.add(c);
            fnew_node = true;
            fwidth_inc = false;
        } else {
            out_mm(id, out);
            dict.new_phrase();
            
            out_mm(wnode->id(), out);
            dict.follow(c);
            
            width++;
            nmm = 0;
            fnew_node = false;
            fmismatch = false;
            fwidth_inc = true;
        }
    } else if (!dict.follow(c)) {
        out_mm(dict.get_id(), out);
        dict.new_phrase();
        dict.follow(c);
        nmm = 0;
        fnew_node = false;
        fmismatch = false;
    }
    
    nmm++;
    nmatches++;
}

void encoder::mismatch(char c, bwriter& out) {
    flush_ins(out);
    flush_del(out);
    
    if (last_op != OP_MATCH && last_op != OP_MISMATCH)
        nmmseqs++;
    last_op = OP_MISMATCH;
    
    fmismatch = true;
    
    if (fnew_node || !dict.follow(c)) {
        out_mm(dict.get_id(), out);
        dict.new_phrase();
        dict.follow(c);
        nmm = 0;
        fnew_node = false;
    }
    
    nmm++;
    nmismatches++;
}

void encoder::ins(char c, bwriter& out) {
    flush_mm(out);
    flush_del(out);
    
    if (last_op != OP_INS)
        niseqs++;
    last_op = OP_INS;
    
    if (dict.follow(c))
        nins++;
    else {
        out_ins(dict.get_id(), out);
        dict.new_phrase();
        dict.follow(c);
        nins = 1;
    }
    
    ninserts++;
}

void encoder::del(bwriter& out) {
    flush_mm(out);
    flush_ins(out);
    
    if (last_op != OP_DEL)
        ndseqs++;
    last_op = OP_DEL;
    
    ndel++;
    ndeletes++;
}

void encoder::out_mm(size_t id, bwriter& out) {
    out.write(id, width);
    nmmbits += width;
    nmmouts++;
}

void encoder::out_ins(size_t id, bwriter& out) {
    ins_queue.push_back(id);
    niouts++;
}

void encoder::out_del(size_t size, bwriter& out) {
    const node* dnode = dict.get_dnode();
    out.write(dnode->id(), width);
    ndbits += out.write_delta(size);
    ndbits += width;
    ndouts++;
}

void encoder::flush_mm(bwriter& out) {
    fmismatch = false;
    fnew_node = false;
    
    if (nmm == 0)
        return;
    
    out_mm(dict.get_id(), out);
    dict.new_phrase();
    
    nmm = 0;
}

void encoder::flush_ins(bwriter& out) {
    if (nins > 0) {
        out_ins(dict.get_id(), out);
        dict.new_phrase();
        nins = 0;
    }
    
    if (ins_queue.empty())
        return;
    
    const node* inode = dict.get_inode();
    out.write(inode->id(), width);
    nibits += width;
    
    nibits += out.write_delta(ins_queue.size());
    
    while (!ins_queue.empty()) {
        out.write(ins_queue.front(), width);
        ins_queue.pop_front();
        nibits += width;
    }
}

void encoder::flush_del(bwriter& out) {
    if (ndel == 0)
        return;
    
    out_del(ndel, out);
    
    ndel = 0;
}

void encoder::flush(bwriter& out) {
    flush_mm(out);
    flush_ins(out);
    flush_del(out);
}

void encoder::sync(bwriter& out) {
    flush_mm(out);
    flush_ins(out);
    flush_del(out);
}

