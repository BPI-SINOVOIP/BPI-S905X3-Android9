package autotest.moblab.rpc;

import com.google.gwt.json.client.JSONObject;
import com.google.gwt.json.client.JSONValue;

/**
 * Base class for entities passing as parameters via the JSON RPC.
 */
public abstract class JsonRpcEntity {
  public abstract void fromJson(JSONObject object);

  public abstract JSONObject toJson();

  protected static String getStringFieldOrDefault(JSONObject object, String field,
      String defaultValue) {
    JSONValue value = object.get(field);
    if (value != null && value.isString() != null) {
      return value.isString().stringValue();
    }
    return defaultValue;
  }

  protected static boolean getBooleanFieldOrDefault(JSONObject object, String field,
      boolean defaultValue) {
    JSONValue value = object.get(field);
    if (value != null && value.isBoolean() != null) {
      return value.isBoolean().booleanValue();
    }
    return defaultValue;
  }
}
