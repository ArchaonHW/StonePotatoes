package io.stonepotatoes.web3agent.llm;

import java.io.IOException;
import java.util.List;

/** Pluggable chat-completion backend. Implementations must be safe to call from one thread at a time. */
public interface LlmProvider {
    String name();

    String complete(List<ChatMessage> messages) throws IOException, InterruptedException;
}
