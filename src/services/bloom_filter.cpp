#include <services/bloom_filter.h>

using namespace std;

BloomFilter::BloomFilter() {}
BloomFilter::BloomFilter(string s) : bit_array(s) {}

void BloomFilter::calculate_hashes(string element, LargeInt *hash1_out, LargeInt *hash2_out)
{
    uint64_t seed = 183510;
    uint64_t murmur_output[2]; // allocate 128 bits
    const char *message = element.c_str();
    uint64_t message_len = strlen(message);
    MurmurHash3_x64_128(message, message_len, seed, murmur_output);

    unsigned char sha256_output[EVP_MAX_MD_SIZE]; //can use 32 instead
    unsigned int output_len;

    sha256((unsigned char *)message, message_len, sha256_output, &output_len);

    *hash1_out = LargeInt(murmur_output, 2);
    *hash2_out = LargeInt(sha256_output, 4);
}
// add an element to the bloom filter
void BloomFilter::add_element(string element)
{
    LargeInt a, b;
    calculate_hashes(element, &a, &b);

    for (uint8_t i = 0; i < NUM_HASH_FNS; i++)
    {
        uint64_t index = (a + b * i) % NUM_BITS;
        bit_array.set(index, 1);
    }
}

// test an element exists in bloom filter
bool BloomFilter::check_element(string element)
{
    LargeInt a, b;
    calculate_hashes(element, &a, &b);

    for (uint8_t i = 0; i < NUM_HASH_FNS; i++)
    {
        uint64_t index = (a + b * i) % NUM_BITS;
        if (!bit_array.test(index))
        {
            return false;
        };
    }
    return true;
}

void BloomFilter::save_to_file(string filename)
{
    ofstream outfile(filename, ofstream::app | ofstream::binary);
    // uint64_t bitset_ull = bit_array.to_ullong();

    uint64_t mask;
    uint64_t temp;
    for (uint64_t i = 0; i < NUM_BITS; i++)
    {
        // initialize mask to 1
        if (i % 64 == 0)
        {
            if (i > 0)
            {
                outfile.write(reinterpret_cast<const char *>(&temp), sizeof(temp));
            }
            mask = 1;
            temp = 0;
        }

        // copy bits to temp with |=
        if (bit_array.test(i))
        {
            temp |= mask;
        }

        mask <<= 1;
    }
    
    // since we write at the beginnings of the loop we need to write again at the end for the last uint64
    if ((NUM_BITS - 1) % 64 != 0){
        outfile.write(reinterpret_cast<const char *>(&temp), sizeof(temp));
    }
    outfile.close();
}

void BloomFilter::init_from_dbfile(ifstream &dbfile)
{
    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    // dbfile
    char c;
    uint64_t index = 0;
    while (dbfile.get(c))
    {
        uint8_t temp = (uint8_t)c;
        for (uint8_t i = 0; i < 8; i++)
        {
            if ((temp >> i) & 1)
            {
                bit_array.set(index);
            }
            index++;
        }
    }
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    auto duration = duration_cast<seconds>(t2 - t1).count();
    cout << "init from db cost " << duration << " seconds \n";
}

void BloomFilter::init_from_textfile(ifstream &inFile)
{
    // for each line add it to the bloom filter
    string element;
    high_resolution_clock::time_point t0 = high_resolution_clock::now();
    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    while (getline(inFile, element))
    {
        string delimiter(":");
        string hash = element.substr(0, element.find(delimiter));
        add_element(hash);
        high_resolution_clock::time_point t2 = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(t2 - t1).count();
        cout << "Loop cost " << duration << " microseconds \n";
        t1 = high_resolution_clock::now();
    }
    
    high_resolution_clock::time_point t3 = high_resolution_clock::now();
    auto duration = duration_cast<minutes>(t3 - t0).count();
    cout << "Reading whole file took: " << duration << " minutes \n";
    // write it to file so we don't have to do it again
    save_to_file("data/pwned_passwords_bloomfilter.db");

    high_resolution_clock::time_point t4 = high_resolution_clock::now();
    duration = duration_cast<seconds>(t4 - t3).count();
    cout << "Writing whole file took: " << duration << " seconds \n";
}