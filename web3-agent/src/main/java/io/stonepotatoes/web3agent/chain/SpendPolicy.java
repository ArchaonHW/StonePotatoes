package io.stonepotatoes.web3agent.chain;

import java.math.BigInteger;
import java.time.Clock;
import java.time.LocalDate;

/**
 * Guard rail between the (untrusted) LLM and the wallet: every state-changing transaction must
 * pass per-transaction and daily native-currency limits and an explicit approval callback (e.g. a
 * human prompt) that sees a human-readable description of the action. Gas fees are not counted.
 */
public final class SpendPolicy {
    @FunctionalInterface
    public interface Approver {
        boolean approve(String description);
    }

    public static final class PolicyViolationException extends Exception {
        public PolicyViolationException(String message) {
            super(message);
        }
    }

    private final BigInteger maxPerTxWei;
    private final BigInteger maxDailyWei;
    private final Approver approver;
    private final Clock clock;
    private LocalDate day;
    private BigInteger spentToday = BigInteger.ZERO;

    public SpendPolicy(BigInteger maxPerTxWei, BigInteger maxDailyWei, Approver approver, Clock clock) {
        this.maxPerTxWei = maxPerTxWei;
        this.maxDailyWei = maxDailyWei;
        this.approver = approver;
        this.clock = clock;
        this.day = LocalDate.now(clock);
    }

    public SpendPolicy(BigInteger maxPerTxWei, BigInteger maxDailyWei, Approver approver) {
        this(maxPerTxWei, maxDailyWei, approver, Clock.systemUTC());
    }

    /** @param valueWei native value attached to the transaction (may be zero for contract calls) */
    public synchronized void authorize(String description, BigInteger valueWei) throws PolicyViolationException {
        rollDay();
        if (valueWei.signum() < 0) throw new PolicyViolationException("Amount must not be negative");
        if (valueWei.compareTo(maxPerTxWei) > 0)
            throw new PolicyViolationException("Amount exceeds per-transaction limit of " + maxPerTxWei + " wei");
        if (spentToday.add(valueWei).compareTo(maxDailyWei) > 0)
            throw new PolicyViolationException("Amount exceeds remaining daily limit of " + remainingTodayWei() + " wei");
        if (!approver.approve(description)) throw new PolicyViolationException("Transaction rejected by approver");
    }

    public synchronized void record(BigInteger valueWei) {
        rollDay();
        spentToday = spentToday.add(valueWei);
    }

    public synchronized BigInteger remainingTodayWei() {
        rollDay();
        return maxDailyWei.subtract(spentToday).max(BigInteger.ZERO);
    }

    private void rollDay() {
        LocalDate today = LocalDate.now(clock);
        if (!today.equals(day)) {
            day = today;
            spentToday = BigInteger.ZERO;
        }
    }
}
