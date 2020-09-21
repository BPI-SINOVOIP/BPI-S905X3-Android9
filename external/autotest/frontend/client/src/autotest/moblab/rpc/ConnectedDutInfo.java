package autotest.moblab.rpc;

import com.google.gwt.json.client.JSONArray;
import com.google.gwt.json.client.JSONObject;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

/**
 * The connected DUT information RPC entity.
 */
public class ConnectedDutInfo extends JsonRpcEntity {

  private Map<String, String> connectedIpsToMacAddresses;
  private Map<String, Boolean> connectedIpsToSshConnection;
  private Map<String, String> configuredIpsToLabels;

  public ConnectedDutInfo() {
    connectedIpsToMacAddresses = new TreeMap<String, String>();
    connectedIpsToSshConnection = new TreeMap<String, Boolean>();
    configuredIpsToLabels = new TreeMap<String, String>();
  }

  public Map<String, String> getConnectedIpsToMacAddress() {
    return connectedIpsToMacAddresses;
  }

  public Map<String, Boolean> getConnectedIpsToSshConnection() {
    return connectedIpsToSshConnection;
  }

  public Map<String, String> getConfiguredIpsToLabels() {
    return configuredIpsToLabels;
  }

  @Override
  public void fromJson(JSONObject object) {
    JSONObject leases = object.get("connected_duts").isObject();
    for (String lease : leases.keySet()) {
      JSONObject leaseObject = leases.get(lease).isObject();
      connectedIpsToMacAddresses.put(lease,
          leaseObject.get("mac_address").isString().stringValue());
      connectedIpsToSshConnection.put(lease,
          leaseObject.get("ssh_connection_ok").isBoolean().booleanValue());
    }
    JSONObject configuredDuts = object.get("configured_duts").isObject();
    for (String ipAddress : configuredDuts.keySet()) {
      configuredIpsToLabels.put(ipAddress, configuredDuts.get(ipAddress).isString().stringValue());
    }
  }

  @Override
  public JSONObject toJson() {
    // This is a read only RPC call so nothing to be submitted back to the
    // server from the UI.
    return null;
  }
}
