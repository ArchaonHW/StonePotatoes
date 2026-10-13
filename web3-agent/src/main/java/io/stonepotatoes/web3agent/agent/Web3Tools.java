package io.stonepotatoes.web3agent.agent;

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
            t.add(new Tool("erc20_balance", "Raw ERC-20 token balance (smallest unit)", "{\"token\":\"0x contract\",\"owner\":\"0x.. (optional)\"}",
                    a -> chain.erc20Balance(required(a, "token"), optional(a, "owner", wallet.address())).toString()));
            t.add(new Tool("send_eth", "Send native ETH. Subject to spend limits and human approval", "{\"to\":\"0x..\",\"amount_eth\":\"decimal\"}", a -> {
                BigInteger wei = Convert.toWei(new BigDecimal(required(a, "amount_eth")), Convert.Unit.ETHER).toBigIntegerExact();
                return "tx=" + chain.sendEther(wallet, required(a, "to"), wei, policy);
            }));
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
}
