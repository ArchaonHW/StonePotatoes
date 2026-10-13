package io.stonepotatoes.web3agent.brain.planning;

/**
 * Types of goals the Agent can pursue.
 */
public enum GoalType {
    /**
     * Long-term strategic goals (e.g., "Build a DeFi portfolio")
     */
    STRATEGIC,

    /**
     * Medium-term tactical goals (e.g., "Deploy a smart contract")
     */
    TACTICAL,

    /**
     * Short-term operational goals (e.g., "Check balance")
     */
    OPERATIONAL,

    /**
     * Immediate action goals (e.g., "Call a specific function")
     */
    ACTION,

    /**
     * Reactive goals (e.g., "Handle an error")
     */
    REACTIVE
}
