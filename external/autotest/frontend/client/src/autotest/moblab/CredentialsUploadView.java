package autotest.moblab;

import autotest.common.JsonRpcCallback;
import autotest.common.JsonRpcProxy;
import autotest.common.ui.NotifyManager;
import autotest.common.ui.TabView;
import autotest.common.ui.ToolTip;
import com.google.gwt.event.dom.client.ClickEvent;
import com.google.gwt.event.dom.client.ClickHandler;
import com.google.gwt.json.client.JSONObject;
import com.google.gwt.json.client.JSONString;
import com.google.gwt.json.client.JSONValue;
import com.google.gwt.user.client.ui.Button;
import com.google.gwt.user.client.ui.FileUpload;
import com.google.gwt.user.client.ui.FormPanel;
import com.google.gwt.user.client.ui.FormPanel.SubmitCompleteEvent;
import com.google.gwt.user.client.ui.FormPanel.SubmitCompleteHandler;
import java.util.HashMap;
import java.util.Map;

/**
 * Widget to upload all moblab credentials.
 */
public class CredentialsUploadView extends TabView {
  private static final String UPLOAD_KEY_BOTO_KEY = "boto_key";
  private static final String UPLOAD_KEY_SERVICE_ACCOUNT = "service_account";
  private static final String UPLOAD_KEY_LAUNCH_CONTROL_KEY = "launch_control_key";

  static Map<String, UploadInfo> uploadMap = new HashMap<String, UploadInfo>();
  static {
    uploadMap.put(
        UPLOAD_KEY_BOTO_KEY,
        new UploadInfo(
            UPLOAD_KEY_BOTO_KEY, "set_boto_key", "boto_key", "Boto Key uploaded.",
            "Boto key is for legacy access to Google Storage."));
    uploadMap.put(
        UPLOAD_KEY_SERVICE_ACCOUNT,
        new UploadInfo(
            UPLOAD_KEY_SERVICE_ACCOUNT,
            "set_service_account_credential",
            "service_account_filename",
            "Service Account Credential file uploaded.",
            "Service Account Credential is for access to Google Cloud Platform resources, "
            + "such as Google Storage, Cloud PubSub or others."));
    uploadMap.put(
        UPLOAD_KEY_LAUNCH_CONTROL_KEY,
        new UploadInfo(
            UPLOAD_KEY_LAUNCH_CONTROL_KEY,
            "set_launch_control_key",
            "launch_control_key",
            "Launch Control Key uploaded.",
            "Launch Control Key is for access to the android build."));
  }

  @Override
  public String getElementId() {
    return "moblab_credentials";
  }

  @Override
  public void initialize() {
    super.initialize();

    addUploadWidget(uploadMap.get(UPLOAD_KEY_BOTO_KEY));
    addUploadWidget(uploadMap.get(UPLOAD_KEY_SERVICE_ACCOUNT));
    addUploadWidget(uploadMap.get(UPLOAD_KEY_LAUNCH_CONTROL_KEY));
  }

  /**
   * Creates widget for an upload.
   *
   * @param uploadInfo the upload information.
   */
  private void addUploadWidget(final UploadInfo uploadInfo) {
    final FormPanel keyUploadForm = new FormPanel();
    keyUploadForm.setAction(JsonRpcProxy.AFE_BASE_URL + "upload/");
    keyUploadForm.setEncoding(FormPanel.ENCODING_MULTIPART);
    keyUploadForm.setMethod(FormPanel.METHOD_POST);

    FileUpload keyUpload = new FileUpload();
    keyUpload.setName(uploadInfo.fileUploadName);
    keyUploadForm.setWidget(keyUpload);

    Button submitButton =
        new Button(
            "Submit",
            new ClickHandler() {
              @Override
              public void onClick(ClickEvent event) {
                keyUploadForm.submit();
              }
            });

    keyUploadForm.addSubmitCompleteHandler(
        new SubmitCompleteHandler() {
          @Override
          public void onSubmitComplete(SubmitCompleteEvent event) {
            String fileName = event.getResults();
            JSONObject params = new JSONObject();
            params.put(uploadInfo.rpcArgName, new JSONString(fileName));
            rpcCall(uploadInfo.rpcName, params, uploadInfo.successMessage);
          }
        });

    addWidget(keyUploadForm, uploadInfo.uploadViewId);
    addWidget(submitButton, uploadInfo.submitButtonId);
    
    if (uploadInfo.helpMessage != null) {
      ToolTip helpToolTip = new ToolTip("?", uploadInfo.helpMessage);
      addWidget(helpToolTip, uploadInfo.helpToolTipId);
    }
  }

  private static void rpcCall(
      final String rpcName, final JSONObject params, final String successMessage) {
    JsonRpcProxy rpcProxy = JsonRpcProxy.getProxy();
    rpcProxy.rpcCall(
        rpcName,
        params,
        new JsonRpcCallback() {
          @Override
          public void onSuccess(JSONValue result) {
            NotifyManager.getInstance().showMessage(successMessage);
          }
        });
  }

  /**
   * Upload related information.
   */
  private static class UploadInfo {
    final String keyName;
    final String rpcName;
    final String rpcArgName;
    final String successMessage;
    final String fileUploadName;
    final String uploadViewId;
    final String submitButtonId;
    final String helpToolTipId;
    final String helpMessage;

    public UploadInfo(
        String keyName,
        String rpcName,
        String rpcArgName,
        String successMessage,
        String helpMessage) {
      this.keyName = keyName;
      this.rpcName = rpcName;
      this.rpcArgName = rpcArgName;
      this.successMessage = successMessage;
      this.fileUploadName = keyName;
      this.uploadViewId = "view_" + this.keyName;
      this.submitButtonId = "view_submit_" + this.keyName;
      this.helpToolTipId = "view_help_" + this.keyName;
      this.helpMessage = helpMessage;
    }
  }
}
