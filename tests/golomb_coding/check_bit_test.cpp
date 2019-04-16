
#include "gtest/gtest.h"
#include "services/golomb_coding.h"
#include <bitset>
#include <iostream>
#include <stdlib.h> /* srand, rand */
#include <time.h>   /* time */
#include <vector>

namespace GolombCodingCheckBitTest
{

class GolombCodingCheckBitTest : public ::testing::Test
{
  protected:
    GolombCoding<uint64_t> g;
};

TEST_F(GolombCodingCheckBitTest, ReturnsFalseWhenNoBitsSet)
{
    g.init(20, 4096);

    for (size_t i = 1; i <= 4096; i++)
    {
        EXPECT_FALSE(g.check_bit(i));
    }
}

TEST_F(GolombCodingCheckBitTest, ReturnsTrueWhenBitsSet)
{
    g.init(693147, 1000000);

    srand(time(NULL));

    vector<uint64_t> added_elements;
    // randomly add 300 bits and test them
    for (size_t i = 0; i <= 300; i++)
    {
        auto random_number = rand() % 1000000 + 1;
        added_elements.push_back(random_number);
        g.add_bit(random_number);
        for (auto it = added_elements.begin(); it != added_elements.end(); ++it)
        {
            if (!g.check_bit(*it))
            {
                cout << "FAILED ON " << *it << "\n";
            }
            ASSERT_TRUE(g.check_bit(*it));
        }
    }
}

} // namespace GolombCodingCheckBitTest