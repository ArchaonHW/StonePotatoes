package io.stonepotatoes.web3agent.brain.execution;

import io.stonepotatoes.web3agent.brain.planning.Task;
import io.stonepotatoes.web3agent.brain.planning.TaskStatus;
import java.util.Map;
import java.util.UUID;

/**
 * The Execution component of the Agent Brain.
 * Responsible for tool invocation, action monitoring, and error recovery.
 */
public class Executor {
    private final ToolRegistry toolRegistry;
    private final ExecutionMonitor monitor;
    private final ErrorRecovery recovery;

    public Executor() {
        this.toolRegistry = new ToolRegistry();
        this.monitor = new ExecutionMonitor();
        this.recovery = new ErrorRecovery();
    }

    /**
     * Execute a task by invoking the appropriate tool.
     */
    public ExecutionResult executeTask(Task task) {
        String taskId = task.id();
        monitor.startExecution(taskId);

        try {
            // Get the tool
            Tool tool = toolRegistry.getTool(task.toolName());
            if (tool == null) {
                throw new ExecutionException("Tool not found: " + task.toolName());
            }

            // Execute the tool
            long startTime = System.currentTimeMillis();
            String result = tool.execute(task.parameters());
            long duration = System.currentTimeMillis() - startTime;

            // Update task status
            task.setStatus(TaskStatus.COMPLETED);
            task.setResult(result);
            task.setActualDurationMs(duration);

            monitor.completeExecution(taskId, duration);
            return new ExecutionResult(taskId, true, result, null, duration);

        } catch (Exception e) {
            // Handle error
            task.setStatus(TaskStatus.FAILED);
            task.setError(e.getMessage());

            monitor.failExecution(taskId, e);

            // Attempt recovery
            RecoveryAction action = recovery.recover(task, e);
            if (action != RecoveryAction.NONE) {
                return executeTask(task); // Retry
            }

            return new ExecutionResult(taskId, false, null, e.getMessage(), 0);
        }
    }

    /**
     * Execute multiple tasks in sequence.
     */
    public SequentialExecutionResult executeSequential(java.util.List<Task> tasks) {
        java.util.List<ExecutionResult> results = new java.util.ArrayList<>();
        boolean allSucceeded = true;

        for (Task task : tasks) {
            ExecutionResult result = executeTask(task);
            results.add(result);
            if (!result.success()) {
                allSucceeded = false;
                break; // Stop on first failure
            }
        }

        return new SequentialExecutionResult(results, allSucceeded);
    }

    /**
     * Execute multiple tasks in parallel.
     */
    public ParallelExecutionResult executeParallel(java.util.List<Task> tasks) {
        // Simple parallel execution - in full implementation would use thread pool
        java.util.List<ExecutionResult> results = new java.util.ArrayList<>();

        for (Task task : tasks) {
            ExecutionResult result = executeTask(task);
            results.add(result);
        }

        boolean allSucceeded = results.stream().allMatch(ExecutionResult::success);
        return new ParallelExecutionResult(results, allSucceeded);
    }

    /**
     * Monitor the execution of a task.
     */
    public ExecutionStatus getExecutionStatus(String taskId) {
        return monitor.getStatus(taskId);
    }

    /**
     * Cancel an ongoing execution.
     */
    public boolean cancelExecution(String taskId) {
        return monitor.cancel(taskId);
    }

    /**
     * Register a tool with the registry.
     */
    public void registerTool(Tool tool) {
        toolRegistry.register(tool);
    }

    /**
     * Get execution statistics.
     */
    public ExecutionStatistics getStatistics() {
        return monitor.getStatistics();
    }

    // Supporting classes
    public static class ToolRegistry {
        private final java.util.Map<String, Tool> tools;

        public ToolRegistry() {
            this.tools = new java.util.HashMap<>();
        }

        public void register(Tool tool) {
            tools.put(tool.name(), tool);
        }

        public Tool getTool(String name) {
            return tools.get(name);
        }
    }

    public static class ExecutionMonitor {
        private final java.util.Map<String, ExecutionStatus> activeExecutions;
        private final java.util.List<ExecutionRecord> history;

        public ExecutionMonitor() {
            this.activeExecutions = new java.util.HashMap<>();
            this.history = new java.util.ArrayList<>();
        }

        public void startExecution(String taskId) {
            activeExecutions.put(taskId, new ExecutionStatus(taskId, ExecutionState.RUNNING));
        }

        public void completeExecution(String taskId, long duration) {
            ExecutionStatus status = activeExecutions.get(taskId);
            if (status != null) {
                status.setState(ExecutionState.COMPLETED);
                status.setDuration(duration);
                history.add(new ExecutionRecord(taskId, ExecutionState.COMPLETED, duration));
                activeExecutions.remove(taskId);
            }
        }

        public void failExecution(String taskId, Exception e) {
            ExecutionStatus status = activeExecutions.get(taskId);
            if (status != null) {
                status.setState(ExecutionState.FAILED);
                status.setError(e.getMessage());
                history.add(new ExecutionRecord(taskId, ExecutionState.FAILED, 0));
                activeExecutions.remove(taskId);
            }
        }

        public ExecutionStatus getStatus(String taskId) {
            return activeExecutions.get(taskId);
        }

        public boolean cancel(String taskId) {
            ExecutionStatus status = activeExecutions.get(taskId);
            if (status != null && status.state() == ExecutionState.RUNNING) {
                status.setState(ExecutionState.CANCELLED);
                activeExecutions.remove(taskId);
                return true;
            }
            return false;
        }

        public ExecutionStatistics getStatistics() {
            return new ExecutionStatistics(
                history.size(),
                (int) history.stream().filter(r -> r.state() == ExecutionState.COMPLETED).count(),
                (int) history.stream().filter(r -> r.state() == ExecutionState.FAILED).count(),
                history.stream().mapToLong(ExecutionRecord::duration).average().orElse(0)
            );
        }
    }

    public static class ErrorRecovery {
        public RecoveryAction recover(Task task, Exception error) {
            // Simple recovery strategy
            if (error instanceof java.net.ConnectException) {
                return RecoveryAction.RETRY;
            } else if (error instanceof java.util.concurrent.TimeoutException) {
                return RecoveryAction.RETRY_WITH_BACKOFF;
            } else {
                return RecoveryAction.NONE;
            }
        }
    }

    // Result and status classes
    public static class ExecutionResult {
        private final String taskId;
        private final boolean success;
        private final String result;
        private final String error;
        private final long duration;

        public ExecutionResult(String taskId, boolean success, String result, String error, long duration) {
            this.taskId = taskId;
            this.success = success;
            this.result = result;
            this.error = error;
            this.duration = duration;
        }

        public boolean success() {
            return success;
        }

        public String result() {
            return result;
        }

        public String error() {
            return error;
        }

        public long duration() {
            return duration;
        }
    }

    public static class SequentialExecutionResult {
        private final java.util.List<ExecutionResult> results;
        private final boolean allSucceeded;

        public SequentialExecutionResult(java.util.List<ExecutionResult> results, boolean allSucceeded) {
            this.results = results;
            this.allSucceeded = allSucceeded;
        }

        public java.util.List<ExecutionResult> results() {
            return results;
        }

        public boolean allSucceeded() {
            return allSucceeded;
        }
    }

    public static class ParallelExecutionResult {
        private final java.util.List<ExecutionResult> results;
        private final boolean allSucceeded;

        public ParallelExecutionResult(java.util.List<ExecutionResult> results, boolean allSucceeded) {
            this.results = results;
            this.allSucceeded = allSucceeded;
        }

        public java.util.List<ExecutionResult> results() {
            return results;
        }

        public boolean allSucceeded() {
            return allSucceeded;
        }
    }

    public static class ExecutionStatus {
        private final String taskId;
        private ExecutionState state;
        private long duration;
        private String error;

        public ExecutionStatus(String taskId, ExecutionState state) {
            this.taskId = taskId;
            this.state = state;
        }

        public ExecutionState state() {
            return state;
        }

        public void setState(ExecutionState state) {
            this.state = state;
        }

        public long duration() {
            return duration;
        }

        public void setDuration(long duration) {
            this.duration = duration;
        }

        public String error() {
            return error;
        }

        public void setError(String error) {
            this.error = error;
        }
    }

    public static class ExecutionStatistics {
        private final int totalExecutions;
        private final int successfulExecutions;
        private final int failedExecutions;
        private final double averageDuration;

        public ExecutionStatistics(int total, int successful, int failed, double averageDuration) {
            this.totalExecutions = total;
            this.successfulExecutions = successful;
            this.failedExecutions = failed;
            this.averageDuration = averageDuration;
        }

        public int totalExecutions() {
            return totalExecutions;
        }

        public int successfulExecutions() {
            return successfulExecutions;
        }

        public int failedExecutions() {
            return failedExecutions;
        }

        public double averageDuration() {
            return averageDuration;
        }
    }

    public static class ExecutionRecord {
        private final String taskId;
        private final ExecutionState state;
        private final long duration;

        public ExecutionRecord(String taskId, ExecutionState state, long duration) {
            this.taskId = taskId;
            this.state = state;
            this.duration = duration;
        }

        public ExecutionState state() {
            return state;
        }

        public long duration() {
            return duration;
        }
    }

    // Enums and interfaces
    public enum ExecutionState {
        PENDING, RUNNING, COMPLETED, FAILED, CANCELLED
    }

    public enum RecoveryAction {
        NONE, RETRY, RETRY_WITH_BACKOFF, FALLBACK, ABORT
    }

    public interface Tool {
        String name();
        String execute(Map<String, Object> parameters) throws Exception;
    }

    public static class ExecutionException extends RuntimeException {
        public ExecutionException(String message) {
            super(message);
        }
    }
}
