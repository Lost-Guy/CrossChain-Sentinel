#include "sentinel/net/peer_manager.hpp"
#include "sentinel/core/logger.hpp"

namespace sentinel::net {

PeerManager::PeerManager(boost::asio::io_context& io_ctx, uint16_t listen_port)
    : m_io_ctx(io_ctx),
      m_acceptor(io_ctx, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), listen_port)) {}

PeerManager::~PeerManager() {
    stop();
}

void PeerManager::start_acceptor() {
    do_accept();
}

void PeerManager::do_accept() {
    m_acceptor.async_accept(
        [this](const boost::system::error_code& ec, boost::asio::ip::tcp::socket socket) {
            if (!ec) {
                uint32_t assigned_id = m_next_peer_id++;
                LOG_INFO("Accepted new incoming peer connection [ID: {}]", assigned_id);

                auto session = PeerSession::create(
                    assigned_id,
                    std::move(socket),
                    m_msg_handler,
                    [this](uint32_t pid) { remove_peer(pid); }
                );

                {
                    std::lock_guard<std::mutex> lock(m_peers_mutex);
                    m_active_peers[assigned_id] = session;
                }

                session->start();
            } else {
                LOG_DEBUG("Acceptor stopped or error: {}", ec.message());
            }

            if (m_acceptor.is_open()) {
                do_accept();
            }
        }
    );
}

bool PeerManager::connect_to_peer(const std::string& host, uint16_t port, uint32_t remote_peer_id) {
    try {
        boost::asio::ip::tcp::resolver resolver(m_io_ctx);
        auto endpoints = resolver.resolve(host, std::to_string(port));
        boost::asio::ip::tcp::socket socket(m_io_ctx);
        boost::asio::connect(socket, endpoints);

        LOG_INFO("Successfully connected outbound to peer [{}] at {}:{}", remote_peer_id, host, port);

        auto session = PeerSession::create(
            remote_peer_id,
            std::move(socket),
            m_msg_handler,
            [this](uint32_t pid) { remove_peer(pid); }
        );

        {
            std::lock_guard<std::mutex> lock(m_peers_mutex);
            m_active_peers[remote_peer_id] = session;
        }

        session->start();
        return true;
    } catch (const std::exception& e) {
        LOG_WARN("Failed outbound connection to peer [{}]: {}", remote_peer_id, e.what());
        return false;
    }
}

void PeerManager::broadcast(const std::vector<std::byte>& packet) {
    std::lock_guard<std::mutex> lock(m_peers_mutex);
    for (const auto& [id, session] : m_active_peers) {
        if (session && session->is_connected()) {
            session->send(packet);
        }
    }
}

void PeerManager::send_to(uint32_t peer_id, const std::vector<std::byte>& packet) {
    std::lock_guard<std::mutex> lock(m_peers_mutex);
    auto it = m_active_peers.find(peer_id);
    if (it != m_active_peers.end() && it->second && it->second->is_connected()) {
        it->second->send(packet);
    }
}

void PeerManager::remove_peer(uint32_t peer_id) {
    std::lock_guard<std::mutex> lock(m_peers_mutex);
    m_active_peers.erase(peer_id);
    LOG_INFO("Removed peer [{}] from active peer manager queue.", peer_id);
}

size_t PeerManager::active_peer_count() const {
    std::lock_guard<std::mutex> lock(m_peers_mutex);
    return m_active_peers.size();
}

void PeerManager::stop() {
    boost::system::error_code ec;
    m_acceptor.close(ec);

    std::unordered_map<uint32_t, PeerSession::pointer> peers_to_stop;
    {
        std::lock_guard<std::mutex> lock(m_peers_mutex);
        peers_to_stop = m_active_peers;
        m_active_peers.clear();
    }

    for (auto& [id, session] : peers_to_stop) {
        if (session) {
            session->stop();
        }
    }
}

} // namespace sentinel::net
