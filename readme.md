## PwnedPasswords (WIP)

This program takes the most recent (Jan 2019) pwned passwords list from haveibeenpwned.com (~24gb) and puts those hashed passwords into a bloomfilter. 

Afterwards you can input a password and see if it is in the hacked passwords list.

For my bloom filter I've used a probability of 1 in a million which means 
there will be a false positive once in a million passwords. 

The optimal number of hash functions used was ~20 and the optimal number of bits per element was ~3.6 bytes. The equations used are from below:

Optimal bits per element = -1.44 * log2 (p)
Optimal hash functions   = -log2 (p)
https://en.wikipedia.org/wiki/Bloom_filter

There are ~550M passwords in the pwned passwords list, at ~3.6 bytes per element, the whole bloom filter is roughly 2gb. Checking for a password takes ~ 40-70 microseconds.

After the bloom filter is created it is saved to a file so it doesn't need to be recreated again. 

Creating the bloom filter takes ~ 1 hr on my 8 core 2.5Ghz i7.

If you are willing to take a higher probability of false positives you can get a smaller bloom filter and quicker access and creation times. You can tune `NUM_BITS` and `NUM_HASH_FNS` defined in `bloom_filter.h`. You can also tune `NUM_THREADS`.

## Build

```
make main
```

creates a file in `./build/main`

## Run

Before running make sure you've downloaded and extracted the pwnedpasswords to `./data/pwned-passwords-sha1-ordered-by-hash-v4.txt`. Note that file is 24gb but you can delete it after the bloom filter has been created.

```
./build/main 
```