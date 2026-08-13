#include "sentinel/net/peer_session.hpp"
#include "sentinel/core/logger.hpp"

namespace sentinel::net {

PeerSession::PeerSession(uint32_t peer_id, boost::asio::ip::tcp::socket socket, MessageHandler msg_handler, DisconnectHandler disc_handler)
    : m_peer_id(peer_id),
      m_socket(std::move(socket)),
      m_msg_handler(std::move(msg_handler)),
      m_disc_handler(std::move(disc_handler)),
      m_is_connected(true) {}

void PeerSession::start() {
    do_read_header();
}

void PeerSession::stop() {
    if (!m_is_connected) return;
    m_is_connected = false;
    boost::system::error_code ec;
    m_socket.close(ec);
    if (m_disc_handler) {
        m_disc_handler(m_peer_id);
    }
}

void PeerSession::send(std::vector<std::byte> packet) {
    if (!m_is_connected) return;

    bool write_in_progress = false;
    {
        std::lock_guard<std::mutex> lock(m_write_mutex);
        m_write_queue.push(std::move(packet));
        write_in_progress = m_is_writing;
        if (!m_is_writing) {
            m_is_writing = true;
        }
    }

    if (!write_in_progress) {
        do_write();
    }
}

void PeerSession::do_read_header() {
    auto self = shared_from_this();
    boost::asio::async_read(
        m_socket,
        boost::asio::buffer(m_header_buffer.data(), sentinel::core::HEADER_SIZE),
        [this, self](const boost::system::error_code& ec, std::size_t /*bytes_transferred*/) {
            if (ec) {
                if (m_is_connected) {
                    LOG_DEBUG("Peer [{}] disconnected reading header: {}", m_peer_id, ec.message());
                    stop();
                }
                return;
            }

            std::span<const std::byte, sentinel::core::HEADER_SIZE> header_span(m_header_buffer.data(), sentinel::core::HEADER_SIZE);
            sentinel::core::Header header = sentinel::core::ProtocolEngine::parse_header(header_span);

            if (header.magic != sentinel::core::PROTOCOL_MAGIC || header.version != sentinel::core::PROTOCOL_VERSION) {
                LOG_WARN("Peer [{}] sent invalid protocol magic/version. Dropping.", m_peer_id);
                stop();
                return;
            }

            if (header.payload_len > 10 * 1024 * 1024) { // 10MB sanity cap
                LOG_WARN("Peer [{}] payload size {} exceeds limit. Dropping.", m_peer_id, header.payload_len);
                stop();
                return;
            }

            if (header.payload_len == 0) {
                if (m_msg_handler) {
                    m_msg_handler(m_peer_id, header, {});
                }
                do_read_header();
            } else {
                do_read_body(header);
            }
        }
    );
}

void PeerSession::do_read_body(sentinel::core::Header header) {
    auto self = shared_from_this();
    m_body_buffer.resize(header.payload_len);

    boost::asio::async_read(
        m_socket,
        boost::asio::buffer(m_body_buffer.data(), header.payload_len),
        [this, self, header](const boost::system::error_code& ec, std::size_t /*bytes_transferred*/) {
            if (ec) {
                if (m_is_connected) {
                    LOG_DEBUG("Peer [{}] disconnected reading body: {}", m_peer_id, ec.message());
                    stop();
                }
                return;
            }

            if (m_msg_handler) {
                m_msg_handler(m_peer_id, header, m_body_buffer);
            }

            do_read_header();
        }
    );
}

void PeerSession::do_write() {
    auto self = shared_from_this();
    std::vector<std::byte>* current_msg = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_write_mutex);
        if (m_write_queue.empty()) {
            m_is_writing = false;
            return;
        }
        current_msg = &m_write_queue.front();
    }

    boost::asio::async_write(
        m_socket,
        boost::asio::buffer(current_msg->data(), current_msg->size()),
        [this, self](const boost::system::error_code& ec, std::size_t /*bytes_transferred*/) {
            if (ec) {
                LOG_DEBUG("Peer [{}] write error: {}", m_peer_id, ec.message());
                stop();
                return;
            }

            {
                std::lock_guard<std::mutex> lock(m_write_mutex);
                m_write_queue.pop();
            }

            do_write();
        }
    );
}

} // namespace sentinel::net
