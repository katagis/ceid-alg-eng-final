CXX = g++   # use the g++ compiler

VERB_LEVEL=0

LEDA_PATH = /usr/local/LEDA# directory where LEDA libraries are stored

CPPFILE = ./src/main.cpp

INCL_LEDA = -I$(LEDA_PATH)/incl
LINK_LEDAPATH = -L$(LEDA_PATH)
LINK_LEDA = -lleda

LEDA_ALL = $(INCL_LEDA) $(LINK_LEDAPATH) $(LINK_LEDA)
CPP_STANDARD = -std=c++0x

default: release

release:
	$(CXX) $(CPPFILE) -o release.out $(LEDA_ALL) -O2 $(CPP_STANDARD)
debug:
	$(CXX) $(CPPFILE) -o debug.out $(LEDA_ALL) -g $(CPP_STANDARD)
bench:
	$(CXX) $(CPPFILE) -o release.out $(LEDA_ALL) -O2 $(CPP_STANDARD) -DCOMPILE_BENCHMARK


run: release
	./release.out
