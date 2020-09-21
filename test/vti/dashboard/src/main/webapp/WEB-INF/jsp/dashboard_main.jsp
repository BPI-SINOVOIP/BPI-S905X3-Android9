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
  <link rel='stylesheet' href='/css/dashboard_main.css'>
  <%@ include file='header.jsp' %>
  <body>
    <script>
        var allTests = ${allTestsJson};
        var testSet = new Set(allTests);
        var subscriptionMap = ${subscriptionMapJson};

        var addFavorite = function() {
            if ($(this).hasClass('disabled')) {
                return;
            }
            var test = $('#input-box').val();
            if (!testSet.has(test) || test in subscriptionMap) {
                return;
            }
            $('#add-button').addClass('disabled');
            $.post('/api/favorites', { testName: test}).then(function(data) {
                if (!data.key) {
                    return;
                }
                subscriptionMap[test] = data.key;
                var wrapper = $('<div></div>');
                var a = $('<a></a>')
                    .attr('href', '${resultsUrl}?testName=' + test);
                var div = $('<div class="col s11 card hoverable option"></div>');
                div.addClass('valign-wrapper waves-effect');
                div.appendTo(a);
                var span = $('<span class="entry valign"></span>').text(test);
                span.appendTo(div);
                a.appendTo(wrapper);

                var btnContainer = $('<div class="col s1 center btn-container"></div>');
                var silence = $('<a class="col s6 btn-flat notification-button active"></a>');
                silence.append('<i class="material-icons">notifications_active</i>');
                silence.attr('test', test);
                silence.attr('title', 'Disable notifications');
                silence.appendTo(btnContainer);
                silence.click(toggleNotifications);

                var clear = $('<a class="col s6 btn-flat remove-button"></a>');
                clear.append('<i class="material-icons">clear</i>');
                clear.attr('test', test);
                clear.attr('title', 'Remove favorite');
                clear.appendTo(btnContainer);
                clear.click(removeFavorite);

                btnContainer.appendTo(wrapper);

                wrapper.prependTo('#options').hide()
                                          .slideDown(150);
                $('#input-box').val(null);
                Materialize.updateTextFields();
            }).always(function() {
                $('#add-button').removeClass('disabled');
            });
        }

        var toggleNotifications = function() {
            var self = $(this);
            if (self.hasClass('disabled')) {
                return;
            }
            self.addClass('disabled');
            var test = self.attr('test');
            if (!(test in subscriptionMap)) {
                return;
            }
            var muteStatus = self.hasClass('active');
            var element = self;
            $.post('/api/favorites', { userFavoritesKey: subscriptionMap[test], muteNotifications: muteStatus}).then(function(data) {
                element = self.clone();
                if (element.hasClass('active')) {
                    element.find('i').text('notifications_off')
                    element.removeClass('active');
                    element.addClass('inactive');
                    element.attr('title', 'Enable notifications');
                } else {
                    element.find('i').text('notifications_active')
                    element.removeClass('inactive');
                    element.addClass('active');
                    element.attr('title', 'Disable notifications');
                }
                element.click(toggleNotifications);
                self.replaceWith(function() {
                    return element;
                });
            }).always(function() {
                element.removeClass('disabled');
            });
        }

        var removeFavorite = function() {
            var self = $(this);
            if (self.hasClass('disabled')) {
                return;
            }
            var test = self.attr('test');
            if (!(test in subscriptionMap)) {
                return;
            }
            self.addClass('disabled');
            $.ajax({
                url: '/api/favorites/' + subscriptionMap[test],
                type: 'DELETE'
            }).always(function() {
                self.removeClass('disabled');
            }).then(function() {
                delete subscriptionMap[test];
                self.parent().parent().slideUp(150, function() {
                    self.remove();
                });
            });
        }

        var addFavoriteButton = function() {
          var self = $(this);
          var test = self.attr('test');

          $.post('/api/favorites', { testName: test}).then(function(data) {
            if (data.key) {
              subscriptionMap[test] = data.key;

              self.children().text("star");
              self.switchClass("add-fav-button", "min-fav-button", 0);

              self.off('click', addFavoriteButton);
              self.on('click', removeFavoriteButton);
            }
          })
          .fail(function() {
            alert( "Error occurred on registering your favorite test case!" );
          });
        }

        var removeFavoriteButton = function() {
          var self = $(this);
          var test = self.attr('test');

          $.ajax({
            url: '/api/favorites/' + subscriptionMap[test],
            type: 'DELETE'
          }).then(function() {
            delete subscriptionMap[test];

            self.children().text("star_border");
            self.switchClass("min-fav-button", "add-fav-button", 0);

            self.off('click', removeFavoriteButton);
            self.on('click', addFavoriteButton);
          });
        }

        $.widget('custom.sizedAutocomplete', $.ui.autocomplete, {
            _resizeMenu: function() {
                this.menu.element.outerWidth($('#input-box').width());
            }
        });

        $(function() {
            $('#input-box').sizedAutocomplete({
                source: allTests,
                classes: {
                    'ui-autocomplete': 'card'
                }
            });

            $('#input-box').keyup(function(event) {
                if (event.keyCode == 13) {  // return button
                    $('#add-button').click();
                }
            });

            $('.remove-button').click(removeFavorite);
            $('.notification-button').click(toggleNotifications);
            $('#add-button').click(addFavorite);

            $('.add-fav-button').click(addFavoriteButton);

            $('.min-fav-button').click(removeFavoriteButton);

            $('#favoritesLink').click(function() {
                window.open('/', '_self');
            });
            $('#allLink').click(function() {
                window.open('/?showAll=true', '_self');
            });
            $('#acksLink').click(function() {
                window.open('/show_test_acknowledgments', '_self');
            });
        });
    </script>
    <div class='container wide'>

      <c:if test="${!showAll}">
        <ul id="guide_collapsible" class="collapsible" data-collapsible="accordion">
          <li>
            <div class="collapsible-header">
              <i class="material-icons">library_books</i>
              Notice
              <span class="new badge right" style="position: inherit;">1</span>
            </div>
            <div class="collapsible-body">
              <div class='row'>
                <div class='col s12' style="margin: 15px 0px 0px 30px;">
                  <c:choose>
                    <c:when test="${fn:endsWith(serverName, 'googleplex.com')}">
                      <c:set var="dataVersion" scope="page" value="new"/>
                      <c:choose>
                        <c:when test="${fn:startsWith(serverName, 'android-vts-internal')}">
                          <c:set var="dataLink" scope="page" value="https://android-vts.appspot.com"/>
                        </c:when>
                        <c:when test="${fn:startsWith(serverName, 'android-vts-staging')}">
                          <c:set var="dataLink" scope="page" value="https://android-vts-staging.appspot.com"/>
                        </c:when>
                        <c:otherwise>
                          <c:set var="dataLink" scope="page" value="https://android-vts-staging.appspot.com"/>
                        </c:otherwise>
                      </c:choose>
                    </c:when>
                    <c:when test="${fn:endsWith(serverName, 'appspot.com')}">
                      <c:set var="dataVersion" scope="page" value="previous"/>
                      <c:choose>
                        <c:when test="${fn:startsWith(serverName, 'android-vts-staging')}">
                          <c:set var="dataLink" scope="page" value="https://android-vts-staging.googleplex.com"/>
                        </c:when>
                        <c:when test="${fn:startsWith(serverName, 'android-vts')}">
                          <c:set var="dataLink" scope="page" value="https://android-vts-internal.googleplex.com"/>
                        </c:when>
                        <c:otherwise>
                          <c:set var="dataLink" scope="page" value="https://android-vts-staging.googleplex.com"/>
                        </c:otherwise>
                      </c:choose>
                    </c:when>
                    <c:otherwise>
                      <c:set var="dataVersion" scope="page" value="local dev"/>
                      <c:set var="dataLink" scope="page" value="http://localhost"/>
                    </c:otherwise>
                  </c:choose>
                  Recently, we launched new appspot servers for dashboard. Thus you will have two diffrent versions of server for each staging and production data.
                  <br/>
                  If you want to find the <c:out value = "${dataVersion}"/> test data, please visit the next url <a href="<c:out value = "${dataLink}"/>"><c:out value = "${dataLink}"/></a>.
                </div>
              </div>
            </div>
          </li>
        </ul>
      </c:if>

      <c:choose>
        <c:when test='${not empty error}'>
          <div id='error-container' class='row card'>
            <div class='col s12 center-align'>
              <h5>${error}</h5>
            </div>
          </div>
        </c:when>
        <c:otherwise>
          <div class='row home-tabs-row'>
            <div class='col s12'>
              <ul class='tabs'>
                <li class='tab col s4' id='favoritesLink'><a class='${showAll ? "inactive" : "active"}'>Favorites</a></li>
                <li class='tab col s4' id='allLink'><a class='${showAll ? "active" : "inactive"}'>All Tests</a></li>
                <li class='tab col s4' id='acksLink'><a>Test Acknowledgements</a></li>
              </ul>
            </div>
          </div>
          <c:set var='width' value='${showAll ? 11 : 11}' />
          <c:if test='${not showAll}'>
            <div class='row'>
              <div class='input-field col s8'>
                <input type='text' id='input-box'></input>
                <label for='input-box'>Search for tests to add to favorites</label>
              </div>
              <div id='add-button-wrapper' class='col s1 valign-wrapper'>
                <a id='add-button' class='btn-floating btn waves-effect waves-light red valign'><i class='material-icons'>add</i></a>
              </div>
            </div>
          </c:if>
          <div class='row' id='options'>
            <c:forEach items='${testNames}' var='test'>
              <div>
                <a href='${resultsUrl}?testName=${test.name}'>
                  <div class='col s${width} card hoverable option valign-wrapper waves-effect'>
                    <span class='entry valign'>${test.name}
                      <c:if test='${test.failCount >= 0 && test.passCount >= 0}'>
                        <c:set var='color' value='${test.failCount > 0 ? "red" : (test.passCount > 0 ? "green" : "grey")}' />
                        <span class='indicator center ${color}'>
                          ${test.passCount} / ${test.passCount + test.failCount}
                        </span>
                      </c:if>
                    </span>
                  </div>
                </a>
                <c:choose>
                  <c:when test="${showAll}">
                    <div class="col s1 center btn-container">
                      <c:choose>
                        <c:when test="${test.isFavorite}">
                          <a class="col s6 btn-flat min-fav-button" test="${test.name}" title="Remove favorite">
                            <i class="material-icons">star</i>
                          </a>
                        </c:when>
                        <c:otherwise>
                          <a class="col s6 btn-flat add-fav-button" test="${test.name}" title="Add favorite">
                            <i class="material-icons">star_border</i>
                          </a>
                        </c:otherwise>
                      </c:choose>
                    </div>
                  </c:when>
                  <c:otherwise>
                    <div class='col s1 center btn-container'>
                      <a class='col s6 btn-flat notification-button ${test.muteNotifications ? "inactive" : "active"}' test='${test.name}' title='${test.muteNotifications ? "Enable" : "Disable"} notifications'>
                        <i class='material-icons'>notifications_${test.muteNotifications ? "off" : "active"}</i>
                      </a>
                      <a class='col s6 btn-flat remove-button' test='${test.name}' title='Remove favorite'>
                        <i class='material-icons'>clear</i>
                      </a>
                    </div>
                  </c:otherwise>
                </c:choose>
              </div>
            </c:forEach>
          </div>
        </c:otherwise>
      </c:choose>
    </div>
    <%@ include file='footer.jsp' %>
  </body>
</html>
