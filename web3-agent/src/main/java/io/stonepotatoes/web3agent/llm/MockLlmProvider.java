package io.stonepotatoes.web3agent.llm;

import io.stonepotatoes.web3agent.agent.Web3Agent;
import io.stonepotatoes.web3agent.util.Json;
import tools.jackson.databind.node.ObjectNode;

import java.util.List;
import java.util.Locale;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Deterministic, offline, rule-based "LLM" that speaks the same tool-call protocol as a real
 * model. Lets the agent run with zero downloads and makes the agent loop testable.
 */
public final class MockLlmProvider implements LlmProvider {
    private static final String ADDR = "(0x[0-9a-fA-F]{40})";
    private static final String CID = "(0x[0-9a-fA-F]{64})";
    private static final Pattern SEND = Pattern.compile("(?:send|transfer|轉帳|發送)\\s*([0-9]*\\.?[0-9]+)\\s*(?:eth|ether)?\\s*(?:to|給|至)?\\s*" + ADDR, Pattern.CASE_INSENSITIVE);
    private static final Pattern ERC20 = Pattern.compile("(?:erc20|token|代幣)\\D*?" + ADDR + "(?:\\D+?" + ADDR + ")?", Pattern.CASE_INSENSITIVE);
    private static final Pattern BALANCE = Pattern.compile("(?:balance|餘額)", Pattern.CASE_INSENSITIVE);
    private static final Pattern ANY_ADDR = Pattern.compile(ADDR);
    private static final Pattern ANY_CID = Pattern.compile(CID);
    private static final Pattern SIGN = Pattern.compile("^(?:sign|簽名)[:：]?\\s+(.+)$", Pattern.CASE_INSENSITIVE | Pattern.DOTALL);
    private static final Pattern BROADCAST = Pattern.compile("^(?:broadcast|say|廣播)[:：]?\\s+(.+)$", Pattern.CASE_INSENSITIVE | Pattern.DOTALL);
    private static final Pattern STORE = Pattern.compile("^(?:store|save|儲存)[:：]?\\s+(.+)$", Pattern.CASE_INSENSITIVE | Pattern.DOTALL);

    @Override
    public String name() {
        return "mock (rule-based, offline)";
    }

    @Override
    public String complete(List<ChatMessage> messages) {
        String last = messages.getLast().content().trim();
        if (last.startsWith(Web3Agent.OBSERVATION_PREFIX)) {
            return Json.write(Json.object().put("answer", last.substring(Web3Agent.OBSERVATION_PREFIX.length()).trim()));
        }
        String lower = last.toLowerCase(Locale.ROOT);
        Matcher m;
        if ((m = SEND.matcher(last)).find()) return call("send_eth", "to", m.group(2), "amount_eth", m.group(1));
        if ((m = SIGN.matcher(last)).find()) return call("sign_message", "message", m.group(1));
        if ((m = BROADCAST.matcher(last)).find()) return call("p2p_broadcast", "message", m.group(1));
        if ((m = STORE.matcher(last)).find()) return call("store_put", "text", m.group(1));
        if ((m = ERC20.matcher(last)).find())
            return m.group(2) == null ? call("erc20_balance", "token", m.group(1)) : call("erc20_balance", "token", m.group(1), "owner", m.group(2));
        if (BALANCE.matcher(last).find()) {
            m = ANY_ADDR.matcher(last);
            return m.find() ? call("get_balance", "address", m.group(1)) : call("get_balance");
        }
        if ((m = ANY_CID.matcher(last)).find())
            return lower.contains("share") || last.contains("分享") ? call("store_share", "cid", m.group(1)) : call("store_get", "cid", m.group(1));
        if (lower.contains("peer") || last.contains("節點")) return call("p2p_peers");
        if (lower.contains("inbox") || lower.contains("messages") || last.contains("訊息")) return call("p2p_inbox");
        if (lower.contains("chain") || lower.contains("block") || last.contains("區塊")) return call("chain_info");
        if (lower.contains("address") || lower.contains("who am i") || last.contains("地址")) return call("wallet_address");
        return Json.write(Json.object().put("answer", """
                Mock LLM understands: "address", "balance [0x..]", "send 0.01 eth to 0x..", "token 0x<erc20> [0x<owner>]",
                "chain info", "sign <text>", "broadcast <text>", "peers", "inbox", "store <text>", "<cid>", "share <cid>".
                Use --llm openai for free-form natural language."""));
    }

    private static String call(String tool, String... kv) {
        ObjectNode n = Json.object().put("tool", tool);
        ObjectNode args = n.putObject("args");
        for (int i = 0; i + 1 < kv.length; i += 2) args.put(kv[i], kv[i + 1]);
        return Json.write(n);
    }
}
