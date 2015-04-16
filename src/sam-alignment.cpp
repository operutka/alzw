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

#include <unordered_map>
#include <deque>
#include <algorithm>
#include <exception>
#include <stdexcept>
#include <sstream>
#include <stdint.h>

#include <sam.h>

#include "sam-alignment.hpp"
#include "utils.hpp"

using namespace alzw;

/**
 * Alignment symbol tuple.
 */
struct alignment_symbol {
    char s;     // symbol
    size_t pos; // position
    int mapq;   // map quality
    
    alignment_symbol() { }
    alignment_symbol(char s, size_t pos, int mapq);
};

/**
 * Insertion.
 */
struct insertion {
    std::string seq; // inserted sequence
    size_t pos;      // position
    uint8_t mapq;    // map quality
    
    insertion() { }
    insertion(const std::string& seq, size_t pos, uint8_t mapq);
};

inline insertion::insertion(const std::string& s, size_t pos, uint8_t mapq)
    : seq(s) {
    this->pos = pos;
    this->mapq = mapq;
}

inline alignment_symbol::alignment_symbol(char s, size_t pos, int mapq) {
    this->s = s;
    this->pos = pos;
    this->mapq = mapq;
}

//                    0123456789012345
#define BAM_BASE_STR " ACMGRSVTWYHKDBN"
#define bam_base_chr(b) (BAM_BASE_STR[b])

/**
 * Parse a given CIGAR sequence into a FASTA-like string.
 *
 * @param cigar  CIGAR sequence
 * @param csize  CIGAR size
 * @param seq    processed subsequence
 * @param buffer output buffer
 * @param bsize  buffer size
 * @returns number of characters put into the buffer
 */
static size_t cigar2str(uint32_t* cigar, size_t csize, uint8_t* seq, 
    char* buffer, size_t bsize) {
    size_t bpos = 0;
    size_t spos = 0;
    size_t len;
    uint32_t op;
    
    for (size_t i = 0; i < csize; i++) {
        len = bam_cigar_oplen(cigar[i]);
        op = bam_cigar_op(cigar[i]);
        switch (op) {
            case 0:                     // match/mismatch
                if ((bpos + len) >= bsize)
                    throw std::overflow_error("insufficient CIGAR buffer size");
                for (size_t j = 0; j < len; j++)
                    buffer[bpos + j] = bam_base_chr(bam1_seqi(seq, spos + j));
                bpos += len;
                spos += len;
                break;
            case 1: spos += len; break; // insertion skip
            case 2:                     // deletion
                if ((bpos + len) >= bsize)
                    throw std::overflow_error("insufficient CIGAR buffer size");
                memset(buffer + bpos, '-', len);
                bpos += len;
                break;
            case 3:                     // skip
                if ((bpos + len) >= bsize)
                    throw std::overflow_error("insufficient CIGAR buffer size");
                memset(buffer + bpos, ' ', len);
                bpos += len;
                break;
            case 4: spos += len; break; // soft-clipping
            default:
                std::stringstream msg;
                msg << "unsupported CIGAR operation: " << op;
                throw std::runtime_error(msg.str());
        }
    }
    
    buffer[bpos] = 0;
    
    return bpos;
}

/**
 * Add insertions into a given insertion map.
 *
 * @param pos   position of the currently processed subsequence
 * @param mapq  map quality of the currently processed subsequence
 * @param cigar CIGAR sequence
 * @param csize CIGAR length
 * @param seq   currently processed subsequence
 * @param imap  insertion map
 */
void add_insertions(size_t pos, int mapq, uint32_t* cigar, size_t csize, 
    uint8_t* seq, std::unordered_map<size_t, insertion>& imap) {
    char buffer[256];
    size_t spos = 0;
    size_t len;
    uint32_t op;
    
    for (size_t i = 0; i < csize; i++) {
        len = bam_cigar_oplen(cigar[i]);
        op = bam_cigar_op(cigar[i]);
        switch (op) {
            case 0:
                pos  += len;
                spos += len;
                break;
            case 1:
                if (len >= sizeof(buffer))
                    throw std::overflow_error("insufficient insertion buffer size");
                for (size_t j = 0; j < len; j++)
                    buffer[j] = bam_base_chr(bam1_seqi(seq, spos + j));
                buffer[len] = 0;
                if (!imap.count(pos) || mapq > imap[pos].mapq)
                    imap[pos] = insertion(buffer, pos, mapq);
                spos += len;
                break;
            case 2: pos  += len; break;
            case 3: pos  += len; break;
            case 4: spos += len; break;
            default:
                std::stringstream msg;
                msg << "unsupported CIGAR operation: " << op;
                throw std::runtime_error(msg.str());
        }
    }
}

/**
 * Move all elements from a given source queue into a given destination queue.
 *
 * @param dst destination queue
 * @param src source queue
 */
template<class T>
static void move_all(std::deque<T>* dst, std::deque<T>* src) {
    while (!src->empty()) {
        dst->push_back(src->front());
        src->pop_front();
    }
}

/**
 * Place alignment symbols from a given queue into a given sequence.
 *
 * @param sq     symbol queue
 * @param seq    output sequence
 * @param maxpos stop processing the queue when maxpos is reached
 */
static void place_symbols(std::deque<alignment_symbol>* sq, 
    std::string& seq, size_t maxpos) {
    alignment_symbol sym;
    
    while (!sq->empty() && sq->front().pos < maxpos) {
        sym = sq->front();
        sq->pop_front();
        seq[sym.pos] = sym.s;
    }
}

/**
 * Process a given aligned subsequence.
 *
 * @param sq     symbol queue
 * @param old_sq temporary symbol queue
 * @param imap   insertion map
 * @param aseq   aligned sequence (output)
 * @param b      aligned subsequence
 */
static void process_alignment(std::deque<alignment_symbol>* sq, 
    std::deque<alignment_symbol>* old_sq, 
    std::unordered_map<size_t, insertion>& imap,
    std::string& aseq, bam1_t* b) {
    size_t    pos   = b->core.pos;
    int       mapq  = b->core.qual;
    size_t    clen  = b->core.n_cigar;
    uint32_t* cigar = bam1_cigar(b);
    uint8_t*  seq   = bam1_seq(b);
    
    alignment_symbol sym;
    char cstr[256];
    
    add_insertions(pos, mapq, cigar, clen, seq, imap);
    
    place_symbols(old_sq, aseq, pos);
    
    clen = cigar2str(cigar, clen, seq, cstr, sizeof(cstr));
    for (size_t i = 0; i < clen; i++) {
        if (!old_sq->empty() && old_sq->front().pos == pos) {
            sym = old_sq->front();
            old_sq->pop_front();
            if (cstr[i] == ' ' || mapq == 255 || mapq <= sym.mapq)
                sq->push_back(sym);
            else
                sq->push_back(alignment_symbol(cstr[i], pos, mapq));
        } else if (cstr[i] != ' ')
            sq->push_back(alignment_symbol(cstr[i], pos, mapq));
        
        pos++;
    }
    
    move_all(sq, old_sq);
}

/**
 * Read a given SAM file.
 *
 * @param samfile SAM file context
 * @param rseq    reference sequence
 * @param imap    insertion map
 * @returns aligned sequence (without insertions)
 */ 
static std::string load_sam(samfile_t* samfile, const std::string& rseq, 
    std::unordered_map<size_t, insertion>& imap) {
    std::string aseq(rseq.size(), 'N');
    
    bam1_t* b = bam_init1();
    
    std::deque<alignment_symbol> sq1;
    std::deque<alignment_symbol> sq2;
    std::deque<alignment_symbol>* psq1 = &sq1;
    std::deque<alignment_symbol>* psq2 = &sq2;
    
    while (samread(samfile, b) > 0) {
        if (b->core.flag & 0xb04)
            continue;
        
        process_alignment(psq1, psq2, imap, aseq, b);
        swap(psq1, psq2);
        psq1->clear();
    }
    
    place_symbols(psq2, aseq, aseq.size());
    
    bam_destroy1(b);
    
    return aseq;
}

/**
 * Read a given binary SAM file.
 *
 * @param samfile path to a binary SAM file
 * @param rseq    reference sequence
 * @param inserts insertion map
 * @returns aligned sequence (without insertions)
 */
static std::string load_sam(const char* samfile, const std::string& rseq, 
    std::unordered_map<size_t, insertion>& inserts) {
    samfile_t *sf = samopen(samfile, "rb", 0);
    if (!sf) {
        std::stringstream msg;
        msg << "unable to open SAM/BAM file: " << samfile;
        throw std::runtime_error(msg.str());
    }
    
    std::string seq = load_sam(sf, rseq, inserts);
    
    samclose(sf);
    
    return seq;
}

/**
 * Generate sequence with insertion placeholders.
 *
 * @param seq     original sequence
 * @param inserts sorted insertions
 * @param count   number of insertions
 * @returns padded sequence
 */
static std::string add_padding(const std::string& seq, 
    const insertion* const* inserts, size_t count) {
    std::stringstream seqs;
    size_t prev = 0;
    size_t size, pos;
    
    for (size_t i = 0; i < count; i++) {
        pos = inserts[i]->pos;
        size = inserts[i]->seq.size();
        for (size_t j = prev; j < pos; j++)
            seqs << seq[j];
        for (size_t j = 0; j < size; j++)
            seqs << '-';
        prev = pos;
    }
    
    pos = seq.size();
    for (size_t i = prev; i < pos; i++)
        seqs << seq[i];
    
    return seqs.str();
}

/**
 * Generate sequence with insertions.
 *
 * @param seq     original sequence
 * @param inserts sorted insertions
 * @param count   number of insertions
 * @returns sequence with insertions
 */
static std::string add_insertions(const std::string& seq, 
    const insertion* const* inserts, size_t count) {
    std::stringstream seqs;
    size_t prev = 0;
    size_t pos;
    
    for (size_t i = 0; i < count; i++) {
        pos = inserts[i]->pos;
        for (size_t j = prev; j < pos; j++)
            seqs << seq[j];
        seqs << inserts[i]->seq;
        prev = pos;
    }
    
    pos = seq.size();
    for (size_t i = prev; i < pos; i++)
        seqs << seq[i];
    
    return seqs.str();
}

/**
 * Compare two insertions by their positions.
 *
 * @param i1 insertion
 * @param i2 insertion
 * @returns true if i1->pos < i2->pos, false otherwise
 */
static bool cmp_insertionp(const insertion* i1, const insertion* i2) {
    return i1->pos < i2->pos;
}

/**
 * Generate sorted insertions from a given insertion map.
 *
 * @param inserts insertion map
 * @returns sorted insertions
 */
static insertion** sorted_inserts(
    std::unordered_map<size_t, insertion>& inserts) {
    size_t count = inserts.size();
    insertion** result = new insertion*[count];
    
    std::unordered_map<size_t, insertion>::iterator it = inserts.begin();
    for (size_t i = 0; it != inserts.end(); ++it, ++i)
        result[i] = &it->second;
    
    std::sort(result, result + count, cmp_insertionp);
    
    return result;
}

sam_alignment sam_alignment::load(const std::string& ref, const char* samfile) {
    std::unordered_map<size_t, insertion> inserts;
    std::string rseq = ref;
    sam_alignment result;
    
    std::string seq = load_sam(samfile, rseq, inserts);
    insertion** sinserts = sorted_inserts(inserts);
    
    result.seqs.push_back(add_padding(rseq, sinserts, inserts.size()));
    result.seqs.push_back(add_insertions(seq, sinserts, inserts.size()));
    
    delete [] sinserts;
    
    return result;
}

sam_alignment sam_alignment::load(const char* fastafile, const char* samfile) {
    std::string rseq = utils::load_fasta(fastafile);
    return load(rseq, samfile);
}

