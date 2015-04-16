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

#include <sstream>
#include <cstring>
#include <stdexcept>

#include "fasta-alignment.hpp"

using namespace alzw;

fasta_alignment fasta_alignment::load(FILE* file) {
    std::stringstream seq_stream;
    fasta_alignment result;
    char line[4096];
    
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '>') {
            if (!strchr(line, '\n'))
                throw std::runtime_error("comment line is too long");
            std::string seq = seq_stream.str();
            if (seq.length() > 0)
                result.seqs.push_back(seq);
            seq_stream.str("");
        } else {
            for (size_t i = 0; line[i] != 0; i++) {
                char c = toupper(line[i]);
                if (isspace(c))
                    continue;
                
                switch (c) {
                    case 'A':
                    case 'C':
                    case 'G':
                    case 'T':
                    case 'N':
                    case '-': seq_stream << c; break;
                    default:  throw std::runtime_error("invalid character");
                }
            }
        }
    }
    
    std::string seq = seq_stream.str();
    if (seq.length() > 0)
        result.seqs.push_back(seq);
    
    return result;
}

fasta_alignment fasta_alignment::load(const char* file) {
    FILE* fin = fopen(file, "rt");
    if (!fin) { // TODO: use IO exception
        std::stringstream msg;
        msg << "unable to open FASTA file: " << file;
        throw std::runtime_error(msg.str());
    }
    
    fasta_alignment result = load(fin);
    
    fclose(fin);
    
    return result;
}

