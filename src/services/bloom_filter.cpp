#include <services/bloom_filter.h>

using namespace std;

BloomFilter::BloomFilter()
{
    threads = new thread[NUM_THREADS];
}

BloomFilter::~BloomFilter()
{
    delete[] threads;
}

void BloomFilter::insert_passwords_thread(ifstream &inFile)
{

    // for each line add it to the bloom filter
    string element;
    uint64_t indices[NUM_HASH_FNS] = {0};
    while (true)
    {
        file_mutex.lock();
        if (inFile.good())
        {
            getline(inFile, element);
            file_mutex.unlock();
        }
        else
        {
            file_mutex.unlock();
            break;
        }

        string delimiter(":");
        element = element.substr(0, element.find(delimiter));
        calculate_indices(element, indices);

        bitset_mutex.lock();
        for (uint8_t i = 0; i < NUM_HASH_FNS; i++)
        {
            bit_array.set(indices[i], 1);
        }
        bitset_mutex.unlock();
        fill(indices, indices + NUM_HASH_FNS, 0);
    }
}

void BloomFilter::init_from_textfile(ifstream &inFile)
{
    for (uint8_t i = 0; i < NUM_THREADS; i++)
    {
        threads[i] = thread(&BloomFilter::insert_passwords_thread, this, ref(inFile));
    }

    for (uint8_t i = 0; i < NUM_THREADS; i++)
    {
        threads[i].join();
    }

    // write it to file so we don't have to do it again
    save_to_file("data/pwned_passwords_bloomfilter.db");
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
void BloomFilter::calculate_indices(string element, uint64_t *indices)
{
    LargeInt a, b;
    calculate_hashes(element, &a, &b);
    for (uint8_t i = 0; i < NUM_HASH_FNS; i++)
    {
        *indices = (a + b * i) % NUM_BITS;
        indices++;
    }
}
// add an element to the bloom filter
void BloomFilter::add_element(string element)
{
    uint64_t indices[NUM_HASH_FNS];
    calculate_indices(element, indices);
    for (uint8_t i = 0; i < NUM_HASH_FNS; i++)
    {
        bit_array.set(indices[i], 1);
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
    if ((NUM_BITS - 1) % 64 != 0)
    {
        outfile.write(reinterpret_cast<const char *>(&temp), sizeof(temp));
    }
    outfile.close();
}