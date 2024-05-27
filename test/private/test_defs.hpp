#pragma once

#include <cassert>

#define TEST(a, b)      static void b()

#define EXPECT_EQ(a, b) assert((a) == (b))
#define EXPECT_NE(a, b) assert((a) != (b))
#define ASSERT_EQ(a, b) assert((a) == (b))