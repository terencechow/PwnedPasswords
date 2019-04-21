template <class Tuint>
GolombSet<Tuint>::~GolombSet()
{
    delete[] codings;
};

template <class Tuint>
Tuint GolombSet<Tuint>::get_m_param()
{
    return round(-1.0 / log2(1.0 - false_positive_rate));
};

template <class Tuint>
uint64_t GolombSet<Tuint>::get_index(string element)
{
    // randomly generated
    uint64_t seed = 0x44824EDFA779737D;

    const char *message = element.c_str();
    uint64_t message_len = strlen(message);

    return XXH64(message, message_len, seed) % golomb_set_size;
};

template <class Tuint>
void GolombSet<Tuint>::get_section_and_position(string element, uint64_t *section, Tuint *position)
{
    uint64_t index = get_index(element);

    *section = index / (Tuint)section_size;
    *position = index % section_size + 1;
};

// hash plaintext consistently
template <class Tuint>
string GolombSet<Tuint>::hash_plaintext(string plaintext)
{
    const char *message = plaintext.c_str();
    uint64_t message_len = strlen(message);

    unsigned char output[EVP_MAX_MD_SIZE];
    unsigned int output_len;
    sha1((unsigned char *)message, message_len, output, &output_len);

    char sha1_hash[output_len * 2 + 1];
    for (unsigned int i = 0; i < output_len; i++)
        sprintf(&sha1_hash[i * 2], "%02X", (unsigned int)output[i]);

    return sha1_hash;
};

// add a password to the bloom filter
template <class Tuint>
void GolombSet<Tuint>::add_password(string element)
{
    string hash = hash_plaintext(element);
    add_hash(hash);
};

// add a hash to the bloom filter
template <class Tuint>
void GolombSet<Tuint>::add_hash(string hash)
{
    uint64_t section;
    Tuint position;
    get_section_and_position(hash, &section, &position);

    codings[section].add_bit(position);
};

// test a hash exists in bloom filter
template <class Tuint>
bool GolombSet<Tuint>::check_hash(string hash)
{
    uint64_t section;
    Tuint position;
    get_section_and_position(hash, &section, &position);
    return codings[section].check_bit(position);
};

template <class Tuint>
bool GolombSet<Tuint>::check_password(string password)
{
    string hash = hash_plaintext(password);
    return check_hash(hash);
}

template <class Tuint>
void GolombSet<Tuint>::insert_passwords_thread(ifstream &inFile, uint8_t thread_index, uint64_t *bit_indices)
{
    // for each line add it to the bloom filter
    string element;
    uint64_t processed_index;
    while (true)
    {
        file_mutex.lock();
        if (inFile.good())
        {
            getline(inFile, element);
            processed_index = processed;
            processed++;
            if (processed % 1000000 == 0)
            {
                cout << "Thread " << (unsigned)thread_index << ": Processing element #" << processed << ".\n";
            }
            file_mutex.unlock();
        }
        else
        {
            file_mutex.unlock();
            break;
        }

        string delimiter(":");
        element = element.substr(0, element.find(delimiter));

        uint64_t index = get_index(element);
        bit_indices[processed_index] = index;
    }
};

template <class Tuint>
uint64_t GolombSet<Tuint>::get_num_passwords(ifstream &inFile)
{
    uint64_t count = 0;
    string line;
    while (getline(inFile, line))
    {
        count++;
    }

    inFile.clear();
    inFile.seekg(0); // rewind
    return count;
}

template <class Tuint>
void GolombSet<Tuint>::init_from_textfile(string in_filename, string out_filename)
{

    ifstream inFile(in_filename, ios::in);
    if (!inFile.is_open())
    {
        throw invalid_argument("Unable to open file");
    }

    cout << "Creating database from textfile. Please wait... This can take up to 30 minutes...\n";
    cout << "Counting number of passwords...\n";
    high_resolution_clock::time_point t0 = high_resolution_clock::now();
    num_passwords = get_num_passwords(inFile);

    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    auto duration0 = duration_cast<seconds>(t1 - t0).count();
    cout << "Found " << num_passwords << " passwords. Took " << duration0 << " seconds.\n";

    // calculate golomb_set_size
    golomb_set_size = num_passwords / false_positive_rate;

    // section_size affects speed of adding and checking elements since the section needs to be decoded
    // before the element can be confirmed to exist or not exist in the section.
    // However it can't be too small or you lose the benefits of encoding so I just use 10x m
    Tuint m = get_m_param();
    section_size = m * 10; // todo dont let this overflow
    num_sections = golomb_set_size / section_size;
    if (golomb_set_size % section_size > 0)
    {
        num_sections++;
    }

    // generate sections
    codings = new GolombCoding<Tuint>[num_sections];

    // create threads & related mutices;
    uint8_t num_threads = thread::hardware_concurrency();
    thread *threads = new thread[num_threads];
    cout << "Created " << num_threads << " to find indices of bloom filter...\n";

    // create a temporary list of indices that will be set which will be converted to the golomb set
    uint64_t *bit_indices = new uint64_t[num_passwords]();

    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    processed = 0;
    for (uint8_t i = 0; i < num_threads; i++)
    {
        threads[i] = thread(&GolombSet<Tuint>::insert_passwords_thread, this, ref(inFile), i, bit_indices);
    }

    for (uint8_t i = 0; i < num_threads; i++)
    {
        threads[i].join();
    }
    delete[] threads;
    inFile.close();

    high_resolution_clock::time_point t3 = high_resolution_clock::now();
    auto duration1 = duration_cast<minutes>(t3 - t2).count();
    cout << "Done finding indices. Took " << duration1 << " minutes.\n";

    // sort
    sort(bit_indices, bit_indices + num_passwords);

    cout << "Converting indices to golomb codes...\n";

    // encode differences and initialize each section
    Tuint previous_position = 0;
    Tuint next_position;
    Tuint section_index = 0;
    Tuint num_ones = 0;
    vector<pair<Tuint, bool>> differences{};
    for (Tuint i = 0; i < num_passwords; i++)
    {
        next_position = bit_indices[i] + 1; // positions are indexed starting at 1

        if (next_position > (section_index * section_size + section_size))
        {
            if (num_ones > 0)
            {
                differences.push_back(make_pair(num_ones, true));
                num_ones = 0;
            }

            Tuint current_difference = section_index * section_size + section_size - previous_position;
            if (current_difference > 0)
            {
                differences.push_back(make_pair(current_difference, false));
            }

            codings[section_index].init(m, section_size, differences);
            differences.clear();

            Tuint num_empty_sections = next_position / section_size - section_index - 1;
            for (Tuint j = 0; j < num_empty_sections; j++)
            {
                codings[section_index].init(m, section_size);
                section_index++;
            }
            previous_position = section_index * section_size + section_size;
            section_index++;
        }

        Tuint current_difference = next_position - previous_position;

        if (current_difference == 1)
        {
            num_ones++;
        }
        else if (current_difference > 1)
        {
            if (num_ones > 0)
            {
                differences.push_back(make_pair(num_ones, true));
                num_ones = 0;
            }
            // current_difference - 1 because the last bit is set to 1.
            differences.push_back(make_pair(current_difference - 1, false));
            num_ones = 1;
        }

        previous_position = next_position;
    }

    // in case we exited the loop without a bit > then the last bit
    if (num_ones > 0)
    {
        differences.push_back(make_pair(num_ones, true));
        num_ones = 0;
    }

    if (!differences.empty())
    {
        Tuint last_difference = section_index * section_size + section_size - previous_position;
        if (last_difference > 0)
        {
            differences.push_back(make_pair(last_difference, false));
        }

        codings[section_index].init(m, section_size, differences);
        section_index++;
    }

    while (section_index < num_sections)
    {
        codings[section_index].init(m, section_size);
        section_index++;
    }

    high_resolution_clock::time_point t4 = high_resolution_clock::now();
    auto duration3 = duration_cast<seconds>(t4 - t3).count();
    cout << "Done converting to golomb codes. Took " << duration3 << " seconds.\n";
    delete[] bit_indices;

    // write it to file so we don't have to do it again
    cout << "Saving to file...\n";
    save_to_file(out_filename);
    high_resolution_clock::time_point t5 = high_resolution_clock::now();
    auto duration4 = duration_cast<seconds>(t5 - t4).count();
    cout << "Done saving to file. Took " << duration4 << " seconds.\n";
};

template <class Tuint>
void GolombSet<Tuint>::init_from_dbfile(string db_filename)
{
    ifstream dbfile(db_filename, ios::in | ios::binary);
    if (!dbfile.is_open())
    {
        throw invalid_argument("Unable to open file");
    }

    high_resolution_clock::time_point t1 = high_resolution_clock::now();

    Tuint m = get_m_param();

    // get golomb_set_size, num_sections, section_size
    dbfile.read(reinterpret_cast<char *>(&golomb_set_size), sizeof(golomb_set_size));
    dbfile.read(reinterpret_cast<char *>(&num_sections), sizeof(num_sections));
    dbfile.read(reinterpret_cast<char *>(&section_size), sizeof(section_size));
    dbfile.read(reinterpret_cast<char *>(&num_passwords), sizeof(num_passwords));

    cout << "Loading database with " << num_passwords << " passwords.\n. Please wait this can take up to a minute...";

    // generate sections
    codings = new GolombCoding<Tuint>[num_sections];

    for (uint64_t i = 0; i < num_sections; i++)
    {

        codings[i].init(m, section_size);
        dbfile.read(reinterpret_cast<char *>(&codings[i].n_bits), sizeof(codings[i].n_bits));

        Tuint num_bytes = codings[i].n_bits / 8 + ((codings[i].n_bits % 8) > 0);
        uint8_t *pBytes = NULL;
        pBytes = (uint8_t *)calloc(num_bytes, sizeof(uint8_t));
        if (pBytes != NULL)
        {
            codings[i].bytes = pBytes;
        }
        else
        {
            throw bad_alloc();
        }

        for (uint j = 0; j < num_bytes; j++)
        {
            dbfile.read(reinterpret_cast<char *>(&codings[i].bytes[j]), sizeof(codings[i].bytes[j]));
        }
    }

    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    auto duration = duration_cast<seconds>(t2 - t1).count();
    cout << "Database loaded. Took " << duration << " seconds.\n";
};

template <class Tuint>
void GolombSet<Tuint>::save_to_file(string filename)
{
    ofstream outfile(filename, ofstream::app | ofstream::binary);

    // store golomb_set_size, num_sections, section_size
    outfile.write(reinterpret_cast<const char *>(&golomb_set_size), sizeof(golomb_set_size));
    outfile.write(reinterpret_cast<const char *>(&num_sections), sizeof(num_sections));
    outfile.write(reinterpret_cast<const char *>(&section_size), sizeof(section_size));
    outfile.write(reinterpret_cast<const char *>(&num_passwords), sizeof(num_passwords));

    for (uint64_t i = 0; i < num_sections; i++)
    {
        // write the number of bits for this section first
        outfile.write(reinterpret_cast<const char *>(&codings[i].n_bits), sizeof(codings[i].n_bits));
        // write the bytes
        Tuint num_bytes = codings[i].n_bits / 8 + ((codings[i].n_bits % 8) > 0);
        for (uint j = 0; j < num_bytes; j++)
        {
            outfile.write(reinterpret_cast<const char *>(&codings[i].bytes[j]), sizeof(codings[i].bytes[j]));
        }
    }
};