<%--
  ~ Copyright (c) 2017 Google Inc. All Rights Reserved.
  ~
  ~ Licensed under the Apache License, Version 2.0 (the "License"); you
  ~ may not use this file except in compliance with the License. You may
  ~ obtain a copy of the License at
  ~
  ~     http://www.apache.org/licenses/LICENSE-2.0
  ~
  ~ Unless required by applicable law or agreed to in writing, software
  ~ distributed under the License is distributed on an "AS IS" BASIS,
  ~ WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
  ~ implied. See the License for the specific language governing
  ~ permissions and limitations under the License.
  --%>
<%@ page contentType='text/html;charset=UTF-8' language='java' %>
<%@ taglib prefix='fn' uri='http://java.sun.com/jsp/jstl/functions' %>
<%@ taglib prefix='c' uri='http://java.sun.com/jsp/jstl/core'%>

<html>
  <%@ include file="header.jsp" %>
  <link rel='stylesheet' href='/css/show_plan_release.css'>
  <link rel='stylesheet' href='/css/plan_runs.css'>
  <link rel='stylesheet' href='/css/search_header.css'>
  <script src='https://www.gstatic.com/external_hosted/moment/min/moment-with-locales.min.js'></script>
  <script src='js/time.js'></script>
  <script src='js/plan_runs.js'></script>
  <script src='js/search_header.js'></script>
  <script type='text/javascript'>
      var search;
      $(document).ready(function() {

      });
  </script>

  <body>
    <div class='wide container'>
      <div class='row' id='release-container'>
        <table class="bordered highlight">
          <thead>
            <tr>
              <th>Branch</th>
              <th>Last Finished Build</th>
              <th>Last Green Build</th>
            </tr>
          </thead>

          <tbody>
          <c:forEach var="branchList" items="${greenBuildInfo}">
            <tr>
              <td> <c:out value="${branchList.key}"></c:out> </td>
              <td>
                <c:forEach varStatus="deviceLoop" var="deviceBuildInfo" items="${branchList.value}">
                  <p>
                    <c:out value="${deviceBuildInfo.deviceBuildTarget}"></c:out> :
                    <c:choose>
                      <c:when test="${deviceBuildInfo.candidateBuildId eq 'No Test Results'}">
                        <c:out value="${deviceBuildInfo.candidateBuildId}"></c:out>
                      </c:when>
                      <c:otherwise>
                        <a href="/show_plan_run?plan=${plan}&time=${deviceBuildInfo.candidateBuildIdTimestamp}">
                          <c:out value="${deviceBuildInfo.candidateBuildId}"></c:out>
                        </a>
                      </c:otherwise>
                    </c:choose>
                  </p>
                  <c:if test="${!deviceLoop.last}">
                    <hr/>
                  </c:if>
                </c:forEach>
              </td>
              <td>
                <c:forEach varStatus="deviceLoop" var="deviceBuildInfo" items="${branchList.value}">
                  <p>
                    <c:choose>
                      <c:when test="${deviceBuildInfo.greenBuildId eq 'N/A'}">
                        <c:out value="${deviceBuildInfo.greenBuildId}"></c:out>
                      </c:when>
                      <c:otherwise>
                        <a href="/show_plan_run?plan=${plan}&time=${deviceBuildInfo.greenBuildIdTimestamp}">
                          <c:out value="${deviceBuildInfo.greenBuildId}"></c:out>
                        </a>
                      </c:otherwise>
                    </c:choose>
                  </p>
                  <c:if test="${!deviceLoop.last}">
                    <hr/>
                  </c:if>
                </c:forEach>
              </td>
            </tr>
          </c:forEach>
          </tbody>
        </table>
      </div>
    </div>
    <%@ include file="footer.jsp" %>
  </body>
</html>
