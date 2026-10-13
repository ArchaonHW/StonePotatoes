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
    public record SentTx(String from, String to, BigInteger value, BigInteger nonce, Long chainId, String data) {}

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
                case "eth_maxPriorityFeePerGas" -> resp.put("result", "0x59682f00");
                case "eth_estimateGas" -> resp.put("result", "0x5208");
                case "eth_getBlockByNumber" -> {
                    ObjectNode block = resp.putObject("result");
                    block.put("number", "0x10");
                    block.put("baseFeePerGas", "0x3b9aca00");
                }
                case "eth_call" -> {
                    // Minimal ABI-encoded uint256 return: selector not used for calls, but return is padded to 32 bytes
                    JsonNode params = req.path("params").path(0);
                    String data = params.path("data").asString();
                    // Balance selector: 0x70a08231 (balanceOf(address))
                    if (data.startsWith("0x70a08231")) resp.put("result", Numeric.toHexStringWithPrefixZeroPadded(tokenBalance, 64));
                    // Decimals selector: 0x313ce567
                    else if (data.startsWith("0x313ce567")) resp.put("result", "0x0000000000000000000000000000000000000000000000000000000000000012"); // 18
                    // Symbol selector: 0x95d89b41 (string) - minimal encoding
                    else if (data.startsWith("0x95d89b41")) resp.put("result", "0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000000004d4f434b000000000000000000000000000000000000000000000000000000");
                    else resp.put("result", "0x");
                }
                case "eth_sendRawTransaction" -> {
                    String raw = req.path("params").path(0).asString();
                    RawTransaction tx = TransactionDecoder.decode(raw);
                    try {
                        SignedRawTransaction s = (SignedRawTransaction) tx;
                        sent.add(new SentTx(s.getFrom(), tx.getTo(), tx.getValue(), tx.getNonce(), s.getChainId(), tx.getData()));
                    } catch (Exception e) {
                        throw new IOException(e);
                    }
                    resp.put("result", "0x" + "ab".repeat(32));
                }
                case "eth_getTransactionReceipt" -> {
                    String hash = req.path("params").path(0).asString();
                    if (hash.startsWith("0x") && hash.contains("ab")) {
                        ObjectNode r = resp.putObject("result");
                        r.put("transactionHash", hash);
                        r.put("blockNumber", "0x10");
                        r.put("gasUsed", "0x5208");
                        r.put("status", "0x1");
                    } else {
                        resp.put("result", (String) null);
                    }
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
