# PwnedPasswords as a service (aka PWNEDPAAS)

## TL:DR;

This program takes the most recent (Jan 2019) pwned passwords list from haveibeenpwned.com (~24gb) and puts those hashed passwords into a bloomfilter. 

Afterwards you can input a password and see if it is in the hacked passwords list. While importing the initial data requires a computer with 24gb disk space (since the password file is 24gb), once the passwords has been imported, a binary file representing the bloom filter is created that is ~1.5gb. From that point on, only the 1.5gb database file is needed to start the service. Note that importing from a text file requires at least 4.5gb RAM and 24gb hard disk. Starting with a db file requires 1.5gb of RAM and 1.5gb of hard disk (for the db file).

Bindings have been created to be able to call into this program and have a microservice that simply returns true when a password is in the hacked list and false when it is not. See `bindings` folder for an example. 

## Details

For my bloom filter I've used a probability of 1 in a million which means 
there will be a false positive once in a million passwords. This was arbitrarily chosen and indeed a smaller number will result in a smaller database and lower RAM / storage requirements. 

Originally this program only created a bloom filter. That resulted in a size of ~2gb. In an effort to lower the storage requirements further, the bloomfilter was then separated into many sections and each section encoded as a Golomb Coding. This data structure is known as a Golomb Set.

https://en.wikipedia.org/wiki/Bloom_filter

https://en.wikipedia.org/wiki/Golomb_coding

Checking for a password takes ~ 30 microseconds or 0.03 milliseconds on my computer.

Importing the pwned passwords list uses all the threads on your computer to speed things up since it needs to process quite a large number of passwords. On my macbook i7 with 8 virtual cores, it processed the entire 550M password list in about 25 minutes. Subsequent runs will load the database file and this took approximately 22 seconds on my macbook.

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

Before running make sure you've downloaded and extracted the pwnedpasswords to `./data/pwned-passwords-sha1-ordered-by-hash-v4.txt`. Note that file is 24gb but you can delete it after the bloom filter has been created. Alternatively if you want to just test an example password dataset, you can run `./generate_sample.sh` which will create a password list of 100 passwords. The passwords in this set are `test1`, `test2`, etc. up to `test100` inclusive. 

Now you can run with:

```
./build/main 
```

This will now process the password file and turn it into the binary file.

## Potential future work
- When initializing from a text file, rather then generate a bloom filter and store 1 bit for each password, I opted to store the `uint64_t` index which results in 8 bytes. Hence initializing from a text file requires more than 4gb (550M uint64_t ~= 4.4 gb). This is a one time cost as initializing from a previously created db only requires 1.5gb. Using a bloom filter would lower this RAM requirement to ~2gb.

- Writing to disk different sections of the golomb set could significantly lower ram requirements at the cost of checking password speed. However the cost might not be significant.
- Although I switched to a golomb set to use lower ram, a bloom filter that is split into separate sections and written to disk would also significantly lower ram requirements. It's not clear whether checking inclusion of a password could be faster in this scenario because many different sections would need to be read from disk and checked, however there would be no decoding that is required with golomb sets.

## Bindings
- The bindings folder holds bindings for other languages. This allows one to start a server in Node.js (for example) and call out to the db. In this way we can have a simple pwned passwords service.