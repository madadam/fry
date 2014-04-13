CFLAGS := -std=c++11 	\
  				-Wall       \
  				-I.

LFLAGS :=

COMPILER := g++
# COMPILER := clang

HEADERS := helpers.h future.h result.h result_future.h

################################################################################
TEST_CFLAGS := $(CFLAGS)                  \
							 -DBOOST_TEST_DYN_LINK 			\
							 -DBOOST_TEST_MODULE=main

TEST_LFLAGS := -lboost_unit_test_framework

################################################################################
TESTS 	 := future_test result_test result_future_test
EXAMPLES := examples/echo_server examples/echo_client

all: $(TESTS) $(EXAMPLES)

tests: $(TESTS)
	@for test in $(TESTS);	do ./$$test;	done

future_test: future_test.cpp $(HEADERS) test_helpers.h
	$(COMPILER) $(TEST_CFLAGS) -o $@ $< $(TEST_LFLAGS)

result_test: result_test.cpp $(HEADERS) test_helpers.h
	$(COMPILER) $(TEST_CFLAGS) -o $@ $< $(TEST_LFLAGS)

result_future_test: result_future_test.cpp $(HEADERS) test_helpers.h
	$(COMPILER) $(TEST_CFLAGS) -o $@ $< $(TEST_LFLAGS)

examples/echo_server: examples/echo_server.cpp $(HEADERS) asio.h
	$(COMPILER) $(CFLAGS) -o $@ $< $(LFLAGS) -lboost_system

examples/echo_client: examples/echo_client.cpp $(HEADERS) asio.h
	$(COMPILER) $(CFLAGS) -o $@ $< $(LFLAGS) -lboost_system -lpthread

clean:
	rm $(TESTS) $(EXAMPLES)
