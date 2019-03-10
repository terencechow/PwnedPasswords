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

void BloomFilter::calculate_hashes(string element, mpz_t &xxh_1_out, mpz_t &xxh_2_out)
{
    // randomly generated
    uint64_t seed = 0x44824EDFA779737D;
    uint64_t seed_2 = 0x90948FBEF5B0B8EB;

    const char *message = element.c_str();
    uint64_t message_len = strlen(message);

    uint64_t xxh_1 = XXH64(message, message_len, seed);
    uint64_t xxh_2 = XXH64(message, message_len, seed_2);

    mpz_import(xxh_1_out, 1, 1, sizeof(xxh_1), 0, 0, &xxh_1);
    mpz_import(xxh_2_out, 1, 1, sizeof(xxh_2), 0, 0, &xxh_2);
}
void BloomFilter::calculate_indices(string element, uint64_t *indices)
{
    mpz_t xxh_1;
    mpz_init2(xxh_1, 64);

    mpz_t xxh_2;
    mpz_init2(xxh_2, 64);

    mpz_t xxh_2_multiple;
    mpz_init2(xxh_2_multiple, sizeof(uint64_t) + ceil(log2(NUM_HASH_FNS)));

    mpz_t result;
    mpz_init2(result, sizeof(uint64_t) + ceil(log2(NUM_HASH_FNS)) + 1);

    calculate_hashes(element, xxh_1, xxh_2);

    uint64_t bloom_filter_size = NUM_BITS;
    mpz_t bf_size;
    mpz_init2(bf_size, 64);
    mpz_import(bf_size, 1, 1, sizeof(uint64_t), 0, 0, &bloom_filter_size);

    for (uint8_t i = 0; i < NUM_HASH_FNS; i++)
    {
        // kirsch-mitzenmacher-optimization https://www.eecs.harvard.edu/~michaelm/postscripts/tr-02-05.pdf
        mpz_mul_si(xxh_2_multiple, xxh_2, i);
        mpz_add(result, xxh_1, xxh_2_multiple);
        mpz_mod(result, result, bf_size);
        uint64_t index;
        mpz_export(&index, NULL, 1, sizeof(uint64_t), 0, 0, result);

        *indices = index;
        indices++;
    }

    mpz_clear(xxh_1);
    mpz_clear(xxh_2);
    mpz_clear(xxh_2_multiple);
    mpz_clear(result);
    mpz_clear(bf_size);
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
    mpz_t xxh_1;
    mpz_init2(xxh_1, 64);

    mpz_t xxh_2;
    mpz_init2(xxh_2, 64);

    mpz_t xxh_2_multiple;
    mpz_init2(xxh_2_multiple, sizeof(uint64_t) + ceil(log2(NUM_HASH_FNS)));

    mpz_t result;
    mpz_init2(result, sizeof(uint64_t) + ceil(log2(NUM_HASH_FNS)) + 1);

    uint64_t bloom_filter_size = NUM_BITS;
    mpz_t bf_size;
    mpz_init2(bf_size, 64);
    mpz_import(bf_size, 1, 1, sizeof(uint64_t), 0, 0, &bloom_filter_size);

    calculate_hashes(element, xxh_1, xxh_2);

    bool element_exists = true;
    for (uint8_t i = 0; i < NUM_HASH_FNS; i++)
    {
        // kirsch-mitzenmacher-optimization https://www.eecs.harvard.edu/~michaelm/postscripts/tr-02-05.pdf
        mpz_mul_si(xxh_2_multiple, xxh_2, i);
        mpz_add(result, xxh_1, xxh_2_multiple);
        mpz_mod(result, result, bf_size);
        uint64_t index;
        mpz_export(&index, NULL, 1, sizeof(uint64_t), 0, 0, result);

        if (!bit_array.test(index))
        {
            element_exists = false;
            break;
        };
    }

    mpz_clear(xxh_1);
    mpz_clear(xxh_2);
    mpz_clear(xxh_2_multiple);
    mpz_clear(result);
    mpz_clear(bf_size);
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