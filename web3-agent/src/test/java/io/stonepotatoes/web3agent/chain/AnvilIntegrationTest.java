package io.stonepotatoes.web3agent.chain;

import io.stonepotatoes.web3agent.wallet.AgentWallet;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.condition.EnabledIf;
import org.web3j.utils.Convert;

import java.math.BigDecimal;
import java.math.BigInteger;
import java.time.Duration;

import static org.junit.jupiter.api.Assertions.*;

/**
 * Integration test against a real Anvil instance. Requires Anvil running on
 * http://127.0.0.1:8545. Enabled only when the environment variable ANVIL_TEST=1 is set.
 */
@EnabledIf("anvilRunning")
class AnvilIntegrationTest {
    static final String ANVIL_URL = "http://127.0.0.1:8545";
    static final String ANVIL_FUNDER = "0xf39Fd6e51aad88F6F4ce6aB8827279cffFb92266";
    static final String ANVIL_PRIVKEY = "0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80";

    static boolean anvilRunning() {
        return "1".equals(System.getenv("ANVIL_TEST"));
    }

    @Test
    void readsChainStateFromRealAnvil() throws Exception {
        try (ChainClient chain = new ChainClient(ANVIL_URL)) {
            assertEquals(31337L, chain.chainId());
            assertTrue(chain.blockNumber().compareTo(BigInteger.ZERO) >= 0);
            BigInteger bal = chain.balanceWei(ANVIL_FUNDER);
            assertTrue(bal.compareTo(BigInteger.ZERO) > 0);
        }
    }

    @Test
    void deploysAndQueriesAgentToken() throws Exception {
        AgentWallet wallet = AgentWallet.fromPrivateKey(ANVIL_PRIVKEY);
        BigInteger supply = BigInteger.TEN.pow(18).multiply(BigInteger.valueOf(1_000_000)); // 1M tokens
        SpendPolicy policy = new SpendPolicy(BigInteger.ZERO, BigInteger.ZERO, desc -> true);
        try (ChainClient chain = new ChainClient(ANVIL_URL)) {
            // Deploy
            String txHash = chain.deployAgentToken(wallet, "TestCoin", "TST", supply, policy);
            assertNotNull(txHash);
            assertTrue(txHash.startsWith("0x"));

            // Wait for receipt
            var receipt = chain.waitForReceipt(txHash, Duration.ofSeconds(30));
            assertTrue(receipt.isPresent());
            assertTrue(receipt.get().success());
            String tokenAddr = receipt.get().contractAddress();
            assertNotNull(tokenAddr);

            // Query info
            ChainClient.TokenInfo info = chain.erc20Info(tokenAddr);
            assertEquals("TST", info.symbol());
            assertEquals(18, info.decimals());

            // Query balance (deployer owns all supply)
            BigInteger bal = chain.erc20Balance(tokenAddr, wallet.address());
            assertEquals(supply, bal);
        }
    }

    @Test
    void sendsEthOnRealAnvil() throws Exception {
        AgentWallet wallet = AgentWallet.fromPrivateKey(ANVIL_PRIVKEY);
        AgentWallet recipient = AgentWallet.random();
        BigInteger amount = Convert.toWei(BigDecimal.ONE, Convert.Unit.ETHER).toBigInteger();
        SpendPolicy policy = new SpendPolicy(amount, amount.multiply(BigInteger.TEN), desc -> true);
        try (ChainClient chain = new ChainClient(ANVIL_URL)) {
            String txHash = chain.sendEther(wallet, recipient.address(), amount, policy);
            assertNotNull(txHash);
            assertTrue(txHash.startsWith("0x"));

            var receipt = chain.waitForReceipt(txHash, Duration.ofSeconds(30));
            assertTrue(receipt.isPresent());
            assertTrue(receipt.get().success());

            BigInteger recipientBal = chain.balanceWei(recipient.address());
            assertEquals(amount, recipientBal);
        }
    }
}
