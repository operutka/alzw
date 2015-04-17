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
#include <ctime>
#include <unistd.h>

#include "utils.hpp"
#include "exception.hpp"

using namespace alzw;
using namespace utils;

std::string alzw::utils::load_fasta(const char* file) {
    FILE* fin = fopen(file, "rt");
    if (!fin)
        throw io_exception("unable to open FASTA file: %s", file);
    
    std::string seq = load_fasta(fin);
    
    fclose(fin);
    
    return seq;
}

std::string alzw::utils::load_fasta(FILE* file) {
    std::stringstream seqs;
    char line[4096];
    
    if (!fgets(line, sizeof(line), file))
        throw parse_exception("malformed FASTA format, unexpected EOF");
    if (line[0] != '>')
        throw parse_exception("malformed FASTA format, missing comment line");
    if (!strchr(line, '\n'))
        throw parse_exception("comment line is too long, maximum supported length is 4095 characters");
    
    while (fgets(line, sizeof(line), file)) {
        for (size_t i = 0; line[i] != 0; i++) {
            char c = toupper(line[i]);
            if (isspace(c))
                continue;
            
            switch (c) {
                case 'A':
                case 'C':
                case 'G':
                case 'T':
                case 'N': seqs << c; break;
                default:  throw parse_exception("unexpected DNA sequence character: %c", c);
            }
        }
    }
    
    if (ferror(file))
        throw io_exception("error while reading from a file");
    
    return seqs.str();
}

size_t alzw::utils::realloc(uint8_t** buffer, size_t size, size_t nsize) {
    if (nsize <= size)
        return size;
    
    delete [] *buffer;
    
    *buffer = new uint8_t[nsize];
    
    return nsize;
}

size_t alzw::utils::crealloc(uint8_t** buffer, size_t size, size_t nsize) {
    if (nsize <= size)
        return size;
    
    uint8_t* nbuffer = new uint8_t[nsize];
    memcpy(nbuffer, *buffer, size);
    
    delete [] *buffer;
    *buffer = nbuffer;
    
    return nsize;
}

uint8_t alzw::utils::char2base(char c) {
    switch (c) {
        case 'a':
        case 'A': return 0;
        case 'c':
        case 'C': return 1;
        case 'g':
        case 'G': return 2;
        case 't':
        case 'T': return 3;
        default: return 4;
    }
}

char alzw::utils::base2char(uint8_t base) {
    return ALPHABET[base];
}

int alzw::utils::number_width(uint64_t n) {
    int w = 0;
    
    while (n > 0) {
        n >>= 1;
        w++;
    }
    
    return w;
}

double alzw::utils::time() {
#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0)
	clockid_t id;
	struct timespec ts;
#if _POSIX_CPUTIME > 0
	if (clock_getcpuclockid(0, &id) == -1)
#endif
#if defined(CLOCK_PROCESS_CPUTIME_ID)
		id = CLOCK_PROCESS_CPUTIME_ID;
#elif defined(CLOCK_VIRTUAL)
		id = CLOCK_VIRTUAL;
#else
		id = (clockid_t)-1;
#endif
	if (id != (clockid_t)-1 && clock_gettime(id, &ts) != -1)
		return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
#endif

#if defined(CLOCKS_PER_SEC)
	clock_t cl = clock();
	if (cl != (clock_t)-1)
		return (double)cl / (double)CLOCKS_PER_SEC;
#endif
    
	return -1;
}

