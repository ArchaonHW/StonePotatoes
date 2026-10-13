package io.stonepotatoes.web3agent.p2p;

import io.stonepotatoes.web3agent.util.Json;
import io.stonepotatoes.web3agent.wallet.AgentWallet;
import tools.jackson.databind.JsonNode;
import tools.jackson.databind.node.ObjectNode;

import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.UUID;

/**
 * A gossip message authenticated by its sender's Ethereum key (EIP-191). Peer identity is the
 * address recovered from the signature, so no central registry or certificate authority is needed.
 */
public record SignedEnvelope(String id, String from, String topic, long timestamp, String payload, String signature) {

    public static SignedEnvelope create(AgentWallet wallet, String topic, String payload) {
        String id = UUID.randomUUID().toString();
        long ts = System.currentTimeMillis();
        String from = wallet.address();
        return new SignedEnvelope(id, from, topic, ts, payload, wallet.signMessage(canonical(id, from, topic, ts, payload)));
    }

    private static byte[] canonical(String id, String from, String topic, long ts, String payload) {
        return Json.write(List.of("sp-web3agent/1", id, from.toLowerCase(), topic, ts, payload)).getBytes(StandardCharsets.UTF_8);
    }

    public boolean verify() {
        return id != null && from != null && topic != null && payload != null && signature != null
                && AgentWallet.verify(canonical(id, from, topic, timestamp, payload), signature, from);
    }

    public String toJson() {
        ObjectNode n = Json.object();
        n.put("id", id).put("from", from).put("topic", topic).put("ts", timestamp).put("payload", payload).put("sig", signature);
        return Json.write(n);
    }

    public static SignedEnvelope fromJson(String json) {
        JsonNode n = Json.parse(json);
        return new SignedEnvelope(text(n, "id"), text(n, "from"), text(n, "topic"), n.path("ts").asLong(), text(n, "payload"), text(n, "sig"));
    }

    private static String text(JsonNode n, String field) {
        JsonNode v = n.get(field);
        return v != null && v.isString() ? v.asString() : null;
    }
}
