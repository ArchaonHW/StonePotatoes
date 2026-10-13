package io.stonepotatoes.web3agent.agent;

import tools.jackson.databind.JsonNode;

/**
 * @param args human/LLM-readable argument spec, e.g. {@code {"to":"0x address","amount_eth":"decimal"}}
 */
public record Tool(String name, String description, String args, Handler handler) {
    @FunctionalInterface
    public interface Handler {
        String execute(JsonNode args) throws Exception;
    }
}
