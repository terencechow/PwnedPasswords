#include <services/bloom_filter.h>

using namespace std;

BloomFilter::BloomFilter()
{
    threads = new thread[NUM_THREADS];

    // large prime chosen here: https://primes.utm.edu/lists/2small/200bit.html
    mpz_init2(large_prime, 262);
    mpz_ui_pow_ui(large_prime, 2, 262);
    mpz_sub_ui(large_prime, large_prime, 71);
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
    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    for (uint8_t i = 0; i < NUM_THREADS; i++)

    {
        threads[i] = thread(&BloomFilter::insert_passwords_thread, this, ref(inFile));
    }

    for (uint8_t i = 0; i < NUM_THREADS; i++)
    {
        threads[i].join();
    }
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    auto duration = duration_cast<minutes>(t2 - t1).count();
    cout << "bit setting took " << duration << " minutes \n";

    // write it to file so we don't have to do it again
    save_to_file("data/pwned_passwords_bloomfilter.db");
    high_resolution_clock::time_point t3 = high_resolution_clock::now();
    auto duration2 = duration_cast<seconds>(t3 - t2).count();
    cout << "saving to file took " << duration2 << " seconds \n";
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
    cout << "Done loading database. Took " << duration << " seconds.\n";
}

void BloomFilter::calculate_hashes(string element, mpz_t &m_out, mpz_t &s_out)
{
    uint64_t seed = 183510;
    uint64_t murmur_output[2]; // allocate 128 bits
    const char *message = element.c_str();
    uint64_t message_len = strlen(message);
    MurmurHash3_x64_128(message, message_len, seed, murmur_output);

    unsigned char sha256_output[EVP_MAX_MD_SIZE]; // can use 32 instead
    unsigned int output_len;

    sha256((unsigned char *)message, message_len, sha256_output, &output_len);

    mpz_import(m_out, 2, 1, sizeof(uint64_t), 0, 0, murmur_output);
    mpz_import(s_out, 32, 1, sizeof(unsigned char), 0, 0, sha256_output);
}
void BloomFilter::calculate_indices(string element, uint64_t *indices)
{
    mpz_t murmur_hash;
    mpz_init2(murmur_hash, 128);

    mpz_t sha256_hash;
    mpz_init2(sha256_hash, 256);

    mpz_t sha256_multiple;
    mpz_init2(sha256_multiple, 261);

    mpz_t result;
    mpz_init2(result, 389);

    calculate_hashes(element, murmur_hash, sha256_hash);

    uint64_t bloom_filter_size = NUM_BITS;
    mpz_t bf_size;
    mpz_init2(bf_size, 64);
    mpz_import(bf_size, 1, 1, sizeof(uint64_t), 0, 0, &bloom_filter_size);

    for (uint8_t i = 0; i < NUM_HASH_FNS; i++)
    {
        // kirsch-mitzenmacher-optimization https://www.eecs.harvard.edu/~michaelm/postscripts/tr-02-05.pdf
        // TODO: make this match the optimization, only mod second
        mpz_mul_si(sha256_multiple, sha256_hash, i);
        mpz_add(result, murmur_hash, sha256_multiple);
        mpz_mod(result, result, large_prime);
        mpz_mod(result, result, bf_size);
        uint64_t index;
        mpz_export(&index, NULL, 1, sizeof(uint64_t), 0, 0, result);

        *indices = index;
        indices++;
    }
    mpz_clear(murmur_hash);
    mpz_clear(sha256_hash);
    mpz_clear(sha256_multiple);
    mpz_clear(result);
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

    mpz_t murmur_hash;
    mpz_init2(murmur_hash, 128);

    mpz_t sha256_hash;
    mpz_init2(sha256_hash, 256);

    mpz_t sha256_multiple;
    mpz_init2(sha256_multiple, 261);

    mpz_t result;
    mpz_init2(result, 389);

    calculate_hashes(element, murmur_hash, sha256_hash);

    uint64_t bloom_filter_size = NUM_BITS;
    mpz_t bf_size;
    mpz_init2(bf_size, 64);
    mpz_import(bf_size, 1, 1, sizeof(uint64_t), 0, 0, &bloom_filter_size);

    bool element_exists = true;
    for (uint8_t i = 0; i < NUM_HASH_FNS; i++)
    {
        // kirsch-mitzenmacher-optimization https://www.eecs.harvard.edu/~michaelm/postscripts/tr-02-05.pdf
        // TODO: make this match the optimization, only mod second
        mpz_mul_si(sha256_multiple, sha256_hash, i);
        mpz_add(result, murmur_hash, sha256_multiple);
        mpz_mod(result, result, large_prime);
        mpz_mod(result, result, bf_size);
        uint64_t index;
        mpz_export(&index, NULL, 1, sizeof(uint64_t), 0, 0, result);

        if (!bit_array.test(index))
        {
            element_exists = false;
            break;
        };
    }

    mpz_clear(murmur_hash);
    mpz_clear(sha256_hash);
    mpz_clear(sha256_multiple);
    mpz_clear(result);
    return element_exists;
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