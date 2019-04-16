SRC_DIR = ./src
BUILD_DIR = ./build
INCLUDE_DIR = ./include
LIB_DIR = ./lib
TEST_DIR = ./tests
OPENSSL_DIR = /usr/local/opt/openssl
gtest_filter?=*

# # Flags passed to the C++ compiler.
CC=g++
CXXFLAGS=-std=c++11 -stdlib=libc++ -O3 -g -Wall -Wextra -pedantic -I$(INCLUDE_DIR) -I$(LIB_DIR)/xxHash -I$(OPENSSL_DIR)/include 


# all : $(TESTS)

clean :
	rm -rf $(addprefix $(BUILD_DIR)/, *.o main main.* *.test)

hash.o : $(SRC_DIR)/services/hash.cpp $(INCLUDE_DIR)/services/hash.h
	$(CC) $(CXXFLAGS) -c $(SRC_DIR)/services/hash.cpp -o $(BUILD_DIR)/$@

main.o : $(SRC_DIR)/main.cpp
	$(CC) $(CXXFLAGS) -c $(SRC_DIR)/main.cpp -o $(BUILD_DIR)/$@

main: hash.o main.o
	$(CC) $(CXXFLAGS) -L$(OPENSSL_DIR)/lib $(addprefix $(BUILD_DIR)/,$^) -lcrypto -lgmp -o $(BUILD_DIR)/main

golomb_coding_unit_test.o : $(INCLUDE_DIR)/services/golomb_coding.h
	$(CC) $(CXXFLAGS) -c $(TEST_DIR)/golomb_coding/test-bundler.cpp -o $(BUILD_DIR)/$@

golomb_coding_unit_test: golomb_coding_unit_test.o
	$(CC) $(CXXFLAGS) $(addprefix $(BUILD_DIR)/,$^) -lgtest_main -lgtest -o $(BUILD_DIR)/$@.test
	$(BUILD_DIR)/golomb_coding_unit_test.test --gtest_filter=$(gtest_filter)