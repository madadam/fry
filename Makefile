.PHONY: tests clean

CFLAGS := -std=c++11 -Wall -Iinclude
LFLAGS :=

# COMPILER := g++
COMPILER := clang++

COMMON_DEPS := include/fry/either.h        \
					     include/fry/future.h        \
					     include/fry/future_result.h \
							 include/fry/helpers.h       \
							 include/fry/repeat_until.h  \
					     include/fry/result.h        \
					     include/fry/when_all.h      \
					     include/fry/when_any.h

################################################################################
TESTS := tests/future_test 						\
				 tests/result_test 						\
				 tests/future_result_test 		\
				 tests/repeat_until_test  		\
				 tests/when_all_test      		\
				 tests/when_any_test      		\
				 tests/when_all_success_test

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

tests/%: tests/%.cpp $(TEST_DEPS)
	$(COMPILER) $(TEST_CFLAGS) -o $@ $< $(TEST_LFLAGS)

examples/echo_server: examples/echo_server.cpp $(EXAMPLE_DEPS)
	$(COMPILER) $(CFLAGS) -o $@ $< $(LFLAGS) -lboost_system

examples/echo_client: examples/echo_client.cpp $(EXAMPLE_DEPS)
	$(COMPILER) $(CFLAGS) -o $@ $< $(LFLAGS) -lboost_system -lpthread

clean:
	rm -f $(TESTS) $(EXAMPLES)
