package io.stonepotatoes.web3agent.brain.planning;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.UUID;

/**
 * The Planner component of the Agent Brain.
 * Responsible for goal decomposition, task sequencing, and resource allocation.
 */
public class Planner {
    private final List<Goal> activeGoals;
    private final List<Task> taskQueue;
    private final List<Task> completedTasks;
    private final List<Task> failedTasks;

    public Planner() {
        this.activeGoals = new ArrayList<>();
        this.taskQueue = new ArrayList<>();
        this.completedTasks = new ArrayList<>();
        this.failedTasks = new ArrayList<>();
    }

    /**
     * Decompose a high-level goal into sub-goals and tasks.
     */
    public List<Goal> decomposeGoal(Goal goal) {
        List<Goal> subGoals = new ArrayList<>();

        switch (goal.type()) {
            case STRATEGIC:
                // Decompose strategic goals into tactical goals
                subGoals.addAll(decomposeStrategicGoal(goal));
                break;
            case TACTICAL:
                // Decompose tactical goals into operational goals
                subGoals.addAll(decomposeTacticalGoal(goal));
                break;
            case OPERATIONAL:
                // Decompose operational goals into action goals
                subGoals.addAll(decomposeOperationalGoal(goal));
                break;
            case ACTION:
                // Action goals map directly to tasks
                taskQueue.addAll(createTasksFromGoal(goal));
                break;
            case REACTIVE:
                // Reactive goals require immediate action
                taskQueue.addAll(createTasksFromGoal(goal));
                break;
        }

        // Set parent relationships
        for (Goal subGoal : subGoals) {
            subGoal.setParentId(goal.id());
        }

        return subGoals;
    }

    /**
     * Create a sequence of tasks to accomplish a goal.
     */
    public List<Task> createTasksFromGoal(Goal goal) {
        List<Task> tasks = new ArrayList<>();
        String goalDesc = goal.description().toLowerCase();

        // Example task creation based on goal description
        if (goalDesc.contains("balance") || goalDesc.contains("check")) {
            tasks.add(new Task(
                UUID.randomUUID().toString(),
                "Check balance",
                "get_balance",
                Map.of()
            ));
        } else if (goalDesc.contains("send") || goalDesc.contains("transfer")) {
            tasks.add(new Task(
                UUID.randomUUID().toString(),
                "Send transaction",
                "send_eth",
                Map.of()
            ));
        } else if (goalDesc.contains("deploy") || goalDesc.contains("contract")) {
            tasks.add(new Task(
                UUID.randomUUID().toString(),
                "Deploy contract",
                "deploy_token",
                Map.of()
            ));
        }

        return tasks;
    }

    /**
     * Sequence tasks for optimal execution order.
     */
    public List<Task> sequenceTasks(List<Task> tasks) {
        // Simple ordering by estimated duration
        // In a full implementation, this would consider dependencies
        return tasks.stream()
                .sorted((t1, t2) -> Long.compare(t1.estimatedDurationMs(), t2.estimatedDurationMs()))
                .toList();
    }

    /**
     * Allocate resources for task execution.
     */
    public ResourceAllocation allocateResources(Task task) {
        // Simple resource allocation
        // In a full implementation, this would consider:
        // - CPU/GPU availability
        // - Network bandwidth
        // - Rate limits
        // - Priority
        return new ResourceAllocation(
            task.id(),
            ResourceLevel.NORMAL,
            1, // CPU cores
            0, // GPU units (for AI tasks)
            1024 * 1024 // Memory in bytes
        );
    }

    /**
     * Get the next task to execute from the queue.
     */
    public Task getNextTask() {
        if (taskQueue.isEmpty()) {
            return null;
        }
        return taskQueue.remove(0);
    }

    /**
     * Add a task to the queue.
     */
    public void enqueueTask(Task task) {
        taskQueue.add(task);
    }

    /**
     * Mark a task as completed.
     */
    public void completeTask(Task task) {
        task.setStatus(TaskStatus.COMPLETED);
        completedTasks.add(task);
    }

    /**
     * Mark a task as failed.
     */
    public void failTask(Task task, String error) {
        task.setStatus(TaskStatus.FAILED);
        task.setError(error);
        failedTasks.add(task);
    }

    /**
     * Get all active goals.
     */
    public List<Goal> getActiveGoals() {
        return new ArrayList<>(activeGoals);
    }

    /**
     * Add a goal to the active goals.
     */
    public void addGoal(Goal goal) {
        activeGoals.add(goal);
    }

    /**
     * Get task queue size.
     */
    public int getTaskQueueSize() {
        return taskQueue.size();
    }

    /**
     * Get completed tasks.
     */
    public List<Task> getCompletedTasks() {
        return new ArrayList<>(completedTasks);
    }

    /**
     * Get failed tasks.
     */
    public List<Task> getFailedTasks() {
        return new ArrayList<>(failedTasks);
    }

    // Helper methods for goal decomposition
    private List<Goal> decomposeStrategicGoal(Goal strategicGoal) {
        List<Goal> tacticalGoals = new ArrayList<>();
        // Example: "Build DeFi portfolio" -> "Research strategies", "Select protocols", "Execute trades"
        tacticalGoals.add(new Goal(
            UUID.randomUUID().toString(),
            "Research DeFi strategies",
            GoalType.TACTICAL,
            strategicGoal.priority() - 1
        ));
        tacticalGoals.add(new Goal(
            UUID.randomUUID().toString(),
            "Select protocols to use",
            GoalType.TACTICAL,
            strategicGoal.priority() - 1
        ));
        return tacticalGoals;
    }

    private List<Goal> decomposeTacticalGoal(Goal tacticalGoal) {
        List<Goal> operationalGoals = new ArrayList<>();
        // Example: "Deploy smart contract" -> "Write code", "Compile", "Deploy", "Verify"
        operationalGoals.add(new Goal(
            UUID.randomUUID().toString(),
            "Prepare deployment",
            GoalType.OPERATIONAL,
            tacticalGoal.priority() - 1
        ));
        operationalGoals.add(new Goal(
            UUID.randomUUID().toString(),
            "Execute deployment",
            GoalType.OPERATIONAL,
            tacticalGoal.priority() - 1
        ));
        return operationalGoals;
    }

    private List<Goal> decomposeOperationalGoal(Goal operationalGoal) {
        List<Goal> actionGoals = new ArrayList<>();
        // Example: "Check balance" -> "Call get_balance tool"
        actionGoals.add(new Goal(
            UUID.randomUUID().toString(),
            operationalGoal.description(),
            GoalType.ACTION,
            operationalGoal.priority() - 1
        ));
        return actionGoals;
    }

    /**
     * Resource allocation details.
     */
    public record ResourceAllocation(
        String taskId,
        ResourceLevel level,
        int cpuCores,
        int gpuUnits,
        long memoryBytes
    ) {}

    /**
     * Resource usage levels.
     */
    public enum ResourceLevel {
        LOW,
        NORMAL,
        HIGH,
        CRITICAL
    }
}
