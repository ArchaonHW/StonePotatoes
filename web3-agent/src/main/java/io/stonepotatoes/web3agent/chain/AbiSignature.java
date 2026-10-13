package io.stonepotatoes.web3agent.chain;

import org.web3j.abi.FunctionEncoder;
import org.web3j.abi.FunctionReturnDecoder;
import org.web3j.abi.TypeDecoder;
import org.web3j.abi.TypeReference;
import org.web3j.abi.datatypes.Function;
import org.web3j.abi.datatypes.Type;
import org.web3j.utils.Numeric;

import java.math.BigInteger;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Human-writable ABI signature in the style of {@code cast}: {@code name(inTypes)(outTypes)}, e.g.
 * {@code balanceOf(address)(uint256)}. Supports atomic types (address, bool, string, bytes, bytesN,
 * uintN, intN); arrays and tuples are rejected.
 */
public record AbiSignature(String name, List<String> inputs, List<String> outputs) {
    private static final Pattern SIG = Pattern.compile("^\\s*([A-Za-z_][A-Za-z0-9_]*)\\s*\\(([^()]*)\\)\\s*(?:\\(([^()]*)\\))?\\s*$");
    private static final Pattern ATOMIC = Pattern.compile("^(address|bool|string|bytes([1-9]|[12][0-9]|3[0-2])?|u?int(8|16|24|32|40|48|56|64|72|80|88|96|104|112|120|128|136|144|152|160|168|176|184|192|200|208|216|224|232|240|248|256)?)$");

    public static AbiSignature parse(String signature) {
        Matcher m = SIG.matcher(signature == null ? "" : signature);
        if (!m.matches()) throw new IllegalArgumentException("Bad signature '" + signature + "', expected e.g. balanceOf(address)(uint256)");
        return new AbiSignature(m.group(1), types(m.group(2)), types(m.group(3)));
    }

    /** Parses constructor parameter types, e.g. {@code (string,string,uint256)}. */
    public static List<String> parseTypes(String types) {
        return types(types.replaceAll("^\\s*\\(|\\)\\s*$", ""));
    }

    private static List<String> types(String list) {
        if (list == null || list.isBlank()) return List.of();
        List<String> out = new ArrayList<>();
        for (String raw : list.split(",")) {
            String t = raw.trim();
            if (t.equals("uint")) t = "uint256";
            if (t.equals("int")) t = "int256";
            if (!ATOMIC.matcher(t).matches()) throw new IllegalArgumentException("Unsupported ABI type '" + t + "'");
            out.add(t);
        }
        return out;
    }

    public String canonical() {
        return name + "(" + String.join(",", inputs) + ")";
    }

    public String encodeCall(List<String> args) {
        return FunctionEncoder.encode(function(args));
    }

    public Function function(List<String> args) {
        return new Function(name, encodeArgs(inputs, args), outputRefs());
    }

    /** ABI-encodes a list of values without a selector (constructor arguments). */
    public static String encodeConstructor(List<String> types, List<String> args) {
        return FunctionEncoder.encodeConstructor(encodeArgs(types, args));
    }

    @SuppressWarnings("rawtypes")
    public List<String> decodeOutputs(String hex) {
        if (outputs.isEmpty()) return List.of();
        List<Type> decoded = FunctionReturnDecoder.decode(hex, function(nullArgs()).getOutputParameters());
        if (decoded.size() != outputs.size()) throw new IllegalStateException("Call returned no data (wrong address or signature?)");
        List<String> out = new ArrayList<>();
        for (Type t : decoded) out.add(t.getValue() instanceof byte[] b ? Numeric.toHexString(b) : String.valueOf(t.getValue()));
        return out;
    }

    private List<String> nullArgs() {
        return Arrays.asList(new String[inputs.size()]).stream().map(x -> "0").toList();
    }

    @SuppressWarnings("rawtypes")
    private List<TypeReference<?>> outputRefs() {
        List<TypeReference<?>> refs = new ArrayList<>();
        try {
            for (String t : outputs) {
                @SuppressWarnings("unchecked")
                TypeReference<?> ref = (TypeReference<?>) TypeReference.makeTypeReference(t);
                refs.add(ref);
            }
        } catch (ClassNotFoundException e) {
            throw new IllegalArgumentException(e.getMessage(), e);
        }
        return refs;
    }

    @SuppressWarnings("rawtypes")
    private static List<Type> encodeArgs(List<String> types, List<String> args) {
        if (args.size() != types.size())
            throw new IllegalArgumentException("Expected " + types.size() + " argument(s) " + types + ", got " + args.size());
        List<Type> out = new ArrayList<>();
        for (int i = 0; i < types.size(); i++) {
            String type = types.get(i), value = args.get(i) == null ? "" : args.get(i).trim();
            Object converted;
            if (type.startsWith("uint") || type.startsWith("int")) converted = value.startsWith("0x") ? Numeric.toBigInt(value) : new BigInteger(value);
            else if (type.equals("bool")) converted = Boolean.parseBoolean(value);
            else if (type.startsWith("bytes")) converted = Numeric.hexStringToByteArray(value);
            else converted = value;
            try {
                out.add(TypeDecoder.instantiateType(type, converted));
            } catch (ReflectiveOperationException | RuntimeException e) {
                throw new IllegalArgumentException("Bad value '" + value + "' for " + type, e);
            }
        }
        return out;
    }
}
