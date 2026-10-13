package io.stonepotatoes.web3ide.agent.controller;

import org.springframework.beans.factory.annotation.Value;
import org.springframework.core.ParameterizedTypeReference;
import org.springframework.web.bind.annotation.*;
import org.springframework.web.reactive.function.client.WebClient;
import reactor.core.publisher.Mono;

import java.util.Map;

@RestController
@RequestMapping("/api/agent")
public class AgentController {

    @Value("${agent.http-url}")
    private String agentHttpUrl;

    private final WebClient webClient;

    public AgentController(WebClient.Builder webClientBuilder) {
        this.webClient = webClientBuilder.build();
    }

    @GetMapping("/info")
    public Mono<Map<String, Object>> info() {
        return webClient.get()
                .uri(agentHttpUrl + "/api/info")
                .retrieve()
                .bodyToMono(new ParameterizedTypeReference<Map<String, Object>>() {});
    }

    @PostMapping("/chat")
    public Mono<Map<String, Object>> chat(@RequestBody Map<String, String> request) {
        return webClient.post()
                .uri(agentHttpUrl + "/api/chat")
                .bodyValue(request)
                .retrieve()
                .bodyToMono(new ParameterizedTypeReference<Map<String, Object>>() {});
    }

    @GetMapping("/balance")
    public Mono<Map<String, Object>> balance(@RequestParam(required = false) String address) {
        String url = agentHttpUrl + "/api/balance";
        if (address != null) {
            url += "?address=" + address;
        }
        return webClient.get()
                .uri(url)
                .retrieve()
                .bodyToMono(new ParameterizedTypeReference<Map<String, Object>>() {});
    }
}
