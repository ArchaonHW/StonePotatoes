package io.stonepotatoes.web3ide.agent.config;

import io.stonepotatoes.web3agent.chain.ChainClient;
import io.stonepotatoes.web3agent.chain.SpendPolicy;
import io.stonepotatoes.web3agent.llm.LlmProvider;
import io.stonepotatoes.web3agent.llm.MockLlmProvider;
import io.stonepotatoes.web3agent.llm.OpenAiCompatibleProvider;
import io.stonepotatoes.web3agent.p2p.ContentStore;
import io.stonepotatoes.web3agent.p2p.PeerNode;
import io.stonepotatoes.web3agent.wallet.AgentWallet;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;

import java.math.BigDecimal;
import java.math.BigInteger;
import java.nio.file.Path;

@Configuration
public class AgentConfig {

    @Value("${agent.password}")
    private String password;

    @Value("${agent.data-dir:${user.home}/.stonepotatoes/web3-ide}")
    private String dataDir;

    @Value("${agent.chain.url:http://127.0.0.1:8545}")
    private String chainUrl;

    @Value("${agent.llm.type:mock}")
    private String llmType;

    @Value("${agent.llm.url:http://127.0.0.1:11434/v1}")
    private String llmUrl;

    @Value("${agent.llm.model:qwen2.5:7b}")
    private String llmModel;

    @Value("${agent.p2p.port:0}")
    private int p2pPort;

    @Value("${agent.max-tx-eth:0.1}")
    private double maxTxEth;

    @Value("${agent.max-daily-eth:1}")
    private double maxDailyEth;

    @Bean
    public AgentWallet wallet() throws Exception {
        return AgentWallet.loadOrCreate(Path.of(dataDir, "keystore"), password, true);
    }

    @Bean
    public ChainClient chainClient() {
        return new ChainClient(chainUrl);
    }

    @Bean
    public SpendPolicy spendPolicy() {
        BigInteger maxTxWei = new BigDecimal(maxTxEth).multiply(BigDecimal.TEN.pow(18)).toBigInteger();
        BigInteger maxDailyWei = new BigDecimal(maxDailyEth).multiply(BigDecimal.TEN.pow(18)).toBigInteger();
        return new SpendPolicy(maxTxWei, maxDailyWei, desc -> true); // Auto-approve for service mode
    }

    @Bean
    public LlmProvider llmProvider() {
        return switch (llmType.toLowerCase()) {
            case "openai" -> new OpenAiCompatibleProvider(llmUrl, llmModel, null);
            default -> new MockLlmProvider();
        };
    }

    @Bean
    public ContentStore contentStore() throws java.io.IOException {
        return new ContentStore(Path.of(dataDir, "blobs"));
    }

    @Bean
    public PeerNode peerNode(AgentWallet wallet) throws Exception {
        return new PeerNode(wallet, "127.0.0.1", p2pPort);
    }
}
