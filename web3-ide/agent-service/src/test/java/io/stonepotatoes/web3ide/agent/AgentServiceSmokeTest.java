package io.stonepotatoes.web3ide.agent;

import io.stonepotatoes.web3agent.chain.SpendPolicy;
import io.stonepotatoes.web3agent.llm.LlmProvider;
import io.stonepotatoes.web3agent.wallet.AgentWallet;
import io.stonepotatoes.web3ide.agent.controller.AgentController;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.test.context.DynamicPropertyRegistry;
import org.springframework.test.context.DynamicPropertySource;

import java.nio.file.Files;
import java.nio.file.Path;

import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * Smoke test: the full application context starts offline (no config-server,
 * eureka, chain, LLM or Kafka broker) with a password and temp data dir.
 * 冒煙測試：在離線環境（無 config-server、eureka、鏈、LLM、Kafka broker）
 * 提供密碼與暫存資料夾時，完整 application context 能啟動。
 */
@SpringBootTest(
        webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT,
        properties = {
                "spring.cloud.config.enabled=false",
                "eureka.client.enabled=false",
                "agent.password=smoke-test-pass",
                "agent.llm.type=mock",
                "agent.p2p.port=0"
        })
class AgentServiceSmokeTest {

    @DynamicPropertySource
    static void dataDir(DynamicPropertyRegistry r) throws Exception {
        Path dir = Files.createTempDirectory("agent-service-smoke");
        r.add("agent.data-dir", dir::toString);
    }

    @Autowired AgentWallet wallet;
    @Autowired SpendPolicy spendPolicy;
    @Autowired LlmProvider llmProvider;
    @Autowired AgentController controller;

    @Test
    void contextLoadsAndWalletIsCreated() {
        assertNotNull(controller);
        assertNotNull(spendPolicy);
        assertNotNull(llmProvider);
        assertTrue(wallet.address().matches("0x[0-9a-fA-F]{40}"),
                "wallet address should be a 20-byte hex address");
    }
}
