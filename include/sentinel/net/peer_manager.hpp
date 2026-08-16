#ifndef SENTINEL_NET_PEER_MANAGER_HPP
#define SENTINEL_NET_PEER_MANAGER_HPP

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <string>
#include <functional>
#include <boost/asio.hpp>
#include "sentinel/net/peer_session.hpp"

namespace sentinel::net {

class PeerManager {
public:
    PeerManager(boost::asio::io_context& io_ctx, uint16_t listen_port);
    ~PeerManager();

    void start_acceptor();
    bool connect_to_peer(const std::string& host, uint16_t port, uint32_t remote_peer_id);
    void broadcast(const std::vector<std::byte>& packet);
    void send_to(uint32_t peer_id, const std::vector<std::byte>& packet);

    void set_message_handler(MessageHandler handler) { m_msg_handler = std::move(handler); }
    [[nodiscard]] size_t active_peer_count() const;
    void stop();

private:
    void do_accept();
    void remove_peer(uint32_t peer_id);

    boost::asio::io_context& m_io_ctx;
    boost::asio::ip::tcp::acceptor m_acceptor;
    uint32_t m_next_peer_id{1};

    std::unordered_map<uint32_t, PeerSession::pointer> m_active_peers;
    mutable std::mutex m_peers_mutex;

    MessageHandler m_msg_handler;
};

} // namespace sentinel::net

#endif // SENTINEL_NET_PEER_MANAGER_HPP
