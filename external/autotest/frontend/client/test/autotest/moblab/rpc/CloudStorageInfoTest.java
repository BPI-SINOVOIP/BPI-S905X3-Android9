package autotest.moblab.rpc;

import com.google.gwt.json.client.JSONObject;

import autotest.moblab.MoblabTest;

public class CloudStorageInfoTest extends MoblabTest {

  public void testSerialization() {
    CloudStorageInfo info = new CloudStorageInfo();
    JSONObject jsonInfo = info.toJson();
    assertNull(jsonInfo.get(CloudStorageInfo.JSON_FIELD_BOTO_KEY_ID));
    assertNull(jsonInfo.get(CloudStorageInfo.JSON_FIELD_BOTO_SECRET_KEY));
    assertNull(jsonInfo.get(CloudStorageInfo.JSON_FIELD_IMAGE_STORAGE_URL));
    assertNull(jsonInfo.get(CloudStorageInfo.JSON_FIELD_RESULT_STORAGE_URL));
    assertFalse(jsonInfo.get(CloudStorageInfo.JSON_FIELD_USE_EXISTING_BOTO_FILE).isBoolean()
        .booleanValue());

    info.setBotoKey("key");
    info.setBotoSecret("secret");
    info.setImageStorageServer("gs://image_bucket");
    info.setResultStorageServer("gs://result_bucket");
    info.setUseExistingBotoFile(true);
    jsonInfo = info.toJson();
    assertEquals("key",
        jsonInfo.get(CloudStorageInfo.JSON_FIELD_BOTO_KEY_ID).isString().stringValue());
    assertEquals("secret",
        jsonInfo.get(CloudStorageInfo.JSON_FIELD_BOTO_SECRET_KEY).isString().stringValue());
    assertEquals("gs://image_bucket",
        jsonInfo.get(CloudStorageInfo.JSON_FIELD_IMAGE_STORAGE_URL).isString().stringValue());
    assertEquals("gs://result_bucket",
        jsonInfo.get(CloudStorageInfo.JSON_FIELD_RESULT_STORAGE_URL).isString().stringValue());
    assertTrue(jsonInfo.get(CloudStorageInfo.JSON_FIELD_USE_EXISTING_BOTO_FILE).isBoolean()
        .booleanValue());

    info = new CloudStorageInfo();
    info.fromJson(jsonInfo);
    assertEquals("key", info.getBotoKey());
    assertEquals("secret", info.getBotoSecret());
    assertEquals("gs://image_bucket", info.getImageStorageServer());
    assertEquals("gs://result_bucket",info.getResultStorageServer());
    assertTrue(info.isUseExistingBotoFile());
  }
}
