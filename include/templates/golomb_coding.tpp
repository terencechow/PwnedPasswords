template <class Tuint>
void GolombCoding<Tuint>::init(Tuint _m, Tuint _size)
{
    vector<pair<Tuint, bool>> encodings{pair<Tuint, bool>(_size, false)};
    init(_m, _size, encodings);
};

template <class Tuint>
void GolombCoding<Tuint>::init(Tuint _m, Tuint _size, vector<pair<Tuint, bool>> const &encodings)
{
    n_decompressed = _size;
    m = _m;
    b = ceil(log2(m));
    cutoff = (2 << (b - 1)) - m;

    Tuint new_length = 0;
    for (auto it = encodings.begin(); it != encodings.end(); ++it)
    {
        // first or last bit encoded as a one
        if (it->second && (it == encodings.begin() || next(it) == encodings.end()))
        {
            new_length += it->first * b;
        }
        else
        {
            if (it->second)
            {
                new_length += (it->first - 1) * b;
            }
            else
            {
                new_length += it->first / m + 1 + (it->first % m < cutoff ? b - 1 : b);
            }
        }
    }

    realloc_bits(new_length);
    n_bits = new_length;

    Tuint bit_index = 0;
    for (auto it = encodings.begin(); it != encodings.end(); ++it)
    {
        if (it->second)
        {
            bit_index = encode_ones(bit_index, it->first);
        }
        else
        {
            bit_index = encode_quotient_and_remainder(bit_index, it->first);
        }
    }
}

template <class Tuint>
void GolombCoding<Tuint>::realloc_bits(Tuint new_length)
{
    if (new_length > n_bits)
    {
        // reallocate
        uint8_t *more_bytes = NULL;
        Tuint new_length_in_bytes = new_length / 8 + (new_length % 8 > 0);
        more_bytes = (uint8_t *)realloc(bytes, new_length_in_bytes * sizeof(uint8_t));
        if (more_bytes != NULL)
        {
            bytes = more_bytes;
            Tuint old_length_in_bytes = n_bits / 8 + (n_bits % 8 > 0);
            // set new bits to be 0
            fill(bytes + old_length_in_bytes, bytes + new_length_in_bytes, 0);
        }
        else
        {
            cout << "Error reallocating memory. Exiting..\n";
            free(bytes);
            exit(1);
        }
    }
}

template <class Tuint>
Tuint GolombCoding<Tuint>::shift_remaining_bytes(Tuint starting_bit, Tuint num_to_shift)
{
    if (starting_bit >= n_bits)
    {
        return 0;
    }

    Tuint num_to_shift_in_bytes = num_to_shift / 8;
    uint8_t shift_offset = num_to_shift % 8;
    uint8_t n_bits_offset = n_bits % 8;

    Tuint byte_index = n_bits / 8;

    // zero out all new bytes
    for (Tuint i = 0; i < num_to_shift_in_bytes + (shift_offset + n_bits_offset > 8); i++)
    {
        *(bytes + byte_index + i + 1) = 0;
    }

    // copy last byte to new location first
    uint8_t byte = *(bytes + byte_index);
    *(bytes + byte_index + num_to_shift_in_bytes) = (byte >> shift_offset);
    if (shift_offset + n_bits_offset > 8)
    {
        *(bytes + byte_index + num_to_shift_in_bytes + 1) = (byte << (8 - shift_offset));
    }
    byte_index--;

    Tuint starting_byte = starting_bit / 8;
    while (byte_index > starting_byte)
    {
        byte = *(bytes + byte_index);

        *(bytes + byte_index + num_to_shift_in_bytes) = (byte >> shift_offset);
        *(bytes + byte_index + num_to_shift_in_bytes + 1) |= (byte << (8 - shift_offset));
        byte_index--;
    };

    uint8_t starting_bit_offset = starting_bit % 8;
    byte = *(bytes + byte_index);
    uint8_t original_bits = byte & (0xFF << (8 - starting_bit_offset));
    byte &= (0xFF >> starting_bit_offset);

    *(bytes + byte_index + num_to_shift_in_bytes) = original_bits | (byte >> shift_offset);
    *(bytes + byte_index + num_to_shift_in_bytes + 1) |= (byte << (8 - shift_offset));
};

template <class Tuint>
Tuint GolombCoding<Tuint>::encode_quotient(Tuint starting_bit, Tuint quotient)
{

    Tuint num_unary_ones = quotient / m;

    Tuint byte_index = starting_bit / 8;
    Tuint bit_index = byte_index * 8;

    Tuint unary_bits_set = 0;
    while (unary_bits_set <= num_unary_ones)
    {
        for (uint8_t i = 0; i < 8; i++)
        {
            if (bit_index < starting_bit)
            {
                bit_index++;
                continue;
            }

            // set unary bits
            if (unary_bits_set < num_unary_ones)
            {
                *(bytes + byte_index) |= (1 << (7 - i));
                unary_bits_set++;
            }
            else if (unary_bits_set == num_unary_ones)
            {
                // set current bit to 0
                *(bytes + byte_index) &= ~(1 << (7 - i));
                return starting_bit + num_unary_ones + 1;
            }
        }
        byte_index++;
    }
}

template <class Tuint>
Tuint GolombCoding<Tuint>::encode_remainder(Tuint starting_bit, Tuint remainder)
{
    Tuint num_remainder_bits;

    if (remainder < cutoff)
    {
        num_remainder_bits = (b - 1);
    }
    else
    {
        remainder += cutoff;
        num_remainder_bits = b;
    }

    Tuint byte_index = starting_bit / 8;
    Tuint num_remainder_bytes = num_remainder_bits / 8;
    Tuint remaining_bits_offset = num_remainder_bits % 8;
    Tuint starting_bit_offset = starting_bit % 8;

    for (uint i = 0; i < num_remainder_bytes; i++)
    {
        // get the bits in remainder that are related to the current byte
        uint8_t mask = (uint8_t)(remainder >> num_remainder_bits - 8 * (num_remainder_bytes + i - 1));
        // accept the bits from the mask or what was in the positions that didn't occupy the mask
        *(bytes + byte_index) &= (0xFF << (8 - starting_bit_offset));
        *(bytes + byte_index) |= (mask >> starting_bit_offset);
        if (starting_bit_offset > 0)
        {
            // technically this would be okay without the if statement but no point |= a mask of 0
            *(bytes + byte_index + 1) &= (0xFF >> starting_bit_offset);
            *(bytes + byte_index + 1) |= mask << (8 - starting_bit_offset);
        }

        byte_index++;
    }

    // handle when num_remainder_bits % 8 > 0
    if (remaining_bits_offset > 0)
    {
        uint8_t mask = 0;
        for (uint j = 0; j < remaining_bits_offset; j++)
        {
            mask |= (1 << j);
        }
        mask &= remainder;
        mask <<= (8 - remaining_bits_offset);
        // take only bits that are not the remaining_bits_offset bits
        *(bytes + byte_index) &= ((0xFF << (8 - starting_bit_offset)) | (0xFF >> (starting_bit_offset + remaining_bits_offset)));
        *(bytes + byte_index) |= mask >> starting_bit_offset;
        if (starting_bit_offset + remaining_bits_offset > 8)
        {
            *(bytes + byte_index + 1) &= (0xFF >> ((starting_bit_offset + remaining_bits_offset) % 8));
            *(bytes + byte_index + 1) |= mask << (8 - starting_bit_offset);
        }
    }

    return starting_bit + num_remainder_bits;
}

template <class Tuint>
Tuint GolombCoding<Tuint>::encode_quotient_and_remainder(Tuint starting_bit, Tuint quotient, Tuint remainder)
{
    Tuint starting_bit_remainder = encode_quotient(starting_bit, quotient);
    return encode_remainder(starting_bit_remainder, remainder);
}

template <class Tuint>
Tuint GolombCoding<Tuint>::encode_quotient_and_remainder(Tuint starting_bit, Tuint number)
{
    Tuint quotient = number / m * m;
    Tuint remainder = number % m;
    return encode_quotient_and_remainder(starting_bit, quotient, remainder);
}

// position is not like an index in that it starts at 1 and ends at n_decompressed
template <class Tuint>
void GolombCoding<Tuint>::add_bit(Tuint position)
{
    if (position > n_decompressed || position == 0)
    {
        return;
    }
    Tuint quotient;
    Tuint remainder;
    Tuint num_ones;
    Tuint bit_index = 0;
    Tuint running_position = 0;

    while (running_position < n_decompressed)
    {
        Tuint ones_ending_position = decode_ones(bit_index, &num_ones);
        Tuint unary_ending_position = decode_unary(ones_ending_position, &quotient);
        Tuint truncated_binary_ending_position = decode_truncated_binary(unary_ending_position, &remainder);
        if (position > running_position && running_position + num_ones >= position)
        {
            // no need to do anything, this already set to 1
            break;
        }

        running_position += num_ones;
        // new one right after a one,
        if (position == running_position + 1)
        {
            // b 0s for the new one
            // when the remainder == cutoff then the coding after will be 1 less bit (cause it goes from b bits to b-1)
            // otherwise the coding after will have the same number of bits
            Tuint new_length = n_bits + b - (remainder == cutoff);
            realloc_bits(new_length);

            // need to shift remaining bytes first
            shift_remaining_bytes(truncated_binary_ending_position, new_length - n_bits);

            encode_ones(bit_index, num_ones + 1);

            // get next quotient & remainder
            Tuint next_quotient = remainder == 0 ? quotient - m : quotient;
            Tuint next_remainder = remainder == 0 ? m - 1 : remainder - 1;

            encode_quotient_and_remainder(ones_ending_position + b, next_quotient, next_remainder);

            n_bits = new_length;
            break;
        }

        running_position += quotient;
        running_position += remainder;

        if (running_position >= position)
        {
            Tuint num_after = running_position - position;
            Tuint num_before = quotient + remainder - num_after - 1; // - 1 for the one that gets added

            Tuint num_before_len = num_before / m + 1 + (num_before % m < cutoff ? b - 1 : b);
            Tuint num_after_len = 0;
            Tuint remaining_bytes_index = 0;

            Tuint previous_length_of_encoding = quotient / m + 1 + (remainder < cutoff ? b - 1 : b);
            if (num_after > 0)
            {
                num_after_len = num_after / m + 1 + (num_after % m < cutoff ? b - 1 : b);
                remaining_bytes_index = truncated_binary_ending_position;
            }
            else
            {
                // position == running_position
                decode_ones(truncated_binary_ending_position, &num_ones);
                if (num_ones > 1)
                {
                    // TODO: handle when starting bit at beginning or end
                    num_after_len = num_ones * b;
                    remaining_bytes_index = truncated_binary_ending_position + (num_ones - 1) * b;
                    previous_length_of_encoding += (num_ones - 1) * b;
                }
                else
                {
                    num_after_len = b;
                    remaining_bytes_index = truncated_binary_ending_position;
                }
            }

            Tuint new_length = n_bits + num_before_len + num_after_len - previous_length_of_encoding;

            realloc_bits(new_length);

            // need to shift remaining bytes first
            shift_remaining_bytes(remaining_bytes_index, new_length - n_bits);

            encode_quotient_and_remainder(remaining_bytes_index - previous_length_of_encoding, num_before);

            if (num_after > 0)
            {
                encode_quotient_and_remainder(remaining_bytes_index - previous_length_of_encoding + num_before_len, num_after);
            }
            else
            {
                encode_ones(remaining_bytes_index - previous_length_of_encoding + num_before_len, num_ones + 1);
            }

            n_bits = new_length;
            break;
        }

        bit_index = truncated_binary_ending_position;
    }
};

template <class Tuint>
bool GolombCoding<Tuint>::check_bit(Tuint position)
{
    if (position > n_decompressed)
    {
        return 0;
    }

    Tuint quotient;
    Tuint remainder;
    Tuint num_ones;
    Tuint bit_index = 0;
    Tuint running_position = 0;

    while (running_position < n_decompressed)
    {
        bit_index = decode_ones(bit_index, &num_ones);
        running_position += num_ones;
        if (running_position >= position && num_ones > 0)
        {
            return true;
        }

        bit_index = decode_unary(bit_index, &quotient);
        running_position += quotient;
        if (running_position >= position)
        {
            return false;
        }
        bit_index = decode_truncated_binary(bit_index, &remainder);
        running_position += remainder;
        if (running_position >= position)
        {
            return false;
        }
    }
    return false;
};

template <class Tuint>
Tuint GolombCoding<Tuint>::decode_unary(Tuint starting_bit, Tuint *quotient)
{
    Tuint byte_index = starting_bit / 8;
    Tuint bit_index = byte_index * 8;
    while (true)
    {
        uint8_t byte = *(bytes + byte_index);
        byte_index++;

        for (uint8_t i = 0; i < 8; i++)
        {
            if (bit_index < starting_bit)
            {
                bit_index++;
                continue;
            }

            if ((byte & (1 << (7 - i))) == 0)
            {
                *quotient = (bit_index - starting_bit) * m;
                bit_index++;
                return bit_index;
            };
            bit_index++;
        }
    }
}

template <class Tuint>
Tuint GolombCoding<Tuint>::decode_truncated_binary(Tuint starting_bit, Tuint *truncated_binary)
{
    Tuint byte_index = starting_bit / 8;
    Tuint bit_index = byte_index * 8;
    uint8_t bits_read = 0;

    *truncated_binary = 0;
    while (true)
    {
        uint8_t byte = *(bytes + byte_index);
        byte_index++;
        for (uint8_t i = 0; i < 8; i++)
        {
            if (bit_index < starting_bit)
            {
                bit_index++;
                continue;
            }

            Tuint temp = (byte & (1 << (7 - i)));

            uint8_t bits_to_shift_left = sizeof(Tuint) * 8 - 8 * (bits_read / 8 + 1);
            uint8_t bits_mod_8 = bits_read % 8;
            if (i > bits_mod_8)
            {
                // ie starting_bit is in middle and we are filling out left side of byte
                // 00011111 << 3 = 11111000
                *truncated_binary |= temp << (i - bits_mod_8 + bits_to_shift_left);
            }
            else
            {
                // ie starting_bit is in middle and we are filling out right side of byte
                // 00011111 << 3 = 11111000 |=
                // 10100000 >> 5 = 00000101 <--- we are here
                // = 11111101
                *truncated_binary |= (temp >> (bits_mod_8 - i)) << bits_to_shift_left;
            }
            bits_read++;
            bit_index++;

            if ((bits_read == b - 1 && (*truncated_binary >> (sizeof(Tuint) * 8 - bits_read)) < cutoff) || bits_read == b)
            {
                *truncated_binary >>= (sizeof(Tuint) * 8 - bits_read);
                if (bits_read == b)
                {
                    *truncated_binary -= cutoff;
                }
                return bit_index;
            }
        }
    }
}

template <class Tuint>
Tuint GolombCoding<Tuint>::decode_ones(Tuint starting_bit, Tuint *num_ones)
{
    Tuint byte_index = starting_bit / 8;
    Tuint bit_index = byte_index * 8;
    *num_ones = 1;
    Tuint zeros_counted = 0;
    // ones are not encoded when there is only one 1 in between golomb codings, only when there are multiple 1s.
    // 0000010000 (1 not encoded) vs 0000011100 (n 1s encoded as b * (n-1) zeros)
    // Ones are encoded as if the golomb encoding was 0 for the quotient and 0 for remainder
    // 1 zero for unary + b-1 zeros for remainder = b zeros.
    bool finished = false;
    while (!finished)
    {
        uint8_t byte = *(bytes + byte_index);
        byte_index++;
        for (uint8_t i = 0; i < 8; i++)
        {
            if (bit_index < starting_bit)
            {
                bit_index++;
                continue;
            }

            if (bit_index >= n_bits)
            {
                finished = true;
                break;
            }

            if ((byte & (1 << (7 - i))) == 0)
            {
                zeros_counted++;
                if (zeros_counted == b)
                {
                    zeros_counted = 0;
                    (*num_ones)++;
                }
            }
            else
            {
                finished = true;
                break;
            }

            bit_index++;
        }
    }

    // special case when 1 in first, or ends in 1s
    if (starting_bit == 0 || starting_bit == n_bits - b * (*num_ones - 1))
    {
        (*num_ones)--;
    }

    bit_index -= zeros_counted;
    return bit_index;
}

template <class Tuint>
Tuint GolombCoding<Tuint>::encode_ones(Tuint starting_bit, Tuint num_ones)
{
    if (num_ones == 0 || (num_ones == 1 && starting_bit != 0 && starting_bit != (n_bits - b)))
    {
        return starting_bit;
    }

    if (starting_bit == 0 || starting_bit == n_bits - b * (num_ones - 1))
    {
        num_ones++;
    }

    Tuint num_bits = (num_ones - 1) * b;

    Tuint byte_index = starting_bit / 8;
    Tuint bit_index = byte_index * 8;
    Tuint bits_set = 0;
    while (true)
    {
        for (uint8_t i = 0; i < 8; i++)
        {
            if (bit_index < starting_bit)
            {
                bit_index++;
                continue;
            }

            if (bits_set == num_bits)
            {
                return starting_bit + num_bits;
            }
            *(bytes + byte_index) &= ~(1 << (7 - i));
            bit_index++;
            bits_set++;
        }
        byte_index++;
    }
}
