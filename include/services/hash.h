#include <openssl/evp.h>
#include <string>
#include <iostream>
using namespace std;

void handleErrors(string message);

void sha256(const unsigned char *message, size_t message_len, unsigned char *digest, unsigned int *digest_len);

void sha1(const unsigned char *message, size_t message_len, unsigned char *digest, unsigned int *digest_len);

void digest_message(const char *name, const unsigned char *message, size_t message_len, unsigned char *digest, unsigned int *digest_len);