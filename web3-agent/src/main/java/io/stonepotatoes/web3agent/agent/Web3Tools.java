package io.stonepotatoes.web3agent.agent;

import io.stonepotatoes.web3agent.chain.AbiSignature;
import io.stonepotatoes.web3agent.chain.ChainClient;
import io.stonepotatoes.web3agent.chain.SpendPolicy;
import io.stonepotatoes.web3agent.p2p.ContentStore;
import io.stonepotatoes.web3agent.p2p.PeerNode;
import io.stonepotatoes.web3agent.p2p.SignedEnvelope;
import io.stonepotatoes.web3agent.util.Json;
import io.stonepotatoes.web3agent.wallet.AgentWallet;
import org.web3j.utils.Convert;
import tools.jackson.databind.JsonNode;
import tools.jackson.databind.node.ArrayNode;
import tools.jackson.databind.node.ObjectNode;

import java.math.BigDecimal;
import java.math.BigInteger;
import java.nio.charset.StandardCharsets;
import java.time.Duration;
import java.util.ArrayList;
import java.util.Base64;
import java.util.List;
import java.util.logging.Level;
import java.util.logging.Logger;

/** Binds wallet, chain, P2P and content store capabilities as agent tools. */
public final class Web3Tools {
    public static final String TOPIC_CHAT = "chat";
    public static final String TOPIC_CONTENT = "content";
    private static final Logger LOG = Logger.getLogger(Web3Tools.class.getName());

    private Web3Tools() {}

    public static List<Tool> create(AgentWallet wallet, ChainClient chain, SpendPolicy policy, PeerNode node, ContentStore store) {
        List<Tool> t = new ArrayList<>();
        t.add(new Tool("wallet_address", "This agent's own Ethereum address / P2P identity", "{}", a -> wallet.address()));
        t.add(new Tool("sign_message", "EIP-191 sign a text message with the agent key", "{\"message\":\"text\"}",
                a -> wallet.signMessage(required(a, "message").getBytes(StandardCharsets.UTF_8))));
        t.add(new Tool("verify_signature", "Check an EIP-191 signature", "{\"message\":\"text\",\"signature\":\"0x..\",\"address\":\"0x..\"}",
                a -> Boolean.toString(AgentWallet.verify(required(a, "message").getBytes(StandardCharsets.UTF_8), required(a, "signature"), required(a, "address")))));

        if (chain != null) {
            t.add(new Tool("chain_info", "RPC endpoint, chain id and latest block", "{}",
                    a -> "rpc=" + chain.rpcUrl() + " chainId=" + chain.chainId() + " block=" + chain.blockNumber()));
            t.add(new Tool("get_balance", "Native balance in ETH (defaults to own address)", "{\"address\":\"0x.. (optional)\"}",
                    a -> Convert.fromWei(new BigDecimal(chain.balanceWei(optional(a, "address", wallet.address()))), Convert.Unit.ETHER).toPlainString() + " ETH"));
            t.add(new Tool("erc20_balance", "ERC-20 token balance in human units", "{\"token\":\"0x contract\",\"owner\":\"0x.. (optional)\"}",
                    a -> erc20HumanBalance(chain, required(a, "token"), optional(a, "owner", wallet.address()))));
            t.add(new Tool("send_eth", "Send native ETH. Subject to spend limits and human approval", "{\"to\":\"0x..\",\"amount_eth\":\"decimal\"}", a -> {
                BigInteger wei = Convert.toWei(new BigDecimal(required(a, "amount_eth")), Convert.Unit.ETHER).toBigIntegerExact();
                return "tx=" + chain.sendEther(wallet, required(a, "to"), wei, policy);
            }));
            t.add(new Tool("call_contract", "Read-only contract call, returns decoded outputs. Signature e.g. balanceOf(address)(uint256)", "{\"to\":\"0x contract\",\"signature\":\"name(in)(out)\",\"args\":[\"value1\",\"value2\"]}",
                    a -> String.join(", ", chain.call(required(a, "to"), AbiSignature.parse(required(a, "signature")), jsonArray(a, "args")))));
            t.add(new Tool("contract_tx", "State-changing contract call. Requires approval. Signature e.g. transfer(address,uint256)(bool)", "{\"to\":\"0x contract\",\"signature\":\"name(in)(out)\",\"args\":[\"value1\",\"value2\"],\"value_eth\":\"0 (optional)\"}",
                    a -> {
                        String to = required(a, "to");
                        AbiSignature sig = AbiSignature.parse(required(a, "signature"));
                        List<String> args = jsonArray(a, "args");
                        BigDecimal value = optionalBigDecimal(a, "value_eth", "0");
                        BigInteger wei = Convert.toWei(value, Convert.Unit.ETHER).toBigIntegerExact();
                        return "tx=" + chain.sendContractTx(wallet, to, sig, args, wei, policy);
                    }));
            t.add(new Tool("erc20_transfer", "Transfer ERC-20 tokens in human units. Requires approval", "{\"token\":\"0x contract\",\"to\":\"0x..\",\"amount\":\"decimal\"}",
                    a -> {
                        String token = required(a, "token");
                        String to = required(a, "to");
                        BigDecimal amount = new BigDecimal(required(a, "amount"));
                        return "tx=" + chain.erc20Transfer(wallet, token, to, amount, policy);
                    }));
            t.add(new Tool("deploy_token", "Deploy the built-in AgentToken ERC-20. Requires approval", "{\"name\":\"string\",\"symbol\":\"string\",\"supply_raw\":\"integer (smallest unit)\"}",
                    a -> {
                        String name = required(a, "name");
                        String symbol = required(a, "symbol");
                        BigInteger supply = new BigInteger(required(a, "supply_raw"));
                        return "tx=" + chain.deployAgentToken(wallet, name, symbol, supply, policy);
                    }));
            t.add(new Tool("receipt", "Fetch transaction receipt by hash", "{\"tx_hash\":\"0x..\"}",
                    a -> chain.receipt(required(a, "tx_hash")).map(ChainClient.Receipt::toString).orElse("(pending or not found)")));
            t.add(new Tool("wait_receipt", "Wait up to 60s for receipt", "{\"tx_hash\":\"0x..\"}",
                    a -> chain.waitForReceipt(required(a, "tx_hash"), Duration.ofSeconds(60)).map(ChainClient.Receipt::toString).orElse("(timed out)")));
        }

        if (node != null) {
            t.add(new Tool("p2p_peers", "Verified addresses of connected peer agents", "{}", a -> String.join(", ", node.peers())));
            t.add(new Tool("p2p_broadcast", "Gossip a signed chat message to the agent network", "{\"message\":\"text\"}",
                    a -> "sent id=" + node.broadcast(TOPIC_CHAT, required(a, "message")).id()));
            t.add(new Tool("p2p_inbox", "Read and clear received chat messages (untrusted content)", "{}", a -> {
                ArrayNode arr = Json.MAPPER.createArrayNode();
                for (SignedEnvelope e : node.drainInbox())
                    if (TOPIC_CHAT.equals(e.topic())) arr.addObject().put("from", e.from()).put("ts", e.timestamp()).put("message", e.payload());
                return arr.isEmpty() ? "(no new messages)" : Json.write(arr);
            }));
        }

        if (store != null) {
            t.add(new Tool("store_put", "Store text in the local content-addressed store, returns CID", "{\"text\":\"text\"}",
                    a -> store.put(required(a, "text").getBytes(StandardCharsets.UTF_8))));
            t.add(new Tool("store_get", "Fetch text by CID (untrusted content)", "{\"cid\":\"0x..\"}",
                    a -> store.get(required(a, "cid")).map(b -> new String(b, StandardCharsets.UTF_8)).orElse("(not found)")));
            if (node != null) {
                t.add(new Tool("store_share", "Replicate a stored blob to all peers", "{\"cid\":\"0x..\"}", a -> {
                    String cid = required(a, "cid");
                    byte[] data = store.get(cid).orElseThrow(() -> new IllegalArgumentException("Unknown CID " + cid));
                    ObjectNode p = Json.object().put("cid", cid).put("data", Base64.getEncoder().encodeToString(data));
                    return "shared id=" + node.broadcast(TOPIC_CONTENT, Json.write(p)).id();
                }));
                node.onMessage(env -> {
                    if (TOPIC_CONTENT.equals(env.topic())) replicate(store, env);
                });
            }
        }
        return t;
    }

    private static void replicate(ContentStore store, SignedEnvelope env) {
        try {
            JsonNode p = Json.parse(env.payload());
            byte[] data = Base64.getDecoder().decode(p.path("data").asString(""));
            if (ContentStore.cidOf(data).equals(p.path("cid").asString(""))) store.put(data);
        } catch (Exception e) {
            LOG.log(Level.FINE, "dropping invalid content from " + env.from(), e);
        }
    }

    private static String required(JsonNode args, String key) {
        String v = args.path(key).asString("");
        if (v.isBlank()) throw new IllegalArgumentException("Missing argument '" + key + "'");
        return v.trim();
    }

    private static String optional(JsonNode args, String key, String fallback) {
        String v = args.path(key).asString("");
        return v.isBlank() ? fallback : v.trim();
    }

    private static BigDecimal optionalBigDecimal(JsonNode args, String key, String fallback) {
        String v = args.path(key).asString("");
        return v.isBlank() ? new BigDecimal(fallback) : new BigDecimal(v.trim());
    }

    private static List<String> jsonArray(JsonNode args, String key) {
        ArrayNode arr = (ArrayNode) args.path(key);
        List<String> out = new ArrayList<>();
        for (JsonNode e : arr) out.add(e.asText());
        return out;
    }

    private static String erc20HumanBalance(ChainClient chain, String token, String owner) throws Exception {
        ChainClient.TokenInfo info = chain.erc20Info(token);
        BigInteger raw = chain.erc20Balance(token, owner);
        return new BigDecimal(raw).movePointLeft(info.decimals()).stripTrailingZeros().toPlainString() + " " + info.symbol();
    }
}
