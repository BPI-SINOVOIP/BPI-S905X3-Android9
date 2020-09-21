package android.security;

public class NetworkSecurityPolicyCleartextUnspecifiedTest extends NetworkSecurityPolicyTestBase {

    public NetworkSecurityPolicyCleartextUnspecifiedTest() {
        super(false // expect cleartext traffic to be permitted
                );
    }
}
