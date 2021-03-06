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
#include <cstring>
#include <cstdio>
#include <fstream>

#include "utils.hpp"
#include "sam-alignment.hpp"
#include "exception.hpp"

using namespace alzw;

/**
 * Save a given sequence in plain lower-case text.
 *
 * @param file output file
 * @param seq  sequence
 */
static void save_seq(const char* file, const std::string& seq) {
    char buffer[4096];
    size_t offset = 0;
    
    std::ofstream fout(file);
    if (!fout)
        throw io_exception("unable to open output file: %s", file);
    
    for (size_t i = 0; i < seq.length(); i++) {
        if (seq[i] != '-')
            buffer[offset++] = tolower(seq[i]);
        if ((offset + 1) == sizeof(buffer)) {
            buffer[offset] = 0;
            fout << buffer;
            if (fout.fail())
                throw io_exception("error while writing into a file");
            offset = 0;
        }
    }
    
    if (offset > 0) {
        buffer[offset] = 0;
        fout << buffer;
        if (fout.fail())
            throw io_exception("error while writing into a file");
    }
    
    fout.flush();
    fout.close();
}

/**
 * Convert given aligned sequences in SAM format into plain text.
 *
 * @param rseq_file path to a FASTA encoded file containing a reference 
 * sequence
 * @param seq_files files in binary SAM format
 * @param count     number of SAM encoded files
 */
void convert(const char* rseq_file, const char** seq_files, int count) {
    char buffer[4096];
    
    std::string rseq = utils::load_fasta(rseq_file);
    
    for (int i = 0; i < count; i++) {
        fprintf(stderr, "%s\n", seq_files[i]);
        snprintf(buffer, sizeof(buffer), "%s.seq", seq_files[i]);
        sam_alignment sa = sam_alignment::load(rseq, seq_files[i]);
        save_seq(buffer, sa[1]);
    }
}

int main(int argc, const char **argv) {
    const char* usage = 
        "USAGE: sam2seq [OPTIONS] RSEQ FILE1 [FILE2 [...]]\n\n"
        "    RSEQ  reference sequence file in FASTA format\n"
        "    FILE# sequence file in binary SAM format\n\n"
        "OPTIONS\n\n"
        "    -h    show help\n";
    
    int  i = 1;
    
    for (; i < argc; i++) {
        if (*argv[i] != '-')
            break;
        
        const char* option = argv[i] + 1;
        
        if (!strcmp("h", option)) {
            printf("%s\n", usage);
            return 0;
        } else {
            fprintf(stderr, "unrecognized option: -%s\n\n", option);
            fprintf(stderr, "%s\n", usage);
            return 1;
        }
    }
    
    argv += i;
    argc -= i;
    
    if (argc < 2) {
        fprintf(stderr, "a reference sequence and a set of sequences in SAM format are required\n\n");
        fprintf(stderr, "%s\n", usage);
        return 1;
    }
    
    try {
        convert(argv[0], argv + 1, argc - 1);
    } catch (std::exception& ex) {
        fprintf(stderr, "ERROR: %s\n", ex.what());
        return 2;
    }
    
    return 0;
}

