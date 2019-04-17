## PwnedPasswords (WIP)

This program takes the most recent (Jan 2019) pwned passwords list from haveibeenpwned.com (~24gb) and puts those hashed passwords into a bloomfilter. 

Afterwards you can input a password and see if it is in the hacked passwords list.

For my bloom filter I've used a probability of 1 in a million which means 
there will be a false positive once in a million passwords. 

The optimal number of hash functions used was 20 and the optimal number of bits per element was ~3.6 bytes. The equations used are from below:

Optimal bits per element = -1.44 * log<sub>2</sub>(p)
<br>
Optimal hash functions   = -log<sub>2</sub>(p)

https://en.wikipedia.org/wiki/Bloom_filter

There are ~550M passwords in the pwned passwords list, at ~3.6 bytes per element, the whole bloom filter is roughly 2gb. However, the bloomfilter is then separated into sections of 4096 bits and encoded with a Golomb Coding. 

https://en.wikipedia.org/wiki/Golomb_coding

Checking for a password takes ~ 30 microseconds or 0.03 milliseconds.

After the golomb set is created it is saved to a file so it doesn't need to be recreated again. Creating the golomb set takes ~25 minutes on my 4 core 2.5Ghz i7 and the set is ~1.5gb. 

If you are willing to take a higher probability of false positives you can get a smaller set and quicker access / creation times. 

## Build

First clone the repo and the submodules.
```
git clone --recurse-submodules git@github.com:terencechow/PwnedPasswords.git
```

You will need to have `openssl` on your computer. I've assumed the source is located at `/usr/local/opt/openssl`. If yours is not, change the `OPENSSL_DIR` in the makefile to point to the path to your openssl source code.

Next run the below command
```
make main
```

This will create a file `main` in the `build` directory.

## Run

Before running make sure you've downloaded and extracted the pwnedpasswords to `./data/pwned-passwords-sha1-ordered-by-hash-v4.txt`. Note that file is 24gb but you can delete it after the bloom filter has been created.

Now you can run with:

```
./build/main 
```

This will now process the 24gb txt file and turn it into a binary file that is a golomb set. Again, on my 4 core 2.5Ghz i7 it took 25 minutes. (Note I have 4 physical cores but 8 logical cores). Subsequent runs will load the bloom filter from the binary file and take approximately 22 seconds.

## Caveats

When initializing from a text file, rather then generate a bloom filter and store 1 bit for each password, I opted to store the `uint64_t` index which results in 8 bytes. Hence initializing from a text file requires more than 4gb. This is a one time cost as initializing from a previously created db only requires 1.5gb.

There is potential to improve the 4gb initialization from textfile requirement. It would require creating a bloom filter instead of storing all the indices. That would lower the ram requirement to perhaps 2gb.

## Potential future work

- Use a bloom filter for initializing from text file.
- Investigate memory mapping and whether that could lower ram requirements. 
- Investigate writing to disk different sections of the golomb set and whether that would lower ram requirements
- Use a bloom filter instead of a golomb set, but write different sections to disk and page in files on demand (like a real database)
- make some bindings to node.js and make a simple microservice app that has an endpoint to simply check whether a password is valid or invalid.