// SPDX-License-Identifier: MIT
pragma solidity ^0.8.20;

contract BridgeSource {
    uint256 public immutable chainId;
    uint256 public nonce;

    event LockEvent(
        uint256 indexed sourceChainId,
        uint256 indexed destinationChainId,
        uint256 indexed nonce,
        address sender,
        address recipient,
        uint256 amount,
        bytes32 nullifier
    );

    constructor(uint256 _chainId) {
        chainId = _chainId;
    }

    function lockTokens(uint256 destinationChainId, address recipient, uint256 amount) external payable {
        require(amount > 0, "Amount must be > 0");
        uint256 currentNonce = nonce++;
        bytes32 nullifier = keccak256(
            abi.encodePacked(chainId, destinationChainId, currentNonce, msg.sender, recipient, amount)
        );

        emit LockEvent(chainId, destinationChainId, currentNonce, msg.sender, recipient, amount, nullifier);
    }
}
