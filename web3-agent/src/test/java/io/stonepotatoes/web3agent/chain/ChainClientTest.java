package io.stonepotatoes.web3agent.chain;

import io.stonepotatoes.web3agent.FakeEvmNode;
import io.stonepotatoes.web3agent.wallet.AgentWallet;
import org.junit.jupiter.api.Test;

import java.math.BigInteger;
import java.time.Clock;
import java.time.Instant;
import java.time.ZoneOffset;
import java.util.concurrent.atomic.AtomicBoolean;

import static org.junit.jupiter.api.Assertions.*;

class ChainClientTest {
    private static final String TO = "0x70997970c51812dc3a010c7d01b50e0d17dc79c8";
    private static final BigInteger ONE_ETH = BigInteger.TEN.pow(18);

    @Test
    void readsChainState() throws Exception {
        try (FakeEvmNode evm = new FakeEvmNode(); ChainClient chain = new ChainClient(evm.url())) {
            assertEquals(FakeEvmNode.CHAIN_ID, chain.chainId());
            assertEquals(evm.balance, chain.balanceWei(TO));
            assertEquals(evm.tokenBalance, chain.erc20Balance(TO, TO));
            assertThrows(IllegalArgumentException.class, () -> chain.balanceWei("0xnotanaddress"));
        }
    }

    @Test
    void sendsSignedEip155TransferWithinPolicy() throws Exception {
        AgentWallet wallet = AgentWallet.random();
        SpendPolicy policy = new SpendPolicy(ONE_ETH, ONE_ETH.multiply(BigInteger.TWO), (to, v) -> true);
        try (FakeEvmNode evm = new FakeEvmNode(); ChainClient chain = new ChainClient(evm.url())) {
            String hash = chain.sendEther(wallet, TO, ONE_ETH, policy);
            assertEquals("0x" + "ab".repeat(32), hash);
            FakeEvmNode.SentTx tx = evm.sent.getFirst();
            assertEquals(wallet.address(), tx.from());
            assertEquals(TO, tx.to());
            assertEquals(ONE_ETH, tx.value());
            assertEquals(FakeEvmNode.CHAIN_ID, tx.chainId());
            assertEquals(ONE_ETH, policy.remainingTodayWei());
        }
    }

    @Test
    void policyBlocksOverLimitAndRejectedTransfers() throws Exception {
        AtomicBoolean approve = new AtomicBoolean(true);
        SpendPolicy policy = new SpendPolicy(ONE_ETH, ONE_ETH.add(ONE_ETH.divide(BigInteger.TWO)), (to, v) -> approve.get());
        try (FakeEvmNode evm = new FakeEvmNode(); ChainClient chain = new ChainClient(evm.url())) {
            AgentWallet w = AgentWallet.random();
            assertThrows(SpendPolicy.PolicyViolationException.class, () -> chain.sendEther(w, TO, ONE_ETH.add(BigInteger.ONE), policy));
            chain.sendEther(w, TO, ONE_ETH, policy);
            assertThrows(SpendPolicy.PolicyViolationException.class, () -> chain.sendEther(w, TO, ONE_ETH, policy));
            approve.set(false);
            assertThrows(SpendPolicy.PolicyViolationException.class, () -> chain.sendEther(w, TO, BigInteger.ONE, policy));
            assertEquals(1, evm.sent.size());
        }
    }

    @Test
    void dailyLimitResetsNextDay() throws Exception {
        MutableClock clock = new MutableClock(Instant.parse("2026-01-01T12:00:00Z"));
        SpendPolicy policy = new SpendPolicy(ONE_ETH, ONE_ETH, (to, v) -> true, clock);
        policy.authorize(TO, ONE_ETH);
        policy.record(ONE_ETH);
        assertThrows(SpendPolicy.PolicyViolationException.class, () -> policy.authorize(TO, BigInteger.ONE));
        clock.now = Instant.parse("2026-01-02T00:00:01Z");
        policy.authorize(TO, ONE_ETH);
    }

    private static final class MutableClock extends Clock {
        Instant now;

        MutableClock(Instant now) {
            this.now = now;
        }

        @Override
        public ZoneOffset getZone() {
            return ZoneOffset.UTC;
        }

        @Override
        public Clock withZone(java.time.ZoneId zone) {
            return this;
        }

        @Override
        public Instant instant() {
            return now;
        }
    }
}
