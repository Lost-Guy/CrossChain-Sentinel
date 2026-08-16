#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <atomic>
#include "sentinel/net/peer_manager.hpp"
#include "sentinel/core/protocol.hpp"

TEST(P2PNetworkTest, ServerClientHandshakeAndMessaging) {
    boost::asio::io_context io_ctx;
    auto work_guard = boost::asio::make_work_guard(io_ctx);

    std::thread io_thread([&io_ctx]() {
        io_ctx.run();
    });

    uint16_t test_port = 19842;
    sentinel::net::PeerManager server(io_ctx, test_port);
    
    std::atomic<bool> message_received{false};
    sentinel::core::LockEventPayload received_payload{};

    server.set_message_handler([&](uint32_t peer_id, sentinel::core::Header header, std::vector<std::byte> payload) {
        (void)peer_id;
        if (header.opcode == static_cast<uint8_t>(sentinel::core::OpCode::LOCK_EVENT)) {
            auto hdr_bytes = sentinel::core::ProtocolEngine::serialize_header(header);
            std::vector<std::byte> full_packet(hdr_bytes.begin(), hdr_bytes.end());
            full_packet.insert(full_packet.end(), payload.begin(), payload.end());

            auto result = sentinel::core::ProtocolEngine::deserialize_lock_event(full_packet);
            if (result.has_value()) {
                received_payload = result.value();
                message_received = true;
            }
        }
    });

    server.start_acceptor();

    sentinel::net::PeerManager client(io_ctx, test_port + 1);
    bool connected = client.connect_to_peer("127.0.0.1", test_port, 100);
    EXPECT_TRUE(connected);

    sentinel::core::LockEventPayload original_payload{
        .source_chain_id = 1,
        .destination_chain_id = 137,
        .nonce = 777,
        .sender = {0x01},
        .recipient = {0x02},
        .amount = 99999
    };

    auto packet = sentinel::core::ProtocolEngine::serialize_lock_event(original_payload);
    client.broadcast(packet);

    int wait_ticks = 0;
    while (!message_received && wait_ticks < 50) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        wait_ticks++;
    }

    EXPECT_TRUE(message_received);
    EXPECT_EQ(received_payload.nonce, 777u);
    EXPECT_EQ(received_payload.amount, 99999u);

    client.stop();
    server.stop();
    work_guard.reset();
    io_ctx.stop();

    if (io_thread.joinable()) {
        io_thread.join();
    }
}
