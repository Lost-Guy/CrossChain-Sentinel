#ifndef SENTINEL_CORE_PROTOCOL_HPP
#define SENTINEL_CORE_PROTOCOL_HPP

#include <cstdint>
#include <vector>
#include <span>
#include <cstddef>
#include <array>
#include <string>
#include <optional>

namespace sentinel::core {

// Protocol Magic Identifier "SN" (Sentinel)
constexpr uint16_t PROTOCOL_MAGIC = 0x534E;
constexpr uint8_t PROTOCOL_VERSION = 0x01;
constexpr size_t HEADER_SIZE = 12;

enum class OpCode : uint8_t {
    HANDSHAKE_REQ = 0x01,
    HANDSHAKE_RESP = 0x02,
    HEARTBEAT     = 0x03,
    LOCK_EVENT    = 0x10,
    MINT_PROOF    = 0x11,
    UNKNOWN       = 0xFF
};

#pragma pack(push, 1)
struct Header {
    uint16_t magic;
    uint8_t version;
    uint8_t opcode;
    uint32_t payload_len;
    uint32_t checksum;
};
#pragma pack(pop)

struct LockEventPayload {
    uint64_t source_chain_id;
    uint64_t destination_chain_id;
    uint64_t nonce;
    std::array<uint8_t, 32> sender;
    std::array<uint8_t, 32> recipient;
    uint64_t amount;
};

struct MintProofPayload {
    uint64_t source_chain_id;
    uint64_t destination_chain_id;
    uint64_t nonce;
    std::array<uint8_t, 32> recipient;
    uint64_t amount;
    std::array<uint8_t, 64> signature; // Ed25519 signature
};

class ProtocolEngine {
public:
    static uint32_t compute_checksum(std::span<const std::byte> data);

    static std::vector<std::byte> serialize_lock_event(const LockEventPayload& payload);
    static std::optional<LockEventPayload> deserialize_lock_event(std::span<const std::byte> buffer);

    static std::vector<std::byte> serialize_mint_proof(const MintProofPayload& payload);
    static std::optional<MintProofPayload> deserialize_mint_proof(std::span<const std::byte> buffer);

    static Header parse_header(std::span<const std::byte, HEADER_SIZE> header_bytes);
    static std::array<std::byte, HEADER_SIZE> serialize_header(const Header& header);
};

} // namespace sentinel::core

#endif // SENTINEL_CORE_PROTOCOL_HPP
