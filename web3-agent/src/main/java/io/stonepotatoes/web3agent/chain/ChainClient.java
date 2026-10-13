package io.stonepotatoes.web3agent.chain;

import io.stonepotatoes.web3agent.wallet.AgentWallet;
import org.web3j.crypto.RawTransaction;
import org.web3j.crypto.WalletUtils;
import org.web3j.protocol.Web3j;
import org.web3j.protocol.core.DefaultBlockParameterName;
import org.web3j.protocol.core.Response;
import org.web3j.protocol.core.methods.request.Transaction;
import org.web3j.protocol.core.methods.response.EthBlock;
import org.web3j.protocol.core.methods.response.TransactionReceipt;
import org.web3j.protocol.http.HttpService;
import org.web3j.utils.Convert;

import java.io.IOException;
import java.io.InputStream;
import java.math.BigDecimal;
import java.math.BigInteger;
import java.nio.charset.StandardCharsets;
import java.time.Duration;
import java.util.List;
import java.util.Optional;

/** EVM JSON-RPC client (Anvil / Hardhat / Geth / any EVM node). Sends EIP-1559 transactions when supported. */
public final class ChainClient implements AutoCloseable {
    private static final BigInteger DEFAULT_PRIORITY_FEE = Convert.toWei("1", Convert.Unit.GWEI).toBigInteger();
    private static final AbiSignature ERC20_BALANCE = AbiSignature.parse("balanceOf(address)(uint256)");
    private static final AbiSignature ERC20_DECIMALS = AbiSignature.parse("decimals()(uint8)");
    private static final AbiSignature ERC20_SYMBOL = AbiSignature.parse("symbol()(string)");
    private static final AbiSignature ERC20_TRANSFER = AbiSignature.parse("transfer(address,uint256)(bool)");
    private static final List<String> AGENT_TOKEN_CTOR = AbiSignature.parseTypes("(string,string,uint256)");

    public record Receipt(String txHash, boolean success, BigInteger blockNumber, BigInteger gasUsed, String contractAddress) {
        @Override
        public String toString() {
            return "tx=" + txHash + " status=" + (success ? "success" : "REVERTED") + " block=" + blockNumber + " gasUsed=" + gasUsed
                    + (contractAddress != null ? " contract=" + contractAddress : "");
        }
    }

    public record TokenInfo(String symbol, int decimals) {}

    private final Web3j web3j;
    private final String rpcUrl;

    public ChainClient(String rpcUrl) {
        this.rpcUrl = rpcUrl;
        this.web3j = Web3j.build(new HttpService(rpcUrl));
    }

    public String rpcUrl() {
        return rpcUrl;
    }

    public long chainId() throws IOException {
        return check(web3j.ethChainId().send()).getChainId().longValueExact();
    }

    public BigInteger blockNumber() throws IOException {
        return check(web3j.ethBlockNumber().send()).getBlockNumber();
    }

    public BigInteger balanceWei(String address) throws IOException {
        requireAddress(address);
        return check(web3j.ethGetBalance(address, DefaultBlockParameterName.LATEST).send()).getBalance();
    }

    /** Read-only {@code eth_call}. */
    public List<String> call(String to, AbiSignature sig, List<String> args) throws IOException {
        requireAddress(to);
        String data = sig.encodeCall(args);
        String result = check(web3j.ethCall(Transaction.createEthCallTransaction(null, to, data), DefaultBlockParameterName.LATEST).send()).getValue();
        return sig.decodeOutputs(result);
    }

    public BigInteger erc20Balance(String token, String owner) throws IOException {
        requireAddress(owner);
        return new BigInteger(call(token, ERC20_BALANCE, List.of(owner)).getFirst());
    }

    public TokenInfo erc20Info(String token) throws IOException {
        return new TokenInfo(call(token, ERC20_SYMBOL, List.of()).getFirst(), Integer.parseInt(call(token, ERC20_DECIMALS, List.of()).getFirst()));
    }

    public String sendEther(AgentWallet wallet, String to, BigInteger valueWei, SpendPolicy policy)
            throws IOException, SpendPolicy.PolicyViolationException {
        requireAddress(to);
        if (valueWei.signum() <= 0) throw new SpendPolicy.PolicyViolationException("Amount must be positive");
        policy.authorize("Send " + eth(valueWei) + " ETH to " + to, valueWei);
        return submit(wallet, to, valueWei, "0x", policy);
    }

    /** State-changing contract call; always requires approval. */
    public String sendContractTx(AgentWallet wallet, String to, AbiSignature sig, List<String> args, BigInteger valueWei, SpendPolicy policy)
            throws IOException, SpendPolicy.PolicyViolationException {
        requireAddress(to);
        String data = sig.encodeCall(args);
        policy.authorize("Call " + to + " " + sig.canonical() + " args=" + args + (valueWei.signum() > 0 ? " value=" + eth(valueWei) + " ETH" : ""), valueWei);
        return submit(wallet, to, valueWei, data, policy);
    }

    /** Transfers {@code amount} whole/decimal token units, scaled by the token's {@code decimals()}. */
    public String erc20Transfer(AgentWallet wallet, String token, String to, BigDecimal amount, SpendPolicy policy)
            throws IOException, SpendPolicy.PolicyViolationException {
        requireAddress(to);
        TokenInfo info = erc20Info(token);
        BigInteger raw = amount.movePointRight(info.decimals()).toBigIntegerExact();
        if (raw.signum() <= 0) throw new SpendPolicy.PolicyViolationException("Amount must be positive");
        String data = ERC20_TRANSFER.encodeCall(List.of(to, raw.toString()));
        policy.authorize("Transfer " + amount.toPlainString() + " " + info.symbol() + " (token " + token + ") to " + to, BigInteger.ZERO);
        return submit(wallet, token, BigInteger.ZERO, data, policy);
    }

    public String deployAgentToken(AgentWallet wallet, String name, String symbol, BigInteger supplyRaw, SpendPolicy policy)
            throws IOException, SpendPolicy.PolicyViolationException {
        String data = agentTokenBytecode() + AbiSignature.encodeConstructor(AGENT_TOKEN_CTOR, List.of(name, symbol, supplyRaw.toString()));
        policy.authorize("Deploy ERC-20 AgentToken name=" + name + " symbol=" + symbol + " supply=" + supplyRaw + " (raw units)", BigInteger.ZERO);
        return submit(wallet, null, BigInteger.ZERO, data, policy);
    }

    public Optional<Receipt> receipt(String txHash) throws IOException {
        Optional<TransactionReceipt> r = check(web3j.ethGetTransactionReceipt(txHash).send()).getTransactionReceipt();
        return r.map(x -> new Receipt(x.getTransactionHash(), x.isStatusOK(), x.getBlockNumber(), x.getGasUsed(), x.getContractAddress()));
    }

    public Optional<Receipt> waitForReceipt(String txHash, Duration timeout) throws IOException, InterruptedException {
        long deadline = System.nanoTime() + timeout.toNanos();
        for (long sleep = 50; ; sleep = Math.min(sleep * 2, 1_000)) {
            Optional<Receipt> r = receipt(txHash);
            if (r.isPresent() || System.nanoTime() > deadline) return r;
            Thread.sleep(sleep);
        }
    }

    private synchronized String submit(AgentWallet wallet, String to, BigInteger value, String data, SpendPolicy policy) throws IOException {
        long chainId = chainId();
        BigInteger nonce = check(web3j.ethGetTransactionCount(wallet.address(), DefaultBlockParameterName.PENDING).send()).getTransactionCount();
        Transaction estimate = Transaction.createFunctionCallTransaction(wallet.address(), nonce, null, null, to, value, data);
        BigInteger gas = check(web3j.ethEstimateGas(estimate).send()).getAmountUsed();
        gas = gas.multiply(BigInteger.valueOf(12)).divide(BigInteger.TEN); // 20% headroom

        EthBlock.Block latest = check(web3j.ethGetBlockByNumber(DefaultBlockParameterName.LATEST, false).send()).getBlock();
        RawTransaction tx;
        if (latest != null && latest.getBaseFeePerGasRaw() != null) {
            var tip = web3j.ethMaxPriorityFeePerGas().send();
            BigInteger priority = tip.hasError() ? DEFAULT_PRIORITY_FEE : tip.getMaxPriorityFeePerGas();
            BigInteger maxFee = latest.getBaseFeePerGas().multiply(BigInteger.TWO).add(priority);
            tx = RawTransaction.createTransaction(chainId, nonce, gas, to == null ? "" : to, value, data, priority, maxFee);
        } else {
            BigInteger gasPrice = check(web3j.ethGasPrice().send()).getGasPrice();
            tx = to == null ? RawTransaction.createContractTransaction(nonce, gasPrice, gas, value, data)
                    : RawTransaction.createTransaction(nonce, gasPrice, gas, to, value, data);
        }
        String hash = check(web3j.ethSendRawTransaction(wallet.signTransaction(tx, chainId)).send()).getTransactionHash();
        policy.record(value);
        return hash;
    }

    static String agentTokenBytecode() {
        try (InputStream in = ChainClient.class.getResourceAsStream("/contracts/AgentToken.bin")) {
            if (in == null) throw new IllegalStateException("AgentToken.bin missing from classpath");
            return new String(in.readAllBytes(), StandardCharsets.US_ASCII).trim();
        } catch (IOException e) {
            throw new IllegalStateException(e);
        }
    }

    private static String eth(BigInteger wei) {
        return Convert.fromWei(new BigDecimal(wei), Convert.Unit.ETHER).stripTrailingZeros().toPlainString();
    }

    static void requireAddress(String address) {
        if (address == null || !address.startsWith("0x") || !WalletUtils.isValidAddress(address))
            throw new IllegalArgumentException("Invalid address: " + address);
    }

    private static <T extends Response<?>> T check(T response) throws IOException {
        if (response.hasError()) throw new IOException("RPC error " + response.getError().getCode() + ": " + response.getError().getMessage());
        return response;
    }

    @Override
    public void close() {
        web3j.shutdown();
    }
}
