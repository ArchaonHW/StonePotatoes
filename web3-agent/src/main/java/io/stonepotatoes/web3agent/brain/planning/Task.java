package io.stonepotatoes.web3agent.brain.planning;

import java.util.Map;

/**
 * Represents a task that can be executed by the Agent.
 * Tasks are atomic units of work that accomplish goals.
 */
public class Task {
    private final String id;
    private final String description;
    private final String toolName;
    private final Map<String, Object> parameters;
    private TaskStatus status;
    private String result;
    private String error;
    private long estimatedDurationMs;
    private long actualDurationMs;

    public Task(String id, String description, String toolName, Map<String, Object> parameters) {
        this.id = id;
        this.description = description;
        this.toolName = toolName;
        this.parameters = parameters;
        this.status = TaskStatus.PENDING;
        this.estimatedDurationMs = 1000; // Default estimate
    }

    public String id() {
        return id;
    }

    public String description() {
        return description;
    }

    public String toolName() {
        return toolName;
    }

    public Map<String, Object> parameters() {
        return parameters;
    }

    public TaskStatus status() {
        return status;
    }

    public void setStatus(TaskStatus status) {
        this.status = status;
    }

    public String result() {
        return result;
    }

    public void setResult(String result) {
        this.result = result;
    }

    public String error() {
        return error;
    }

    public void setError(String error) {
        this.error = error;
    }

    public long estimatedDurationMs() {
        return estimatedDurationMs;
    }

    public void setEstimatedDurationMs(long estimatedDurationMs) {
        this.estimatedDurationMs = estimatedDurationMs;
    }

    public long actualDurationMs() {
        return actualDurationMs;
    }

    public void setActualDurationMs(long actualDurationMs) {
        this.actualDurationMs = actualDurationMs;
    }

    public boolean isCompleted() {
        return status == TaskStatus.COMPLETED;
    }

    public boolean isFailed() {
        return status == TaskStatus.FAILED;
    }

    public boolean isInProgress() {
        return status == TaskStatus.IN_PROGRESS;
    }
}
