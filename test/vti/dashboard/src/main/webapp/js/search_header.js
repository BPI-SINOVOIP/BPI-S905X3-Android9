/**
 * Copyright (c) 2017 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License. You may
 * obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

(function ($) {
  var _inequalityRegex = '(^)(<|>|<=|>=|=)?[ ]*?[0-9]+$';
  var _inequalityHint = 'e.g. 5, >0, <=10';

  function _validate(input, valueSet) {
    var value = input.val();
    if (valueSet.has(value) || !value) {
      input.removeClass('invalid');
    } else {
      input.addClass('invalid');
    }
  }

  function _createInput(key, config) {
    var value = config.value;
    var values = config.options.corpus;
    var displayName = config.displayName;
    var width = config.options.width || 's4';
    var div = $('<div class="input-field col"></div>');
    div.addClass(width);
    var input = $('<input class="filter-input"></input>');
    input.attr('type', config.options.type || 'text');
    input.appendTo(div);
    var label = $('<label></label>').text(displayName).appendTo(div);
    if (value) {
      input.attr('value', value);
      label.addClass('active');
    }
    if (config.options.validate == 'inequality') {
      input.addClass('validate');
      input.attr('pattern', _inequalityRegex);
      input.attr('placeholder', _inequalityHint);
      label.addClass('active');
    }
    input.focusout(function() {
      config.value = input.val();
    });
    if (values && values.length > 0) {
      var valueSet = new Set(values);
      input.sizedAutocomplete({
        source: values,
        classes: {
          'ui-autocomplete': 'card search-bar-menu'
        }
      });
      input.focusout(function() {
        _validate(input, valueSet);
      });
    }
    if (values && values.length > 0 && value) {
      _validate(input, valueSet);
    }
    return div;
  }

  function _verifyCheckboxes(checkboxes, refreshObject) {
    var oneChecked = checkboxes.presubmit || checkboxes.postsubmit;
    if (!oneChecked) {
      refreshObject.addClass('disabled');
    } else {
      refreshObject.removeClass('disabled');
    }
  }

  function _createRunTypeBoxes(checkboxes, refreshObject) {
    var container = $('<div class="run-type-wrapper col s12"></div>');
    var presubmit = $('<input type="checkbox" id="presubmit"></input>');
    presubmit.appendTo(container);
    if (checkboxes.presubmit) {
      presubmit.prop('checked', true);
    }
    container.append('<label for="presubmit">Presubmit</label>');
    var postsubmit = $('<input type="checkbox" id="postsubmit"></input>');
    postsubmit.appendTo(container);
    if (checkboxes.postsubmit) {
      postsubmit.prop('checked', true);
    }
    container.append('<label for="postsubmit">Postsubmit</label>');
    presubmit.change(function() {
      checkboxes.presubmit = presubmit.prop('checked');
      _verifyCheckboxes(checkboxes, refreshObject);
    });
    postsubmit.change(function() {
      checkboxes.postsubmit = postsubmit.prop('checked');
      _verifyCheckboxes(checkboxes, refreshObject);
    });
    return container;
  }

  function _expand(
      container, filters, checkboxes, onRefreshCallback, animate=true) {
    var wrapper = $('<div class="search-wrapper"></div>');
    var col = $('<div class="col s9"></div>');
    col.appendTo(wrapper);
    Object.keys(filters).forEach(function(key) {
      col.append(_createInput(key, filters[key]));
    });
    var refreshCol = $('<div class="col s3 refresh-wrapper"></div>');
    var refresh = $('<a class="btn-floating btn-medium red right waves-effect waves-light"></a>')
      .append($('<i class="medium material-icons">cached</i>'))
      .appendTo(refreshCol);
    refresh.click(onRefreshCallback);
    refreshCol.appendTo(wrapper);
    if (Object.keys(checkboxes).length > 0) {
      col.append(_createRunTypeBoxes(checkboxes, refresh));
    }
    if (animate) {
      wrapper.hide().appendTo(container).slideDown({
        duration: 200,
        easing: "easeOutQuart",
        queue: false
      });
    } else {
      wrapper.appendTo(container);
    }
    container.addClass('expanded')
  }

  function _renderHeader(
      container, label, value, filters, checkboxes, expand, onRefreshCallback) {
    var div = $('<div class="row card search-bar"></div>');
    var wrapper = $('<div class="header-wrapper"></div>');
    var header = $('<h5 class="section-header"></h5>');
    $('<b></b>').text(label).appendTo(header);
    $('<span></span>').text(value).appendTo(header);
    header.appendTo(wrapper);
    var iconWrapper = $('<div class="search-icon-wrapper"></div>');
    $('<i class="material-icons">search</i>').appendTo(iconWrapper);
    iconWrapper.appendTo(wrapper);
    wrapper.appendTo(div);
    if (expand) {
      _expand(div, filters, checkboxes, onRefreshCallback, false);
    } else {
      var expanded = false;
      iconWrapper.click(function() {
        if (expanded) return;
        expanded = true;
        _expand(div, filters, checkboxes, onRefreshCallback);
      });
    }
    div.appendTo(container);
  }

  function _addFilter(filters, displayName, keyName, options, defaultValue) {
    filters[keyName] = {};
    filters[keyName].displayName = displayName;
    filters[keyName].value = defaultValue;
    filters[keyName].options = options;
  }

  function _getOptionString(filters, checkboxes) {
    var args = Object.keys(filters).reduce(function(acc, key) {
      if (filters[key].value) {
        return acc + '&' + key + '=' + encodeURIComponent(filters[key].value);
      }
      return acc;
    }, '');
    if (checkboxes.presubmit != undefined && checkboxes.presubmit) {
      args += '&showPresubmit='
    }
    if (checkboxes.postsubmit != undefined && checkboxes.postsubmit) {
      args += '&showPostsubmit='
    }
    return args;
  }

  /**
   * Create a search header element.
   * @param label The header label.
   * @param value The value to display next to the label.
   * @param onRefreshCallback The function to call on refresh.
   */
  $.fn.createSearchHeader = function(label, value, onRefreshCallback) {
    var self = $(this);
    $.widget('custom.sizedAutocomplete', $.ui.autocomplete, {
      _resizeMenu : function() {
        this.menu.element.outerWidth($('.search-bar .filter-input').width());
      }
    });
    var filters = {};
    var checkboxes = {};
    var expandOnRender = false;
    var displayed = false;
    return {
      /**
       * Add a filter to the display.
       * @param displayName The input placeholder/label text.
       * @param keyName The URL key to use for the filter options.
       * @param options A dict of additional options (e.g. width, type).
       * @param defaultValue A default filter value.
       */
      addFilter : function(displayName, keyName, options, defaultValue) {
        if (displayed) return;
        _addFilter(filters, displayName, keyName, options, defaultValue);
        if (defaultValue) expandOnRender = true;
      },
      /**
       * Enable run type checkboxes in the filter options.
       *
       * This will display two checkboxes for selecting pre-/postsubmit runs.
       * @param showPresubmit True if presubmit runs are selected.
       * @param showPostsubmit True if postsubmit runs are selected.
       *
       */
      addRunTypeCheckboxes: function(showPresubmit, showPostsubmit) {
        if (displayed) return;
        checkboxes['presubmit'] = showPresubmit;
        checkboxes['postsubmit'] = showPostsubmit;
        if (!showPostsubmit || showPresubmit) {
          expandOnRender = true;
        }
      },
      /**
       * Display the created search bar.
       *
       * This must be called after filters have been added. After displaying, no
       * modifications to the filter options will take effect.
       */
      display : function() {
        displayed = true;
        _renderHeader(
          self, label, value, filters, checkboxes, expandOnRender,
          onRefreshCallback);
      },
      /**
       * Get the URL arguments string for the current set of filters.
       * @returns a URI-encoded component with the search bar keys and values.
       */
      args : function () {
        return _getOptionString(filters, checkboxes);
      }
    }
  }

})(jQuery);
