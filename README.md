# Alignment-based LZW (ALZW)

The ALZW algorithm is designed for compression of DNA sequences. Compressed 
sequences must be aligned to a single reference sequence. Pairwise alignments 
are expected in 
[FASTA format](http://www.bioperl.org/wiki/FASTA_multiple_alignment_format).

## compilation

GCC C++ compiler with support of C++ 2011 is needed for compilation. The 
source files can be compiled using GNU make. All executables will be placed 
in the `bin` subdirectory. The following libraries are required for 
compilation:

 - libz (zlib >= 1.2.3)
 - libm
 - libpthread

