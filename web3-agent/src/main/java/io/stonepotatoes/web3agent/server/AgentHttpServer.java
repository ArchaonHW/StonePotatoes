package io.stonepotatoes.web3agent.server;

import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpHandler;
import io.stonepotatoes.web3agent.agent.Web3Agent;
import io.stonepotatoes.web3agent.chain.ChainClient;
import io.stonepotatoes.web3agent.llm.LlmProvider;
import io.stonepotatoes.web3agent.p2p.PeerNode;
import io.stonepotatoes.web3agent.wallet.AgentWallet;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.math.BigInteger;
import java.net.InetSocketAddress;
import java.nio.charset.StandardCharsets;

public class AgentHttpServer implements AutoCloseable {
    private final com.sun.net.httpserver.HttpServer server;
    private final Web3Agent agent;
    private final AgentWallet wallet;
    private final ChainClient chain;
    private final PeerNode peerNode;
    private final LlmProvider llm;

    public AgentHttpServer(int port, Web3Agent agent, AgentWallet wallet, ChainClient chain, PeerNode peerNode, LlmProvider llm) throws IOException {
        this.server = com.sun.net.httpserver.HttpServer.create(new InetSocketAddress(port), 0);
        this.agent = agent;
        this.wallet = wallet;
        this.chain = chain;
        this.peerNode = peerNode;
        this.llm = llm;

        server.createContext("/api/info", new InfoHandler());
        server.createContext("/api/chat", new ChatHandler());
        server.createContext("/api/balance", new BalanceHandler());
        server.setExecutor(null);
        server.start();
    }

    @Override
    public void close() {
        server.stop(0);
    }

    private void sendResponse(HttpExchange exchange, int statusCode, String response) throws IOException {
        exchange.getResponseHeaders().set("Content-Type", "application/json");
        exchange.getResponseHeaders().set("Access-Control-Allow-Origin", "*");
        exchange.sendResponseHeaders(statusCode, response.getBytes(StandardCharsets.UTF_8).length);
        try (OutputStream os = exchange.getResponseBody()) {
            os.write(response.getBytes(StandardCharsets.UTF_8));
        }
    }

    private String readRequestBody(HttpExchange exchange) throws IOException {
        try (InputStream is = exchange.getRequestBody()) {
            return new String(is.readAllBytes(), StandardCharsets.UTF_8);
        }
    }

    private class InfoHandler implements HttpHandler {
        @Override
        public void handle(HttpExchange exchange) throws IOException {
            try {
                if (!"GET".equals(exchange.getRequestMethod())) {
                    sendResponse(exchange, 405, "{\"error\":\"Method not allowed\"}");
                    return;
                }

                StringBuilder sb = new StringBuilder();
                sb.append("{");
                sb.append("\"address\":\"").append(wallet.address()).append("\",");
                sb.append("\"llm\":\"").append(llm.name()).append("\",");
                sb.append("\"peers\":[");
                if (peerNode != null) {
                    for (int i = 0; i < peerNode.peers().size(); i++) {
                        if (i > 0) sb.append(",");
                        sb.append("\"").append(peerNode.peers().get(i)).append("\"");
                    }
                }
                sb.append("]");
                sb.append("}");

                sendResponse(exchange, 200, sb.toString());
            } catch (Exception e) {
                e.printStackTrace();
                sendResponse(exchange, 500, "{\"error\":\"" + escapeJson(e.getMessage()) + "\"}");
            }
        }

        private String escapeJson(String s) {
            if (s == null) return "";
            return s.replace("\\", "\\\\").replace("\"", "\\\"").replace("\n", "\\n").replace("\r", "\\r");
        }
    }

    private class ChatHandler implements HttpHandler {
        @Override
        public void handle(HttpExchange exchange) throws IOException {
            try {
                if (!"POST".equals(exchange.getRequestMethod())) {
                    sendResponse(exchange, 405, "{\"error\":\"Method not allowed\"}");
                    return;
                }

                String body = readRequestBody(exchange);
                String message = extractMessage(body);

                try {
                    String response = agent.chat(message);
                    String json = "{\"address\":\"" + wallet.address() + "\",\"response\":\"" + escapeJson(response) + "\"}";
                    sendResponse(exchange, 200, json);
                } catch (Exception e) {
                    e.printStackTrace();
                    sendResponse(exchange, 500, "{\"error\":\"" + escapeJson(e.getMessage()) + "\"}");
                }
            } catch (Exception e) {
                e.printStackTrace();
                sendResponse(exchange, 500, "{\"error\":\"" + escapeJson(e.getMessage()) + "\"}");
            }
        }

        private String extractMessage(String body) {
            // Simple JSON parsing for {"message":"..."}
            int start = body.indexOf("\"message\":\"");
            if (start == -1) return body;
            start += 11;
            int end = body.indexOf("\"", start);
            if (end == -1) return body;
            return body.substring(start, end);
        }

        private String escapeJson(String s) {
            if (s == null) return "";
            return s.replace("\\", "\\\\").replace("\"", "\\\"").replace("\n", "\\n").replace("\r", "\\r");
        }
    }

    private class BalanceHandler implements HttpHandler {
        @Override
        public void handle(HttpExchange exchange) throws IOException {
            try {
                if (!"GET".equals(exchange.getRequestMethod())) {
                    sendResponse(exchange, 405, "{\"error\":\"Method not allowed\"}");
                    return;
                }

                String query = exchange.getRequestURI().getQuery();
                String address = wallet.address();
                if (query != null && query.startsWith("address=")) {
                    address = query.substring(8);
                }

                try {
                    BigInteger balance = chain.balanceWei(address);
                    String json = "{\"address\":\"" + address + "\",\"balanceWei\":\"" + balance + "\"}";
                    sendResponse(exchange, 200, json);
                } catch (Exception e) {
                    sendResponse(exchange, 500, "{\"error\":\"" + escapeJson(e.getMessage()) + "\"}");
                }
            } catch (Exception e) {
                e.printStackTrace();
                sendResponse(exchange, 500, "{\"error\":\"" + escapeJson(e.getMessage()) + "\"}");
            }
        }

        private String escapeJson(String s) {
            if (s == null) return "";
            return s.replace("\\", "\\\\").replace("\"", "\\\"").replace("\n", "\\n").replace("\r", "\\r");
        }
    }
}
