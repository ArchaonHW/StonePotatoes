package io.stonepotatoes.web3ide.agent.controller;

import io.stonepotatoes.web3agent.agent.Web3Agent;
import io.stonepotatoes.web3agent.agent.Web3Tools;
import io.stonepotatoes.web3agent.chain.ChainClient;
import io.stonepotatoes.web3agent.chain.SpendPolicy;
import io.stonepotatoes.web3agent.llm.LlmProvider;
import io.stonepotatoes.web3agent.p2p.ContentStore;
import io.stonepotatoes.web3agent.p2p.PeerNode;
import io.stonepotatoes.web3agent.wallet.AgentWallet;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;

import java.math.BigInteger;
import java.util.Map;

@RestController
@RequestMapping("/api/agent")
public class AgentController {

    @Autowired
    private AgentWallet wallet;

    @Autowired
    private ChainClient chainClient;

    @Autowired
    private SpendPolicy spendPolicy;

    @Autowired
    private LlmProvider llmProvider;

    @Autowired
    private PeerNode peerNode;

    @Autowired
    private ContentStore contentStore;

    @PostMapping("/chat")
    public Map<String, Object> chat(@RequestBody ChatRequest request) {
        Web3Agent agent = new Web3Agent(llmProvider,
                Web3Tools.create(wallet, chainClient, spendPolicy, peerNode, contentStore), 6);
        String response = agent.chat(request.message());
        return Map.of(
                "address", wallet.address(),
                "response", response
        );
    }

    @GetMapping("/info")
    public Map<String, Object> info() throws Exception {
        return Map.of(
                "address", wallet.address(),
                "chain", Map.of(
                        "url", chainClient.rpcUrl(),
                        "chainId", chainClient.chainId(),
                        "blockNumber", chainClient.blockNumber()
                ),
                "llm", llmProvider.name(),
                "peers", peerNode.peers()
        );
    }

    @GetMapping("/balance")
    public Map<String, Object> balance(@RequestParam(required = false) String address) throws Exception {
        String addr = address != null ? address : wallet.address();
        BigInteger bal = chainClient.balanceWei(addr);
        return Map.of("address", addr, "balanceWei", bal.toString());
    }

    record ChatRequest(String message) {}
}
