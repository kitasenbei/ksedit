#include "test.h"
#include "../include/history.h"

TEST(test_history_init)
{
    PosHistory h;
    history_init(&h);
    ASSERT(!history_can_go_back(&h));
    ASSERT(!history_can_go_forward(&h));
}

TEST(test_history_push)
{
    PosHistory h;
    history_init(&h);
    history_push(&h, 100);
    history_push(&h, 200);
    ASSERT(history_can_go_back(&h));
}

TEST(test_history_back)
{
    PosHistory h;
    history_init(&h);
    history_push(&h, 100);
    history_push(&h, 200);
    history_push(&h, 300);

    size_t pos = history_back(&h, 300);
    ASSERT_EQ(pos, 200);

    pos = history_back(&h, 200);
    ASSERT_EQ(pos, 100);
}

TEST(test_history_forward)
{
    PosHistory h;
    history_init(&h);
    history_push(&h, 100);
    history_push(&h, 200);
    history_push(&h, 300);

    history_back(&h, 300);
    history_back(&h, 200);

    size_t pos = history_forward(&h);
    ASSERT_EQ(pos, 200);

    pos = history_forward(&h);
    ASSERT_EQ(pos, 300);
}

TEST(test_history_no_duplicate)
{
    PosHistory h;
    history_init(&h);
    history_push(&h, 100);
    history_push(&h, 100); // Duplicate, should be ignored
    history_push(&h, 100); // Duplicate, should be ignored

    // Should only have one entry
    ASSERT(!history_can_go_back(&h) || h.count == 1);
}

TEST(test_history_truncate_on_new)
{
    PosHistory h;
    history_init(&h);
    history_push(&h, 100);
    history_push(&h, 200);
    history_push(&h, 300);

    history_back(&h, 300); // At 200
    history_back(&h, 200); // At 100

    // Push new position - should truncate forward history
    history_push(&h, 150);

    ASSERT(!history_can_go_forward(&h));
}

TEST(test_history_overflow)
{
    PosHistory h;
    history_init(&h);

    // Push more than POS_HISTORY_SIZE
    for (int i = 0; i < POS_HISTORY_SIZE + 10; i++) {
        history_push(&h, i * 100);
    }

    // Should still work, oldest entries shifted out
    ASSERT(h.count == POS_HISTORY_SIZE);
}

int main(void)
{
    printf("History tests:\n");
    RUN_TEST(test_history_init);
    RUN_TEST(test_history_push);
    RUN_TEST(test_history_back);
    RUN_TEST(test_history_forward);
    RUN_TEST(test_history_no_duplicate);
    RUN_TEST(test_history_truncate_on_new);
    RUN_TEST(test_history_overflow);
    TEST_SUMMARY();
}
