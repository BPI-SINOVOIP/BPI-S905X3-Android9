package autotest.moblab.rpc;

import com.google.gwt.json.client.JSONNull;
import com.google.gwt.json.client.JSONNumber;
import com.google.gwt.json.client.JSONObject;
import com.google.gwt.json.client.JSONString;
import com.google.gwt.json.client.JSONValue;

import autotest.common.JsonRpcCallback;
import autotest.common.JsonRpcProxy;
import autotest.common.SimpleCallback;

import com.google.gwt.user.client.Window;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

/**
 * A helper class for moblab RPC call.
 */
public class MoblabRpcHelper {
  public static final String RPC_PARAM_CLOUD_STORAGE_INFO = "cloud_storage_info";
  public static final String RPC_PARAM_WIFI_INFO = "wifi_info";

  private MoblabRpcHelper() {}

  /**
   * Fetches config data.
   */
  public static void fetchConfigData(final SimpleCallback callback) {
    fetchConfigData(new JsonRpcCallback() {
      @Override
      public void onSuccess(JSONValue result) {
        if (callback != null)
          callback.doCallback(result);
      }
    });
  }

  /**
   * Fetch config data.
   */
  public static void fetchConfigData(final JsonRpcCallback callback) {
    JsonRpcProxy rpcProxy = JsonRpcProxy.getProxy();
    rpcProxy.rpcCall("get_config_values", null, callback);
  }

  /**
   * Submits config data.
   */
  public static void submitConfigData(JSONObject configValues, JsonRpcCallback callback) {
    JSONObject params = new JSONObject();
    params.put("config_values", configValues);
    JsonRpcProxy rpcProxy = JsonRpcProxy.getProxy();
    rpcProxy.rpcCall("update_config_handler", params, callback);
  }

  /**
   * Resets config data on Moblab.
   */
  public static void resetConfigData(final JsonRpcCallback callback) {
    JsonRpcProxy rpcProxy = JsonRpcProxy.getProxy();
    rpcProxy.rpcCall("reset_config_settings", null, callback);
  }

  /**
   * Reboot Moblab device.
   */
  public static void rebootMoblab(final JsonRpcCallback callback) {
    JsonRpcProxy rpcProxy = JsonRpcProxy.getProxy();
    rpcProxy.rpcCall("reboot_moblab", null, callback);
  }

  /**
   * Fetches the server network information.
   */
  public static void fetchNetworkInfo(
      final MoblabRpcCallbacks.FetchNetworkInfoRpcCallback callback) {
    JsonRpcProxy rpcProxy = JsonRpcProxy.getProxy();
    rpcProxy.rpcCall("get_network_info", null, new JsonRpcCallback() {
      @Override
      public void onSuccess(JSONValue result) {
        NetworkInfo networkInfo = new NetworkInfo();
        networkInfo.fromJson(result.isObject());
        callback.onNetworkInfoFetched(networkInfo);
      }
    });
  }

  /**
   * Fetches the cloud storage configuration information.
   */
  public static void fetchCloudStorageInfo(
      final MoblabRpcCallbacks.FetchCloudStorageInfoCallback callback) {
    JsonRpcProxy rpcProxy = JsonRpcProxy.getProxy();
    rpcProxy.rpcCall("get_cloud_storage_info", null, new JsonRpcCallback() {
      @Override
      public void onSuccess(JSONValue result) {
        CloudStorageInfo info = new CloudStorageInfo();
        info.fromJson(result.isObject());
        callback.onCloudStorageInfoFetched(info);
      }
    });
  }

  /**
   * Validates the cloud storage configuration information.
   */
  public static void validateCloudStorageInfo(CloudStorageInfo cloudStorageInfo,
      final MoblabRpcCallbacks.ValidateCloudStorageInfoCallback callback) {
    JSONObject params = new JSONObject();
    params.put(RPC_PARAM_CLOUD_STORAGE_INFO, cloudStorageInfo.toJson());
    JsonRpcProxy rpcProxy = JsonRpcProxy.getProxy();
    rpcProxy.rpcCall("validate_cloud_storage_info", params, new JsonRpcCallback() {
      @Override
      public void onSuccess(JSONValue result) {
        OperationStatus status = new OperationStatus();
        status.fromJson(result.isObject());
        callback.onCloudStorageInfoValidated(status);
      }
    });
  }

  /**
   * Submits the wizard configuration data.
   */
  public static void submitWizardConfigData(Map<String, JSONObject> configDataMap,
      final MoblabRpcCallbacks.SubmitWizardConfigInfoCallback callback) {
    JSONObject params = new JSONObject();
    if (configDataMap.containsKey(RPC_PARAM_CLOUD_STORAGE_INFO)) {
      params.put(RPC_PARAM_CLOUD_STORAGE_INFO, configDataMap.get(RPC_PARAM_CLOUD_STORAGE_INFO));
    } else {
      params.put(RPC_PARAM_CLOUD_STORAGE_INFO, new JSONObject());
    }
    if (configDataMap.containsKey(RPC_PARAM_WIFI_INFO)) {
      params.put(RPC_PARAM_WIFI_INFO, configDataMap.get(RPC_PARAM_WIFI_INFO));
    } else {
      params.put(RPC_PARAM_WIFI_INFO, new JSONObject());
    }
    JsonRpcProxy rpcProxy = JsonRpcProxy.getProxy();
    rpcProxy.rpcCall("submit_wizard_config_info", params, new JsonRpcCallback() {
      @Override
      public void onSuccess(JSONValue result) {
        OperationStatus status = new OperationStatus();
        status.fromJson(result.isObject());
        callback.onWizardConfigInfoSubmitted(status);
      }
    });
  }

  /**
   * Fetches the version information.
   */
  public static void fetchMoblabBuildVersionInfo(
      final MoblabRpcCallbacks.FetchVersionInfoCallback callback) {
    JsonRpcProxy rpcProxy = JsonRpcProxy.getProxy();
    rpcProxy.rpcCall("get_version_info", null, new JsonRpcCallback() {
          @Override
          public void onSuccess(JSONValue result) {
            VersionInfo info = new VersionInfo();
            info.fromJson(result.isObject());
            callback.onVersionInfoFetched(info);
          }
        });
  }

  /**
   * Apply update and reboot Moblab device
   */
  public static void updateMoblab(final JsonRpcCallback callback) {
    JsonRpcProxy rpcProxy = JsonRpcProxy.getProxy();
    rpcProxy.rpcCall("update_moblab", null, callback);
  }

   /**
   * Get information about the DUT's connected to the moblab.
   */
  public static void fetchDutInformation(
      final MoblabRpcCallbacks.FetchConnectedDutInfoCallback callback) {
    JsonRpcProxy rpcProxy = JsonRpcProxy.getProxy();
    rpcProxy.rpcCall("get_connected_dut_info", null, new JsonRpcCallback() {
      @Override
      public void onSuccess(JSONValue result) {
        ConnectedDutInfo info = new ConnectedDutInfo();
        info.fromJson(result.isObject());
        callback.onFetchConnectedDutInfoSubmitted(info);
      }
    });
  }

  /**
   * Enroll a device into the autotest syste.
   * @param dutIpAddress ipAddress of the new DUT.
   * @param callback Callback execute when the RPC is complete.
   */
  public static void addMoblabDut(String dutIpAddress,
      final MoblabRpcCallbacks.LogActionCompleteCallback callback) {
    JsonRpcProxy rpcProxy = JsonRpcProxy.getProxy();
    JSONObject params = new JSONObject();
    params.put("ipaddress", new JSONString(dutIpAddress));
    rpcProxy.rpcCall("add_moblab_dut", params, new JsonRpcCallback() {
      @Override
      public void onSuccess(JSONValue result) {
        boolean didSucceed = result.isArray().get(0).isBoolean().booleanValue();
        String information = result.isArray().get(1).isString().stringValue();
        callback.onLogActionComplete(didSucceed, information);
      }
    });
  }

  /**
   * Remove a device from the autotest system.
   * @param dutIpAddress ipAddress of the DUT to remove.
   * @param callback Callback to execute when the RPC is complete.
   */
  public static void removeMoblabDut(String dutIpAddress,
      final MoblabRpcCallbacks.LogActionCompleteCallback callback) {
    JsonRpcProxy rpcProxy = JsonRpcProxy.getProxy();
    JSONObject params = new JSONObject();
    params.put("ipaddress", new JSONString(dutIpAddress));
    rpcProxy.rpcCall("remove_moblab_dut", params, new JsonRpcCallback() {
      @Override
      public void onSuccess(JSONValue result) {
        boolean didSucceed = result.isArray().get(0).isBoolean().booleanValue();
        String information = result.isArray().get(1).isString().stringValue();
        callback.onLogActionComplete(didSucceed, information);
      }
    });
  }

  /**
   * Add a label to a specific DUT.
   * @param dutIpAddress  ipAddress of the device to have the new label applied.
   * @param labelName the label to apply
   * @param callback callback to execute when the RPC is complete.
   */
  public static void addMoblabLabel(String dutIpAddress, String labelName,
      final MoblabRpcCallbacks.LogActionCompleteCallback callback) {
    JsonRpcProxy rpcProxy = JsonRpcProxy.getProxy();
    JSONObject params = new JSONObject();
    params.put("ipaddress", new JSONString(dutIpAddress));
    params.put("label_name", new JSONString(labelName));
    rpcProxy.rpcCall("add_moblab_label", params, new JsonRpcCallback() {
      @Override
      public void onSuccess(JSONValue result) {
        boolean didSucceed = result.isArray().get(0).isBoolean().booleanValue();
        String information = result.isArray().get(1).isString().stringValue();
        callback.onLogActionComplete(didSucceed, information);
      }
    });
  }

  /**
   * Remove a label from a specific DUT.
   * @param dutIpAddress ipAddress of the device to have the label removed.
   * @param labelName the label to remove
   * @param callback callback to execute when the RPC is complete.
   */
  public static void removeMoblabLabel(String dutIpAddress, String labelName,
      final MoblabRpcCallbacks.LogActionCompleteCallback callback) {
    JsonRpcProxy rpcProxy = JsonRpcProxy.getProxy();
    JSONObject params = new JSONObject();
    params.put("ipaddress", new JSONString(dutIpAddress));
    params.put("label_name", new JSONString(labelName));
    rpcProxy.rpcCall("remove_moblab_label", params, new JsonRpcCallback() {
      @Override
      public void onSuccess(JSONValue result) {
        boolean didSucceed = result.isArray().get(0).isBoolean().booleanValue();
        String information = result.isArray().get(1).isString().stringValue();
        callback.onLogActionComplete(didSucceed, information);
      }
    });
  }

  /**
   * add an attribute to a specific dut.
   * @param dutIpAddress  ipaddress of the device to have the new attribute applied.
   * @param attributeName the attribute name
   * @param attributeValue the attribute value to be associated with the name
   * @param callback callback to execute when the rpc is complete.
   */
  public static void setMoblabAttribute(String dutIpAddress, String attributeName,
      String attributeValue, final MoblabRpcCallbacks.LogActionCompleteCallback callback) {
    JsonRpcProxy rpcProxy = JsonRpcProxy.getProxy();
    JSONObject params = new JSONObject();
    params.put("ipaddress", new JSONString(dutIpAddress));
    params.put("attribute", new JSONString(attributeName));
    params.put("value", new JSONString(attributeValue));
    rpcProxy.rpcCall("set_host_attrib", params, new JsonRpcCallback() {
      @Override
      public void onSuccess(JSONValue result) {
        boolean didSucceed = result.isArray().get(0).isBoolean().booleanValue();
        String information = result.isArray().get(1).isString().stringValue();
        callback.onLogActionComplete(didSucceed, information);
      }
    });
  }

  /**
   * remove an attribute from a specific dut.
   * @param dutIpAddress  ipaddress of the device to have the new attribute applied.
   * @param attributeName the attribute name
   * @param callback callback to execute when the rpc is complete.
   */
  public static void removeMoblabAttribute(String dutIpAddress, String attributeName,
      final  MoblabRpcCallbacks.LogActionCompleteCallback callback) {
    JsonRpcProxy rpcProxy = JsonRpcProxy.getProxy();
    JSONObject params = new JSONObject();
    params.put("ipaddress", new JSONString(dutIpAddress));
    params.put("attribute", new JSONString(attributeName));
    rpcProxy.rpcCall("delete_host_attrib", params, new JsonRpcCallback() {
      @Override
      public void onSuccess(JSONValue result) {
        boolean didSucceed = result.isArray().get(0).isBoolean().booleanValue();
        String information = result.isArray().get(1).isString().stringValue();
        callback.onLogActionComplete(didSucceed, information);
      }
    });
  }


  public static void fetchConnectedBoards(
      final MoblabRpcCallbacks.FetchConnectedBoardsCallback callback) {
    JsonRpcProxy rpcProxy = JsonRpcProxy.getProxy();
    rpcProxy.rpcCall("get_connected_boards", null, new JsonRpcCallback() {
      @Override
      public void onSuccess(JSONValue result) {
        List<ConnectedBoard> boards = new LinkedList<ConnectedBoard>();
        int boardListSize = result.isArray().size();
        for (int i = 0; i < boardListSize; i++) {
          ConnectedBoard board = new ConnectedBoard();
          board.fromJson(result.isArray().get(i).isObject());
          boards.add(board);
        }
        callback.onFetchConnectedBoardsSubmitted(boards);
      }
    });
  }

  public static void fetchConnectedPools(
      final MoblabRpcCallbacks.FetchConnectedPoolsCallback callback) {
    JsonRpcProxy rpcProxy = JsonRpcProxy.getProxy();
    rpcProxy.rpcCall("get_connected_pools", null, new JsonRpcCallback() {
      @Override
      public void onSuccess(JSONValue result) {
        List<String> pools = new LinkedList<String>();
        int poolListSize = result.isArray().size();
        for (int i = 0; i < poolListSize; i++) {
          pools.add(result.isArray().get(i).isString().stringValue());
        }
        callback.onFetchConnectedPoolsSubmitted(pools);
      }
    });
  }

  public static void fetchBuildsForBoard(String board_name,
      final MoblabRpcCallbacks.FetchBuildsForBoardCallback callback) {
    JsonRpcProxy rpcProxy = JsonRpcProxy.getProxy();
    JSONObject params = new JSONObject();
    params.put("board_name", new JSONString(board_name));
    rpcProxy.rpcCall("get_builds_for_board", params, new JsonRpcCallback() {
      @Override
      public void onSuccess(JSONValue result) {
        List<String> builds = new LinkedList<String>();
        for (int i = 0; i < result.isArray().size(); i++) {
          builds.add(result.isArray().get(i).isString().stringValue());
        }
        callback.onFetchBuildsForBoardCallbackSubmitted(builds);
      }
    });
  }

  public static void fetchFirmwareForBoard(String board_name,
      final MoblabRpcCallbacks.FetchFirmwareForBoardCallback callback) {
    JsonRpcProxy rpcProxy = JsonRpcProxy.getProxy();
    JSONObject params = new JSONObject();
    params.put("board_name", new JSONString(board_name));
    rpcProxy.rpcCall("get_firmware_for_board", params, new JsonRpcCallback() {
      @Override
      public void onSuccess(JSONValue result) {
        List<String> firmwareBuilds = new LinkedList<String>();
        for (int i = 0; i < result.isArray().size(); i++) {
          firmwareBuilds.add(result.isArray().get(i).isString().stringValue());
        }
        callback.onFetchFirmwareForBoardCallbackSubmitted(firmwareBuilds);
      }
    });
  }

  public static void runSuite(String board, String model,
      String build, String suite, String pool, String rwFirmware,
      String roFirmware, String suiteArgs, String bugId, String partId,
      final MoblabRpcCallbacks.RunSuiteCallback callback) {
    JsonRpcProxy rpcProxy = JsonRpcProxy.getProxy();
    JSONObject params = new JSONObject();
    params.put("board", new JSONString(board));
    params.put("model", new JSONString(model));
    params.put("build", new JSONString(build));
    params.put("suite", new JSONString(suite));
    params.put("pool", new JSONString(pool));
    params.put("rw_firmware", new JSONString(rwFirmware));
    params.put("ro_firmware", new JSONString(roFirmware));
    params.put("suite_args", new JSONString(suiteArgs));
    params.put("bug_id", new JSONString(bugId));
    params.put("part_id", new JSONString(partId));
    rpcProxy.rpcCall("run_suite", params, new JsonRpcCallback() {
      @Override
      public void onSuccess(JSONValue result) {
        callback.onRunSuiteComplete();
      }
    });
  }

  /**
   * Fetches the DUT wifi configuration information to use in tests.
   */
  public static void fetchWifiInfo(
      final MoblabRpcCallbacks.FetchWifiInfoCallback callback) {
    JsonRpcProxy rpcProxy = JsonRpcProxy.getProxy();
    rpcProxy.rpcCall("get_dut_wifi_info", null, new JsonRpcCallback() {
      @Override
      public void onSuccess(JSONValue result) {
        WifiInfo info = new WifiInfo();
        info.fromJson(result.isObject());
        callback.onWifiInfoFetched(info);
      }
    });
  }


}
