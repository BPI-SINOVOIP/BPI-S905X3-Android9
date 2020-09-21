package autotest.moblab.rpc;

import com.google.gwt.json.client.JSONBoolean;
import com.google.gwt.json.client.JSONObject;
import com.google.gwt.json.client.JSONString;
import com.google.gwt.json.client.JSONValue;

/**
 * Moblab RPC operation status.
 */
public class OperationStatus extends JsonRpcEntity {
  public static final String JSON_FIELD_STATUS_OK = "status_ok";
  public static final String JSON_FIELD_STATUS_DETAILS = "status_details";

  private boolean ok;
  private String details;

  public OperationStatus() {
    this(true);
  }

  public OperationStatus(boolean valid) {
    this(valid, null);
  }

  public OperationStatus(boolean valid, String details) {
    this.ok = valid;
    this.details = details;
  }

  public boolean isOk() {
    return ok;
  }

  public String getDetails() {
    return details;
  }

  @Override
  public void fromJson(JSONObject object) {
    JSONValue value = object.get(OperationStatus.JSON_FIELD_STATUS_OK);
    ok = value != null && value.isBoolean() != null && value.isBoolean().booleanValue();
    details = null;
    value = object.get(OperationStatus.JSON_FIELD_STATUS_DETAILS);
    if (value != null && value.isString() != null) {
      details = value.isString().stringValue();
    }
  }

  @Override
  public JSONObject toJson() {
    JSONObject object = new JSONObject();
    object.put(JSON_FIELD_STATUS_OK, JSONBoolean.getInstance(ok));
    if (details != null) {
      object.put(JSON_FIELD_STATUS_DETAILS, new JSONString(details));
    }
    return object;
  }
}
