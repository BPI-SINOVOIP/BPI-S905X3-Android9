package autotest.moblab.rpc;

import com.google.gwt.json.client.JSONObject;
import com.google.gwt.json.client.JSONValue;

import autotest.moblab.MoblabTest;
import autotest.moblab.rpc.NetworkInfo;

public class NetworkInfoTest extends MoblabTest {
  public void testSerialization() {
    // Test null
    NetworkInfo info = new NetworkInfo(null, true);
    JSONObject jsonInfo = info.toJson();
    JSONValue ips = jsonInfo.get(NetworkInfo.JSON_FIELD_SERVER_IPS);
    assertEquals(0, ips.isArray().size());
    assertTrue(jsonInfo.get(NetworkInfo.JSON_FIELD_IS_CONNECTED).isBoolean()
        .booleanValue());

    info = new NetworkInfo();
    info.fromJson(jsonInfo);
    assertEquals(0, info.getServerIps().length);
    assertTrue(info.isConnectedToGoogle());

    // Test serialization
    String[] serverIps = new String[] { "10.0.0.1", "10.0.0.2"};
    info = new NetworkInfo(serverIps, true);
    jsonInfo = info.toJson();
    ips = jsonInfo.get(NetworkInfo.JSON_FIELD_SERVER_IPS);
    assertEquals(2, ips.isArray().size());
    assertEquals("10.0.0.1", ips.isArray().get(0).isString().stringValue());
    assertEquals("10.0.0.2", ips.isArray().get(1).isString().stringValue());
    assertTrue(jsonInfo.get(NetworkInfo.JSON_FIELD_IS_CONNECTED).isBoolean()
        .booleanValue());

    // Test de-serialization.
    info = new NetworkInfo();
    info.fromJson(jsonInfo);
    assertEquals(2, info.getServerIps().length);
    assertEquals("10.0.0.1", info.getServerIps()[0]);
    assertEquals("10.0.0.2", info.getServerIps()[1]);
    assertTrue(info.isConnectedToGoogle());
  }
}
