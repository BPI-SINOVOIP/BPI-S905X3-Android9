package android.keystore.cts;

import java.security.AlgorithmParameters;
import java.security.spec.AlgorithmParameterSpec;
import java.security.spec.InvalidParameterSpecException;

import javax.crypto.spec.IvParameterSpec;

public class DESedeCBCNoPaddingCipherTest extends DESedeCipherTestBase {

    private static final byte[] KAT_IV = HexEncoding.decode("DFCA366848DEA6BB");

    @Override
    protected String getTransformation() {
        return "DESede/CBC/NoPadding";
    }

    @Override
    protected byte[] getIv(AlgorithmParameters params) throws InvalidParameterSpecException {
        IvParameterSpec spec = params.getParameterSpec(IvParameterSpec.class);
        return spec.getIV();
    }

    @Override
    protected AlgorithmParameterSpec getKatAlgorithmParameterSpec() {
        return new IvParameterSpec(getKatIv());
    }

    @Override
    protected byte[] getKatCiphertext() {
        return HexEncoding.decode("e70bb5761d796d7b0eb40b5b60deb6a9");
    }

    @Override
    protected byte[] getKatIv() {
        return KAT_IV.clone();
    }
}
