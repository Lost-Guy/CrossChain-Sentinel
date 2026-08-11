#include <gtest/gtest.h>
#include "sentinel/core/protocol.hpp"

TEST(ProtocolTest, LockEventSerializationRoundtrip) {
    sentinel::core::LockEventPayload original{
        .source_chain_id = 1,
        .destination_chain_id = 137,
        .nonce = 42,
        .sender = {0x01, 0x02, 0x03},
        .recipient = {0xAA, 0xBB, 0xCC},
        .amount = 1000000
    };

    auto packet = sentinel::core::ProtocolEngine::serialize_lock_event(original);
    EXPECT_GT(packet.size(), sentinel::core::HEADER_SIZE);

    auto result = sentinel::core::ProtocolEngine::deserialize_lock_event(packet);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->source_chain_id, original.source_chain_id);
    EXPECT_EQ(result->destination_chain_id, original.destination_chain_id);
    EXPECT_EQ(result->nonce, original.nonce);
    EXPECT_EQ(result->sender, original.sender);
    EXPECT_EQ(result->recipient, original.recipient);
    EXPECT_EQ(result->amount, original.amount);
}

TEST(ProtocolTest, ChecksumTamperingDetection) {
    sentinel::core::LockEventPayload payload{
        .source_chain_id = 1,
        .destination_chain_id = 137,
        .nonce = 100,
        .sender = {},
        .recipient = {},
        .amount = 5000
    };

    auto packet = sentinel::core::ProtocolEngine::serialize_lock_event(payload);
    
    // Corrupt one byte in the body
    packet[packet.size() - 1] ^= static_cast<std::byte>(0xFF);

    auto result = sentinel::core::ProtocolEngine::deserialize_lock_event(packet);
    EXPECT_FALSE(result.has_value());
}
