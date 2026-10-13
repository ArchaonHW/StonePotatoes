package io.stonepotatoes.web3agent.chain;

import java.math.BigInteger;
import java.time.Clock;
import java.time.LocalDate;

/**
 * Guard rail between the (untrusted) LLM and the wallet: every outgoing transfer must pass
 * per-transaction and daily limits and an explicit approval callback (e.g. a human prompt).
 */
public final class SpendPolicy {
    @FunctionalInterface
    public interface Approver {
        boolean approve(String to, BigInteger valueWei);
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

    public synchronized void authorize(String to, BigInteger valueWei) throws PolicyViolationException {
        rollDay();
        if (valueWei.signum() <= 0) throw new PolicyViolationException("Amount must be positive");
        if (valueWei.compareTo(maxPerTxWei) > 0)
            throw new PolicyViolationException("Amount exceeds per-transaction limit of " + maxPerTxWei + " wei");
        if (spentToday.add(valueWei).compareTo(maxDailyWei) > 0)
            throw new PolicyViolationException("Amount exceeds remaining daily limit of " + remainingTodayWei() + " wei");
        if (!approver.approve(to, valueWei)) throw new PolicyViolationException("Transfer rejected by approver");
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
