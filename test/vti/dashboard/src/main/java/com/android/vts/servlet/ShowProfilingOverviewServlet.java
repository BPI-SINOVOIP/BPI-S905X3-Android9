/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License. You may
 * obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

package com.android.vts.servlet;

import com.android.vts.entity.ProfilingPointEntity;
import com.android.vts.entity.ProfilingPointSummaryEntity;
import com.android.vts.util.BoxPlot;
import com.android.vts.util.DatastoreHelper;
import com.android.vts.util.FilterUtil;
import com.android.vts.util.GraphSerializer;
import com.google.appengine.api.datastore.DatastoreService;
import com.google.appengine.api.datastore.DatastoreServiceFactory;
import com.google.appengine.api.datastore.Entity;
import com.google.appengine.api.datastore.Query;
import com.google.appengine.api.datastore.Query.Filter;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.TimeUnit;
import java.util.logging.Level;
import java.util.stream.Collectors;
import javax.servlet.RequestDispatcher;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

/** Servlet for handling requests to load graphs. */
public class ShowProfilingOverviewServlet extends BaseServlet {
    private static final String PROFILING_OVERVIEW_JSP = "WEB-INF/jsp/show_profiling_overview.jsp";

    @Override
    public PageType getNavParentType() {
        return PageType.PROFILING_LIST;
    }

    @Override
    public List<Page> getBreadcrumbLinks(HttpServletRequest request) {
        List<Page> links = new ArrayList<>();
        String testName = request.getParameter("testName");
        links.add(new Page(PageType.PROFILING_OVERVIEW, testName, "?testName=" + testName));
        return links;
    }

    @Override
    public void doGetHandler(HttpServletRequest request, HttpServletResponse response)
            throws IOException {
        RequestDispatcher dispatcher = null;
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();

        String testName = request.getParameter("testName");
        long endTime = TimeUnit.MILLISECONDS.toMicros(System.currentTimeMillis());
        long startTime = endTime - TimeUnit.DAYS.toMicros(30);

        // Create a query for test runs matching the time window filter

        Map<String, Object> parameterMap = request.getParameterMap();
        boolean hasBranchFilter = parameterMap.containsKey(FilterUtil.FilterKey.BRANCH.getValue());
        Filter deviceFilter;
        if (hasBranchFilter) {
            deviceFilter =
                    FilterUtil.FilterKey.BRANCH.getFilterForString(
                            FilterUtil.getFirstParameter(
                                    parameterMap, FilterUtil.FilterKey.BRANCH.getValue()));
        } else {
            deviceFilter =
                    FilterUtil.FilterKey.BRANCH.getFilterForString(ProfilingPointSummaryEntity.ALL);
        }

        boolean hasTargetFilter = parameterMap.containsKey(FilterUtil.FilterKey.TARGET.getValue());
        if (hasTargetFilter) {
            deviceFilter =
                    Query.CompositeFilterOperator.and(
                            deviceFilter,
                            FilterUtil.FilterKey.TARGET.getFilterForString(
                                    FilterUtil.getFirstParameter(
                                            parameterMap, FilterUtil.FilterKey.TARGET.getValue())));
        } else {
            deviceFilter =
                    Query.CompositeFilterOperator.and(
                            deviceFilter,
                            FilterUtil.FilterKey.TARGET.getFilterForString(
                                    ProfilingPointSummaryEntity.ALL));
        }

        Filter startFilter =
                new Query.FilterPredicate(
                        ProfilingPointSummaryEntity.START_TIME,
                        Query.FilterOperator.GREATER_THAN_OR_EQUAL,
                        startTime);
        Filter endFilter =
                new Query.FilterPredicate(
                        ProfilingPointSummaryEntity.START_TIME,
                        Query.FilterOperator.LESS_THAN_OR_EQUAL,
                        endTime);
        Filter timeFilter = Query.CompositeFilterOperator.and(startFilter, endFilter);

        Filter filter = Query.CompositeFilterOperator.and(timeFilter, deviceFilter);

        Query profilingPointQuery =
                new Query(ProfilingPointEntity.KIND)
                        .setFilter(
                                new Query.FilterPredicate(
                                        ProfilingPointEntity.TEST_NAME,
                                        Query.FilterOperator.EQUAL,
                                        testName));

        List<ProfilingPointEntity> profilingPoints = new ArrayList<>();
        for (Entity e :
                datastore
                        .prepare(profilingPointQuery)
                        .asIterable(DatastoreHelper.getLargeBatchOptions())) {
            ProfilingPointEntity pp = ProfilingPointEntity.fromEntity(e);
            if (pp == null) continue;
            profilingPoints.add(pp);
        }

        Map<ProfilingPointEntity, Iterable<Entity>> asyncEntities = new HashMap<>();
        for (ProfilingPointEntity pp : profilingPoints) {
            Query profilingQuery =
                    new Query(ProfilingPointSummaryEntity.KIND)
                            .setAncestor(pp.key)
                            .setFilter(filter);
            asyncEntities.put(
                    pp,
                    datastore
                            .prepare(profilingQuery)
                            .asIterable(DatastoreHelper.getLargeBatchOptions()));
        }

        Map<String, BoxPlot> plotMap = new HashMap<>();
        for (ProfilingPointEntity pp : profilingPoints) {
            if (!plotMap.containsKey(pp.profilingPointName)) {
                plotMap.put(
                        pp.profilingPointName, new BoxPlot(pp.profilingPointName, null, pp.xLabel));
            }
            BoxPlot plot = plotMap.get(pp.profilingPointName);
            Set<Long> timestamps = new HashSet<>();
            for (Entity e : asyncEntities.get(pp)) {
                ProfilingPointSummaryEntity pps = ProfilingPointSummaryEntity.fromEntity(e);
                if (pps == null) continue;
                plot.addSeriesData(Long.toString(pps.startTime), pps.series, pps.globalStats);
                timestamps.add(pps.startTime);
            }
            List<Long> timestampList = new ArrayList<>(timestamps);
            timestampList.sort(Comparator.reverseOrder());
            List<String> timestampStrings =
                    timestampList.stream().map(Object::toString).collect(Collectors.toList());
            plot.setLabels(timestampStrings);
        }

        List<BoxPlot> plots = new ArrayList<>();
        for (String key : plotMap.keySet()) {
            BoxPlot plot = plotMap.get(key);
            if (plot.size() == 0) continue;
            plots.add(plot);
        }
        plots.sort((b1, b2) -> b1.getName().compareTo(b2.getName()));

        Gson gson =
                new GsonBuilder()
                        .registerTypeHierarchyAdapter(BoxPlot.class, new GraphSerializer())
                        .create();

        FilterUtil.setAttributes(request, parameterMap);
        request.setAttribute("plots", gson.toJson(plots));
        request.setAttribute("testName", request.getParameter("testName"));
        request.setAttribute("branches", new Gson().toJson(DatastoreHelper.getAllBranches()));
        request.setAttribute("devices", new Gson().toJson(DatastoreHelper.getAllBuildFlavors()));
        dispatcher = request.getRequestDispatcher(PROFILING_OVERVIEW_JSP);
        try {
            dispatcher.forward(request, response);
        } catch (ServletException e) {
            logger.log(Level.SEVERE, "Servlet Exception caught : ", e);
        }
    }
}
