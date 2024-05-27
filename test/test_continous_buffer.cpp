#include "private/test_defs.hpp"
#include <algorithm>
#include <stpp/continuous_buffer.hpp>
using namespace stpp;

TEST(ContinuousBufferTest, Construction)
{
    ContinuousBuffer buffer(10);
    EXPECT_EQ(buffer.GetCapacity(), 10);
    EXPECT_EQ(buffer.GetSize(), 0);
    auto result = buffer.GetAvailableSpace();
    EXPECT_EQ(result, 10);
}

TEST(ContinuousBufferTest, PushBackAndPopFront)
{
    ContinuousBuffer buffer(10);
    uint8_t data[5] = {1, 2, 3, 4, 5};

    EXPECT_EQ(buffer.PushBack(data, 5), 5);
    EXPECT_EQ(buffer.GetSize(), 5);
    EXPECT_EQ(buffer.GetAvailableSpace(), 5);

    uint8_t out[5];
    EXPECT_EQ(buffer.PopFront(out, 5), 5);
    EXPECT_EQ(buffer.GetSize(), 0);
    EXPECT_EQ(buffer.GetAvailableSpace(), 10);
    EXPECT_EQ(memcmp(out, data, 5), 0);
}

TEST(ContinuousBufferTest, PushBackOverflow)
{
    ContinuousBuffer buffer(10);
    uint8_t data[15] = {0};

    EXPECT_EQ(buffer.PushBack(data, 15), 10);
    EXPECT_EQ(buffer.GetSize(), 10);
    EXPECT_EQ(buffer.GetAvailableSpace(), 0);
}

TEST(ContinuousBufferTest, PopFrontUnderflow)
{
    ContinuousBuffer buffer(10);
    uint8_t data[5] = {1, 2, 3, 4, 5};
    uint8_t out[10] = {0};

    buffer.PushBack(data, 5);
    EXPECT_EQ(buffer.PopFront(out, 10), 5);
    EXPECT_EQ(buffer.GetSize(), 0);
    EXPECT_EQ(buffer.GetAvailableSpace(), 10);
    EXPECT_EQ(memcmp(out, data, 5), 0);
}

TEST(ContinuousBufferTest, ExpandAndShrink)
{
    ContinuousBuffer buffer(10);
    uint8_t *data;
    std::size_t size;

    std::tie(data, size) = buffer.Expand(5);
    ASSERT_EQ(size, 5);
    std::fill_n(data, 5, 1);
    EXPECT_EQ(buffer.GetSize(), 5);
    EXPECT_EQ(buffer.GetAvailableSpace(), 5);

    EXPECT_EQ(buffer.Shrink(3), 3);
    EXPECT_EQ(buffer.GetSize(), 2);
}

TEST(ContinuousBufferTest, Clear)
{
    ContinuousBuffer buffer(10);
    uint8_t data[5] = {1, 2, 3, 4, 5};

    buffer.PushBack(data, 5);
    buffer.Clear();
    EXPECT_EQ(buffer.GetSize(), 0);
    EXPECT_EQ(buffer.GetAvailableSpace(), 10);
}

TEST(ContinuousBufferTest, MoveConstructor)
{
    ContinuousBuffer buffer1(10);
    uint8_t data[5] = {1, 2, 3, 4, 5};
    buffer1.PushBack(data, 5);

    ContinuousBuffer buffer2(std::move(buffer1));
    EXPECT_EQ(buffer2.GetSize(), 5);
    EXPECT_EQ(buffer2.GetAvailableSpace(), 5);

    EXPECT_EQ(buffer1.GetCapacity(), 0);
    EXPECT_EQ(buffer1.GetSize(), 0);
    EXPECT_EQ(buffer1.GetAvailableSpace(), 0);
}

TEST(ContinuousBufferTest, MoveAssignment)
{
    ContinuousBuffer buffer1(10);
    uint8_t data[5] = {1, 2, 3, 4, 5};
    buffer1.PushBack(data, 5);

    ContinuousBuffer buffer2(10);
    buffer2 = std::move(buffer1);
    EXPECT_EQ(buffer2.GetSize(), 5);
    EXPECT_EQ(buffer2.GetAvailableSpace(), 5);

    EXPECT_EQ(buffer1.GetCapacity(), 0);
    EXPECT_EQ(buffer1.GetSize(), 0);
    EXPECT_EQ(buffer1.GetAvailableSpace(), 0);
}

TEST(ContinuousBufferTest, GetBuffer)
{
    ContinuousBuffer buffer(10);
    uint8_t data[5] = {1, 2, 3, 4, 5};
    buffer.PushBack(data, 5);

    uint8_t *buf;
    std::size_t size;
    std::tie(buf, size) = buffer.GetBuffer();
    EXPECT_EQ(size, 5);
    EXPECT_EQ(memcmp(buf, data, 5), 0);
}

void TestContinuousBuffer()
{
    Construction();
    PushBackAndPopFront();
    PushBackOverflow();
    PopFrontUnderflow();
    ExpandAndShrink();
    Clear();
    MoveConstructor();
    MoveAssignment();
    GetBuffer();
}