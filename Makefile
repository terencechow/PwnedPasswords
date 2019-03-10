SRC_DIR = ./src
BUILD_DIR = ./build
INCLUDE_DIR = ./include
LIB_DIR = ./lib
TESTS_DIR = ./tests
OPENSSL_DIR = /usr/local/opt/openssl

# # Flags passed to the C++ compiler.
CC=g++
CXXFLAGS=-std=c++11 -stdlib=libc++ -O3 -g -Wall -Wextra -pedantic -I$(INCLUDE_DIR) -I$(LIB_DIR)/xxHash -I$(OPENSSL_DIR)/include 


# all : $(TESTS)

clean :
	rm -rf $(addprefix $(BUILD_DIR)/, *.o main main.*)

hash.o : $(SRC_DIR)/services/hash.cpp $(INCLUDE_DIR)/services/hash.h
	$(CC) $(CXXFLAGS) -c $(SRC_DIR)/services/hash.cpp -o $(BUILD_DIR)/$@

bloom_filter.o : $(SRC_DIR)/services/bloom_filter.cpp $(INCLUDE_DIR)/services/bloom_filter.h
	$(CC) $(CXXFLAGS) -c $(SRC_DIR)/services/bloom_filter.cpp -o $(BUILD_DIR)/$@

main.o : $(SRC_DIR)/main.cpp
	$(CC) $(CXXFLAGS) -c $(SRC_DIR)/main.cpp -o $(BUILD_DIR)/$@

main: hash.o bloom_filter.o main.o
	$(CC) $(CXXFLAGS) -L$(OPENSSL_DIR)/lib $(wildcard $(SRC_DIR)/services/*.cpp) $(SRC_DIR)/main.cpp -lcrypto -lgmp -o $(BUILD_DIR)/main