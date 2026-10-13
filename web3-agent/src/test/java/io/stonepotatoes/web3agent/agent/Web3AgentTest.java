package io.stonepotatoes.web3agent.agent;

import io.stonepotatoes.web3agent.FakeEvmNode;
import io.stonepotatoes.web3agent.chain.ChainClient;
import io.stonepotatoes.web3agent.chain.SpendPolicy;
import io.stonepotatoes.web3agent.llm.ChatMessage;
import io.stonepotatoes.web3agent.llm.LlmProvider;
import io.stonepotatoes.web3agent.llm.MockLlmProvider;
import io.stonepotatoes.web3agent.p2p.ContentStore;
import io.stonepotatoes.web3agent.p2p.PeerNode;
import io.stonepotatoes.web3agent.util.Json;
import io.stonepotatoes.web3agent.wallet.AgentWallet;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.math.BigInteger;
import java.nio.charset.StandardCharsets;
import java.nio.file.Path;
import java.util.ArrayDeque;
import java.util.Deque;
import java.util.List;

import static io.stonepotatoes.web3agent.TestUtil.await;
import static org.junit.jupiter.api.Assertions.*;

class Web3AgentTest {
    private static final String TO = "0x70997970c51812dc3a010c7d01b50e0d17dc79c8";
    private static final BigInteger ONE_ETH = BigInteger.TEN.pow(18);

    @TempDir
    Path dir;

    @Test
    void mockLlmDrivesWalletChainAndStoreTools() throws Exception {
        AgentWallet wallet = AgentWallet.random();
        try (FakeEvmNode evm = new FakeEvmNode(); ChainClient chain = new ChainClient(evm.url())) {
            SpendPolicy policy = new SpendPolicy(ONE_ETH, ONE_ETH, desc -> true);
            ContentStore store = new ContentStore(dir);
            Web3Agent agent = new Web3Agent(new MockLlmProvider(), Web3Tools.create(wallet, chain, policy, null, store), 4);

            assertEquals(wallet.address(), agent.chat("what is my address?"));
            assertEquals("5 ETH", agent.chat("check my balance"));
            assertFalse(agent.chat("token 0x5fbdb2315678afecb367f032d93f642f64180aa3").contains("ERROR"));
            assertTrue(agent.chat("chain info").contains("chainId=31337"));

            String sig = agent.chat("sign hello agent");
            assertTrue(AgentWallet.verify("hello agent".getBytes(StandardCharsets.UTF_8), sig, wallet.address()));

            String cid = agent.chat("store decentralized memo");
            assertTrue(ContentStore.isValidCid(cid));
            assertEquals("decentralized memo", agent.chat("get " + cid));

            assertEquals("tx=0x" + "ab".repeat(32), agent.chat("send 0.25 eth to " + TO));
            assertEquals(ONE_ETH.divide(BigInteger.valueOf(4)), evm.sent.getFirst().value());
            assertTrue(agent.chat("send 5 eth to " + TO).startsWith("ERROR: Amount exceeds per-transaction limit"));
            assertEquals(1, evm.sent.size());
        }
    }

    @Test
    void contentSharedOverP2pIsReplicatedAndVerified() throws Exception {
        AgentWallet wa = AgentWallet.random(), wb = AgentWallet.random();
        try (PeerNode a = new PeerNode(wa, "127.0.0.1", 0); PeerNode b = new PeerNode(wb, "127.0.0.1", 0)) {
            ContentStore storeA = new ContentStore(dir.resolve("a")), storeB = new ContentStore(dir.resolve("b"));
            Web3Agent agentA = new Web3Agent(new MockLlmProvider(), Web3Tools.create(wa, null, null, a, storeA), 4);
            Web3Agent agentB = new Web3Agent(new MockLlmProvider(), Web3Tools.create(wb, null, null, b, storeB), 4);
            a.connect("127.0.0.1", b.port());
            await(() -> a.peers().size() == 1 && b.peers().size() == 1);
            assertEquals(wa.address(), agentB.chat("list peers"));

            String cid = agentA.chat("store shared knowledge");
            assertTrue(agentA.chat("share " + cid).startsWith("shared id="));
            await(() -> {
                try {
                    return storeB.get(cid).isPresent();
                } catch (Exception e) {
                    return false;
                }
            });
            assertEquals("shared knowledge", agentB.chat(cid));

            agentA.chat("broadcast hello network");
            String[] inbox = {""};
            await(() -> {
                try {
                    return (inbox[0] += agentB.chat("inbox")).contains("hello network");
                } catch (Exception e) {
                    return false;
                }
            });
            assertTrue(inbox[0].contains(wa.address()));
        }
    }

    @Test
    void handlesProseWrappedJsonUnknownToolsAndStepLimit() throws Exception {
        Deque<String> script = new ArrayDeque<>(List.of(
                "Sure! {\"tool\":\"nope\",\"args\":{}}",
                "Let me check. {\"tool\":\"wallet_address\",\"args\":{}} done",
                "{\"answer\":\"ok\"}"));
        LlmProvider scripted = new LlmProvider() {
            public String name() { return "scripted"; }
            public String complete(List<ChatMessage> messages) { return script.isEmpty() ? "{\"tool\":\"wallet_address\"}" : script.pop(); }
        };
        AgentWallet w = AgentWallet.random();
        Web3Agent agent = new Web3Agent(scripted, Web3Tools.create(w, null, null, null, null), 3);
        assertEquals("ok", agent.chat("hi"));
        assertTrue(agent.systemPrompt().contains("wallet_address"));
        assertTrue(agent.invoke("nope", Json.object()).startsWith("ERROR: unknown tool"));
        assertTrue(agent.chat("loop").startsWith("Stopped after 3 steps"));
    }
}
