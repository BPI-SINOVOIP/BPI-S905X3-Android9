package autotest.tko;

import autotest.common.JsonRpcProxy;
import autotest.common.Utils;
import autotest.common.CustomHistory.HistoryToken;
import autotest.tko.TableView.TableSwitchListener;
import autotest.tko.TableView.TableViewConfig;

import com.google.gwt.core.client.EntryPoint;
import com.google.gwt.core.client.GWT;
import com.google.gwt.core.client.JavaScriptObject;
import com.google.gwt.core.client.GWT.UncaughtExceptionHandler;
import com.google.gwt.dom.client.Element;

public class EmbeddedTkoClient implements EntryPoint, TableSwitchListener {
    private String autotestServerUrl; 
    private TestDetailView testDetailView; // we'll use this to generate history tokens

    public void onModuleLoad() {
        JsonRpcProxy.setDefaultBaseUrl(JsonRpcProxy.TKO_BASE_URL);
        testDetailView = new TestDetailView();
    }

    @SuppressWarnings("unused") // called from native
    private void initialize(String autotestServerUrl) {
        this.autotestServerUrl = autotestServerUrl;
        JsonRpcProxy proxy = JsonRpcProxy.createProxy(autotestServerUrl + JsonRpcProxy.TKO_BASE_URL,
                                                      true);
        JsonRpcProxy.setProxy(JsonRpcProxy.TKO_BASE_URL, proxy);
    }

    public HistoryToken getSelectTestHistoryToken(int testId) {
        testDetailView.updateObjectId(Integer.toString(testId));
        return testDetailView.getHistoryArguments();
    }

    public void onSelectTest(int testId) {
        String fullUrl = autotestServerUrl + "/new_tko/#" + getSelectTestHistoryToken(testId);
        Utils.openUrlInNewWindow(fullUrl);
    }

    public void onSwitchToTable(TableViewConfig config) {
        throw new UnsupportedOperationException();
    }
}
