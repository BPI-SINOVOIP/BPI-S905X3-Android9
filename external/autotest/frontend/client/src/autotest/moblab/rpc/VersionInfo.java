package autotest.moblab.rpc;

/**
 * The version information RPC entity.
 */
import com.google.gwt.json.client.JSONObject;

public class VersionInfo extends JsonRpcEntity {

  public enum UPDATE_STATUS {
    IDLE,
    CHECKING_FOR_UPDATE,
    UPDATE_AVAILABLE,
    DOWNLOADING,
    UPDATED_NEED_REBOOT,
    UNKNOWN
  }

  private static final String NO_MILESTONE_FOUND = "NO MILESTONE FOUND";
  private static final String NO_VERSION_FOUND = "NO VERSION FOUND";
  private static final String NO_TRACK_FOUND = "NO TRACK FOUND";
  private static final String NO_DESCRIPTION_FOUND = "NO DESCRIPTION FOUND";
  private static final String NO_ID_FOUND = "NO ID FOUND";
  private static final String NO_SERIAL_NUMBER_FOUND = "NO SERIAL NUMBER FOUND";
  private static final String NO_UPDATE_VERSION_FOUND =
      "NO UPDATE VERSION FOUND";

  private String milestoneInfo;
  private String versionInfo;
  private String releaseTrack;
  private String releaseDescription;
  private String moblabIdentification;
  private String moblabSerialNumber;
  private String moblabUpdateVersion;
  private double moblabUpdateProgress;
  private UPDATE_STATUS moblabUpdateStatus;

  public VersionInfo() { reset(); }

  public String getVersion() {
    return new StringBuilder("R").append(milestoneInfo).append("-").append(versionInfo).toString();
  }
  public String getReleaseTrack() { return releaseTrack; }
  public String getReleaseDescription() { return releaseDescription; }
  public String getMoblabIdentification() { return moblabIdentification; }
  public String getMoblabSerialNumber() { return moblabSerialNumber; }
  public String getMoblabUpdateVersion() { return moblabUpdateVersion; }
  public double getMoblabUpdateProgress() { return moblabUpdateProgress; }
  public UPDATE_STATUS getMoblabUpdateStatus() { return moblabUpdateStatus; }

  private void reset() {
    milestoneInfo = new String(NO_MILESTONE_FOUND);
    versionInfo = new String(NO_VERSION_FOUND);
    releaseTrack = new String(NO_TRACK_FOUND);
    releaseDescription = new String(NO_DESCRIPTION_FOUND);
    moblabIdentification = new String(NO_ID_FOUND);
    moblabSerialNumber = new String(NO_SERIAL_NUMBER_FOUND);
    moblabUpdateVersion = new String(NO_UPDATE_VERSION_FOUND);
    moblabUpdateStatus = UPDATE_STATUS.UNKNOWN;
    moblabUpdateProgress = 0.0;
  }

  @Override
  public void fromJson(JSONObject object) {
    reset();
    milestoneInfo = getStringFieldOrDefault(object, "CHROMEOS_RELEASE_CHROME_MILESTONE",
        NO_MILESTONE_FOUND).trim();
    versionInfo = getStringFieldOrDefault(object, "CHROMEOS_RELEASE_VERSION",
        NO_VERSION_FOUND).trim();
    releaseTrack = getStringFieldOrDefault(object, "CHROMEOS_RELEASE_TRACK",
        NO_TRACK_FOUND).trim();
    releaseDescription = getStringFieldOrDefault(object, "CHROMEOS_RELEASE_DESCRIPTION",
        NO_DESCRIPTION_FOUND).trim();
    moblabIdentification = getStringFieldOrDefault(object, "MOBLAB_ID",
        NO_ID_FOUND).trim();
    moblabSerialNumber = getStringFieldOrDefault(object, "MOBLAB_SERIAL_NUMBER",
        NO_SERIAL_NUMBER_FOUND).trim();
    moblabUpdateVersion = getStringFieldOrDefault(
        object, "MOBLAB_UPDATE_VERSION", NO_UPDATE_VERSION_FOUND).trim();
    moblabUpdateStatus = getUpdateStatus(object);

    String progressString = getStringFieldOrDefault(
        object, "MOBLAB_UPDATE_PROGRESS", "0.0").trim();
    try {
      moblabUpdateProgress = Double.parseDouble(progressString);
    }
    catch (NumberFormatException e) {
      moblabUpdateProgress = 0.0;
    }
  }

  private UPDATE_STATUS getUpdateStatus(JSONObject object) {
    String status = getStringFieldOrDefault(
        object, "MOBLAB_UPDATE_STATUS", "").trim();

    if(status.contains("IDLE")) {
      return UPDATE_STATUS.IDLE;
    }
    else if(status.contains("CHECKING_FOR_UPDATE")) {
      return UPDATE_STATUS.CHECKING_FOR_UPDATE;
    }
    else if(status.contains("UPDATE_AVAILABLE") || status.contains("VERIFYING")
        || status.contains("FINALIZING")) {
      return UPDATE_STATUS.UPDATE_AVAILABLE;
    }
    else if(status.contains("DOWNLOADING")) {
      return UPDATE_STATUS.DOWNLOADING;
    }
    else if(status.contains("NEED_REBOOT")) {
      return UPDATE_STATUS.UPDATED_NEED_REBOOT;
    }
    else {
      return UPDATE_STATUS.UNKNOWN;
    }
  }

  public String getUpdateString() {
    switch(moblabUpdateStatus){
      case CHECKING_FOR_UPDATE:
        return "Checking for update..";
      case UPDATE_AVAILABLE:
        return "Version " + moblabUpdateVersion + " is available";
      case DOWNLOADING:
        int percent = (int)(moblabUpdateProgress * 100.0);
        return "Downloading version " + moblabUpdateVersion
            + " (" + percent + "%)";
      case UPDATED_NEED_REBOOT:
        return "Version " + moblabUpdateVersion
            + " is available, reboot required";
      case IDLE:
      case UNKNOWN:
      default:
        return "No update found";
    }
  }

  public String getUpdateAction() {
    switch(moblabUpdateStatus){
      case CHECKING_FOR_UPDATE:
      case UPDATE_AVAILABLE:
      case DOWNLOADING:
      case UPDATED_NEED_REBOOT:
        return "Update Now";
      case IDLE:
      case UNKNOWN:
      default:
        return "Force Update";
    }
  }

  @Override
  public JSONObject toJson() {
    // Required override - but this is a read only UI so the write function does not need to be
    // implemented.
    return new JSONObject();
  }
}
