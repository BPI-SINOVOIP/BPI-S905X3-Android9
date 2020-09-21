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

import com.google.appengine.api.users.User;
import com.google.appengine.api.users.UserService;
import com.google.appengine.api.users.UserServiceFactory;
import com.google.gson.Gson;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.servlet.RequestDispatcher;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.servlet.http.HttpSession;

public abstract class BaseServlet extends HttpServlet {
    protected final Logger logger = Logger.getLogger(getClass().getName());

    // Environment variables
    protected static final String GERRIT_URI = System.getProperty("GERRIT_URI");
    protected static final String GERRIT_SCOPE = System.getProperty("GERRIT_SCOPE");
    protected static final String CLIENT_ID = System.getProperty("CLIENT_ID");
    protected static final String ANALYTICS_ID = System.getProperty("ANALYTICS_ID");

    protected static final String TREE_DEFAULT_PARAM = "treeDefault";

    public enum PageType {
        TOT("Test", "/"),
        RELEASE("Release", "/show_release"),
        COVERAGE_OVERVIEW("Coverage", "/show_coverage_overview"),
        PROFILING_LIST("Profiling", "/show_profiling_list"),
        TABLE("", "/show_table"),
        TREE("", "/show_tree"),
        GRAPH("Profiling", "/show_graph"),
        COVERAGE("Coverage", "/show_coverage"),
        PERFORMANCE_DIGEST("Performance Digest", "/show_performance_digest"),
        PLAN_RELEASE("", "/show_plan_release"),
        PLAN_RUN("Plan Run", "/show_plan_run"),
        PROFILING_OVERVIEW("", "/show_profiling_overview");

        public final String defaultName;
        public final String defaultUrl;

        PageType(String defaultName, String defaultUrl) {
            this.defaultName = defaultName;
            this.defaultUrl = defaultUrl;
        }
    }

    public static class Page {
        private final PageType type;
        private final String name;
        private final String url;

        public Page(PageType type) {
            this.type = type;
            this.name = type.defaultName;
            this.url = type.defaultUrl;
        }

        public Page(PageType type, String name, String url) {
            this.type = type;
            this.name = type.defaultName + name;
            this.url = type.defaultUrl + url;
        }

        public Page(PageType type, String url) {
            this.type = type;
            this.name = type.defaultName;
            this.url = type.defaultUrl + url;
        }

        public String getName() {
            return name;
        }

        public String getUrl() {
            return url;
        }
    }

    public static final List<Page> navbarLinks;

    static {
        List<Page> links = new ArrayList<>();
        links.add(new Page(PageType.TOT));
        links.add(new Page(PageType.RELEASE));
        links.add(new Page(PageType.COVERAGE_OVERVIEW));
        links.add(new Page(PageType.PROFILING_LIST));
        navbarLinks = links;
    }

    public abstract PageType getNavParentType();

    /**
     * Get a list of URL/Display name pairs for the breadcrumb hierarchy.
     *
     * @param request The HttpServletRequest object for the page request.
     * @return a list of Page entries.
     */
    public abstract List<Page> getBreadcrumbLinks(HttpServletRequest request);

    @Override
    public void doGet(HttpServletRequest request, HttpServletResponse response) throws IOException {
        // If the user is logged out, allow them to log back in and return to the page.
        // Set the logout URL to direct back to a login page that directs to the current request.
        UserService userService = UserServiceFactory.getUserService();
        Optional<User> currentUser = Optional.ofNullable(userService.getCurrentUser());
        String currentUserEmail =
                currentUser.isPresent()
                        ? currentUser.map(user -> user.getEmail().trim()).orElse("")
                        : "";
        String requestUri = request.getRequestURI();
        String requestArgs = request.getQueryString();
        String loginURI = userService.createLoginURL(requestUri + '?' + requestArgs);
        String logoutURI = userService.createLogoutURL(loginURI);
        if (currentUserEmail != "") {

            int activeIndex;
            switch (getNavParentType()) {
                case PROFILING_LIST:
                    activeIndex = 3;
                    break;
                case COVERAGE_OVERVIEW:
                    activeIndex = 2;
                    break;
                case RELEASE:
                    activeIndex = 1;
                    break;
                default:
                    activeIndex = 0;
                    break;
            }
            if (request.getParameter(TREE_DEFAULT_PARAM) != null) {
                HttpSession session = request.getSession(true);
                boolean treeDefault = request.getParameter(TREE_DEFAULT_PARAM).equals("true");
                session.setAttribute(TREE_DEFAULT_PARAM, treeDefault);
            }

            request.setAttribute("serverName", request.getServerName());
            request.setAttribute("logoutURL", logoutURI);
            request.setAttribute("email", currentUserEmail);
            request.setAttribute("analyticsID", new Gson().toJson(ANALYTICS_ID));
            request.setAttribute("breadcrumbLinks", getBreadcrumbLinks(request));
            request.setAttribute("navbarLinks", navbarLinks);
            request.setAttribute("activeIndex", activeIndex);
            response.setContentType("text/html");

            if (currentUserEmail.endsWith("@google.com")) {
                doGetHandler(request, response);
            } else {
                RequestDispatcher dispatcher =
                        request.getRequestDispatcher("WEB-INF/jsp/auth_error.jsp");
                try {
                    dispatcher.forward(request, response);
                } catch (ServletException e) {
                    logger.log(Level.SEVERE, "Servlet Exception caught : ", e);
                }
            }
        } else {
            response.sendRedirect(loginURI);
        }
    }

    /**
     * Implementation of the doGet method to be executed by servlet subclasses.
     *
     * @param request The HttpServletRequest object.
     * @param response The HttpServletResponse object.
     * @throws IOException
     */
    public abstract void doGetHandler(HttpServletRequest request, HttpServletResponse response)
            throws IOException;
}
