CC=g++
CFLAGS=-std=c++17 -c -g -Wall -fgnu-tm -lnuma -finstrument-functions -ldl -rdynamic -larmadillo -lpthread
INCLUDES=-I../ -I/home/zsq/prototype/include/
SOURCES=$(wildcard *.cc)
OBJECTS=$(SOURCES:.cc=.o)

ifeq ($(PREDICT_ENABLED),1)
  CFLAGS += -DPREDICT_ENABLED
else ifeq ($(USE_CXL),1)
  CFLAGS += -DUSE_CXL
else ifeq ($(USE_LOCAL),1)
  CFLAGS += -DUSE_LOCAL
endif

all: $(SOURCES) $(OBJECTS)

.cc.o:
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@

clean:
	$(RM) $(OBJECTS)

