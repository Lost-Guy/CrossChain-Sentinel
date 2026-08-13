#ifndef SENTINEL_NET_PEER_SESSION_HPP
#define SENTINEL_NET_PEER_SESSION_HPP

#include <cstdint>
#include <memory>
#include <vector>
#include <queue>
#include <span>
#include <functional>
#include <mutex>
#include <boost/asio.hpp>
#include "sentinel/core/protocol.hpp"

namespace sentinel::net {

using MessageHandler = std::function<void(uint32_t peer_id, sentinel::core::Header header, std::vector<std::byte> payload)>;
using DisconnectHandler = std::function<void(uint32_t peer_id)>;

class PeerSession : public std::enable_shared_from_this<PeerSession> {
public:
    using pointer = std::shared_ptr<PeerSession>;

    PeerSession(uint32_t peer_id, boost::asio::ip::tcp::socket socket, MessageHandler msg_handler, DisconnectHandler disc_handler);
    ~PeerSession() = default;

    static pointer create(uint32_t peer_id, boost::asio::ip::tcp::socket socket, MessageHandler msg_handler, DisconnectHandler disc_handler) {
        return std::make_shared<PeerSession>(peer_id, std::move(socket), std::move(msg_handler), std::move(disc_handler));
    }

    void start();
    void send(std::vector<std::byte> packet);
    void stop();

    [[nodiscard]] uint32_t id() const { return m_peer_id; }
    [[nodiscard]] bool is_connected() const { return m_is_connected; }

private:
    void do_read_header();
    void do_read_body(sentinel::core::Header header);
    void do_write();

    uint32_t m_peer_id;
    boost::asio::ip::tcp::socket m_socket;
    MessageHandler m_msg_handler;
    DisconnectHandler m_disc_handler;

    std::array<std::byte, sentinel::core::HEADER_SIZE> m_header_buffer{};
    std::vector<std::byte> m_body_buffer{};

    std::queue<std::vector<std::byte>> m_write_queue;
    std::mutex m_write_mutex;
    bool m_is_writing{false};
    bool m_is_connected{false};
};

} // namespace sentinel::net

#endif // SENTINEL_NET_PEER_SESSION_HPP
