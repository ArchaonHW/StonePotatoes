package io.stonepotatoes.web3agent.wallet;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Path;

import static org.junit.jupiter.api.Assertions.*;

class AgentWalletTest {
    @TempDir
    Path dir;

    @Test
    void keystoreIsPersistedAndReloaded() throws Exception {
        AgentWallet a = AgentWallet.loadOrCreate(dir, "pw", false);
        AgentWallet b = AgentWallet.loadOrCreate(dir, "pw", false);
        assertEquals(a.address(), b.address());
        assertThrows(IOException.class, () -> AgentWallet.loadOrCreate(dir, "wrong", false));
    }

    @Test
    void signAndVerify() throws Exception {
        AgentWallet w = AgentWallet.random();
        byte[] msg = "hello web3".getBytes(StandardCharsets.UTF_8);
        String sig = w.signMessage(msg);
        assertEquals(w.address(), AgentWallet.recoverAddress(msg, sig));
        assertTrue(AgentWallet.verify(msg, sig, w.address().toUpperCase().replace("0X", "0x")));
        assertFalse(AgentWallet.verify("tampered".getBytes(StandardCharsets.UTF_8), sig, w.address()));
        assertFalse(AgentWallet.verify(msg, sig, AgentWallet.random().address()));
        assertFalse(AgentWallet.verify(msg, "0x1234", w.address()));
    }
}
