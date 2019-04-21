
#include "services/golomb_set.h"
#include <fstream>
#include <iostream>

using namespace std;

int main()
{

    OpenSSL_add_all_digests();

    GolombSet<uint64_t> golomb_set(0.000001);

    // check if database file exists
    string db_filename = "data/pwned_passwords.db";
    ifstream dbFile(db_filename, ios::in | ios::binary);
    if (!dbFile.is_open())
    {
        // string password_filename("data/pwned-passwords-sha1-ordered-by-hash-v4.txt");
        // string password_filename("data/100000-pwned-passwords-sample.txt");
        string password_filename("data/pwned-passwords-sample.txt");

        golomb_set.init_from_textfile(password_filename, db_filename);
    }
    else
    {
        golomb_set.init_from_dbfile(db_filename);
    }
    dbFile.close();

    // take inputs and test for whether they are in the bloomfilter
    while (true)
    {
        string test_password;
        cout << "Please enter a password: \n";
        getline(cin, test_password);

        if (golomb_set.check_password(test_password))
        {
            cout << "Password is in hacked password list!\n";
        }
        else
        {
            cout << "You're safe! Password is NOT in hacked password list!\n";
        }
    }

    EVP_cleanup();

    return 0;
}