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

#include <string>
#include <vector>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <stdint.h>
#include <fstream>
#include <iostream>

#include "fasta-alignment.hpp"
#include "encoder.hpp"
#include "decoder.hpp"
#include "utils.hpp"
#include "exception.hpp"

using namespace alzw;

/**
 * Print encoding stats.
 *
 * @param enc          encoder
 * @param totalASeqLen sum of lengths of all encoded sequences
 */
static void print_stats(const encoder& enc, size_t totalASeqLen) {
    size_t kb_used = enc.used_memory() / 1000;
    size_t bit_size = enc.mmbits() + enc.ibits() + enc.dbits();
    size_t mms = enc.matches() + enc.mismatches();
    size_t ms = enc.matches();
    size_t ds = enc.deletes();
    size_t is = enc.inserts();
    double avg_mmbpb = mms > 0 ? (double)enc.mmbits() / mms : 0;
    double avg_ibpb = is > 0 ? (double)enc.ibits() / is : 0;
    double avg_dbpb = ds > 0 ? (double)enc.dbits() / ds : 0;
    double avg_mmseq_len = enc.mmseqs() > 0 ? (double)mms / enc.mmseqs() : 0;
    double avg_mseq_len = enc.mseqs() > 0 ? (double)ms / enc.mseqs() : 0;
    double avg_iseq_len = enc.iseqs() > 0 ? (double)is / enc.iseqs() : 0;
    double avg_dseq_len = enc.dseqs() > 0 ? (double)ds / enc.dseqs() : 0;
    double avg_mmout = enc.mmouts() > 0 ? (double)mms / enc.mmouts() : 0;
    double avg_iout = enc.iouts() > 0 ? (double)is / enc.iouts() : 0;
    double avg_dout = enc.douts() > 0 ? (double)ds / enc.douts() : 0;
    
    fprintf(stderr, "Used memory:     %9lu kB\n", (unsigned long)kb_used);
    fprintf(stderr, "Used nodes:      %9lu\n", (unsigned long)enc.used_nodes());
    fprintf(stderr, "Nodes in memory: %9lu\n\n", (unsigned long)enc.real_nodes());
    
    fprintf(stderr, "Length of compressed sequences: %lu\n\n", (unsigned long)totalASeqLen);
    
    fprintf(stderr, "Compressed size: %lu B\n", (unsigned long)(bit_size + 7) >> 3);
    fprintf(stderr, "    ratio:   %9f %%\n", 100.0 * bit_size / (totalASeqLen * 2));
    fprintf(stderr, "    bpb:     %9f\n", (double)bit_size / totalASeqLen);
    fprintf(stderr, "    M/Rs:    %9lu B (%7.4f bpb)\n", (unsigned long)enc.mmbits() / 8, avg_mmbpb);
    fprintf(stderr, "    inserts: %9lu B (%7.4f bpb)\n", (unsigned long)enc.ibits() / 8, avg_ibpb);
    fprintf(stderr, "    deletes: %9lu B (%7.4f bpb)\n", (unsigned long)enc.dbits() / 8, avg_dbpb);
    
    fprintf(stderr, "\nOther stats:\n\n");
    
    fprintf(stderr, "    Matches:  %9lu\n", (unsigned long)ms);
    fprintf(stderr, "    Replaces: %9lu\n", (unsigned long)enc.mismatches());
    fprintf(stderr, "    Inserts:  %9lu\n", (unsigned long)is);
    fprintf(stderr, "    Deletes:  %9lu\n\n", (unsigned long)ds);
    
    fprintf(stderr, "    M/R seqs: %9lu (avg len: %9.3f)\n", (unsigned long)enc.mmseqs(), avg_mmseq_len);
    fprintf(stderr, "    M seqs:   %9lu (avg len: %9.3f)\n", (unsigned long)enc.mseqs(), avg_mseq_len);
    fprintf(stderr, "    I seqs:   %9lu (avg len: %9.3f)\n", (unsigned long)enc.iseqs(), avg_iseq_len);
    fprintf(stderr, "    D seqs:   %9lu (avg len: %9.3f)\n\n", (unsigned long)enc.dseqs(), avg_dseq_len);
    
    fprintf(stderr, "    M/R outs: %9lu (avg len: %9.3f)\n", (unsigned long)enc.mmouts(), avg_mmout);
    fprintf(stderr, "    I outs:   %9lu (avg len: %9.3f)\n", (unsigned long)enc.iouts(), avg_iout);
    fprintf(stderr, "    D outs:   %9lu (avg len: %9.3f)\n\n", (unsigned long)enc.douts(), avg_dout);
}

/**
 * Get length of a given sequence.
 *
 * @param seq sequence
 * @returns sequence length
 */
static size_t get_seq_len(const std::string& seq) {
    size_t len = 0;
    
    for (size_t i = 0; i < seq.length(); i++) {
        if (seq[i] != '-')
            len++;
    }
    
    return len;
}

/**
 * Encode a given pairwise alignment.
 *
 * @param enc      encoder
 * @param bw       output
 * @param a        alignment
 * @param sync_map synchronization map for adaptive synchronization (may be 
 * NULL)
 */
static size_t compress(encoder& enc, bwriter& bw, const alignment& a,
    std::vector<uint32_t>* sync_map) {
    const std::string& seq1 = a[0];
    const std::string& seq2 = a[1];
    
    enc.encode(seq1, seq2, bw, sync_map);
    
    return get_seq_len(seq2);
}

/**
 * Save a given sequence.
 *
 * @param file output file path
 * @param seq  sequence
 */
/*static void save_seq(const char* file, const std::string& seq) {
    char buffer[4096];
    size_t boffset = 0;
    size_t offset = 0;
    
    std::ofstream fout(file);
    if (!fout)
        throw io_exception("unable to open output file: %s", file);
    
    for (size_t i = 0; i < seq.length(); i++) {
        if ((boffset + 3) >= sizeof(buffer)) {
            buffer[boffset] = 0;
            fout << buffer;
            if (fout.fail())
                throw io_exception("error while writing into a file");
            boffset = 0;
        }
        
        if (seq[i] == '-')
            continue;
        
        buffer[boffset++] = seq[i];
        if ((++offset % 60) == 0)
            buffer[boffset++] = '\n';
    }
    
    if (boffset > 0) {
        buffer[boffset] = 0;
        fout << buffer;
        if (fout.fail())
            throw io_exception("error while writing into a file");
    }
    
    fout.flush();
    fout.close();
}*/

/**
 * Create cumulative change vector for given pairwise alignments.
 *
 * @param seq_files pairwise alignments in FASTA format
 * @param count     number of pairwise alignments
 * @returns change vector
 */
static std::vector<bool> create_change_vector(
    const char** seq_files, int count) {
    std::vector<bool> changes;
    
    for (int i = 0; i < count; i++) {
        fasta_alignment a = fasta_alignment::load(seq_files[i]);
        if (!changes.size())
            changes = std::vector<bool>(get_seq_len(a[0]) + 1, false);
        const std::string& seq1 = a[0];
        const std::string& seq2 = a[1];
        size_t roffset = 0;
        char c1, c2;
        
        // find all changes
        for (size_t j = 0; j < seq1.length(); j++) {
            c1 = seq1[j];
            c2 = seq2[j];
            if (c1 == '-' && roffset > 0)
                changes[roffset - 1] = true;
            else if (c1 != c2)
                changes[roffset] = true;
            if (c1 != '-')
                roffset++;
        }
    }
    
    return changes;
}

/**
 * Create synchronization map for given pairwise alignments.
 *
 * @param seq_files pairwise alignments in FASTA format
 * @param count     number of pairwise alignments
 * @param sync_map  output
 */
static void create_sync_map(const char** seq_files, int count, 
    std::vector<uint32_t>& sync_map) {
    std::vector<bool> changes = create_change_vector(seq_files, count);
    bool sync_needed = false;
    uint32_t period = 0;
    
    for (size_t i = 0; i < changes.size(); i++) {
        if (changes[i])
            sync_needed = true;
        else if (sync_needed) {
            sync_map.push_back(period);
            sync_needed = false;
            period = 0;
        }
        
        period++;
    }
}

/**
 * Encode given pairwise alignments.
 *
 * @param sync_period synchronization period (or minimum phrase length in case 
 * of adaptive synchronizatioin)
 * @param async       use adaptive synchronization
 * @param seq_files   pairwise alignments in FASTA format
 * @param seq_count   number of pairwise alignments
 */
static void compress(int sync_period, bool async, 
    const char** seq_files, size_t seq_count) {
    stream_bwriter bw(stdout);
    encoder enc(sync_period);
    size_t total_aseq_len = 0;
    //char buffer[4096];
    
    std::vector<uint32_t> sync_map;
    std::vector<uint32_t>* smap_p;
    if (async) {
        create_sync_map(seq_files, seq_count, sync_map);
        smap_p = &sync_map;
    } else
        smap_p = NULL;
    
    bw.write(seq_count, sizeof(int) << 3);
    for (size_t i = 0; i < seq_count; i++)
        bw.write_str(seq_files[i]);
    
    for (size_t i = 0; i < seq_count; i++) {
        fprintf(stderr, "%s\n", seq_files[i]);
        //snprintf(buffer, sizeof(buffer), "%s.fa.orig", seq_files[i]);
        fasta_alignment fa = fasta_alignment::load(seq_files[i]);
        //save_seq(buffer, fa[1]);
        total_aseq_len += compress(enc, bw, fa, smap_p);
    }
    
    print_stats(enc, total_aseq_len);
}

/**
 * Decode a single sequence from a given ALZW stream.
 *
 * @param br       input
 * @param dec      decoder
 * @param seq_name name of the sequence
 * @param out_file path to an output file
 */
static void decompress(breader& br, decoder& dec, 
    const std::string& seq_name, const char* out_file) {
    fprintf(stderr, "%s\n", out_file);
    
    std::ofstream fout(out_file);
    if (!fout)
        throw io_exception("unable to open output file: %s", out_file);
    
    fout << ">" << seq_name << std::endl;
    
    dec.decode(br, fout);
    
    fout.flush();
    fout.close();
}

/**
 * Decode a given ALZW stream.
 *
 * @param rseq_file reference sequence in FASTA format
 * @param alzw_file ALZW file
 */
static void decompress(const char* rseq_file, const char* alzw_file) {
    std::string rseq = utils::load_fasta(rseq_file);
    std::vector<std::string> fnames;
    decoder dec(rseq, false);
    breader* br;
    
    char buffer[4096];
    
    if (strcmp("-", alzw_file))
        br = new file_breader(alzw_file);
    else
        br = new stream_breader(stdin);
    
    int seqc = br->read_int();
    for (int i = 0; i < seqc; i++) {
        if (br->read_str(buffer, sizeof(buffer)) < 0)
            throw runtime_exception("ALZW sequence file name is too long, maximum supported length is 4095 characters");
        fnames.push_back(buffer);
    }
    
    if (seqc < 0)
        throw runtime_exception("negative number of ALZW sequences");
    else if (seqc > 0) {
        for (int i = 0; i < seqc; i++) {
            snprintf(buffer, sizeof(buffer), "%s.fa", fnames[i].c_str());
            decompress(*br, dec, fnames[i], buffer);
        }
    } else
        dec.decode(*br, std::cout);
    
    delete br;
}

int main(int argc, const char **argv) {
    const char* usage = 
        "USAGE: alzw [OPTIONS] [RSEQ] [ALZW] [A1 [A2 [...]]]\n\n"
        "    RSEQ  reference sequence file in FASTA format (used only in case of\n"
        "          decompression)\n"
        "    ALZW  ALZW compressed file (used only in case of decompression)\n"
        "    A#    sequence alignment in FASTA format (used only in case of\n"
        "          compression)\n\n"
        "OPTIONS\n\n"
        "    -d     decompression\n"
        "    -s num synchronization period, only valid for compression [200]\n"
        "    -a     adaptive synchronization\n"
        "    -h     print this help\n";
    
    int  i = 1;
    
    bool d = false;
    int  s = 200;
    bool a = false;
    
    for (; i < argc; i++) {
        if (*argv[i] != '-')
            break;
        
        const char* option = argv[i] + 1;
        
        if (!strcmp("", option))
            break;
        else if (!strcmp("h", option)) {
            printf("%s\n", usage);
            return 0;
        } else if (!strcmp("d", option)) {
            d = true;
        } else if (!strcmp("s", option)) {
            s = atoi(argv[++i]);
        } else if (!strcmp("a", option)) {
            a = true;
        } else {
            fprintf(stderr, "unrecognized option: -%s\n\n", option);
            fprintf(stderr, "%s\n", usage);
            return 1;
        }
    }
    
    argv += i;
    argc -= i;
    
    if (d && argc < 2) {
        fprintf(stderr, "a reference sequence and a set of compressed sequences are required\n"
                        "    for decompression\n\n");
        fprintf(stderr, "%s\n", usage);
        return 1;
    } else if (!d && argc < 1) {
        fprintf(stderr, "at least a single FASTA alignment is required for compression\n\n");
        fprintf(stderr, "%s\n", usage);
        return 1;
    }
    
    if (s < 0)
        s = 0;
    
    double t = utils::time();
    
    try {
        if (d)
            decompress(argv[0], argv[1]);
        else
            compress(s, a, argv, argc);
    } catch (std::exception& ex) {
        fprintf(stderr, "ERROR: %s\n", ex.what());
        return 2;
    }
    
    t = utils::time() - t;
    fprintf(stderr, "elapsed time [s]: %f\n", t);
    
    return 0;
}

