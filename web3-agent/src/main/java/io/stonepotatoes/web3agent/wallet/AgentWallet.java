package io.stonepotatoes.web3agent.wallet;

import org.web3j.crypto.Credentials;
import org.web3j.crypto.Keys;
import org.web3j.crypto.RawTransaction;
import org.web3j.crypto.Sign;
import org.web3j.crypto.TransactionEncoder;
import org.web3j.crypto.WalletUtils;
import org.web3j.utils.Numeric;

import java.io.IOException;
import java.math.BigInteger;
import java.nio.file.Files;
import java.nio.file.Path;
import java.security.GeneralSecurityException;
import java.security.SignatureException;
import java.util.Arrays;
import java.util.stream.Stream;

/**
 * Local secp256k1 identity. The private key never leaves this class: callers can only obtain
 * signatures, never the key itself. The key is persisted as an encrypted Ethereum V3 keystore.
 */
public final class AgentWallet {
    private final Credentials credentials;

    private AgentWallet(Credentials credentials) {
        this.credentials = credentials;
    }

    public static AgentWallet fromCredentials(Credentials credentials) {
        return new AgentWallet(credentials);
    }

    public static AgentWallet random() {
        try {
            return new AgentWallet(Credentials.create(Keys.createEcKeyPair()));
        } catch (GeneralSecurityException e) {
            throw new IllegalStateException("Cannot generate key pair", e);
        }
    }

    /**
     * Loads the first keystore in {@code keystoreDir}, or creates a new one.
     *
     * @param fullScrypt use the standard (slow, stronger) scrypt parameters; tests may pass false
     */
    public static AgentWallet loadOrCreate(Path keystoreDir, String password, boolean fullScrypt) throws IOException {
        Files.createDirectories(keystoreDir);
        try {
            Path existing;
            try (Stream<Path> files = Files.list(keystoreDir)) {
                existing = files.filter(p -> p.getFileName().toString().startsWith("UTC--")).sorted().findFirst().orElse(null);
            }
            if (existing == null) {
                String name = WalletUtils.generateNewWalletFile(password, keystoreDir.toFile(), fullScrypt);
                existing = keystoreDir.resolve(name);
            }
            return new AgentWallet(WalletUtils.loadCredentials(password, existing.toFile()));
        } catch (IOException e) {
            throw e;
        } catch (Exception e) {
            throw new IOException("Cannot open keystore (wrong password?): " + e.getMessage(), e);
        }
    }

    public String address() {
        return credentials.getAddress();
    }

    /** EIP-191 personal_sign. Returns 65-byte r||s||v hex. */
    public String signMessage(byte[] message) {
        Sign.SignatureData sig = Sign.signPrefixedMessage(message, credentials.getEcKeyPair());
        byte[] out = new byte[65];
        System.arraycopy(sig.getR(), 0, out, 0, 32);
        System.arraycopy(sig.getS(), 0, out, 32, 32);
        out[64] = sig.getV()[0];
        return Numeric.toHexString(out);
    }

    /** Signs a transaction with EIP-155 replay protection and returns the raw encoded hex. */
    public String signTransaction(RawTransaction tx, long chainId) {
        return Numeric.toHexString(TransactionEncoder.signMessage(tx, chainId, credentials));
    }

    public static String recoverAddress(byte[] message, String signatureHex) throws SignatureException {
        byte[] sig = Numeric.hexStringToByteArray(signatureHex);
        if (sig.length != 65) throw new SignatureException("Signature must be 65 bytes");
        byte v = sig[64];
        if (v < 27) v += 27;
        Sign.SignatureData data = new Sign.SignatureData(v, Arrays.copyOfRange(sig, 0, 32), Arrays.copyOfRange(sig, 32, 64));
        BigInteger publicKey = Sign.signedPrefixedMessageToKey(message, data);
        return "0x" + Keys.getAddress(publicKey);
    }

    public static boolean verify(byte[] message, String signatureHex, String expectedAddress) {
        try {
            return expectedAddress != null && recoverAddress(message, signatureHex).equalsIgnoreCase(expectedAddress);
        } catch (SignatureException | RuntimeException e) {
            return false;
        }
    }
}
