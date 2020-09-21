<%--
  ~ Copyright (c) 2018 Google Inc. All Rights Reserved.
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
        <h3>Directory List</h3>
        <c:forEach varStatus="dirLoop" var="dirName" items="${dirList}">
          <p>
            <a href="${requestScope['javax.servlet.forward.servlet_path']}?path=${dirName}">
              <c:out value="${dirName}"></c:out>
                <c:if test="${dirLoop.first && path ne '/'}">
                    (Move to Parent)
                </c:if>
            </a>
          </p>
          <c:if test="${!dirLoop.last}">
          </c:if>
        </c:forEach>
        <hr/>
        <h5>Current Directory Path : ${path}</h5>
        <hr/>
        <h3>File List</h3>
        <c:forEach varStatus="fileLoop" var="fileName" items="${fileList}">
          <p>
            <a href="${requestScope['javax.servlet.forward.servlet_path']}?path=${fileName}">
              <c:out value="${fileName}"></c:out>
            </a>
          </p>
          <c:if test="${!fileLoop.last}">
          </c:if>
        </c:forEach>
      </div>
    </div>
    <%@ include file="footer.jsp" %>
  </body>
</html>
