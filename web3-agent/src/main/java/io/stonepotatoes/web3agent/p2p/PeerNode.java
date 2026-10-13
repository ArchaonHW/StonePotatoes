package io.stonepotatoes.web3agent.p2p;

import io.stonepotatoes.web3agent.wallet.AgentWallet;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.io.Writer;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.charset.StandardCharsets;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.function.Consumer;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * Serverless agent-to-agent transport: newline-delimited JSON {@link SignedEnvelope}s over TCP with
 * flood gossip. Every message is signature-checked, de-duplicated and freshness-checked before it
 * is delivered or relayed, so a peer cannot forge another agent's identity or replay old messages.
 */
public final class PeerNode implements AutoCloseable {
    public static final String TOPIC_HELLO = "hello";
    static final int MAX_LINE_CHARS = 512 * 1024;
    static final long MAX_CLOCK_SKEW_MS = 5 * 60_000;
    private static final int SEEN_CAPACITY = 20_000;
    private static final int INBOX_CAPACITY = 256;
    private static final Logger LOG = Logger.getLogger(PeerNode.class.getName());

    private final AgentWallet wallet;
    private final ServerSocket server;
    private final List<Connection> connections = new CopyOnWriteArrayList<>();
    private final List<Consumer<SignedEnvelope>> handlers = new CopyOnWriteArrayList<>();
    private final Set<String> seen = Collections.newSetFromMap(new LinkedHashMap<>(1024, 0.75f, false) {
        @Override
        protected boolean removeEldestEntry(Map.Entry<String, Boolean> eldest) {
            return size() > SEEN_CAPACITY;
        }
    });
    private final ArrayDeque<SignedEnvelope> inbox = new ArrayDeque<>();
    private volatile boolean closed;

    public PeerNode(AgentWallet wallet, String bindHost, int port) throws IOException {
        this.wallet = wallet;
        this.server = new ServerSocket();
        server.bind(new InetSocketAddress(InetAddress.getByName(bindHost), port));
        Thread.ofVirtual().name("p2p-accept").start(this::acceptLoop);
    }

    public int port() {
        return server.getLocalPort();
    }

    public String address() {
        return wallet.address();
    }

    public void onMessage(Consumer<SignedEnvelope> handler) {
        handlers.add(handler);
    }

    public void connect(String host, int port) throws IOException {
        Socket socket = new Socket();
        socket.connect(new InetSocketAddress(host, port), 5_000);
        attach(socket).send(SignedEnvelope.create(wallet, TOPIC_HELLO, Integer.toString(port())).toJson());
    }

    public SignedEnvelope broadcast(String topic, String payload) {
        if (TOPIC_HELLO.equals(topic)) throw new IllegalArgumentException("Reserved topic: " + topic);
        SignedEnvelope env = SignedEnvelope.create(wallet, topic, payload);
        markSeen(env.id());
        relay(env.toJson(), null);
        return env;
    }

    /** Verified peer identities (addresses) currently connected. */
    public List<String> peers() {
        List<String> out = new ArrayList<>();
        for (Connection c : connections) if (c.peerAddress != null) out.add(c.peerAddress);
        return out;
    }

    public List<SignedEnvelope> drainInbox() {
        synchronized (inbox) {
            List<SignedEnvelope> out = new ArrayList<>(inbox);
            inbox.clear();
            return out;
        }
    }

    private void acceptLoop() {
        while (!closed) {
            try {
                Connection c = attach(server.accept());
                c.send(SignedEnvelope.create(wallet, TOPIC_HELLO, Integer.toString(port())).toJson());
            } catch (IOException e) {
                if (!closed) LOG.log(Level.FINE, "accept failed", e);
            }
        }
    }

    private Connection attach(Socket socket) throws IOException {
        Connection c = new Connection(socket);
        connections.add(c);
        Thread.ofVirtual().name("p2p-conn").start(c::readLoop);
        return c;
    }

    private boolean markSeen(String id) {
        synchronized (seen) {
            return seen.add(id);
        }
    }

    private void relay(String line, Connection except) {
        for (Connection c : connections) if (c != except) c.send(line);
    }

    private void handle(Connection from, String line) {
        SignedEnvelope env;
        try {
            env = SignedEnvelope.fromJson(line);
        } catch (RuntimeException e) {
            return;
        }
        if (Math.abs(System.currentTimeMillis() - env.timestamp()) > MAX_CLOCK_SKEW_MS || !env.verify()) return;
        if (env.from().equalsIgnoreCase(wallet.address()) || !markSeen(env.id())) return;
        if (TOPIC_HELLO.equals(env.topic())) {
            from.peerAddress = env.from();
            return;
        }
        if (from.peerAddress == null) return; // must authenticate first
        synchronized (inbox) {
            if (inbox.size() >= INBOX_CAPACITY) inbox.pollFirst();
            inbox.addLast(env);
        }
        for (Consumer<SignedEnvelope> h : handlers) {
            try {
                h.accept(env);
            } catch (RuntimeException e) {
                LOG.log(Level.WARNING, "message handler failed", e);
            }
        }
        relay(line, from);
    }

    @Override
    public void close() throws IOException {
        closed = true;
        server.close();
        for (Connection c : connections) c.close();
    }

    private final class Connection {
        private final Socket socket;
        private final Writer out;
        volatile String peerAddress;

        Connection(Socket socket) throws IOException {
            this.socket = socket;
            this.out = new BufferedWriter(new OutputStreamWriter(socket.getOutputStream(), StandardCharsets.UTF_8));
        }

        synchronized void send(String line) {
            try {
                out.write(line);
                out.write('\n');
                out.flush();
            } catch (IOException e) {
                close();
            }
        }

        void readLoop() {
            try (BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream(), StandardCharsets.UTF_8))) {
                StringBuilder sb = new StringBuilder();
                int ch;
                while ((ch = in.read()) != -1) {
                    if (ch == '\n') {
                        handle(this, sb.toString());
                        sb.setLength(0);
                    } else if (sb.length() >= MAX_LINE_CHARS) {
                        return; // oversized frame: drop the peer
                    } else {
                        sb.append((char) ch);
                    }
                }
            } catch (IOException ignored) {
                // peer disconnected
            } finally {
                close();
            }
        }

        void close() {
            connections.remove(this);
            try {
                socket.close();
            } catch (IOException ignored) {
            }
        }
    }
}
