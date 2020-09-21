package autotest.common.ui;

import autotest.common.CustomHistory;
import autotest.common.CustomHistory.CustomHistoryListener;

import com.google.gwt.event.dom.client.ClickEvent;
import com.google.gwt.event.dom.client.ClickHandler;
import com.google.gwt.event.logical.shared.BeforeSelectionEvent;
import com.google.gwt.event.logical.shared.BeforeSelectionHandler;
import com.google.gwt.event.logical.shared.SelectionEvent;
import com.google.gwt.event.logical.shared.SelectionHandler;
import com.google.gwt.event.logical.shared.ValueChangeEvent;
import com.google.gwt.event.logical.shared.ValueChangeHandler;
import com.google.gwt.user.client.Timer;
import com.google.gwt.user.client.ui.Button;
import com.google.gwt.user.client.ui.CheckBox;
import com.google.gwt.user.client.ui.Composite;
import com.google.gwt.user.client.ui.DeckPanel;
import com.google.gwt.user.client.ui.HorizontalPanel;
import com.google.gwt.user.client.ui.Panel;
import com.google.gwt.user.client.ui.TabPanel;
import com.google.gwt.user.client.ui.VerticalPanel;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;

public class CustomTabPanel extends Composite implements CustomHistoryListener,
                                                         BeforeSelectionHandler<Integer>,
                                                         SelectionHandler<Integer> {
    private static final int AUTOREFRESH_INTERVAL = 5000; // millisecond

    protected TabPanel tabPanel = new TabPanel();
    protected Panel otherWidgetsPanel = new HorizontalPanel();
    private Panel commonAreaPanel = new VerticalPanel();
    protected Button refreshButton = new Button("Refresh");
    protected boolean autorefreshEnabled = false;
    protected CheckBox autorefreshCheckbox = new CheckBox("Auto Refresh");
    protected int topBarHeight = 0;
    protected List<TabView> tabViews = new ArrayList<TabView>();
    private boolean doUpdateHistory = true;
    private Timer autorefreshTimer;

    public CustomTabPanel() {
      this(false);
    }

    public CustomTabPanel(boolean enableAutorefresh) {
        VerticalPanel container = new VerticalPanel();
        HorizontalPanel top = new HorizontalPanel();
        VerticalPanel bottom = new VerticalPanel();
        container.add(top);
        container.add(bottom);

        // put the TabBar at the top left
        top.add(tabPanel.getTabBar());
        top.setCellHeight(tabPanel.getTabBar(), "100%");
        tabPanel.getTabBar().setHeight("100%");

        // make a place for other widgets next to the tab bar
        top.add(otherWidgetsPanel);

        // put a common area above the tab deck
        commonAreaPanel.setWidth("100%");
        bottom.add(commonAreaPanel);

        // put the TabPanel's DeckPanel below
        DeckPanel tabDeck = tabPanel.getDeckPanel();
        bottom.add(tabDeck);
        bottom.setCellHeight(tabDeck, "100%");

        tabPanel.addBeforeSelectionHandler(this);
        tabPanel.addSelectionHandler(this);

        // transfer the DeckPanel's class to the entire bottom panel
        String tabDeckClass = tabDeck.getStyleName();
        tabDeck.setStyleName("");
        bottom.setStyleName(tabDeckClass);
        bottom.setWidth("100%");

        refreshButton.addClickHandler(new ClickHandler() {
            public void onClick(ClickEvent event) {
                getSelectedTabView().refresh();
            }
        });
        otherWidgetsPanel.add(refreshButton);

        this.autorefreshEnabled = enableAutorefresh;
        if (enableAutorefresh) {
            autorefreshCheckbox.addValueChangeHandler(new ValueChangeHandler() {
              public void onValueChange(ValueChangeEvent event) {
                  // Ensure Initialization.
                  if (autorefreshTimer == null)
                      initialize();

                  Boolean value = autorefreshCheckbox.getValue();
                  TabView selectedTabView = tabViews.get(
                          tabPanel.getTabBar().getSelectedTab());
                  if (selectedTabView != null)
                      selectedTabView.setAutorefresh(value);
                  if (value) {
                      autorefreshTimer.scheduleRepeating(AUTOREFRESH_INTERVAL);
                  } else {
                      autorefreshTimer.cancel();
                  }
              }
          });
          otherWidgetsPanel.add(autorefreshCheckbox);
        }

        CustomHistory.addHistoryListener(this);

        top.setStyleName("custom-tab-top");
        container.setStyleName("custom-tab-panel");
        initWidget(container);
    }

    /**
     * This must be called after this widget has been added to the page.
     */
    public void initialize() {
        // if the history token didn't provide a selected tab, default to the
        // first tab
        if (getSelectedTabView() == null)
            tabPanel.selectTab(0);
        autorefreshTimer = new Timer() {
            @Override
            public void run() {
                getSelectedTabView().refresh();
            }
        };
    }

    public void addTabView(TabView tabView) {
        tabView.attachToDocument();
        tabViews.add(tabView);
        tabPanel.add(tabView.getWidget(), tabView.getTitle());
    }

    public List<TabView> getTabViews() {
        return Collections.unmodifiableList(tabViews);
    }

    public TabView getSelectedTabView() {
        int selectedTab = tabPanel.getTabBar().getSelectedTab();
        if (selectedTab == -1)
            return null;
        return tabViews.get(selectedTab);
    }

    public void selectTabView(TabView tabView) {
        for (int i = 0; i < tabViews.size(); i++) {
            if (tabViews.get(i) == tabView) {
                tabPanel.selectTab(i);
                return;
            }
        }

        throw new IllegalArgumentException("Tab not found");
    }

    public TabPanel getTabPanel() {
        return tabPanel;
    }

    public Panel getOtherWidgetsPanel() {
        return otherWidgetsPanel;
    }

    public Panel getCommonAreaPanel() {
        return commonAreaPanel;
    }

    public void onHistoryChanged(Map<String, String> arguments) {
        String tabId = arguments.get("tab_id");
        if (tabId == null) {
            return;
        }

        for (TabView tabView : tabViews) {
            if (tabId.equals(tabView.getElementId())) {
                tabView.ensureInitialized();
                if (arguments.size() > 1) {
                    // only pass along arguments if there's more than just tab_id
                    tabView.handleHistoryArguments(arguments);
                }

                if (getSelectedTabView() != tabView) {
                    doUpdateHistory = false;
                    selectTabView(tabView);
                    doUpdateHistory = true;
                } else {
                    tabView.display();
                }

                return;
            }
        }
    }

    @Override
    public void onBeforeSelection(BeforeSelectionEvent<Integer> event) {
        // do nothing if the user clicks the selected tab
        if (tabPanel.getTabBar().getSelectedTab() == event.getItem())
            event.cancel();
        TabView selectedTabView = getSelectedTabView();
        if (selectedTabView != null) {
            selectedTabView.hide();
        }
        tabViews.get(event.getItem()).ensureInitialized();
        tabViews.get(event.getItem()).display();
    }

    @Override
    public void onSelection(SelectionEvent<Integer> event) {
        TabView selectedTabView = tabViews.get(event.getSelectedItem());
        if (doUpdateHistory)
            selectedTabView.updateHistory();
        // The second parameter is to fire a valueChange event if value changed.
        if (autorefreshEnabled) {
          autorefreshCheckbox.setValue(selectedTabView.isAutorefreshOn(),
             true);
        }
    }
}
