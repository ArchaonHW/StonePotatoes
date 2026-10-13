package io.stonepotatoes.web3agent.util;

import tools.jackson.databind.JsonNode;
import tools.jackson.databind.ObjectMapper;
import tools.jackson.databind.json.JsonMapper;
import tools.jackson.databind.node.ObjectNode;

public final class Json {
    public static final ObjectMapper MAPPER = JsonMapper.builder().build();

    private Json() {}

    public static ObjectNode object() {
        return MAPPER.createObjectNode();
    }

    public static JsonNode parse(String text) {
        return MAPPER.readTree(text);
    }

    public static String write(Object value) {
        return MAPPER.writeValueAsString(value);
    }

    /** Extracts the first balanced top-level JSON object from free-form text (e.g. LLM output), or null. */
    public static JsonNode extractObject(String text) {
        if (text == null) return null;
        for (int start = text.indexOf('{'); start >= 0; start = text.indexOf('{', start + 1)) {
            int depth = 0;
            boolean inString = false, escaped = false;
            for (int i = start; i < text.length(); i++) {
                char c = text.charAt(i);
                if (inString) {
                    if (escaped) escaped = false;
                    else if (c == '\\') escaped = true;
                    else if (c == '"') inString = false;
                } else if (c == '"') inString = true;
                else if (c == '{') depth++;
                else if (c == '}' && --depth == 0) {
                    try {
                        JsonNode node = parse(text.substring(start, i + 1));
                        if (node.isObject()) return node;
                    } catch (RuntimeException ignored) {
                        // not valid JSON, try the next '{'
                    }
                    break;
                }
            }
        }
        return null;
    }
}
