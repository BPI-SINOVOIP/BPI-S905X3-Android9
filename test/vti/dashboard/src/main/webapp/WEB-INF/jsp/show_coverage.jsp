<%--
  ~ Copyright (c) 2016 Google Inc. All Rights Reserved.
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
  <link rel="stylesheet" href="/css/show_coverage.css">
  <script src="https://apis.google.com/js/api.js" type="text/javascript"></script>
  <body>
    <script type="text/javascript">
        var coverageVectors = ${coverageVectors};
        $(document).ready(function() {
            // Initialize AJAX for CORS
            $.ajaxSetup({
                xhrFields : {
                    withCredentials: true
                }
            });

            // Initialize auth2 client and scope for requests to Gerrit
            gapi.load('auth2', function() {
                var auth2 = gapi.auth2.init({
                    client_id: ${clientId},
                    scope: ${gerritScope}
                });
                auth2.then(displayEntries);
            });
        });

        /* Open a window to Gerrit so that user can login.
           Minimize the previously clicked entry.
        */
        var gerritLogin = function(element) {
            window.open(${gerritURI}, "Ratting", "toolbar=0,status=0");
            element.click();
        }

        /* Loads source code for a particular entry and displays it with
           coverage information as the accordion entry expands.
        */
        var onClick = function() {
            // Remove source code from the accordion entry that was open before
            var self = $(this);
            var prev = self.parent().siblings('li.active');
            if (prev.length > 0) {
                prev.find('.table-container').empty();
            }
            var url = self.parent().attr('url');
            var i = self.parent().attr('index');
            var container = self.parent().find('.table-container');
            container.html('<div class="center-align">Loading...</div>');
            if (self.parent().hasClass('active')) {
                // Remove the code from display
                container.empty();
            } else {
                /* Fetch and display the code.
                   Note: a coverageVector may be shorter than sourceContents due
                   to non-executable (i.e. comments or language-specific syntax)
                   lines in the code. Trailing source lines that have no
                   coverage information are assumed to be non-executable.
                */
                $.ajax({
                    url: url,
                    dataType: 'text'
                }).promise().done(function(src) {
                    src = atob(src);
                    if (!src) return;
                    srcLines = src.split('\n');
                    covered = 0;
                    total = 0;
                    var table = $('<table class="table"></table>');
                    var rows = srcLines.forEach(function(line, j) {
                        var count = coverageVectors[i][j];
                        var row = $('<tr></tr>');
                        if (typeof count == 'undefined' || count < 0) {
                            count = "--";
                        } else if (count == 0) {
                            row.addClass('uncovered');
                            total += 1;
                        } else {
                            row.addClass('covered');
                            total += 1;
                        }
                        row.append('<td class="count">' + String(count) + '</td>');
                        row.append('<td class="line_no">' + String(j+1) + '</td>');
                        code = $('<td class="code"></td>');
                        code.text(String(line));
                        code.appendTo(row);
                        row.appendTo(table);
                    });
                    container.empty();
                    container.append(table);
                }).fail(function(error) {
                    if (error.status == 0) {  // origin error, refresh cookie
                        container.empty();
                        container.html('<div class="center-align">' +
                                       '<span class="login-button">' +
                                       'Click to authorize Gerrit access' +
                                       '</span></div>');
                        container.find('.login-button').click(function() {
                            gerritLogin(self);
                        });
                    } else {
                        container.html('<div class="center-align">' +
                                       'Not found.</div>');
                    }
                });
            }
        }

        /* Appends a row to the display with test name and aggregated coverage
           information. On expansion, source code is loaded with coverage
           highlighted by calling 'onClick'.
        */
        var displayEntries = function() {
            var sourceFilenames = ${sourceFiles};
            var sectionMap = ${sectionMap};
            var gerritURI = ${gerritURI};
            var projects = ${projects};
            var commits = ${commits};
            var indicators = ${indicators};
            Object.keys(sectionMap).forEach(function(section) {
                var indices = sectionMap[section];
                var html = String();
                indices.forEach(function(i) {
                    var url = gerritURI + '/projects/' +
                              encodeURIComponent(projects[i]) + '/commits/' +
                              encodeURIComponent(commits[i]) + '/files/' +
                              encodeURIComponent(sourceFilenames[i]) +
                              '/content';
                    html += '<li url="' + url + '" index="' + i + '">' +
                            '<div class="collapsible-header">' +
                            '<i class="material-icons">library_books</i>' +
                            '<b>' + projects[i] + '/</b>' +
                            sourceFilenames[i] + indicators[i] + '</div>';
                    html += '<div class="collapsible-body row">' +
                            '<div class="html-container">' +
                            '<div class="table-container"></div>' +
                            '</div></div></li>';
                });
                if (html) {
                    html = '<h4 class="section-title"><b>Coverage:</b> ' +
                           section + '</h4><ul class="collapsible popout" ' +
                           'data-collapsible="accordion">' + html + '</ul>';
                    $('#coverage-container').append(html);
                }
            });
            $('.collapsible.popout').collapsible({
               accordion : true
            }).find('.collapsible-header').click(onClick);
        }
    </script>
    <div id='coverage-container' class='wide container'>
    </div>
    <%@ include file="footer.jsp" %>
  </body>
</html>
