#include <services/hash.h>

using namespace std;

void handleErrors(string message)
{
    cout << "failed at " << message << "\n";
}

void sha1(const unsigned char *message, size_t message_len, unsigned char *digest, unsigned int *digest_len)
{
    const char *name = "sha1";
    digest_message(name, message, message_len, digest, digest_len);
}

void digest_message(const char *name, const unsigned char *message, size_t message_len, unsigned char *digest, unsigned int *digest_len)
{

    EVP_MD_CTX *mdctx;
    if ((mdctx = EVP_MD_CTX_create()) == NULL)
        handleErrors("EVP_MD_CTX_create()");

    const EVP_MD *ssl_function = EVP_get_digestbyname(name);

    if (1 != EVP_DigestInit_ex(mdctx, ssl_function, NULL))
        handleErrors("EVP_DigestInit_ex");
    if (1 != EVP_DigestUpdate(mdctx, message, message_len))
        handleErrors("EVP_DigestUpdate");
    if (1 != EVP_DigestFinal_ex(mdctx, digest, digest_len))
        handleErrors("EVP_DigestFinal_ex");

    EVP_MD_CTX_destroy(mdctx);
}
