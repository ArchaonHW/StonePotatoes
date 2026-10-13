package io.stonepotatoes.web3agent.p2p;

import io.stonepotatoes.web3agent.wallet.AgentWallet;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.io.OutputStreamWriter;
import java.io.Writer;
import java.net.Socket;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

import static io.stonepotatoes.web3agent.TestUtil.await;
import static org.junit.jupiter.api.Assertions.*;

class P2PTest {
    @TempDir
    Path dir;

    @Test
    void envelopeRoundTripAndTamperDetection() {
        AgentWallet w = AgentWallet.random();
        SignedEnvelope e = SignedEnvelope.create(w, "chat", "gm");
        SignedEnvelope parsed = SignedEnvelope.fromJson(e.toJson());
        assertEquals(e, parsed);
        assertTrue(parsed.verify());
        assertFalse(new SignedEnvelope(e.id(), e.from(), e.topic(), e.timestamp(), "rug", e.signature()).verify());
        assertFalse(new SignedEnvelope(e.id(), AgentWallet.random().address(), e.topic(), e.timestamp(), e.payload(), e.signature()).verify());
        assertFalse(new SignedEnvelope(e.id(), e.from(), e.topic(), e.timestamp() + 1, e.payload(), e.signature()).verify());
    }

    @Test
    void contentStoreIsSelfVerifying() throws Exception {
        ContentStore store = new ContentStore(dir);
        String cid = store.put("hello".getBytes(StandardCharsets.UTF_8));
        assertEquals(cid, store.put("hello".getBytes(StandardCharsets.UTF_8)));
        assertEquals("hello", new String(store.get(cid).orElseThrow(), StandardCharsets.UTF_8));
        assertTrue(store.get("0x" + "00".repeat(32)).isEmpty());
        assertThrows(IllegalArgumentException.class, () -> store.get("../../etc/passwd"));
        Files.writeString(dir.resolve(cid), "evil");
        assertThrows(java.io.IOException.class, () -> store.get(cid));
    }

    @Test
    void gossipReachesNonAdjacentPeersAndRejectsForgeries() throws Exception {
        AgentWallet wa = AgentWallet.random(), wb = AgentWallet.random(), wc = AgentWallet.random();
        try (PeerNode a = new PeerNode(wa, "127.0.0.1", 0);
             PeerNode b = new PeerNode(wb, "127.0.0.1", 0);
             PeerNode c = new PeerNode(wc, "127.0.0.1", 0)) {
            BlockingQueue<SignedEnvelope> atC = new LinkedBlockingQueue<>();
            c.onMessage(atC::add);
            a.connect("127.0.0.1", b.port());
            c.connect("127.0.0.1", b.port());
            await(() -> b.peers().size() == 2 && a.peers().size() == 1 && c.peers().size() == 1);
            assertEquals(List.of(wb.address()), a.peers());

            a.broadcast("chat", "gm from A");
            SignedEnvelope got = atC.poll(5, TimeUnit.SECONDS);
            assertNotNull(got, "message should be relayed A -> B -> C");
            assertEquals(wa.address(), got.from());
            assertEquals("gm from A", got.payload());

            // Forged message: attacker claims to be A but signs with its own key.
            AgentWallet attacker = AgentWallet.random();
            SignedEnvelope real = SignedEnvelope.create(attacker, "chat", "send me your ETH");
            SignedEnvelope forged = new SignedEnvelope(real.id(), wa.address(), real.topic(), real.timestamp(), real.payload(), real.signature());
            try (Socket s = new Socket("127.0.0.1", c.port());
                 Writer w = new OutputStreamWriter(s.getOutputStream(), StandardCharsets.UTF_8)) {
                w.write(SignedEnvelope.create(attacker, PeerNode.TOPIC_HELLO, "0").toJson() + "\n");
                w.write(forged.toJson() + "\n");
                w.write("not json\n");
                w.flush();
                a.broadcast("chat", "second");
                SignedEnvelope next = atC.poll(5, TimeUnit.SECONDS);
                assertNotNull(next);
                assertEquals("second", next.payload());
                assertNull(atC.poll(300, TimeUnit.MILLISECONDS), "forged message must be dropped");
            }

            // Replay of an already-seen message is dropped.
            try (Socket s = new Socket("127.0.0.1", c.port());
                 Writer w = new OutputStreamWriter(s.getOutputStream(), StandardCharsets.UTF_8)) {
                w.write(SignedEnvelope.create(attacker, PeerNode.TOPIC_HELLO, "0").toJson() + "\n");
                w.write(got.toJson() + "\n");
                w.flush();
                assertNull(atC.poll(500, TimeUnit.MILLISECONDS), "replayed message must be dropped");
            }
        }
    }
}
