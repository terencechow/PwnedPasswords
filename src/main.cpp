
#include "services/bloom_filter.h"
#include <fstream>
#include <iostream>

using namespace std;

int main()
{

    OpenSSL_add_all_digests();

    BloomFilter *bloom_filter = new BloomFilter();

    // check if database file exists
    ifstream dbFile("data/pwned_passwords_bloomfilter.db", ios::in | ios::binary);
    if (!dbFile)
    {
        ifstream inFile;
        string password_filename("data/pwned-passwords-sha1-ordered-by-hash-v4.txt");
        // string password_filename("data/pwned-passwords-sample.txt");
        inFile.open(password_filename);
        if (!inFile)
        {
            cerr << "Unable to open file " << password_filename << "\n";
            exit(1); // call system to stop
        }
        cout << "Creating database from textfile. Please wait... This can take over an hour.\n";
        bloom_filter->init_from_textfile(inFile);
        inFile.close();
    }
    else
    {
        cout << "Loading database... Please wait... This may take up to a minute.\n";
        bloom_filter->init_from_dbfile(dbFile);
    }
    dbFile.close();

    // take inputs and test for whether they are in the bloomfilter
    while (true)
    {
        string test_password;
        cout << "Please enter a password: \n";
        getline(cin, test_password);

        const char *message = test_password.c_str();
        uint64_t message_len = strlen(message);

        unsigned char output[EVP_MAX_MD_SIZE];
        unsigned int output_len;
        sha1((unsigned char *)message, message_len, output, &output_len);

        char sha1_hash[output_len * 2 + 1];
        for (unsigned int i = 0; i < output_len; i++)
            sprintf(&sha1_hash[i * 2], "%02X", (unsigned int)output[i]);

        if (bloom_filter->check_element(sha1_hash))
        {
            cout << "Password is in hacked password list!\n";
        }
        else
        {
            cout << "You're safe! Password is NOT in hacked password list!\n";
        }
    }

    EVP_cleanup();
    delete bloom_filter;

    return 0;
}