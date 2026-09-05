// SPDX-License-Identifier: MIT
pragma solidity ^0.8.20;

import "forge-std/Test.sol";
import "../contracts/BridgeSource.sol";
import "../contracts/BridgeDestination.sol";

contract BridgeTest is Test {
    BridgeSource public source;
    BridgeDestination public destination;

    address public alice = address(0x1);
    address public bob = address(0x2);

    function setUp() public {
        source = new BridgeSource(1);
        destination = new BridgeDestination(137);
    }

    function testLockAndMintFlow() public {
        vm.prank(alice);
        source.lockTokens(137, bob, 1000);

        bytes32 nullifier = keccak256(
            abi.encodePacked(uint256(1), uint256(137), uint256(0), alice, bob, uint256(1000))
        );

        bytes memory dummySig = new bytes(64);
        destination.mintTokens(bob, 1000, nullifier, dummySig);

        assertTrue(destination.isNullifierSpent(nullifier));
    }

    function testReplayMintRejection() public {
        vm.prank(alice);
        source.lockTokens(137, bob, 1000);

        bytes32 nullifier = keccak256(
            abi.encodePacked(uint256(1), uint256(137), uint256(0), alice, bob, uint256(1000))
        );

        bytes memory dummySig = new bytes(64);
        destination.mintTokens(bob, 1000, nullifier, dummySig);

        vm.expectRevert("Nullifier already processed");
        destination.mintTokens(bob, 1000, nullifier, dummySig);
    }
}
