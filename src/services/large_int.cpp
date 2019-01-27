#include <services/large_int.h>
#include <iostream>
using namespace std;

LargeInt::LargeInt() {}
LargeInt::LargeInt(uint64_t *u64_bits, uint8_t len)
{

    for (uint8_t i = 0; i < len; i++)
    {
        bits[i] = *(u64_bits + i);
    }
    for (uint8_t i = len; i < 4; i++)
    {
        bits[i] = 0;
    }
}

LargeInt::LargeInt(unsigned char *bytes, uint8_t len)
{
    // assumes bytes is little endian
    for (uint8_t i = 0; i < len; i++)
    {
        bits[i] = (bytes[0 + 8 * i] << 56) |
                  (bytes[1 + 8 * i] << 48) |
                  (bytes[2 + 8 * i] << 40) |
                  (bytes[3 + 8 * i] << 32) |
                  (bytes[4 + 8 * i] << 24) |
                  (bytes[5 + 8 * i] << 16) |
                  (bytes[6 + 8 * i] << 8) |
                  (bytes[7 + 8 * i]);
    }
}

uint64_t LargeInt::hi(uint64_t x)
{
    return x >> 32;
}

uint64_t LargeInt::lo(uint64_t x)
{
    return ((1L << 32) - 1) & x;
}

LargeInt LargeInt::operator+(const LargeInt other)
{
    LargeInt l;
    for (int i = 0; i < 4; i++)
    {
        l.bits[i] = bits[i] + other.bits[i];
        if (i > 0)
        {
            // previous uint64 overflowed
            if (l.bits[i - 1] < bits[i - 1])
            {
                l.bits[i] += 1;
            }
        }
    }
    return l;
}

LargeInt LargeInt::operator*(uint8_t multiplier)
{
    LargeInt l;
    uint64_t prior_carry = 0;
    for (int i = 0; i < 4; i++)
    {
        // https: //stackoverflow.com/questions/1815367/catch-and-compute-overflow-during-multiplication-of-two-large-integers
        uint64_t product = bits[i] * multiplier;
        uint64_t carry = 0;

        // multiplication overflow
        if (bits[i] != 0 && product / bits[i] != multiplier)
        {
            // actually uint32_t would do, but the casting is annoying
            uint64_t s0, s1, s2, s3;

            uint64_t x = lo(bits[i]) * lo(multiplier);

            s0 = lo(x);

            x = hi(bits[i]) * lo(multiplier) + hi(x);
            s1 = lo(x);
            s2 = hi(x);

            x = s1 + lo(bits[i]) * hi(multiplier);
            s1 = lo(x);

            x = s2 + hi(bits[i]) * hi(multiplier) + hi(x);
            s2 = lo(x);
            s3 = hi(x);

            l.bits[i] = s1 << 32 | s0; // result
            carry = s3 << 32 | s2;     // carry
        }
        else
        {
            // no overflow from multiplication
            l.bits[i] = product;
        }

        // add any prior overflows
        if (prior_carry > 0)
        {
            // previous uint overflowed, add carry
            l.bits[i] += prior_carry;

            // addition overflow
            if (l.bits[i] < prior_carry)
            {
                // store carry for next calculation
                prior_carry = 1;
            }
            else
            {
                prior_carry = 0;
            }
        }

        // store current carry for next calculation
        if (carry > 0)
        {
            prior_carry += carry;
        }
    }
    return l;
}

uint64_t LargeInt::operator%(uint64_t divisor)
{
    uint64_t result = 0;
    uint64_t uint64_max_mod_divisor = (UINT64_MAX % divisor + 1) % divisor;

    // (2^64)^0 * bits[0] + (2^64)^1 * bits[1] +(2^64)^2 * bits[2] +(2^64)^3 * bits[3]
    // = our large int representation
    result += bits[0] % divisor;
    result += multmod(bits[1] % divisor, uint64_max_mod_divisor, divisor);
    result %= divisor;
    uint64_t temp = multmod(uint64_max_mod_divisor, uint64_max_mod_divisor, divisor);
    result += multmod(bits[2] % divisor, temp, divisor);
    result %= divisor;
    result += (bits[3] % divisor * uint64_max_mod_divisor * uint64_max_mod_divisor * uint64_max_mod_divisor) % divisor;
    result += multmod(bits[3] % divisor, multmod(temp, uint64_max_mod_divisor, divisor), divisor);
    result %= divisor;
    return result;
}

// https://stackoverflow.com/questions/21030153/modulo-of-multiplication-of-large-numbers
uint64_t LargeInt::addmod(uint64_t x, uint64_t y, uint64_t m)
{
    x %= m;
    y %= m;
    uint64_t sum = x - m + y; // -m <= sum < m-1
    return sum < 0 ? sum + m : sum;
}

// https://stackoverflow.com/questions/21030153/modulo-of-multiplication-of-large-numbers
uint64_t LargeInt::multmod(uint64_t x, uint64_t y, uint64_t m)
{
    x %= m;
    y %= m;
    uint64_t a = x < y ? x : y; // min
    uint64_t b = x < y ? y : x; // max
    uint64_t product = 0;
    for (; a != 0; a >>= 1, b = addmod(b, b, m))
    {
        if (a & 1)
        {
            product = addmod(product, b, m);
        }
    }
    return product;
}