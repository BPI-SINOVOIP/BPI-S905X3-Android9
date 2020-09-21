package android.keystore.cts;

import java.security.AlgorithmParameters;
import java.security.spec.AlgorithmParameterSpec;
import java.security.spec.InvalidParameterSpecException;

public class DESedeECBPKCS7PaddingCipherTest extends DESedeCipherTestBase {

    @Override
    protected String getTransformation() {
        return "DESede/ECB/PKCS7Padding";
    }

    @Override
    protected byte[] getIv(AlgorithmParameters params) throws InvalidParameterSpecException {
        if (params != null) {
            fail("ECB does not use IV");
        }
        return null;
    }

    @Override
    protected AlgorithmParameterSpec getKatAlgorithmParameterSpec() {
        return null;
    }

    @Override
    protected byte[] getKatCiphertext() {
        return HexEncoding.decode("ade119f9e35ab3e9ade119f9e35ab3e94bcb01bbc0d05526");
    }

    @Override
    protected byte[] getKatIv() {
        return null;
    }
}
