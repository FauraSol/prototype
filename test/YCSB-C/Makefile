CC=g++
CFLAGS=-std=c++11 -g -Wall -pthread -I./ 
LDFLAGS= -lpthread -ltbb -lhiredis -lnuma -finstrument-functions -ldl -rdynamic
SUBDIRS=core db redis
SUBSRCS=$(wildcard core/*.cc) $(wildcard db/*.cc)
OBJECTS=$(SUBSRCS:.cc=.o)
PREDICT_ENABLED = 0
USE_LOCAL = 0
USE_CXL = 0
EXEC=ycsbc

all: $(SUBDIRS) $(EXEC)

$(SUBDIRS):
	$(MAKE) -C $@ PREDICT_ENABLED=$(PREDICT_ENABLED) \
                   USE_LOCAL=$(USE_LOCAL) \
                   USE_CXL=$(USE_CXL)

$(EXEC): $(wildcard *.cc) $(OBJECTS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir $@; \
	done
	$(RM) $(EXEC)

.PHONY: $(SUBDIRS) $(EXEC)

