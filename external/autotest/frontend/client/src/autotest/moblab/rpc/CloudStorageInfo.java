package autotest.moblab.rpc;

import com.google.gwt.json.client.JSONBoolean;
import com.google.gwt.json.client.JSONObject;
import com.google.gwt.json.client.JSONString;

/**
 * Cloud storage configuration RPC entity.
 */
public class CloudStorageInfo extends JsonRpcEntity {
  public static final String JSON_FIELD_BOTO_KEY_ID = "gs_access_key_id";
  public static final String JSON_FIELD_BOTO_SECRET_KEY = "gs_secret_access_key";
  public static final String JSON_FIELD_USE_EXISTING_BOTO_FILE = "use_existing_boto_file";
  public static final String JSON_FIELD_IMAGE_STORAGE_URL = "image_storage_server";
  public static final String JSON_FIELD_RESULT_STORAGE_URL = "results_storage_server";

  /**
   * The boto key id.
   */
  private String botoKey;

  /**
   * The boto secret.
   */
  private String botoSecret;

  /**
   * Uses existing boto file on the Moblab
   */
  private boolean useExistingBotoFile;

  private String imageStorageServer;
  private String resultStorageServer;

  public CloudStorageInfo() {
    reset();
  }

  public String getBotoKey() {
    return botoKey;
  }

  public String getBotoSecret() {
    return botoSecret;
  }

  public String getImageStorageServer() {
    return imageStorageServer;
  }

  public String getResultStorageServer() {
    return resultStorageServer;
  }

  public void setBotoKey(String botoKey) {
    this.botoKey = botoKey.trim();
  }

  public void setBotoSecret(String botoSecret) {
    this.botoSecret = botoSecret.trim();
  }

  public void setImageStorageServer(String imageStorageServer) {
    this.imageStorageServer = imageStorageServer.trim();
  }

  public void setResultStorageServer(String resultStorageServer) {
    this.resultStorageServer = resultStorageServer.trim();
  }

  public boolean isUseExistingBotoFile() {
    return useExistingBotoFile;
  }

  public void setUseExistingBotoFile(boolean useExistingBotoFile) {
    this.useExistingBotoFile = useExistingBotoFile;
  }

  private void reset() {
    botoKey = null;
    botoSecret = null;
    imageStorageServer = null;
    resultStorageServer = null;
  }

  @Override
  public void fromJson(JSONObject object) {
    if (object != null) {
      botoKey = getStringFieldOrDefault(object, JSON_FIELD_BOTO_KEY_ID, null);
      botoSecret = getStringFieldOrDefault(object, JSON_FIELD_BOTO_SECRET_KEY, null);
      imageStorageServer = getStringFieldOrDefault(object, JSON_FIELD_IMAGE_STORAGE_URL, null);
      resultStorageServer = getStringFieldOrDefault(object, JSON_FIELD_RESULT_STORAGE_URL, null);
      useExistingBotoFile =
          getBooleanFieldOrDefault(object, JSON_FIELD_USE_EXISTING_BOTO_FILE, false);
    }
  }

  @Override
  public JSONObject toJson() {
    JSONObject object = new JSONObject();
    if (botoKey != null) {
      object.put(JSON_FIELD_BOTO_KEY_ID, new JSONString(botoKey));
    }
    if (botoSecret != null) {
      object.put(JSON_FIELD_BOTO_SECRET_KEY, new JSONString(botoSecret));
    }
    if (imageStorageServer != null) {
      object.put(JSON_FIELD_IMAGE_STORAGE_URL, new JSONString(imageStorageServer));
    }
    if (resultStorageServer != null) {
      object.put(JSON_FIELD_RESULT_STORAGE_URL, new JSONString(resultStorageServer));
    } else {
      object.put(JSON_FIELD_RESULT_STORAGE_URL, new JSONString(""));
    }
    object.put(JSON_FIELD_USE_EXISTING_BOTO_FILE, JSONBoolean.getInstance(useExistingBotoFile));
    return object;
  }
}
