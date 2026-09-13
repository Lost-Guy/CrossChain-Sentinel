# Crosschain-Sentinel: High-Performance C++20 P2P Cross-Chain Security & Replay Simulator

**`crosschain-sentinel`** is a high-performance C++20 P2P simulator designed to model decentralized cross-chain bridge architecture, relayer quorum networks, and adversarial attack vectors (such as proof replays, network fault injection, and split-brain double-spends).

Developed as a cybersecurity research project, it bridges low-level systems engineering (multi-threaded network I/O, lock-free datastructures, and binary serialization) with Web3 protocol security (EVM state machine equivalence and Ed25519 threshold multi-signatures).

---

## Architecture Overview

The system simulates an asynchronous cross-chain message-passing protocol operating across two independent blockchains (**Source Chain A** and **Destination Chain B**), coordinated by a decentralized network of off-chain relayer nodes and protected by an in-memory nullifier index.

```
+-----------------------------------------------------------------------------------+
|                                 SOURCE CHAIN (A)                                  |
|   +-----------------------+     +-------------------+     +-------------------+   |
|   | Account Balances      | --> |  Mempool Priority | --> | Block Producer    |   |
|   | (Bridge Lock Tx)      |     |  Queue (Max-Heap) |     | & Event Logs      |   |
|   +-----------------------+     +-------------------+     +-------------------+   |
+-------------------------------------------|---------------------------------------+
                                            | LockEvent (Nullifier)
                                            v
+-----------------------------------------------------------------------------------+
|                     DECENTRALIZED RELAYER NETWORK (M-of-N Quorum)                 |
|   +-----------------------+     +-------------------+     +-------------------+   |
|   | Event Observer Node 1 |     | Validator Node 2  | ... | Validator Node N  |   |
|   +-----------------------+     +-------------------+     +-------------------+   |
|               \                           |                           /           |
|                +--------------------------+--------------------------+            |
|                                           | Threshold Signature Collection        |
|                                           v                                       |
|                             [ Signature Verifier Engine ]                         |
+-------------------------------------------|---------------------------------------+
                                            | Mint Proof Submission
                                            v
+-----------------------------------------------------------------------------------+
|                               DESTINATION CHAIN (B)                               |
|   +-----------------------+     +-------------------+     +-------------------+   |
|   |  Mempool & Signature  | --> | Nullifier Index   | --> | State Transition  |   |
|   |  Validation           |     | (Replay Revert)   |     | (Mint Balance)    |   |
|   +-----------------------+     +-------------------+     +-------------------+   |
+-----------------------------------------------------------------------------------+

```

---

## Core Capabilities & Technical Features

### 1. Multi-Threaded P2P Networking (`Boost.Asio`)

* Asynchronous TCP socket management using native C++20 `std::jthread` and Boost.Asio event loops.
* Framing protocol using 32-bit CRC checksums, magic headers, and length-prefixed payload binary serialization.

### 2. Cryptographic Engine & EVM Parity (`libsodium` / `OpenSSL`)

* **Ed25519 Quorum Signatures:** Fast curve-based signing for relayer node attestations.
* **EVM Nullifier Equivalence:** Compute byte-for-byte identical $\text{Keccak256}$ nullifiers matching EVM `abi.encodePacked(...)` memory layouts for ground-truth EVM specification testing:

$$\text{Nullifier} = \text{Keccak256}\big(\text{abi.encodePacked}(\text{srcChainId}, \text{destChainId}, \text{nonce}, \text{sender}, \text{recipient}, \text{amount})\big)$$



### 3. Dual-Ledger State Machines & Prioritized Mempools

* **Ledger State:** Account balances, nonce tracking, and expended nullifier registries (`m_spent_nullifiers`).
* **Fee-Prioritized Mempool:** Thread-safe max-heap queue supporting dynamic gas/fee transaction prioritization.
* **Deterministic Block Producer:** Links cryptographic block headers, updates Merkle roots, and records cross-chain `LockEvent` logs.

### 4. Adversarial Harness & Fault Injector

* **Network Fault Injector:** Simulates dynamic latency jitter (exponential decay model), packet drop probabilities (Bernoulli trial), and split-brain network partitions.
* **Replay Attack Harness:** Intercepts valid lock payloads and attempts duplicate minting against the destination ledger.
* **Partition Double-Spend Simulator:** Attempts rogue relayer quorum submissions during simulated network partitions.

### 5. High-Throughput Benchmarking & Metrics

* Atomic counters and microsecond-precision high-resolution timers tracking TPS (Transactions Per Second), mean relay latency, P99 latency, and total replay rejections.

---

## EVM Reference Specification (Solidity / Foundry)

To validate simulator correctness against production EVM standard behavior, the repository includes native Solidity reference contracts in `contracts/`:

* **`BridgeSource.sol`**: Manages token locking on Chain A and emits `LockEvent` with `keccak256(abi.encodePacked(...))` nullifier calculations.
* **`BridgeDestination.sol`**: Maintains state mapping (`processedNullifiers`) and executes minting operations with strict double-spend checks.
* **`test/BridgeTest.t.sol`**: Ground-truth test suite executable with Foundry (`forge test`).

---

## Project Structure

```text
crosschain-sentinel/
├── CMakeLists.txt                 # Master CMake build configuration (C++20, Ninja, GTest)
├── foundry.toml                   # EVM Foundry suite configuration
├── README.md                      # Project documentation
├── contracts/                     # Solidity EVM ground-truth contracts
│   ├── BridgeSource.sol
│   └── BridgeDestination.sol
├── include/sentinel/              # Public C++ Architecture Headers
│   ├── adversary/                 # Fault injector & attack harness headers
│   ├── core/                      # Logger, Random Engine, Protocol headers
│   ├── crypto/                    # Crypto engine & signature verifier headers
│   ├── evm/                       # EVM nullifier parity verifier headers
│   ├── ledger/                    # Ledger state, Mempool, Chain state headers
│   ├── metrics/                   # Metrics collector & benchmark headers
│   ├── net/                       # Boost.Asio P2P peer manager headers
│   ├── relayer/                   # Relayer node & engine pool headers
│   └── utils/                     # Config parser headers
├── src/                           # Core Implementation Units
│   ├── main.cpp                   # Main driver executable
│   ├── adversary/                 # Network fault injector & attack execution
│   ├── core/                      # Event scheduler & binary protocol framing
│   ├── crypto/                    # Ed25519 & Keccak256 crypto wrappers
│   ├── evm/                       # C++ EVM abi.encodePacked hash parity
│   ├── ledger/                    # Mempool & block execution engine
│   ├── metrics/                   # Thread-safe metrics aggregator
│   ├── net/                       # Async TCP socket peer sessions
│   ├── relayer/                   # Threshold relayer consensus logic
│   └── utils/                     # Configuration parameters
├── test/                          # Solidity Foundry integration tests
│   └── BridgeTest.t.sol
└── tests/                         # C++ GoogleTest Suite
    ├── test_adversary.cpp         # Replay attack & partition unit tests
    ├── test_crypto.cpp            # Cryptographic & quorum test suite
    ├── test_evm_validation.cpp    # EVM nullifier byte-level parity test
    ├── test_ledger.cpp            # Dual-ledger state transition tests
    ├── test_logger.cpp            # Thread-safe logger initialization test
    ├── test_metrics.cpp           # Metrics collector unit tests
    ├── test_p2p_network.cpp       # Boost.Asio client/server handshake test
    ├── test_protocol.cpp          # Protocol binary serialization roundtrip test
    └── test_relayer.cpp           # End-to-end cross-chain relay test

```

---

## Prerequisites & Dependencies

### Arch Linux / Ubuntu Dependencies

| Dependency | Minimum Version | Usage |
| --- | --- | --- |
| **GCC / Clang** | GCC 11+ / Clang 13+ | C++20 standard compiler (`std::jthread`, `std::span`) |
| **CMake** | `>= 3.20` | Multi-platform build system |
| **Ninja** | `>= 1.10` | High-speed build execution |
| **spdlog** | `>= 1.9.0` | Thread-safe, multi-sink logging engine |
| **Boost** | `>= 1.75` | Asynchronous network I/O (`Boost.Asio`, `Boost.Thread`) |
| **OpenSSL** | `>= 1.1.1` | Keccak-256 / SHA3 cryptographic primitives |
| **libsodium** | `>= 1.0.18` | Ed25519 digital signature creation & verification |
| **GoogleTest** | `>= 1.10.0` | C++ unit testing framework |

#### Installation Command (Arch Linux)

```bash
sudo pacman -S gcc cmake ninja boost spdlog openssl libsodium googletest

```

#### Installation Command (Ubuntu 22.04+)

```bash
sudo apt-get update && sudo apt-get install -y \
    build-essential cmake ninja-build libspdlog-dev \
    libboost-system-dev libboost-thread-dev libssl-dev \
    libsodium-dev libgtest-dev

```

---

## Building and Running

### 1. Build using CMake and Ninja

```bash
# Generate build configuration
cmake -B build -G Ninja

# Compile binary and test targets
cmake --build build

```

### 2. Run Main Simulator Binary

```bash
./build/crosschain-sentinel

```

#### Example Output:

```text
======================================================================
   CROSSCHAIN-SENTINEL: P2P Cross-Chain Security & Replay Simulator   
======================================================================
[INFO] Starting Concurrency Benchmark: 4 worker threads, 250 transactions/thread...
[INFO] Chain [1] Block #1 committed with 1000 transactions.
[INFO] Chain [137] Block #1 committed with 1000 transactions.
[INFO] Executing Adversarial Replay Attack Test...
[WARN] Adversary launching REPLAY ATTACK: Replaying nullifier 5 times on Chain 137
[INFO] SUCCESS: Defense held. Nullifier index rejected replayed mint proofs.
----------------------------------------------------------------------
                  BENCHMARK & RESILIENCE REPORT                       
----------------------------------------------------------------------
 Total Simulated Lock Txs  : 1000
 Total Completed Mint Txs  : 1000
 Peak Lock Throughput      : 42150.22 tx/sec
 Peak Mint Throughput      : 38910.15 tx/sec
 Mean Relay Processing Time: 0.1245 ms
----------------------------------------------------------------------
CROSSCHAIN-SENTINEL Simulation Execution Finished Successfully.

```

### 3. Run GoogleTest Suite (CTest)

```bash
ctest --test-dir build --output-on-failure

```

---

## Threat Model & Security Validations

| Threat Vector | Attack Mechanism | Simulator Defense Strategy | Verification Test |
| --- | --- | --- | --- |
| **Proof Replay Attack** | Intercepting a valid `LockEvent` proof and resubmitting it to Chain B multiple times. | Storage of spent nullifiers in `LedgerState::m_spent_nullifiers`. Second execution attempt returns `false` and drops transaction. | `AdversaryTest.ReplayAttackBlockedByNullifier` |
| **Split-Brain Double Mint** | Partitioning relayers to collect quorum on isolated state branches before state settles. | Threshold multi-sig validation (`SignatureVerifier::verify_quorum`) combined with global nullifier deduplication. | `AdversaryTest.PartitionDoubleSpendDefense` |
| **Payload Tampering** | Modifying recipient address or amount during P2P transit. | CRC32 binary framing verification on `ProtocolEngine` layer; signature verification failure on relayer validation. | `ProtocolTest.ChecksumTamperingDetection` |
| **Mempool Front-Running** | Submitting duplicate transactions with higher fees. | Seen transaction hash registry (`Mempool::m_seen_tx_hashes`) rejects duplicate hashes before queuing. | `LedgerTest.BridgeLockAndDoubleMintRejection` |

---

## Optional: Running EVM Reference Contracts (Foundry)

If Foundry is installed, initialize the `forge-std` testing library and execute the Solidity reference contract tests:

```bash
# Install Foundry CLI (if not already installed)
curl -L [https://foundry.paradigm.xyz](https://foundry.paradigm.xyz) | bash
foundryup

# Install forge-std dependency library into lib/
forge install foundry-rs/forge-std --no-commit

# Run EVM test suite with call traces
forge test -vvv

```
