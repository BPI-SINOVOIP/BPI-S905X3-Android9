package autotest.afe;

import autotest.afe.HostDetailView.HostDetailListener;
import autotest.afe.HostListView.HostListListener;
import autotest.afe.JobDetailView.JobDetailListener;
import autotest.afe.JobListView.JobSelectListener;
import autotest.afe.create.CreateJobViewPresenter.JobCreateListener;
import autotest.afe.create.CreateJobViewTab;
import autotest.common.CustomHistory;
import autotest.common.JsonRpcProxy;
import autotest.common.SiteCommonClassFactory;
import autotest.common.StaticDataRepository;
import autotest.common.ui.CustomTabPanel;
import autotest.common.ui.NotifyManager;
import autotest.common.ui.TabView;

import com.google.gwt.core.client.EntryPoint;
import com.google.gwt.dom.client.Document;
import com.google.gwt.json.client.JSONValue;
import com.google.gwt.user.client.ui.RootPanel;
import com.google.gwt.user.client.Window.Location;


public class AfeClient implements EntryPoint {
    private JobListView jobList;
    private JobDetailView jobDetail;
    private CreateJobViewTab createJob;
    private HostListView hostListView;
    private HostDetailView hostDetailView;

    public CustomTabPanel mainTabPanel;

    /**
     * Application entry point.
     */
    public void onModuleLoad() {
        JsonRpcProxy.setDefaultBaseUrl(JsonRpcProxy.AFE_BASE_URL);
        NotifyManager.getInstance().initialize();

        // initialize static data, and don't show main UI until that's done
        StaticDataRepository.getRepository().refresh(
                                 new StaticDataRepository.FinishedCallback() {
            public void onFinished() {
                finishLoading();
            }
        });
    }

    private JobCreateListener jobCreateListener = new JobCreateListener() {
        public void onJobCreated(int jobId) {
            showJob(jobId);
        }
    };

    protected void finishLoading() {
        SiteCommonClassFactory.globalInitialize();

        String wmatrixUrl = StaticDataRepository.getRepository().getData(
            "wmatrix_url").isString().stringValue();
        if (!wmatrixUrl.equals("")) {
            Document.get().getElementById("wmatrix-link").setAttribute(
                "href", wmatrixUrl);
            Document.get().getElementById("wmatrix").removeClassName("hidden");
        }
        String stainlessUrl = StaticDataRepository.getRepository().getData(
            "stainless_url").isString().stringValue();
        if (!stainlessUrl.equals("")) {
            Document.get().getElementById("stainless-link").setAttribute(
                "href", stainlessUrl);
            Document.get().getElementById("stainless").removeClassName(
                "hidden");
        }
        boolean is_moblab = StaticDataRepository.getRepository().getData(
            "is_moblab").isBoolean().booleanValue();
        mainTabPanel = new CustomTabPanel(is_moblab);
        if (is_moblab) {
            Document.get().getElementById("moblab_setup").removeClassName("hidden");
            Document.get().getElementById("mobmonitor_link").setAttribute("href",
                "http://" + Location.getHostName() + ":9991");
        }

        jobList = new JobListView(new JobSelectListener() {
            public void onJobSelected(int jobId) {
                showJob(jobId);
            }
        });
        jobDetail = new JobDetailView(new JobDetailListener() {
            public void onHostSelected(String hostId) {
                showHost(hostId);
            }

            public void onCloneJob(JSONValue cloneInfo) {
                createJob.ensureInitialized();
                mainTabPanel.selectTabView(createJob);
                // Makes sure we set cloning mode after the tab is selected. Otherwise,
                // the mode will be reset.
                createJob.cloneJob(cloneInfo);
            }
        });

        createJob = AfeUtils.factory.getCreateJobView(jobCreateListener);

        hostListView = new HostListView(new HostListListener() {
            public void onHostSelected(String hostId) {
                showHost(hostId);
            }
        }, jobCreateListener);

        hostDetailView = new HostDetailView(new HostDetailListener() {
            public void onJobSelected(int jobId) {
                showJob(jobId);
            }
        }, jobCreateListener);

        TabView[] tabViews = new TabView[] {jobList, jobDetail, createJob,
                                            hostListView, hostDetailView};
        for (TabView tabView : tabViews) {
            mainTabPanel.addTabView(tabView);
        }

        final RootPanel tabsRoot = RootPanel.get("tabs");
        tabsRoot.add(mainTabPanel);
        CustomHistory.processInitialToken();
        mainTabPanel.initialize();
        tabsRoot.setStyleName("");
    }

    protected void showJob(int jobId) {
        jobDetail.ensureInitialized();
        jobDetail.updateObjectId(Integer.toString(jobId));
        mainTabPanel.selectTabView(jobDetail);
    }

    protected void showHost(String hostId) {
        hostDetailView.ensureInitialized();
        hostDetailView.updateObjectId(hostId);
        mainTabPanel.selectTabView(hostDetailView);
    }
}
