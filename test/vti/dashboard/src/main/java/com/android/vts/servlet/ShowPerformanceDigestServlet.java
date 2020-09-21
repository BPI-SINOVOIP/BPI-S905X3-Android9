/*
 * Copyright (c) 2016 Google Inc. All Rights Reserved.
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

import com.android.vts.util.DatastoreHelper;
import com.android.vts.util.PerformanceSummary;
import com.android.vts.util.PerformanceUtil;
import com.android.vts.util.ProfilingPointSummary;
import com.android.vts.util.StatSummary;
import java.io.IOException;
import java.math.RoundingMode;
import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.TimeUnit;
import java.util.logging.Level;
import javax.servlet.RequestDispatcher;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

/** Servlet for producing tabular performance summaries. */
public class ShowPerformanceDigestServlet extends BaseServlet {
    private static final String PERF_DIGEST_JSP = "WEB-INF/jsp/show_performance_digest.jsp";

    private static final String MEAN = "Mean";
    private static final String MIN = "Min";
    private static final String MAX = "Max";
    private static final String MEAN_DELTA = "&Delta;Mean (%)";
    private static final String HIGHER_IS_BETTER =
            "Note: Higher values are better. Maximum is the best-case performance.";
    private static final String LOWER_IS_BETTER =
            "Note: Lower values are better. Minimum is the best-case performance.";
    private static final String STD = "Std";

    private static final DecimalFormat FORMATTER;

    /** Initialize the decimal formatter. */
    static {
        FORMATTER = new DecimalFormat("#.##");
        FORMATTER.setRoundingMode(RoundingMode.HALF_UP);
    }

    @Override
    public PageType getNavParentType() {
        return PageType.TOT;
    }

    @Override
    public List<Page> getBreadcrumbLinks(HttpServletRequest request) {
        List<Page> links = new ArrayList<>();
        String testName = request.getParameter("testName");
        links.add(new Page(PageType.TABLE, testName, "?testName=" + testName));
        links.add(new Page(PageType.PERFORMANCE_DIGEST, "?testName=" + testName));
        return links;
    }

    /**
     * Generates an HTML summary of the performance changes for the profiling results in the
     * specified table.
     *
     * <p>Retrieves the past 24 hours of profiling data and compares it to the 24 hours that
     * preceded it. Creates a table representation of the mean and standard deviation for each
     * profiling point. When performance degrades, the cell is shaded red.
     *
     * @param profilingPoint The name of the profiling point to summarize.
     * @param testSummary The ProfilingPointSummary object to compare against.
     * @param perfSummaries List of PerformanceSummary objects for each profiling run (in reverse
     *     chronological order).
     * @param sectionLabels HTML string for the section labels (i.e. for each time interval).
     * @returns An HTML string for a table comparing the profiling point results across time
     *     intervals.
     */
    public static String getPerformanceSummary(
            String profilingPoint,
            ProfilingPointSummary testSummary,
            List<PerformanceSummary> perfSummaries,
            String sectionLabels) {
        String tableHTML = "<table>";

        // Format section labels
        tableHTML += "<tr>";
        tableHTML += "<th class='section-label grey lighten-2'>";
        tableHTML += testSummary.yLabel + "</th>";
        tableHTML += sectionLabels;
        tableHTML += "</tr>";

        String bestCaseString;
        switch (testSummary.getRegressionMode()) {
            case VTS_REGRESSION_MODE_DECREASING:
                bestCaseString = MAX;
                break;
            default:
                bestCaseString = MIN;
                break;
        }

        // Format column labels
        tableHTML += "<tr>";
        for (int i = 0; i <= perfSummaries.size() + 1; i++) {
            if (i > 1) {
                tableHTML += "<th class='section-label grey lighten-2'>" + MEAN_DELTA + "</th>";
            }
            if (i == 0) {
                tableHTML += "<th class='section-label grey lighten-2'>";
                tableHTML += testSummary.xLabel + "</th>";
            } else if (i > 0) {
                tableHTML += "<th class='section-label grey lighten-2'>" + bestCaseString + "</th>";
                tableHTML += "<th class='section-label grey lighten-2'>" + MEAN + "</th>";
                tableHTML += "<th class='section-label grey lighten-2'>" + STD + "</th>";
            }
        }
        tableHTML += "</tr>";

        // Populate data cells
        for (StatSummary stats : testSummary) {
            String label = stats.getLabel();
            tableHTML += "<tr><td class='axis-label grey lighten-2'>" + label;
            tableHTML += "</td><td class='cell inner-cell'>";
            tableHTML += FORMATTER.format(stats.getBestCase()) + "</td>";
            tableHTML += "<td class='cell inner-cell'>";
            tableHTML += FORMATTER.format(stats.getMean()) + "</td>";
            tableHTML += "<td class='cell outer-cell'>";
            if (stats.getCount() < 2) {
                tableHTML += " - </td>";
            } else {
                tableHTML += FORMATTER.format(stats.getStd()) + "</td>";
            }
            for (PerformanceSummary prevPerformance : perfSummaries) {
                if (prevPerformance.hasProfilingPoint(profilingPoint)) {
                    StatSummary baseline =
                            prevPerformance
                                    .getProfilingPointSummary(profilingPoint)
                                    .getStatSummary(label);
                    tableHTML +=
                            PerformanceUtil.getAvgCasePerformanceComparisonHTML(
                                    baseline, stats, "cell inner-cell", "cell outer-cell", "", "");
                } else {
                    tableHTML += "<td></td><td></td><td></td><td></td>";
                }
            }
            tableHTML += "</tr>";
        }
        tableHTML += "</table>";
        return tableHTML;
    }

    @Override
    public void doGetHandler(HttpServletRequest request, HttpServletResponse response)
            throws IOException {
        RequestDispatcher dispatcher = null;
        String testName = request.getParameter("testName");
        String selectedDevice = request.getParameter("device");
        Long startTime = null;
        if (request.getParameter("startTime") != null) {
            String time = request.getParameter("startTime");
            try {
                startTime = Long.parseLong(time);
            } catch (NumberFormatException e) {
                logger.log(Level.WARNING, "Invalid start time passed to digest servlet: " + time);
            }
        }
        if (startTime == null) {
            startTime = TimeUnit.MILLISECONDS.toMicros(System.currentTimeMillis());
        }

        // Add today to the list of time intervals to analyze
        List<PerformanceSummary> summaries = new ArrayList<>();
        PerformanceSummary today =
                new PerformanceSummary(startTime - TimeUnit.DAYS.toMicros(1), startTime);
        summaries.add(today);

        // Add yesterday as a baseline time interval for analysis
        long oneDayAgo = startTime - TimeUnit.DAYS.toMicros(1);
        PerformanceSummary yesterday =
                new PerformanceSummary(oneDayAgo - TimeUnit.DAYS.toMicros(1), oneDayAgo);
        summaries.add(yesterday);

        // Add last week as a baseline time interval for analysis
        long oneWeek = TimeUnit.DAYS.toMicros(7);
        long oneWeekAgo = startTime - oneWeek;
        String spanString = "<span class='date-label'>";
        String label =
                spanString + TimeUnit.MICROSECONDS.toMillis(oneWeekAgo - oneWeek) + "</span>";
        label += " - " + spanString + TimeUnit.MICROSECONDS.toMillis(oneWeekAgo) + "</span>";
        PerformanceSummary lastWeek =
                new PerformanceSummary(oneWeekAgo - oneWeek, oneWeekAgo, label);
        summaries.add(lastWeek);
        PerformanceUtil.updatePerformanceSummary(
                testName, oneWeekAgo - oneWeek, startTime, selectedDevice, summaries);

        int colCount = 0;
        String sectionLabels = "";
        List<PerformanceSummary> nonEmptySummaries = new ArrayList<>();
        for (PerformanceSummary perfSummary : summaries) {
            if (perfSummary.size() == 0) continue;

            nonEmptySummaries.add(perfSummary);
            String content = perfSummary.label;
            sectionLabels += "<th class='section-label grey lighten-2 center' ";
            if (colCount++ == 0) sectionLabels += "colspan='3'";
            else sectionLabels += "colspan='4'";
            sectionLabels += ">" + content + "</th>";
        }

        List<String> tables = new ArrayList<>();
        List<String> tableTitles = new ArrayList<>();
        List<String> tableSubtitles = new ArrayList<>();
        if (nonEmptySummaries.size() != 0) {
            PerformanceSummary todayPerformance = nonEmptySummaries.remove(0);
            String[] profilingNames = todayPerformance.getProfilingPointNames();

            for (String profilingPointName : profilingNames) {
                ProfilingPointSummary baselinePerformance =
                        todayPerformance.getProfilingPointSummary(profilingPointName);
                String table =
                        getPerformanceSummary(
                                profilingPointName,
                                baselinePerformance,
                                nonEmptySummaries,
                                sectionLabels);
                if (table != null) {
                    tables.add(table);
                    tableTitles.add(profilingPointName);
                    switch (baselinePerformance.getRegressionMode()) {
                        case VTS_REGRESSION_MODE_DECREASING:
                            tableSubtitles.add(HIGHER_IS_BETTER);
                            break;
                        default:
                            tableSubtitles.add(LOWER_IS_BETTER);
                            break;
                    }
                }
            }
        }

        request.setAttribute("testName", testName);
        request.setAttribute("tables", tables);
        request.setAttribute("tableTitles", tableTitles);
        request.setAttribute("tableSubtitles", tableSubtitles);
        request.setAttribute("startTime", Long.toString(startTime));
        request.setAttribute("selectedDevice", selectedDevice);
        request.setAttribute("devices", DatastoreHelper.getAllBuildFlavors());

        dispatcher = request.getRequestDispatcher(PERF_DIGEST_JSP);
        try {
            dispatcher.forward(request, response);
        } catch (ServletException e) {
            logger.log(Level.SEVERE, "Servlet Exception caught : " + e.toString());
        }
    }
}
