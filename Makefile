SRC_DIR = ./src
BUILD_DIR = ./build
INCLUDE_DIR = ./include
LIB_DIR = ./lib/smhasher/src
TESTS_DIR = ./tests
OPENSSL_DIR = /usr/local/opt/openssl

# # Flags passed to the C++ compiler.
CC=g++
CXXFLAGS=-std=c++11 -stdlib=libc++ -O3 -g -Wall -Wextra -pedantic -I$(INCLUDE_DIR) -I$(LIB_DIR) -I$(OPENSSL_DIR)/include 


# all : $(TESTS)

clean :
	rm -rf $(addprefix $(BUILD_DIR)/, *.o main main.*)

murmur3.o : $(LIB_DIR)/MurmurHash3.cpp $(LIB_DIR)/MurmurHash3.h
	$(CC) $(CXXFLAGS) -c $(LIB_DIR)/MurmurHash3.cpp -o $(BUILD_DIR)/$@

hash.o : $(SRC_DIR)/services/hash.cpp $(INCLUDE_DIR)/services/hash.h
	$(CC) $(CXXFLAGS) -c $(SRC_DIR)/services/hash.cpp -o $(BUILD_DIR)/$@

bloom_filter.o : $(SRC_DIR)/services/bloom_filter.cpp $(INCLUDE_DIR)/services/bloom_filter.h
	$(CC) $(CXXFLAGS) -c $(SRC_DIR)/services/bloom_filter.cpp -o $(BUILD_DIR)/$@

main.o : $(SRC_DIR)/main.cpp
	$(CC) $(CXXFLAGS) -c $(SRC_DIR)/main.cpp -o $(BUILD_DIR)/$@

main: murmur3.o hash.o bloom_filter.o main.o
	$(CC) $(CXXFLAGS) -L$(OPENSSL_DIR)/lib $(LIB_DIR)/MurmurHash3.cpp $(wildcard $(SRC_DIR)/services/*.cpp) $(SRC_DIR)/main.cpp -lcrypto -o $(BUILD_DIR)/main