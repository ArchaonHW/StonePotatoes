package io.stonepotatoes.web3agent.brain.planning;

import java.util.List;

/**
 * Represents a goal in the Agent's planning system.
 * Goals can be decomposed into sub-goals and tasks.
 */
public class Goal {
    private final String id;
    private final String description;
    private final GoalType type;
    private final int priority;
    private final List<Goal> subGoals;
    private GoalStatus status;
    private String parentId;

    public Goal(String id, String description, GoalType type, int priority) {
        this.id = id;
        this.description = description;
        this.type = type;
        this.priority = priority;
        this.subGoals = List.of();
        this.status = GoalStatus.PENDING;
    }

    public Goal(String id, String description, GoalType type, int priority, List<Goal> subGoals) {
        this.id = id;
        this.description = description;
        this.type = type;
        this.priority = priority;
        this.subGoals = subGoals;
        this.status = GoalStatus.PENDING;
    }

    public String id() {
        return id;
    }

    public String description() {
        return description;
    }

    public GoalType type() {
        return type;
    }

    public int priority() {
        return priority;
    }

    public List<Goal> subGoals() {
        return subGoals;
    }

    public GoalStatus status() {
        return status;
    }

    public void setStatus(GoalStatus status) {
        this.status = status;
    }

    public String parentId() {
        return parentId;
    }

    public void setParentId(String parentId) {
        this.parentId = parentId;
    }

    public boolean isLeaf() {
        return subGoals.isEmpty();
    }

    public boolean isCompleted() {
        return status == GoalStatus.COMPLETED;
    }

    public boolean isFailed() {
        return status == GoalStatus.FAILED;
    }

    public boolean isInProgress() {
        return status == GoalStatus.IN_PROGRESS;
    }
}
