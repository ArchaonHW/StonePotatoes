package io.stonepotatoes.web3agent.chain;

import io.stonepotatoes.web3agent.wallet.AgentWallet;
import org.web3j.abi.FunctionEncoder;
import org.web3j.abi.FunctionReturnDecoder;
import org.web3j.abi.TypeReference;
import org.web3j.abi.datatypes.Address;
import org.web3j.abi.datatypes.Function;
import org.web3j.abi.datatypes.Type;
import org.web3j.abi.datatypes.generated.Uint256;
import org.web3j.crypto.RawTransaction;
import org.web3j.crypto.WalletUtils;
import org.web3j.protocol.Web3j;
import org.web3j.protocol.core.DefaultBlockParameterName;
import org.web3j.protocol.core.Response;
import org.web3j.protocol.core.methods.request.Transaction;
import org.web3j.protocol.http.HttpService;

import java.io.IOException;
import java.math.BigInteger;
import java.util.List;

/** Thin EVM JSON-RPC client (Anvil / Hardhat / Geth / any EVM node). */
public final class ChainClient implements AutoCloseable {
    private static final BigInteger TRANSFER_GAS = BigInteger.valueOf(21_000);

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

    public BigInteger erc20Balance(String token, String owner) throws IOException {
        requireAddress(token);
        requireAddress(owner);
        Function fn = new Function("balanceOf", List.of(new Address(owner)), List.of(new TypeReference<Uint256>() {}));
        String result = check(web3j.ethCall(
                Transaction.createEthCallTransaction(owner, token, FunctionEncoder.encode(fn)),
                DefaultBlockParameterName.LATEST).send()).getValue();
        @SuppressWarnings("rawtypes")
        List<Type> decoded = FunctionReturnDecoder.decode(result, fn.getOutputParameters());
        if (decoded.isEmpty()) throw new IOException("Token returned no data (not an ERC-20 contract?)");
        return (BigInteger) decoded.getFirst().getValue();
    }

    /** Sends native currency after the policy authorizes it. Returns the transaction hash. */
    public String sendEther(AgentWallet wallet, String to, BigInteger valueWei, SpendPolicy policy)
            throws IOException, SpendPolicy.PolicyViolationException {
        requireAddress(to);
        policy.authorize(to, valueWei);
        long chainId = chainId();
        BigInteger nonce = check(web3j.ethGetTransactionCount(wallet.address(), DefaultBlockParameterName.PENDING).send())
                .getTransactionCount();
        BigInteger gasPrice = check(web3j.ethGasPrice().send()).getGasPrice();
        RawTransaction tx = RawTransaction.createEtherTransaction(nonce, gasPrice, TRANSFER_GAS, to, valueWei);
        String hash = check(web3j.ethSendRawTransaction(wallet.signTransaction(tx, chainId)).send()).getTransactionHash();
        policy.record(valueWei);
        return hash;
    }

    private static void requireAddress(String address) {
        if (address == null || !WalletUtils.isValidAddress(address) || !address.startsWith("0x"))
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
