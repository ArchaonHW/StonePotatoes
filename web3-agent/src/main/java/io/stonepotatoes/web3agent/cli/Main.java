package io.stonepotatoes.web3agent.cli;

import io.stonepotatoes.web3agent.agent.Tool;
import io.stonepotatoes.web3agent.agent.Web3Agent;
import io.stonepotatoes.web3agent.agent.Web3Tools;
import io.stonepotatoes.web3agent.chain.ChainClient;
import io.stonepotatoes.web3agent.chain.SpendPolicy;
import io.stonepotatoes.web3agent.llm.LlmProvider;
import io.stonepotatoes.web3agent.llm.MockLlmProvider;
import io.stonepotatoes.web3agent.llm.OpenAiCompatibleProvider;
import io.stonepotatoes.web3agent.p2p.ContentStore;
import io.stonepotatoes.web3agent.p2p.PeerNode;
import io.stonepotatoes.web3agent.wallet.AgentWallet;
import org.web3j.utils.Convert;

import java.io.BufferedReader;
import java.io.Console;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.math.BigDecimal;
import java.math.BigInteger;
import java.nio.charset.StandardCharsets;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;

public final class Main {
    private static final String USAGE = """
            StonePotatoes Web3 Local AI Agent

            Options:
              --data-dir <dir>       keystore + content store (default: ~/.stonepotatoes/web3-agent)
              --rpc <url>            EVM JSON-RPC (default: http://127.0.0.1:8545, e.g. Anvil)
              --bind <host>          P2P bind address (default: 127.0.0.1; use 0.0.0.0 for LAN)
              --port <n>             P2P listen port (default: 30333, 0 = random)
              --peer <host:port>     connect to a peer agent (repeatable)
              --llm <mock|openai>    LLM backend (default: mock)
              --llm-url <url>        OpenAI-compatible base URL (default: http://127.0.0.1:11434/v1, Ollama)
              --llm-model <name>     model name (default: qwen2.5:7b)
              --max-tx-eth <dec>     per-transfer limit (default: 0.1)
              --max-daily-eth <dec>  daily transfer limit (default: 1)
              --max-steps <n>        tool steps per request (default: 6)

            Environment:
              WEB3AGENT_PASSWORD     keystore password (otherwise prompted)
              LLM_API_KEY            bearer token for the LLM endpoint (optional)
            """;

    public static void main(String[] argv) throws Exception {
        Map<String, String> opts = new HashMap<>();
        List<String> peers = new ArrayList<>();
        for (int i = 0; i < argv.length; i++) {
            String a = argv[i];
            if (a.equals("-h") || a.equals("--help")) {
                System.out.println(USAGE);
                return;
            }
            if (!a.startsWith("--") || i + 1 >= argv.length) fail("Bad argument: " + a + "\n\n" + USAGE);
            String v = argv[++i];
            if (a.equals("--peer")) peers.add(v);
            else opts.put(a.substring(2), v);
        }

        PrintStream out = new PrintStream(System.out, true, StandardCharsets.UTF_8);
        BufferedReader in = new BufferedReader(new InputStreamReader(System.in, StandardCharsets.UTF_8));
        Path dataDir = Path.of(opts.getOrDefault("data-dir", Path.of(System.getProperty("user.home"), ".stonepotatoes", "web3-agent").toString()));

        AgentWallet wallet = AgentWallet.loadOrCreate(dataDir.resolve("keystore"), password(in), true);
        ContentStore store = new ContentStore(dataDir.resolve("blobs"));
        SpendPolicy policy = new SpendPolicy(eth(opts.getOrDefault("max-tx-eth", "0.1")), eth(opts.getOrDefault("max-daily-eth", "1")),
                desc -> confirm(out, in, desc));
        LlmProvider llm = switch (opts.getOrDefault("llm", "mock").toLowerCase(Locale.ROOT)) {
            case "mock" -> new MockLlmProvider();
            case "openai" -> new OpenAiCompatibleProvider(opts.getOrDefault("llm-url", "http://127.0.0.1:11434/v1"),
                    opts.getOrDefault("llm-model", "qwen2.5:7b"), System.getenv("LLM_API_KEY"));
            default -> throw fail("Unknown --llm " + opts.get("llm"));
        };

        try (ChainClient chain = new ChainClient(opts.getOrDefault("rpc", "http://127.0.0.1:8545"));
             PeerNode node = new PeerNode(wallet, opts.getOrDefault("bind", "127.0.0.1"), Integer.parseInt(opts.getOrDefault("port", "30333")))) {
            for (String p : peers) {
                int colon = p.lastIndexOf(':');
                try {
                    node.connect(p.substring(0, colon), Integer.parseInt(p.substring(colon + 1)));
                } catch (IOException | RuntimeException e) {
                    out.println("! could not connect to peer " + p + ": " + e.getMessage());
                }
            }
            node.onMessage(env -> {
                if (Web3Tools.TOPIC_CHAT.equals(env.topic())) out.println("\n[p2p] " + env.from() + ": " + env.payload());
            });

            List<Tool> tools = Web3Tools.create(wallet, chain, policy, node, store);
            Web3Agent agent = new Web3Agent(llm, tools, Integer.parseInt(opts.getOrDefault("max-steps", "6")));

            out.println("StonePotatoes Web3 Agent");
            out.println("  identity : " + wallet.address());
            out.println("  p2p      : listening on " + opts.getOrDefault("bind", "127.0.0.1") + ":" + node.port());
            out.println("  chain    : " + chain.rpcUrl());
            out.println("  llm      : " + llm.name());
            out.println("  tools    : " + String.join(", ", tools.stream().map(Tool::name).toList()));
            out.println("Type a request, or 'exit'.");

            String line;
            while (true) {
                out.print("> ");
                if ((line = in.readLine()) == null || line.equalsIgnoreCase("exit") || line.equalsIgnoreCase("quit")) break;
                if (line.isBlank()) continue;
                try {
                    out.println(agent.chat(line));
                } catch (IOException e) {
                    out.println("! LLM error: " + e.getMessage());
                }
            }
        }
    }

    private static boolean confirm(PrintStream out, BufferedReader in, String description) {
        out.print("Approve: " + description + "? [y/N] ");
        try {
            String answer = in.readLine();
            return answer != null && answer.trim().equalsIgnoreCase("y");
        } catch (IOException e) {
            return false;
        }
    }

    private static String password(BufferedReader in) throws IOException {
        String env = System.getenv("WEB3AGENT_PASSWORD");
        if (env != null && !env.isEmpty()) return env;
        Console console = System.console();
        if (console != null) {
            char[] pw = console.readPassword("Keystore password: ");
            if (pw != null && pw.length > 0) return new String(pw);
        } else {
            System.out.print("Keystore password (input visible; prefer WEB3AGENT_PASSWORD): ");
            String pw = in.readLine();
            if (pw != null && !pw.isEmpty()) return pw;
        }
        throw fail("A non-empty keystore password is required");
    }

    private static BigInteger eth(String value) {
        return Convert.toWei(new BigDecimal(value), Convert.Unit.ETHER).toBigIntegerExact();
    }

    private static IllegalArgumentException fail(String message) {
        System.err.println(message);
        System.exit(2);
        return new IllegalArgumentException(message);
    }
}
