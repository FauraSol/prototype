SRC ?= test

$(addsuffix .out, $(SRC)) : $(addprefix src/, $(addsuffix .cc, $(SRC)))
	g++ $< -std=c++17 -O0 -g -lpthread -lnuma -o $@ -Iinclude 