package autotest.moblab.rpc;

import com.google.gwt.json.client.JSONArray;
import com.google.gwt.json.client.JSONBoolean;
import com.google.gwt.json.client.JSONObject;
import com.google.gwt.json.client.JSONString;
import com.google.gwt.json.client.JSONValue;

/**
 * The network information RPC entity.
 */
public class NetworkInfo extends JsonRpcEntity {
  public static final String JSON_FIELD_IS_CONNECTED = "is_connected";
  public static final String JSON_FIELD_SERVER_IPS = "server_ips";
  private String[] serverIps;
  private boolean connectedToGoogle;

  public NetworkInfo() {
    this(new String[0], false);
  }

  public NetworkInfo(String[] serverIps, boolean connectedToGoogle) {
    reset();
    if (serverIps != null) {
      this.serverIps = serverIps;
    }
    this.connectedToGoogle = connectedToGoogle;
  }

  public String[] getServerIps() {
    return serverIps;
  }

  public boolean isConnectedToGoogle() {
    return connectedToGoogle;
  }

  private void reset() {
    serverIps = new String[0];
    connectedToGoogle = false;
  }

  @Override
  public void fromJson(JSONObject object) {
    reset();
    if (object != null) {
      JSONValue serverIpsObject = object.get(JSON_FIELD_SERVER_IPS);
      if (serverIpsObject != null) {
        JSONArray serverIpsArray = serverIpsObject.isArray();
        serverIps = new String[serverIpsArray.size()];
        for (int i = 0; i < serverIps.length; i++) {
          serverIps[i] = serverIpsArray.get(i).isString().stringValue();
        }
      } else {
        serverIps = new String[0];
      }
      JSONValue connectedObject = object.get(JSON_FIELD_IS_CONNECTED);
      if (connectedObject != null) {
        connectedToGoogle = connectedObject.isBoolean().booleanValue();
      }
    }
  }

  @Override
  public JSONObject toJson() {
    JSONObject object = new JSONObject();
    if (serverIps != null) {
      JSONArray serverIpsArray = new JSONArray();
      for (int index = 0; index < serverIps.length; index++) {
        serverIpsArray.set(index, new JSONString(serverIps[index]));
      }
      object.put(JSON_FIELD_SERVER_IPS, serverIpsArray);
    }
    object.put(JSON_FIELD_IS_CONNECTED, JSONBoolean.getInstance(connectedToGoogle));
    return object;
  }
}
