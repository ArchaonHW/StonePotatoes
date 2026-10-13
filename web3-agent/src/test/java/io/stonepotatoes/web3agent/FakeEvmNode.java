package io.stonepotatoes.web3agent;

import com.sun.net.httpserver.HttpServer;
import io.stonepotatoes.web3agent.util.Json;
import org.web3j.crypto.RawTransaction;
import org.web3j.crypto.SignedRawTransaction;
import org.web3j.crypto.TransactionDecoder;
import org.web3j.utils.Numeric;
import tools.jackson.databind.JsonNode;
import tools.jackson.databind.node.ObjectNode;

import java.io.IOException;
import java.math.BigInteger;
import java.net.InetSocketAddress;
import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

/** Minimal in-process EVM JSON-RPC stub so chain code can be tested without Anvil. */
public final class FakeEvmNode implements AutoCloseable {
    public static final long CHAIN_ID = 31337;
    public record SentTx(String from, String to, BigInteger value, BigInteger nonce, Long chainId) {}

    public final List<SentTx> sent = new CopyOnWriteArrayList<>();
    public volatile BigInteger balance = new BigInteger("5000000000000000000");
    public volatile BigInteger tokenBalance = BigInteger.valueOf(42_000);
    private final HttpServer server;

    public FakeEvmNode() throws IOException {
        server = HttpServer.create(new InetSocketAddress("127.0.0.1", 0), 0);
        server.createContext("/", ex -> {
            JsonNode req = Json.parse(new String(ex.getRequestBody().readAllBytes(), StandardCharsets.UTF_8));
            ObjectNode resp = Json.object().put("jsonrpc", "2.0");
            resp.set("id", req.path("id"));
            String method = req.path("method").asString();
            switch (method) {
                case "eth_chainId" -> resp.put("result", Numeric.toHexStringWithPrefix(BigInteger.valueOf(CHAIN_ID)));
                case "eth_blockNumber" -> resp.put("result", "0x10");
                case "eth_getBalance" -> resp.put("result", Numeric.toHexStringWithPrefix(balance));
                case "eth_getTransactionCount" -> resp.put("result", Numeric.toHexStringWithPrefix(BigInteger.valueOf(sent.size())));
                case "eth_gasPrice" -> resp.put("result", "0x3b9aca00");
                case "eth_call" -> resp.put("result", Numeric.toHexStringWithPrefixZeroPadded(tokenBalance, 64));
                case "eth_sendRawTransaction" -> {
                    String raw = req.path("params").path(0).asString();
                    RawTransaction tx = TransactionDecoder.decode(raw);
                    try {
                        SignedRawTransaction s = (SignedRawTransaction) tx;
                        sent.add(new SentTx(s.getFrom(), tx.getTo(), tx.getValue(), tx.getNonce(), s.getChainId()));
                    } catch (Exception e) {
                        throw new IOException(e);
                    }
                    resp.put("result", "0x" + "ab".repeat(32));
                }
                default -> resp.putObject("error").put("code", -32601).put("message", "Method not found: " + method);
            }
            byte[] body = Json.write(resp).getBytes(StandardCharsets.UTF_8);
            ex.getResponseHeaders().add("Content-Type", "application/json");
            ex.sendResponseHeaders(200, body.length);
            ex.getResponseBody().write(body);
            ex.close();
        });
        server.start();
    }

    public String url() {
        return "http://127.0.0.1:" + server.getAddress().getPort();
    }

    @Override
    public void close() {
        server.stop(0);
    }
}
