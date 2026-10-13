package io.stonepotatoes.web3agent.brain.planning;

/**
 * Status of a task in the execution system.
 */
public enum TaskStatus {
    /**
     * Task is pending execution
     */
    PENDING,

    /**
     * Task is currently being executed
     */
    IN_PROGRESS,

    /**
     * Task has been completed successfully
     */
    COMPLETED,

    /**
     * Task execution failed
     */
    FAILED,

    /**
     * Task has been cancelled
     */
    CANCELLED,

    /**
     * Task is waiting for dependencies
     */
    BLOCKED
}
