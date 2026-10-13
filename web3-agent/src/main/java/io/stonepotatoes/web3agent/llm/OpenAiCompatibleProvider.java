package io.stonepotatoes.web3agent.llm;

import io.stonepotatoes.web3agent.util.Json;
import tools.jackson.databind.JsonNode;
import tools.jackson.databind.node.ArrayNode;
import tools.jackson.databind.node.ObjectNode;

import java.io.IOException;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.time.Duration;
import java.util.List;

/**
 * Any server exposing {@code POST /chat/completions} in the OpenAI format: Ollama ({@code
 * http://127.0.0.1:11434/v1}), llama.cpp server, LM Studio, vLLM, etc.
 */
public final class OpenAiCompatibleProvider implements LlmProvider {
    private final HttpClient http = HttpClient.newBuilder().connectTimeout(Duration.ofSeconds(10)).build();
    private final URI endpoint;
    private final String model;
    private final String apiKey;

    public OpenAiCompatibleProvider(String baseUrl, String model, String apiKey) {
        this.endpoint = URI.create(baseUrl.replaceAll("/+$", "") + "/chat/completions");
        this.model = model;
        this.apiKey = apiKey;
    }

    @Override
    public String name() {
        return "openai-compatible(" + model + " @ " + endpoint + ")";
    }

    @Override
    public String complete(List<ChatMessage> messages) throws IOException, InterruptedException {
        ObjectNode body = Json.object().put("model", model).put("temperature", 0).put("stream", false);
        ArrayNode arr = body.putArray("messages");
        for (ChatMessage m : messages) arr.addObject().put("role", m.role()).put("content", m.content());

        HttpRequest.Builder req = HttpRequest.newBuilder(endpoint)
                .timeout(Duration.ofMinutes(5))
                .header("Content-Type", "application/json")
                .POST(HttpRequest.BodyPublishers.ofString(Json.write(body)));
        if (apiKey != null && !apiKey.isBlank()) req.header("Authorization", "Bearer " + apiKey);

        HttpResponse<String> resp = http.send(req.build(), HttpResponse.BodyHandlers.ofString());
        if (resp.statusCode() / 100 != 2) throw new IOException("LLM HTTP " + resp.statusCode() + ": " + truncate(resp.body()));
        JsonNode content = Json.parse(resp.body()).path("choices").path(0).path("message").path("content");
        if (!content.isString()) throw new IOException("Unexpected LLM response: " + truncate(resp.body()));
        return content.asString();
    }

    private static String truncate(String s) {
        return s.length() > 500 ? s.substring(0, 500) + "..." : s;
    }
}
