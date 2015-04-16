CPP=g++
CFLAGS= -Wall -Wno-long-long -pedantic -O3 -g -std=c++11
CLIBS=  -lm \
        -lbam \
        -lpthread
INCLUDE=-I./include \
        -I/usr/include/samtools

BIN=bin
OBJ=obj
SRC=src
INC=include
ALZW_OUT_FILE=$(BIN)/alzw
ALZWQ_OUT_FILE=$(BIN)/alzwq
S2FASTA_OUT_FILE=$(BIN)/sam2fasta
S2SEQ_OUT_FILE=$(BIN)/sam2seq

ALZW_SRCS=$(SRC)/alzw.cpp \
          $(SRC)/bit-io.cpp \
          $(SRC)/dictionary.cpp \
          $(SRC)/encoder.cpp \
          $(SRC)/decoder.cpp \
          $(SRC)/fasta-alignment.cpp \
          $(SRC)/sam-alignment.cpp \
          $(SRC)/utils.cpp

ALZWQ_SRCS=$(SRC)/alzwq.cpp \
           $(SRC)/bit-io.cpp \
           $(SRC)/decoder.cpp \
           $(SRC)/dictionary.cpp \
           $(SRC)/fautomaton.cpp \
           $(SRC)/search-engine.cpp \
           $(SRC)/utils.cpp

S2FASTA_SRCS=$(SRC)/sam2fasta.cpp \
             $(SRC)/sam-alignment.cpp \
             $(SRC)/utils.cpp

S2SEQ_SRCS=$(SRC)/sam2seq.cpp \
           $(SRC)/sam-alignment.cpp \
           $(SRC)/utils.cpp
             

ALZW_OBJS=$(ALZW_SRCS:$(SRC)/%.cpp=$(OBJ)/%.o)
ALZWQ_OBJS=$(ALZWQ_SRCS:$(SRC)/%.cpp=$(OBJ)/%.o)
S2FASTA_OBJS=$(S2FASTA_SRCS:$(SRC)/%.cpp=$(OBJ)/%.o)
S2SEQ_OBJS=$(S2SEQ_SRCS:$(SRC)/%.cpp=$(OBJ)/%.o)

HPPS=$(wildcard $(INC)/*.hpp)

all: link

$(OBJ)/%.o: $(SRC)/%.cpp
	${CPP} ${CFLAGS} -o $@ -c $< ${INCLUDE}

link: ${ALZW_OBJS} ${ALZWD_OBJS} ${ALZWQ_OBJS} ${S2FASTA_OBJS} ${S2SEQ_OBJS}
	${CPP} ${CFLAGS} ${ALZW_OBJS} -o ${ALZW_OUT_FILE} ${CLIBS}
	${CPP} ${CFLAGS} ${ALZWQ_OBJS} -o ${ALZWQ_OUT_FILE} ${CLIBS}
	${CPP} ${CFLAGS} ${S2FASTA_OBJS} -o ${S2FASTA_OUT_FILE} ${CLIBS}
	${CPP} ${CFLAGS} ${S2SEQ_OBJS} -o ${S2SEQ_OUT_FILE} ${CLIBS}

${ALZW_OBJS}: ${HPPS}

${ALZWQ_OBJS}: ${HPPS}

${S2FASTA_OBJS}: ${HPPS}

${S2SEQ_OBJS}: ${HPPS}

doc: ${HPPS} ${SRCS}
	doxygen doxygen.conf

clean:
	-rm -f $(OBJ)/*.o $(ALZW_OUT_FILE) $(ALZWQ_OUT_FILE) $(S2FASTA_OUT_FILE) $(S2SEQ_OUT_FILE)
	-rm -f $(TEST_OBJ)/*.o $(TEST_OUT_FILE)

