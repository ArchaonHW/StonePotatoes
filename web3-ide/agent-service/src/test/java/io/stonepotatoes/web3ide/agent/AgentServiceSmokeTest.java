package io.stonepotatoes.web3ide.agent;

import io.stonepotatoes.web3ide.agent.controller.AgentController;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.context.SpringBootTest;
import org.springframework.web.reactive.function.client.WebClient;

import static org.junit.jupiter.api.Assertions.assertNotNull;

/**
 * Smoke test: the application context starts offline (no config-server,
 * eureka, Kafka broker or web3-agent HTTP service needed). agent-service is
 * an HTTP proxy to web3-agent (agent.http-url); keystore/password handling
 * lives in web3-agent itself.
 * 冒煙測試：離線環境下 application context 可啟動。agent-service 為 web3-agent
 * 的 HTTP 代理（agent.http-url）；keystore/密碼由 web3-agent 本身處理。
 */
@SpringBootTest(
        webEnvironment = SpringBootTest.WebEnvironment.RANDOM_PORT,
        properties = {
                "spring.cloud.config.enabled=false",
                "eureka.client.enabled=false",
                "agent.http-url=http://127.0.0.1:1"
        })
class AgentServiceSmokeTest {

    @Autowired AgentController controller;
    @Autowired WebClient.Builder webClientBuilder;

    @Test
    void contextLoads() {
        assertNotNull(controller);
        assertNotNull(webClientBuilder);
    }
}