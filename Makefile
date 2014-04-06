CFLAGS := -std=c++11 	                \
					-Wall       								\
					-DBOOST_TEST_DYN_LINK 			\
					-DBOOST_TEST_MODULE=main

LFLAGS := -lboost_unit_test_framework

COMPILER := g++
# COMPILER := clang

TESTS := future_test result_test

all: $(TESTS)

future_test: future_test.cpp future.h helpers.h test_helpers.h
	$(COMPILER) $(CFLAGS) -o $@ $< $(LFLAGS)

result_test: result_test.cpp result.h helpers.h
	$(COMPILER) $(CFLAGS) -o $@ $< $(LFLAGS)

tests: $(TESTS)
	@for test in $(TESTS);	do ./$$test;	done
