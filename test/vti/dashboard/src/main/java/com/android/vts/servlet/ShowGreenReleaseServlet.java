/*
 * Copyright (c) 2018 Google Inc. All Rights Reserved.
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

import com.android.vts.entity.TestPlanEntity;
import com.android.vts.entity.TestPlanRunEntity;
import com.android.vts.util.FilterUtil;
import com.google.appengine.api.datastore.DatastoreService;
import com.google.appengine.api.datastore.DatastoreServiceFactory;
import com.google.appengine.api.datastore.Entity;
import com.google.appengine.api.datastore.Key;
import com.google.appengine.api.datastore.KeyFactory;
import com.google.appengine.api.datastore.Query.Filter;
import com.google.appengine.api.datastore.Query.SortDirection;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.logging.Level;
import java.util.stream.Collectors;
import javax.servlet.RequestDispatcher;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

public class ShowGreenReleaseServlet extends BaseServlet {
    private static final String PLAN_RELEASE_JSP = "WEB-INF/jsp/show_green_release.jsp";
    private static final int MAX_RUNS_PER_PAGE = 9999;

    /** Helper class for displaying each device build info on the green build page. */
    public class DeviceBuildInfo implements Comparable<DeviceBuildInfo>, Cloneable {

        /** Device model name ex) marlin, walleye */
        private String deviceBuildTarget;
        /** Candidate Build ID */
        private String candidateBuildId;
        /** Candidate Build ID Timestamp for url parameter */
        private Long candidateBuildIdTimestamp;
        /** Green Build ID */
        private String greenBuildId;
        /** Green Build ID Timestamp for url parameter */
        private Long greenBuildIdTimestamp;

        /**
         * Device Build Info constructor.
         *
         * @param deviceBuildTarget The key of the test.
         * @param candidateBuildId The number of tests failing.
         */
        public DeviceBuildInfo(String deviceBuildTarget, String candidateBuildId) {
            this.deviceBuildTarget = deviceBuildTarget;
            this.candidateBuildId = candidateBuildId;
            this.candidateBuildIdTimestamp = 0L;
            this.greenBuildId = "N/A";
            this.greenBuildIdTimestamp = 0L;
        }

        /**
         * Get the device name.
         *
         * @return The device name.
         */
        public String getDeviceBuildTarget() {
            return this.deviceBuildTarget;
        }

        /**
         * Get the candidate build ID.
         *
         * @return The candidate build ID.
         */
        public String getCandidateBuildId() {
            return this.candidateBuildId;
        }

        /** Set the candidate build ID. */
        public void setCandidateBuildId(String candidateBuildId) {
            this.candidateBuildId = candidateBuildId;
        }

        /**
         * Get the candidate build ID timestamp.
         *
         * @return The candidate build ID timestamp.
         */
        public Long getCandidateBuildIdTimestamp() {
            return this.candidateBuildIdTimestamp;
        }

        /** Set the candidate build ID timestamp. */
        public void setCandidateBuildIdTimestamp(Long candidateBuildIdTimestamp) {
            this.candidateBuildIdTimestamp = candidateBuildIdTimestamp;
        }

        /**
         * Get the green build ID.
         *
         * @return The green build ID.
         */
        public String getGreenBuildId() {
            return this.greenBuildId;
        }

        /** Set the green build ID. */
        public void setGreenBuildId(String greenBuildId) {
            this.greenBuildId = greenBuildId;
        }

        /**
         * Get the candidate build ID timestamp.
         *
         * @return The candidate build ID timestamp.
         */
        public Long getGreenBuildIdTimestamp() {
            return this.greenBuildIdTimestamp;
        }

        /** Set the candidate build ID timestamp. */
        public void setGreenBuildIdTimestamp(Long greenBuildIdTimestamp) {
            this.greenBuildIdTimestamp = greenBuildIdTimestamp;
        }

        @Override
        public int compareTo(DeviceBuildInfo deviceBuildInfo) {
            return this.deviceBuildTarget.compareTo(deviceBuildInfo.getDeviceBuildTarget());
        }

        @Override
        protected Object clone() throws CloneNotSupportedException {
            return super.clone();
        }
    }

    @Override
    public PageType getNavParentType() {
        return PageType.RELEASE;
    }

    @Override
    public List<Page> getBreadcrumbLinks(HttpServletRequest request) {
        List<Page> links = new ArrayList<>();
        String planName = request.getParameter("plan");
        links.add(new Page(PageType.PLAN_RELEASE, planName, "?plan=" + planName));
        return links;
    }

    // This function will build and return basic parameter HashMap based on parameter information
    // and
    // this value will also be used on green build page to show the basic structure.
    private Map<String, List<DeviceBuildInfo>> getBasicParamMap(Map<String, List<String>> param) {
        Map<String, List<DeviceBuildInfo>> basicParamMap = new HashMap<>();
        param.forEach(
                (branch, buildTargetList) -> {
                    List<DeviceBuildInfo> deviceBuildTargetList = new ArrayList<>();
                    buildTargetList.forEach(
                            buildTargetName -> {
                                deviceBuildTargetList.add(
                                        new DeviceBuildInfo(buildTargetName, "N/A"));
                            });
                    basicParamMap.put(branch, deviceBuildTargetList);
                });
        return basicParamMap;
    }

    @Override
    public void doGetHandler(HttpServletRequest request, HttpServletResponse response)
            throws IOException {
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();

        Map<String, List<String>> paramInfoMap =
                new HashMap<String, List<String>>() {
                    {
                        put("master", Arrays.asList("sailfish-userdebug", "marlin-userdebug"));
                        put(
                                "oc-mr1",
                                Arrays.asList(
                                        "sailfish-userdebug",
                                        "marlin-userdebug",
                                        "taimen-userdebug",
                                        "walleye-userdebug",
                                        "aosp_arm_a-userdebug"));
                        put("oc", Arrays.asList("sailfish-userdebug", "marlin-userdebug"));
                    }
                };

        String testPlan = request.getParameter("plan");

        Map<String, List<DeviceBuildInfo>> baseParamMap = getBasicParamMap(paramInfoMap);
        baseParamMap.forEach(
                (branchKey, deviceBuildInfoList) -> {
                    List<List<String>> allPassIdLists = new ArrayList<>();
                    Map<String, List<TestPlanRunEntity>> allTestPlanRunEntityMap = new HashMap<>();
                    deviceBuildInfoList.forEach(
                            deviceBuildInfo -> {
                                Map<String, Object> paramMap =
                                        new HashMap<String, Object>() {
                                            {
                                                put("branch", new String[] {branchKey});
                                                put(
                                                        "device",
                                                        new String[] {
                                                            deviceBuildInfo.getDeviceBuildTarget()
                                                        });
                                            }
                                        };

                                Calendar cal = Calendar.getInstance();
                                cal.add(Calendar.DATE, -7);
                                Long startTime = cal.getTime().getTime() * 1000;
                                Long endTime = Calendar.getInstance().getTime().getTime() * 1000;

                                SortDirection dir = SortDirection.DESCENDING;

                                boolean unfiltered = false;
                                boolean showPresubmit = false;
                                boolean showPostsubmit = true;
                                Filter typeFilter =
                                        FilterUtil.getTestTypeFilter(
                                                showPresubmit, showPostsubmit, unfiltered);
                                Key testPlanKey =
                                        KeyFactory.createKey(TestPlanEntity.KIND, testPlan);
                                Filter testPlanRunFilter =
                                        FilterUtil.getTimeFilter(
                                                testPlanKey,
                                                TestPlanRunEntity.KIND,
                                                startTime,
                                                endTime,
                                                typeFilter);

                                List<Filter> userTestFilters =
                                        FilterUtil.getUserTestFilters(paramMap);
                                userTestFilters.add(0, testPlanRunFilter);
                                Filter userDeviceFilter = FilterUtil.getUserDeviceFilter(paramMap);

                                List<Key> matchingKeyList =
                                        FilterUtil.getMatchingKeys(
                                                testPlanKey,
                                                TestPlanRunEntity.KIND,
                                                userTestFilters,
                                                userDeviceFilter,
                                                dir,
                                                MAX_RUNS_PER_PAGE);

                                logger.log(
                                        Level.INFO,
                                        "the number of matching key => " + matchingKeyList.size());
                                if (matchingKeyList.size() > 0) {
                                    Map<Key, Entity> entityMap = datastore.get(matchingKeyList);

                                    List<TestPlanRunEntity> testPlanRunEntityList =
                                            entityMap
                                                    .values()
                                                    .stream()
                                                    .map(
                                                            entity ->
                                                                    TestPlanRunEntity.fromEntity(
                                                                            entity))
                                                    .collect(Collectors.toList());

                                    allTestPlanRunEntityMap.put(
                                            branchKey
                                                    + "-"
                                                    + deviceBuildInfo.getDeviceBuildTarget(),
                                            testPlanRunEntityList);

                                    // The passBuildIdList containing all passed buildId List for device
                                    List<String> passBuildIdList = testPlanRunEntityList.stream()
                                          .filter(entity -> entity.failCount == 0L)
                                          .map(entity -> entity.testBuildId)
                                          .collect(Collectors.toList());
                                    allPassIdLists.add(passBuildIdList);
                                    logger.log(Level.INFO, "passBuildIdList => " + passBuildIdList);

                                    // The logic for candidate build ID is starting from here
                                    Comparator<TestPlanRunEntity> byPassing =
                                            Comparator.comparingLong(
                                                    elemFirst -> elemFirst.passCount);

                                    Comparator<TestPlanRunEntity> byNonPassing =
                                            Comparator.comparingLong(
                                                    elemFirst -> elemFirst.failCount);

                                    // This will get the TestPlanRunEntity having maximum number of
                                    // passing and minimum number of fail
                                    Optional<TestPlanRunEntity> testPlanRunEntity =
                                            testPlanRunEntityList
                                                    .stream()
                                                    .sorted(
                                                            byPassing
                                                                    .reversed()
                                                                    .thenComparing(byNonPassing))
                                                    .findFirst();

                                    String buildId =
                                            testPlanRunEntity
                                                    .map(entity -> entity.testBuildId)
                                                    .orElse("");
                                    deviceBuildInfo.setCandidateBuildId(buildId);
                                    Long buildIdTimestamp =
                                            testPlanRunEntity
                                                    .map(
                                                            entity -> {
                                                                return entity.startTimestamp;
                                                            })
                                                    .orElse(0L);
                                    deviceBuildInfo.setCandidateBuildIdTimestamp(buildIdTimestamp);
                                } else {
                                    allPassIdLists.add(new ArrayList<>());
                                    deviceBuildInfo.setCandidateBuildId("No Test Results");
                                }
                            });
                    Set<String> greenBuildIdList = FilterUtil.getCommonElements(allPassIdLists);
                    if (greenBuildIdList.size() > 0) {
                        String greenBuildId = greenBuildIdList.iterator().next();
                        deviceBuildInfoList.forEach(
                                deviceBuildInfo -> {
                                    // This is to get the timestamp for greenBuildId
                                    Optional<TestPlanRunEntity> testPlanRunEntity =
                                            allTestPlanRunEntityMap
                                                    .get(
                                                            branchKey
                                                                    + "-"
                                                                    + deviceBuildInfo
                                                                            .getDeviceBuildTarget())
                                                    .stream()
                                                    .filter(
                                                            entity ->
                                                                    entity.testBuildId
                                                                            .equalsIgnoreCase(
                                                                                    greenBuildId))
                                                    .findFirst();
                                    // Setting the greenBuildId value and timestamp to
                                    // deviceBuildInfo object
                                    deviceBuildInfo.setGreenBuildId(greenBuildId);
                                    Long buildIdTimestamp =
                                            testPlanRunEntity
                                                    .map(entity -> entity.startTimestamp)
                                                    .orElse(0L);
                                    deviceBuildInfo.setGreenBuildIdTimestamp(buildIdTimestamp);
                                });
                    }
                });

        request.setAttribute("plan", request.getParameter("plan"));
        request.setAttribute("greenBuildInfo", baseParamMap);
        response.setStatus(HttpServletResponse.SC_OK);
        RequestDispatcher dispatcher = request.getRequestDispatcher(PLAN_RELEASE_JSP);
        try {
            dispatcher.forward(request, response);
        } catch (ServletException e) {
            logger.log(Level.SEVERE, "Servlet Exception caught : ", e);
        }
    }
}
