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

using namespace alzw;

/**
 * Save a given pairwise alignment in FASTA format.
 *
 * @param file output file
 * @param a    pairwise alignment
 */
static void save_alignment(const char* file, const alignment& a) {
    std::ofstream fout(file);
    // TODO: check for error
    
    const std::string& rseq = a[0];
    const std::string& aseq = a[1];
    size_t i;
    
    fout << ">reference sequence" << std::endl;
    for (i = 0; i < rseq.length(); i++) {
        fout << rseq[i];
        if ((i % 60) == 59)
            fout << std::endl;
    }
    
    if ((i % 60) != 0)
        fout << std::endl;
    
    fout << ">aligned sequence" << std::endl;
    for (i = 0; i < aseq.length(); i++) {
        fout << aseq[i];
        if ((i % 60) == 59)
            fout << std::endl;
    }
    
    fout.flush();
    fout.close();
}

/**
 * Convert given alignments in SAM format to FASTA alignments.
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
        snprintf(buffer, sizeof(buffer), "%s.afasta", seq_files[i]);
        sam_alignment sa = sam_alignment::load(rseq, seq_files[i]);
        save_alignment(buffer, sa);
    }
}

int main(int argc, const char **argv) {
    const char* usage = 
        "USAGE: sam2fasta [OPTIONS] REF FILE1 [FILE2 [...]]\n\n"
        "    REF   file containing a reference sequence in FASTA format\n"
        "    FILE# file containing a sequence in binary SAM format\n\n"
        "OPTIONS\n\n"
        "    -h    print this help\n";
    
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
    
    convert(argv[0], argv + 1, argc - 1);
    
    return 0;
}

