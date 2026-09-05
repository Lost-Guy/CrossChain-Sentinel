// SPDX-License-Identifier: MIT
pragma solidity ^0.8.20;

contract BridgeDestination {
    uint256 public immutable chainId;
    mapping(bytes32 => bool) public processedNullifiers;

    event MintExecuted(
        address indexed recipient,
        uint256 amount,
        bytes32 indexed nullifier
    );

    constructor(uint256 _chainId) {
        chainId = _chainId;
    }

    function mintTokens(
        address recipient,
        uint256 amount,
        bytes32 nullifier,
        bytes memory signature
    ) external {
        require(!processedNullifiers[nullifier], "Nullifier already processed");
        require(signature.length == 64 || signature.length == 65, "Invalid signature length");

        processedNullifiers[nullifier] = true;
        emit MintExecuted(recipient, amount, nullifier);
    }

    function isNullifierSpent(bytes32 nullifier) external view returns (bool) {
        return processedNullifiers[nullifier];
    }
}
