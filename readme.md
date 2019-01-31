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

There are ~550M passwords in the pwned passwords list, at ~3.6 bytes per element, the whole bloom filter is roughly 2gb. Checking for a password takes ~ 50 microseconds or 0.05 milliseconds.

After the bloom filter is created it is saved to a file so it doesn't need to be recreated again. Creating the bloom filter takes ~1 hr on my 4 core 2.5Ghz i7.

If you are willing to take a higher probability of false positives you can get a smaller bloom filter and quicker access / creation times. You can tune `NUM_BITS` and `NUM_HASH_FNS` defined in `bloom_filter.h`. You can also tune `NUM_THREADS` to match the number of cores on your computer.

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

This will now process the 24gb txt file and turn it into a 2gb binary file  bloom filter. Again, on my 4 core 2.5Ghz i7 it took 1 hour. (Note I have 4 physical cores but 8 logical cores). Subsequent runs will load the bloom filter from the 2gb binary file.