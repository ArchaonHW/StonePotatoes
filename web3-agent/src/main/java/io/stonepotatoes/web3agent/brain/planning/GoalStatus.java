package io.stonepotatoes.web3agent.brain.planning;

/**
 * Status of a goal in the planning system.
 */
public enum GoalStatus {
    /**
     * Goal is pending execution
     */
    PENDING,

    /**
     * Goal is currently being executed
     */
    IN_PROGRESS,

    /**
     * Goal has been completed successfully
     */
    COMPLETED,

    /**
     * Goal execution failed
     */
    FAILED,

    /**
     * Goal has been blocked (waiting for dependencies)
     */
    BLOCKED,

    /**
     * Goal has been cancelled
     */
    CANCELLED
}
