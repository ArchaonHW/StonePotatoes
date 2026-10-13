package io.stonepotatoes.web3agent.p2p;

import org.web3j.crypto.Hash;
import org.web3j.utils.Numeric;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.Optional;
import java.util.regex.Pattern;

/** Local content-addressed blob store. CID = keccak256(content), so data is self-verifying. */
public final class ContentStore {
    private static final Pattern CID = Pattern.compile("^0x[0-9a-f]{64}$");
    public static final int MAX_BLOB_BYTES = 256 * 1024;

    private final Path dir;

    public ContentStore(Path dir) throws IOException {
        this.dir = Files.createDirectories(dir);
    }

    public static String cidOf(byte[] data) {
        return Numeric.toHexString(Hash.sha3(data));
    }

    public static boolean isValidCid(String cid) {
        return cid != null && CID.matcher(cid).matches();
    }

    public String put(byte[] data) throws IOException {
        if (data.length > MAX_BLOB_BYTES) throw new IOException("Blob exceeds " + MAX_BLOB_BYTES + " bytes");
        String cid = cidOf(data);
        Path target = dir.resolve(cid);
        if (!Files.exists(target)) {
            Path tmp = Files.createTempFile(dir, "put-", ".tmp");
            Files.write(tmp, data);
            Files.move(tmp, target, StandardCopyOption.REPLACE_EXISTING, StandardCopyOption.ATOMIC_MOVE);
        }
        return cid;
    }

    public Optional<byte[]> get(String cid) throws IOException {
        if (!isValidCid(cid)) throw new IllegalArgumentException("Invalid CID: " + cid);
        Path file = dir.resolve(cid);
        if (!Files.exists(file)) return Optional.empty();
        byte[] data = Files.readAllBytes(file);
        if (!cidOf(data).equals(cid)) throw new IOException("Content corrupted: hash mismatch for " + cid);
        return Optional.of(data);
    }
}
