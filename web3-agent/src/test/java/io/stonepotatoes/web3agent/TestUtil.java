package io.stonepotatoes.web3agent;

import java.util.function.BooleanSupplier;

import static org.junit.jupiter.api.Assertions.fail;

public final class TestUtil {
    private TestUtil() {}

    public static void await(BooleanSupplier cond) throws InterruptedException {
        long deadline = System.currentTimeMillis() + 5_000;
        while (!cond.getAsBoolean()) {
            if (System.currentTimeMillis() > deadline) fail("condition not met in time");
            Thread.sleep(20);
        }
    }
}
