package autotest.moblab.rpc;

import com.google.gwt.json.client.JSONObject;
import com.google.gwt.json.client.JSONString;

/**
 * Connected board and model information
 */
public class ConnectedBoard extends JsonRpcEntity {
  public static final String JSON_FIELD_MODEL = "model";
  public static final String JSON_FIELD_BOARD = "board";

  private static final String NO_MODEL_FOUND = "NO_MODEL_FOUND";
  private static final String NO_BOARD_FOUND = "NO_BOARD_FOUND";

  private String model;
  private String board;

  public ConnectedBoard() {

  }

  public String getModel() {
    return model;
  }

  public String getBoard() {
    return board;
  }

  @Override
  public void fromJson(JSONObject object) {
    this.model = getStringFieldOrDefault(object, JSON_FIELD_MODEL, "");
    this.board = getStringFieldOrDefault(object, JSON_FIELD_BOARD, "");
  }

  @Override
  public JSONObject toJson() {
    JSONObject object = new JSONObject();
    object.put(JSON_FIELD_BOARD, new JSONString(board));
    object.put(JSON_FIELD_MODEL, new JSONString(model));
    return object;
  }
}
