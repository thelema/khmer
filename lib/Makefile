NO_UNIQUE_RC=0

CXXFLAGS+= -DNO_UNIQUE_RC=$(NO_UNIQUE_RC)

ZLIB_DIR=zlib
ZLIB_OBJS_BASE=\
	adler32.o compress.o crc32.o gzio.o uncompr.o deflate.o trees.o \
	zutil.o inflate.o infback.o inftrees.o inffast.o
ZLIB_OBJS=$(addprefix $(ZLIB_DIR)/, $(ZLIB_OBJS_BASE))

BZIP2_DIR=bzip2
BZIP2_OBJS_BASE= \
	blocksort.o huffman.o crctable.o randtable.o compress.o \
	decompress.o bzlib.o
BZIP2_OBJS=$(addprefix $(BZIP2_DIR)/, $(BZIP2_OBJS_BASE))

DRV_PROGS=bittest ktable_test test-StreamReader test-CacheManager test-Parser test-HashTables smpFiltering 
DRV_PROGS+=#graphtest #consume_prof
AUX_PROGS=ht-diff

CORE_OBJS= khmer_config.o trace_logger.o ktable.o
PARSERS_OBJS=parsers.o threadedParsers.o read_parsers.o

all: $(ZLIB_OBJS) $(BZIP2_OBJS) $(CORE_OBJS) $(PARSERS_OBJS) hashtable.o hashbits.o subset.o counting.o test

clean:
	(cd $(ZLIB_DIR) && make clean)
	(cd $(BZIP2_DIR) && make -f Makefile-libbz2_so clean)
	rm -f *.o $(DRV_PROGS) $(AUX_PROGS)

test: $(DRV_PROGS) $(AUX_PROGS)

DRV_TEST_STREAM_READER_OBJS=test-StreamReader.o read_parsers.o $(CORE_OBJS) $(ZLIB_OBJS) $(BZIP2_OBJS)
DRV_TEST_CACHE_MANAGER_OBJS=test-CacheManager.o read_parsers.o $(CORE_OBJS) $(ZLIB_OBJS) $(BZIP2_OBJS)
DRV_TEST_PARSER_OBJS=test-Parser.o read_parsers.o $(CORE_OBJS) $(ZLIB_OBJS) $(BZIP2_OBJS)
DRV_TEST_HASHTABLES_OBJS= \
	test-HashTables.o counting.o hashbits.o hashtable.o subset.o \
	$(PARSERS_OBJS) $(CORE_OBJS) $(ZLIB_OBJS) $(BZIP2_OBJS)
DRV_SMP_FILTERING_OBJS=smpFiltering.o counting.o hashtable.o $(PARSERS_OBJS) $(CORE_OBJS) $(ZLIB_OBJS) $(BZIP2_OBJS)
HT_DIFF_OBJS=ht-diff.o counting.o hashtable.o $(PARSERS_OBJS) $(CORE_OBJS) $(ZLIB_OBJS) $(BZIP2_OBJS)

test-StreamReader: $(DRV_TEST_STREAM_READER_OBJS)
	$(CXX) -o $@ $(DRV_TEST_STREAM_READER_OBJS) $(LIBS)

test-CacheManager: $(DRV_TEST_CACHE_MANAGER_OBJS)
	$(CXX) -o $@ $(DRV_TEST_CACHE_MANAGER_OBJS) $(LIBS)

test-Parser: $(DRV_TEST_PARSER_OBJS)
	$(CXX) -o $@ $(DRV_TEST_PARSER_OBJS) $(LIBS)

test-HashTables: $(DRV_TEST_HASHTABLES_OBJS)
	$(CXX) -o $@ $(DRV_TEST_HASHTABLES_OBJS) $(LIBS)

smpFiltering: $(DRV_SMP_FILTERING_OBJS) primes.hh
	$(CXX) -o $@ $(DRV_SMP_FILTERING_OBJS) $(LIBS)

ht-diff: $(HT_DIFF_OBJS)
	$(CXX) -o $@ $(HT_DIFF_OBJS) $(LIBS)

# TODO: Move 'main' to a driver program.
#parsetest: parsers.o 
#	$(CXX) -o $@ parsers.o $(ZLIB_OBJS)

bittest: bittest.o ktable.o
	$(CXX) -o $@ bittest.o ktable.o

ktable_test: ktable_test.o hashtable.o $(PARSERS_OBJS) $(CORE_OBJS) $(ZLIB_OBJS) $(BZIP2_OBJS)
	$(CXX) -o $@ ktable_test.o hashtable.o $(PARSERS_OBJS) $(CORE_OBJS) $(ZLIB_OBJS) $(BZIP2_OBJS) $(LIBS)

# NOTE: Disabled due to broken constructor call.
#graphtest: graphtest.o ktable.o hashtable.o
#	$(CXX) -o $@ graphtest.o ktable.o hashtable.o

# NOTE: Disabled due to broken constructor call.
#consume_prof: consume_prof.o hashtable.o ktable.o $(PARSERS_OBJS)
#	$(CXX) -o $@ consume_prof.o hashtable.o ktable.o $(PARSERS_OBJS) $(LIBS)

$(ZLIB_OBJS):
	(cd $(ZLIB_DIR) && ./configure --shared && make libz.so.1.2.3)

$(BZIP2_OBJS):
	(cd $(BZIP2_DIR) && make -f Makefile-libbz2_so all)

khmer_config.o: khmer_config.cc khmer_config.hh

trace_logger.o: trace_logger.cc trace_logger.hh

read_parsers.o: read_parsers.cc read_parsers.hh

threadedParsers.o: threadedParsers.cc threadedParsers.hh

parsers.o: parsers.cc parsers.hh

ktable.o: ktable.cc ktable.hh

hashtable.o: hashtable.cc hashtable.hh ktable.hh khmer.hh

hashbits.o: hashbits.cc hashbits.hh subset.hh hashtable.hh ktable.hh khmer.hh counting.hh

subset.o: subset.cc subset.hh hashbits.hh ktable.hh khmer.hh

counting.o: counting.cc counting.hh hashtable.hh ktable.hh khmer.hh

test-StreamReader.o: read_parsers.hh

test-CacheManager.o: read_parsers.hh

test-Parser.o: read_parsers.hh

test-HashTables.o: read_parsers.hh primes.hh

ht-diff.o: counting.hh hashtable.hh ktable.hh khmer.hh

