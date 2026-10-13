package io.stonepotatoes.web3agent.llm;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.condition.EnabledIf;

import java.util.List;

import static org.junit.jupiter.api.Assertions.*;

/**
 * Integration test against a real local LLM server (llama.cpp). Requires the server running on
 * http://127.0.0.1:11434. Enabled only when the environment variable LLM_TEST=1 is set.
 */
@EnabledIf("llmRunning")
class RealLlmTest {
    static final String LLM_URL = "http://127.0.0.1:11434/v1";
    static final String LLM_MODEL = "qwen2.5:7b";

    static boolean llmRunning() {
        return "1".equals(System.getenv("LLM_TEST"));
    }

    @Test
    void completesAgainstRealLlm() throws Exception {
        LlmProvider llm = new OpenAiCompatibleProvider(LLM_URL, LLM_MODEL, null);
        List<ChatMessage> msgs = List.of(new ChatMessage("user", "Say exactly: hello world"));
        String resp = llm.complete(msgs);
        assertNotNull(resp);
        assertTrue(resp.toLowerCase().contains("hello") || resp.toLowerCase().contains("world"),
                "Response should contain 'hello' or 'world': " + resp);
    }
}
