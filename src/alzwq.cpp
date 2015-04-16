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

#include "utils.hpp"
#include "search-engine.hpp"

using namespace alzw;

/**
 * Match handler.
 *
 * @param seq    sequence no.
 * @param offset offset within the sequence
 * @param misc   nothing
 */
static void match_handler(size_t seq, size_t offset, void* misc) {
    fprintf(stderr, "match (seq: %lu, offset: %lu)\n", seq, offset);
}

/**
 * Process a given query.
 *
 * @param alg pattern-matching algorithm
 * @param q   query
 * @param se  search engine
 * @returns true to continue, false otherwise
 */
static bool process_query(int alg, const std::string& q, search_engine& se) {
    if (q.length() == 0)
        return false;
    
    se.search(alg, q, &match_handler, NULL);
    
    return true;
}

/**
 * Read a next query from the standard input and process it.
 *
 * @param alg pattern-matching algorithm
 * @param se  search engine
 * @returns true to continue, false otherwise
 */
static int process_query(int alg, search_engine& se) {
    std::stringstream query;
    char buffer[4096];
    bool nl = false;
    
    while (!nl && fgets(buffer, sizeof(buffer), stdin)) {
        for (size_t i = 0; i < sizeof(buffer); i++) {
            if (buffer[i] == '\n')
                nl = true;
            if (buffer[i] == '\n' || buffer[i] == 0)
                break;
            
            query << buffer[i];
        }
    }
    
    if (!process_query(alg, query.str(), se))
        return false;
    
    return !feof(stdin);
}

int main(int argc, const char** argv) {
    const char* usage = 
        "USAGE: alzwq [OPTIONS] [RSEQ] [ALZW]\n\n"
        "    RSEQ  reference sequence file in FASTA format\n"
        "    ALZW  ALZW compressed file\n\n"
        "OPTIONS\n\n"
        "    -a alg searching algorithm [lm], valid options are:\n"
        "               lm  Lahoda-Melichar\n"
        "               dfa deterministic finite automaton\n"
        "               bmh Boyer-Moore-Horspool\n"
        "               s   simple search (naive algorithm)\n"
        "    -h     print this help\n";
    
    int  i = 1;
    
    int  a = SE_ALG_LM;
    
    for (; i < argc; i++) {
        if (*argv[i] != '-')
            break;
        
        const char* option = argv[i] + 1;
        
        if (!strcmp("h", option)) {
            printf("%s\n", usage);
            return 0;
        } else if (!strcmp("a", option)) {
            option = argv[++i];
            if (!strcmp("lm", option))
                a = SE_ALG_LM;
            else if (!strcmp("dfa", option))
                a = SE_ALG_DFA;
            else if (!strcmp("bmh", option))
                a = SE_ALG_BMH;
            else if (!strcmp("s", option))
                a = SE_ALG_SIMPLE;
            else {
                fprintf(stderr, "unknown algorithm: %s\n\n", option);
                fprintf(stderr, "%s\n", usage);
                return 1;
            }
        } else {
            fprintf(stderr, "unrecognized option: -%s\n\n", option);
            fprintf(stderr, "%s\n", usage);
            return 1;
        }
    }
    
    argv += i;
    argc -= i;
    
    if (argc < 2) {
        fprintf(stderr, "a reference sequence and a set of compressed sequences are required\n\n");
        fprintf(stderr, "%s\n", usage);
        return 1;
    }
    
    fprintf(stderr, "loading index...\n");
    search_engine se(argv[0], argv[1]);
    
    fprintf(stderr, "enter query:\n");
    while (process_query(a, se))
        fprintf(stderr, "enter query:\n");
    
    return 0;
}

