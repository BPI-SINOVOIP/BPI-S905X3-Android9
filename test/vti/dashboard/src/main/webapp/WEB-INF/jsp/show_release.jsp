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
  <link rel='stylesheet' href='/css/show_release.css'>
  <%@ include file='header.jsp' %>
  <script type='text/javascript' src='https://ajax.googleapis.com/ajax/libs/jqueryui/1.12.0/jquery-ui.min.js'></script>
  <body>
    <div class='container'>
      <div class='row'>
        <div class='col s12'>
          <h4 id='section-header'>Test Plans</h4>
        </div>
      </div>
      <div class='row' id='options'>
        <c:forEach items='${planNames}' var='plan'>
          <c:choose>
            <c:when test="${isAdmin}">
              <div class="col s11 center">
                <a href='/show_plan_release?plan=${plan}'>
                  <div class='col s12 card hoverable option valign-wrapper waves-effect'>
                    <span class='entry valign'>${plan}</span>
                  </div>
                </a>
              </div>
              <div class="col s1 center btn-container" style="margin-top: 9px;">
                <a href='/show_green_release?plan=${plan}' class="waves-effect waves-light btn">Green</a>
              </div>
            </c:when>
            <c:otherwise>
              <div class="col s12 center">
                <a href='/show_plan_release?plan=${plan}'>
                  <div class='col s12 card hoverable option valign-wrapper waves-effect'>
                    <span class='entry valign'>${plan}</span>
                  </div>
                </a>
              </div>
            </c:otherwise>
          </c:choose>
        </c:forEach>
      </div>
    </div>
    <%@ include file='footer.jsp' %>
  </body>
</html>
