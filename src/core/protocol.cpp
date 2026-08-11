#include "sentinel/core/protocol.hpp"
#include <cstring>
#include <zlib.h> // CRC32 implementation

namespace sentinel::core {

namespace {

void write_u64_be(std::vector<std::byte>& buffer, uint64_t val) {
    for (int i = 7; i >= 0; --i) {
        buffer.push_back(static_cast<std::byte>((val >> (i * 8)) & 0xFF));
    }
}

uint64_t read_u64_be(std::span<const std::byte>& stream) {
    uint64_t val = 0;
    for (size_t i = 0; i < 8; ++i) {
        val = (val << 8) | static_cast<uint64_t>(stream[i]);
    }
    stream = stream.subspan(8);
    return val;
}

void write_bytes(std::vector<std::byte>& buffer, std::span<const uint8_t> data) {
    for (uint8_t b : data) {
        buffer.push_back(static_cast<std::byte>(b));
    }
}

void read_bytes(std::span<const std::byte>& stream, std::span<uint8_t> out_buffer) {
    for (size_t i = 0; i < out_buffer.size(); ++i) {
        out_buffer[i] = static_cast<uint8_t>(stream[i]);
    }
    stream = stream.subspan(out_buffer.size());
}

} // anonymous namespace

uint32_t ProtocolEngine::compute_checksum(std::span<const std::byte> data) {
    uLong crc = crc32(0L, Z_NULL, 0);
    return static_cast<uint32_t>(crc32(crc, reinterpret_cast<const Bytef*>(data.data()), static_cast<uInt>(data.size())));
}

Header ProtocolEngine::parse_header(std::span<const std::byte, HEADER_SIZE> header_bytes) {
    Header header{};
    std::memcpy(&header, header_bytes.data(), HEADER_SIZE);
    return header;
}

std::array<std::byte, HEADER_SIZE> ProtocolEngine::serialize_header(const Header& header) {
    std::array<std::byte, HEADER_SIZE> out{};
    std::memcpy(out.data(), &header, HEADER_SIZE);
    return out;
}

std::vector<std::byte> ProtocolEngine::serialize_lock_event(const LockEventPayload& payload) {
    std::vector<std::byte> body;
    body.reserve(8 + 8 + 8 + 32 + 32 + 8);

    write_u64_be(body, payload.source_chain_id);
    write_u64_be(body, payload.destination_chain_id);
    write_u64_be(body, payload.nonce);
    write_bytes(body, payload.sender);
    write_bytes(body, payload.recipient);
    write_u64_be(body, payload.amount);

    Header hdr{
        .magic = PROTOCOL_MAGIC,
        .version = PROTOCOL_VERSION,
        .opcode = static_cast<uint8_t>(OpCode::LOCK_EVENT),
        .payload_len = static_cast<uint32_t>(body.size()),
        .checksum = compute_checksum(body)
    };

    auto hdr_bytes = serialize_header(hdr);
    std::vector<std::byte> packet(hdr_bytes.begin(), hdr_bytes.end());
    packet.insert(packet.end(), body.begin(), body.end());
    return packet;
}

std::optional<LockEventPayload> ProtocolEngine::deserialize_lock_event(std::span<const std::byte> buffer) {
    if (buffer.size() < HEADER_SIZE) return std::nullopt;

    auto hdr_bytes = buffer.subspan<0, HEADER_SIZE>();
    Header hdr = parse_header(hdr_bytes);

    if (hdr.magic != PROTOCOL_MAGIC || hdr.version != PROTOCOL_VERSION) return std::nullopt;
    if (hdr.opcode != static_cast<uint8_t>(OpCode::LOCK_EVENT)) return std::nullopt;

    auto payload_stream = buffer.subspan(HEADER_SIZE);
    if (payload_stream.size() < hdr.payload_len) return std::nullopt;

    uint32_t calculated_checksum = compute_checksum(payload_stream.first(hdr.payload_len));
    if (calculated_checksum != hdr.checksum) return std::nullopt;

    LockEventPayload payload{};
    payload.source_chain_id = read_u64_be(payload_stream);
    payload.destination_chain_id = read_u64_be(payload_stream);
    payload.nonce = read_u64_be(payload_stream);
    read_bytes(payload_stream, payload.sender);
    read_bytes(payload_stream, payload.recipient);
    payload.amount = read_u64_be(payload_stream);

    return payload;
}

std::vector<std::byte> ProtocolEngine::serialize_mint_proof(const MintProofPayload& payload) {
    std::vector<std::byte> body;
    body.reserve(8 + 8 + 8 + 32 + 8 + 64);

    write_u64_be(body, payload.source_chain_id);
    write_u64_be(body, payload.destination_chain_id);
    write_u64_be(body, payload.nonce);
    write_bytes(body, payload.recipient);
    write_u64_be(body, payload.amount);
    write_bytes(body, payload.signature);

    Header hdr{
        .magic = PROTOCOL_MAGIC,
        .version = PROTOCOL_VERSION,
        .opcode = static_cast<uint8_t>(OpCode::MINT_PROOF),
        .payload_len = static_cast<uint32_t>(body.size()),
        .checksum = compute_checksum(body)
    };

    auto hdr_bytes = serialize_header(hdr);
    std::vector<std::byte> packet(hdr_bytes.begin(), hdr_bytes.end());
    packet.insert(packet.end(), body.begin(), body.end());
    return packet;
}

std::optional<MintProofPayload> ProtocolEngine::deserialize_mint_proof(std::span<const std::byte> buffer) {
    if (buffer.size() < HEADER_SIZE) return std::nullopt;

    auto hdr_bytes = buffer.subspan<0, HEADER_SIZE>();
    Header hdr = parse_header(hdr_bytes);

    if (hdr.magic != PROTOCOL_MAGIC || hdr.version != PROTOCOL_VERSION) return std::nullopt;
    if (hdr.opcode != static_cast<uint8_t>(OpCode::MINT_PROOF)) return std::nullopt;

    auto payload_stream = buffer.subspan(HEADER_SIZE);
    if (payload_stream.size() < hdr.payload_len) return std::nullopt;

    uint32_t calculated_checksum = compute_checksum(payload_stream.first(hdr.payload_len));
    if (calculated_checksum != hdr.checksum) return std::nullopt;

    MintProofPayload payload{};
    payload.source_chain_id = read_u64_be(payload_stream);
    payload.destination_chain_id = read_u64_be(payload_stream);
    payload.nonce = read_u64_be(payload_stream);
    read_bytes(payload_stream, payload.recipient);
    payload.amount = read_u64_be(payload_stream);
    read_bytes(payload_stream, payload.signature);

    return payload;
}

} // namespace sentinel::core
