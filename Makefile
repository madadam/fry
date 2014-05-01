.PHONY: tests clean

CFLAGS := -std=c++11 -Wall -Iinclude
LFLAGS :=

COMPILER := g++
# COMPILER := clang

COMMON_DEPS := include/fry/helpers.h       \
					     include/fry/future.h        \
					     include/fry/result.h        \
					     include/fry/future_result.h \
					     include/fry/combinators.h

################################################################################
TESTS := tests/future_test 				\
				 tests/result_test 				\
				 tests/future_result_test \
				 tests/combinators_test

TEST_CFLAGS := $(CFLAGS)                  \
							 -DBOOST_TEST_DYN_LINK 			\
							 -DBOOST_TEST_MODULE=main

TEST_LFLAGS := -lboost_unit_test_framework

TEST_DEPS := $(COMMON_DEPS) tests/test_helpers.h

################################################################################
EXAMPLES := examples/echo_server examples/echo_client

EXAMPLE_DEPS := $(COMMON_DEPS) include/fry/asio.h

################################################################################
all: $(TESTS) $(EXAMPLES)

tests: $(TESTS)
	@for test in $(TESTS);	do ./$$test;	done

tests/future_test: tests/future_test.cpp $(TEST_DEPS)
	$(COMPILER) $(TEST_CFLAGS) -o $@ $< $(TEST_LFLAGS)

tests/result_test: tests/result_test.cpp $(TEST_DEPS)
	$(COMPILER) $(TEST_CFLAGS) -o $@ $< $(TEST_LFLAGS)

tests/future_result_test: tests/future_result_test.cpp $(TEST_DEPS)
	$(COMPILER) $(TEST_CFLAGS) -o $@ $< $(TEST_LFLAGS)

tests/combinators_test: tests/combinators_test.cpp $(TEST_DEPS)
	$(COMPILER) $(TEST_CFLAGS) -o $@ $< $(TEST_LFLAGS)

examples/echo_server: examples/echo_server.cpp $(EXAMPLE_DEPS)
	$(COMPILER) $(CFLAGS) -o $@ $< $(LFLAGS) -lboost_system

examples/echo_client: examples/echo_client.cpp $(EXAMPLE_DEPS)
	$(COMPILER) $(CFLAGS) -o $@ $< $(LFLAGS) -lboost_system -lpthread

clean:
	rm -f $(TESTS) $(EXAMPLES)
