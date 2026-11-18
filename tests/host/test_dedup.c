#include "test_support.h"

#include <string.h>

#include "svc/dedup/dedup.h"

TEST(dedup, stores_and_replays_a_response)
{
    dedup_t dedup;
    dedup_entry_t entry;
    static const uint8_t payload[] = {0x11u, 0x22u};

    dedup_init(&dedup);
    CHECK(!dedup_lookup(&dedup, 0x30u, 7u, &entry));

    dedup_store(&dedup, 0x30u, 7u, 0x11u, payload, sizeof(payload));
    CHECK(dedup_lookup(&dedup, 0x30u, 7u, &entry));
    CHECK_EQ(entry.command, 0x30u);
    CHECK_EQ(entry.sequence, 7u);
    CHECK_EQ(entry.code, 0x11u);
    CHECK_EQ(entry.payload_length, 2u);
    CHECK(memcmp(entry.payload, payload, sizeof(payload)) == 0);
    CHECK(!dedup_lookup(&dedup, 0x07u, 7u, &entry));
}

TEST(dedup, replaces_the_oldest_entry_and_counts_overflow)
{
    dedup_t dedup;
    uint8_t index;

    dedup_init(&dedup);
    for (index = 0u; index < PSG_DEDUP_DEPTH; index++) {
        dedup_store(&dedup, 0x30u, index, 0x00u, NULL, 0u);
    }
    CHECK_EQ(dedup_lookup(&dedup, 0x30u, 0u, NULL), true);

    dedup_store(&dedup, 0x30u, 100u, 0x00u, NULL, 0u);
    CHECK_EQ(dedup_lookup(&dedup, 0x30u, 0u, NULL), false);
    CHECK_EQ(dedup_lookup(&dedup, 0x30u, 1u, NULL), true);
    CHECK_EQ(dedup_lookup(&dedup, 0x30u, 100u, NULL), true);
    CHECK_EQ(dedup_overflow_count(&dedup), 1u);
}

TEST(dedup, clear_drops_every_entry)
{
    dedup_t dedup;

    dedup_init(&dedup);
    dedup_store(&dedup, 0x30u, 1u, 0x00u, NULL, 0u);
    dedup_clear(&dedup);
    CHECK(!dedup_lookup(&dedup, 0x30u, 1u, NULL));
}

TEST(dedup, sequence_wrap_is_a_distinct_key)
{
    dedup_t dedup;

    dedup_init(&dedup);
    dedup_store(&dedup, 0x30u, 0xFFu, 0x00u, NULL, 0u);
    dedup_store(&dedup, 0x30u, 0x00u, 0x11u, NULL, 0u);
    CHECK(dedup_lookup(&dedup, 0x30u, 0xFFu, NULL));
    CHECK(dedup_lookup(&dedup, 0x30u, 0x00u, NULL));
}
