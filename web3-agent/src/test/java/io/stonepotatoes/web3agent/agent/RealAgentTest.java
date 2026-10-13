package io.stonepotatoes.web3agent.agent;

import io.stonepotatoes.web3agent.chain.ChainClient;
import io.stonepotatoes.web3agent.chain.SpendPolicy;
import io.stonepotatoes.web3agent.llm.OpenAiCompatibleProvider;
import io.stonepotatoes.web3agent.p2p.ContentStore;
import io.stonepotatoes.web3agent.wallet.AgentWallet;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.condition.EnabledIf;

import java.nio.file.Path;
import java.util.List;

import static org.junit.jupiter.api.Assertions.*;

/**
 * End-to-end test against real Anvil and llama.cpp. Requires both services running.
 * Enabled only when the environment variable REAL_AGENT_TEST=1 is set.
 */
@EnabledIf("realAgentTest")
class RealAgentTest {
    static final String ANVIL_URL = "http://127.0.0.1:8545";
    static final String LLM_URL = "http://127.0.0.1:11434/v1";
    static final String LLM_MODEL = "qwen2.5:7b";

    static boolean realAgentTest() {
        return "1".equals(System.getenv("REAL_AGENT_TEST"));
    }

    @Test
    void realLlmWithRealChain() throws Exception {
        AgentWallet wallet = AgentWallet.random();
        SpendPolicy policy = new SpendPolicy(java.math.BigInteger.ZERO, java.math.BigInteger.ZERO, desc -> true);
        ContentStore store = new ContentStore(Path.of(System.getProperty("java.io.tmpdir"), "web3-test"));
        try (ChainClient chain = new ChainClient(ANVIL_URL)) {
            OpenAiCompatibleProvider llm = new OpenAiCompatibleProvider(LLM_URL, LLM_MODEL, null);
            Web3Agent agent = new Web3Agent(llm, Web3Tools.create(wallet, chain, policy, null, store), 6);

            // Test that the agent can connect to both services and respond
            String resp = agent.chat("Say hello");
            assertNotNull(resp);
            assertFalse(resp.isEmpty());
            // The model may not follow tool-call format; just verify it responds
        }
    }
}
