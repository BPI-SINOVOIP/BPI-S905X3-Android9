/**
 * Copyright (c) 2017 The Android Open Source Project
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

(function($) {

  var _isModalOpen = false;
  var _isReadOnly = true;
  var _allTestsSet = new Set();
  var _allBranches = [];
  var _allDevices = [];

  var _writableSummary = 'Known test failures are acknowledged below for specific branch and \
    device configurations, and corresponding test breakage alerts will be silenced. Click an \
    entry to edit or see more information about the test failure.'
  var _readOnlySummary = 'Known test failures are acknowledged below for specific branch and \
    device configurations, and corresponding test breakage alerts will be silenced. Click an \
    entry to see  more information about the test failure. To add, edit, or remove a test \
    acknowledgment, contact a VTS Dashboard administrator.'

  $.widget('custom.sizedAutocomplete', $.ui.autocomplete, {
    options: {
      parent: ''
    },
    _resizeMenu: function() {
      this.menu.element.outerWidth($(this.options.parent).width());
    }
  });

  /**
   * Remove an acknowledgment from the list.
   * @param ack (jQuery object) The object for acknowledgment.
   * @param key (String) The value to display next to the label.
   */
  function removeAcknowledgment(ack, key) {
    if (ack.hasClass('disabled')) {
      return;
    }
    ack.addClass('disabled');
    $.ajax({
      url: '/api/test_acknowledgments/' + key,
      type: 'DELETE'
    }).always(function() {
      ack.removeClass('disabled');
    }).then(function() {
      ack.slideUp(150, function() {
        ack.remove();
      });
    });
  }

  /**
   * Callback for when a chip is removed from a chiplist.
   * @param text (String) The value stored in the chip.
   * @param allChipsSet (Set) The set of all chip values.
   * @param allIndicator (jQuery object) The object for "All" indicator adjacent to the chips.
   */
  function chipRemoveCallback(text, allChipsSet, allIndicator) {
    allChipsSet.delete(text);
    if (allChipsSet.size == 0) {
      allIndicator.show();
    }
  }

  /**
   * Add chips to the chip UI.
   * @param allChipsSet (Set) The set of all chip values.
   * @param container (jQuery object) The object in which to insert the chips.
   * @param chipList (list) The list of chip values to insert.
   * @param allIndicator (jQuery object) The object for "All" indicator adjacent to the chips.
   */
  function addChips(allChipsSet, container, chipList, allIndicator) {
    if (chipList && chipList.length > 0) {
      chipList.forEach(function(text) {
        if (allChipsSet.has(text)) return;
        var chip = $('<span class="chip">' + text + '</span>');
        if (!_isReadOnly) {
          var icon = $('<i class="material-icons">clear</i>').appendTo(chip);
          icon.click(function() {
            chipRemoveCallback(text, allChipsSet, allIndicator);
          });
        }
        chip.appendTo(container);
        allChipsSet.add(text);
      });
      allIndicator.hide();
    }
  }

  /**
   * Create a chip input UI.
   * @param container (jQuery object) The object in which to insert the input box.
   * @param placeholder (String) The placeholder text to display in the input.
   * @param allChipsSet (Set) The set of all chip values.
   * @param chipContainer (jQuery object) The object in which to insert new chips from the input.
   * @param allIndicator (jQuery object) The object for "All" indicator adjacent to the chips.
   * @returns The chip input jQuery object.
   */
  function addChipInput(container, placeholder, allChipsSet, chipContainer, allIndicator) {
    var input = $('<input type="text"></input>');
    input.attr('placeholder', placeholder);
    input.keyup(function(e) {
      if (e.keyCode === 13 && input.val().trim()) {
        addChips(allChipsSet, chipContainer, [input.val()], allIndicator);
        input.val('');
      }
    });
    var addButton = $('<i class="material-icons add-button">add</i>');
    addButton.click(function() {
      if (input.val().trim()) {
        addChips(allChipsSet, chipContainer, [input.val()], allIndicator);
        input.val('');
        addButton.hide();
      }
    });
    addButton.hide();
    input.focus(function() {
      addButton.show();
    });
    input.focusout(function() {
      if (!input.val().trim()) {
        addButton.hide();
      }
    });
    var holder = $('<div class="col s12 input-container"></div>').appendTo(container);
    input.appendTo(holder);
    addButton.appendTo(holder);
    return input;
  }

  /**
   * Callback to save changes to the acknowledgment.
   * @param ack (jQuery object) The object for acknowledgment.
   * @param modal (jQuery object) The jQueryUI modal object which invoked the callback.
   * @param key (String) The key associated with the acknowledgment.
   * @param test (String) The test name in the acknowledgment.
   * @param branchSet (Set) The set of all branches in the acknowledgment.
   * @param deviceSet (Set) The set of all devoces in the acknowledgment.
   * @param testCaseSet (Set) The set of all test cases in the acknowledgment.
   * @param note (String) The note in the acknowledgment.
   */
  function saveCallback(ack, modal, key, test, branchSet, deviceSet, testCaseSet, note) {
    var allEmpty = true;
    var firstUnemptyInput = null;
    var vals = modal.find('.modal-section>.input-container>input').each(function(_, input) {
      if (!!$(input).val()) {
        allEmpty = false;
        if (!firstUnemptyInput) firstUnemptyInput = $(input);
      }
    });
    if (!allEmpty) {
      firstUnemptyInput.focus();
      return false;
    }
    var branches = Array.from(branchSet);
    branches.sort();
    var devices = Array.from(deviceSet);
    devices.sort();
    var testCaseNames = Array.from(testCaseSet);
    testCaseNames.sort();
    var data = {
      'key' : key,
      'testName' : test,
      'branches' : branches,
      'devices' : devices,
      'testCaseNames' : testCaseNames,
      'note': note
    };
    $.post('/api/test_acknowledgments', JSON.stringify(data)).done(function(newKey) {
      var newAck = createAcknowledgment(newKey, test, branches, devices, testCaseNames, note);
      if (key == null) {
        ack.replaceWith(newAck.hide());
        newAck.slideDown(150);
      } else {
        ack.replaceWith(newAck);
      }
    }).always(function() {
      modal.modal({
        complete: function() { _isModalOpen = false; }
      });
      modal.modal('close');
    });
  }

  /**
   * Callback to save changes to the acknowledgment.
   * @param ack (jQuery object) The object for the acknowledgment.
   * @param key (String) The key associated with the acknowledgment.
   * @param test (String) The test name in the acknowledgment.
   * @param branches (list) The list of all branches in the acknowledgment.
   * @param devices (Set) The list of all devoces in the acknowledgment.
   * @param testCases (Set) The list of all test cases in the acknowledgment.
   * @param note (String) The note in the acknowledgment.
   */
  function showModal(ack, key, test, branches, devices, testCases, note) {
    if (_isModalOpen) {
      return;
    }
    _isModalOpen = true;
    var wrapper = $('#modal');
    wrapper.empty();
    wrapper.modal();
    var content = $('<div class="modal-content"><h4>Test Acknowledgment</h4></div>');
    var row = $('<div class="row"></div>').appendTo(content);
    row.append('<div class="col s12"><h5><b>Test: </b>' + test + '</h5></div>');

    var branchSet = new Set();
    var branchContainer = $('<div class="col l4 s12 modal-section"></div>').appendTo(row);
    var branchHeader = $('<h5></h5>').appendTo(branchContainer);
    branchHeader.append('<b>Branches:</b>');
    var allBranchesLabel = $('<span> All</span>').appendTo(branchHeader);
    var branchChips = $('<div class="col s12 chips branch-chips"></div>').appendTo(branchContainer);
    addChips(branchSet, branchChips, branches, allBranchesLabel);
    if (!_isReadOnly) {
      var branchInput = addChipInput(
        branchContainer, 'Specify a branch...', branchSet, branchChips, allBranchesLabel);
      branchInput.sizedAutocomplete({
        source: _allBranches,
        classes: {
          'ui-autocomplete': 'card autocomplete-dropdown'
        },
        parent: branchInput
      });
    }

    var deviceSet = new Set();
    var deviceContainer = $('<div class="col l4 s12 modal-section"></div>').appendTo(row);
    var deviceHeader = $('<h5></h5>').appendTo(deviceContainer);
    deviceHeader.append('<b>Devices:</b>');
    var allDevicesLabel = $('<span> All</span>').appendTo(deviceHeader);
    var deviceChips = $('<div class="col s12 chips device-chips"></div>').appendTo(deviceContainer);
    addChips(deviceSet, deviceChips, devices, allDevicesLabel);
    if (!_isReadOnly) {
      var deviceInput = addChipInput(
        deviceContainer, 'Specify a device...', deviceSet, deviceChips, allDevicesLabel);
      deviceInput.sizedAutocomplete({
        source: _allDevices,
        classes: {
          'ui-autocomplete': 'card autocomplete-dropdown'
        },
        parent: deviceInput
      });
    }

    var testCaseSet = new Set();
    var testCaseContainer = $('<div class="col l4 s12 modal-section"></div>').appendTo(row);
    var testCaseHeader = $('<h5></h5>').appendTo(testCaseContainer);
    testCaseHeader.append('<b>Test Cases:</b>');
    var allTestCasesLabel = $('<span> All</span>').appendTo(testCaseHeader);
    var testCaseChips = $('<div class="col s12 chips test-case-chips"></div>').appendTo(
      testCaseContainer);
    addChips(testCaseSet, testCaseChips, testCases, allTestCasesLabel);
    var testCaseInput = null;
    if (!_isReadOnly) {
      testCaseInput = addChipInput(
        testCaseContainer, 'Specify a test case...', testCaseSet, testCaseChips, allTestCasesLabel);
    }

    row.append('<div class="col s12"><h5><b>Note:</b></h5></div>');
    var inputField = $('<div class="input-field col s12"></div>').appendTo(row);
    var textArea = $('<textarea placeholder="Type a note..."></textarea>');
    textArea.addClass('materialize-textarea note-field');
    textArea.appendTo(inputField);
    textArea.val(note);
    if (_isReadOnly) {
      textArea.attr('disabled', true);
    }

    content.appendTo(wrapper);
    var footer = $('<div class="modal-footer"></div>');
    if (!_isReadOnly) {
      var save = $('<a class="btn">Save</a></div>').appendTo(footer);
      save.click(function() {
        saveCallback(ack, wrapper, key, test, branchSet, deviceSet, testCaseSet, textArea.val());
      });
    }
    var close = $('<a class="btn-flat">Close</a></div>').appendTo(footer);
    close.click(function() {
      wrapper.modal({
        complete: function() { _isModalOpen = false; }
      });
      wrapper.modal('close');
    })
    footer.appendTo(wrapper);
    if (!_isReadOnly) {
      $.get('/api/test_run?test=' + test + '&timestamp=latest').done(function(data) {
        var allTestCases = data.reduce(function(array, column) {
          return array.concat(column.data);
        }, []);
        testCaseInput.sizedAutocomplete({
          source: allTestCases,
          classes: {
            'ui-autocomplete': 'card autocomplete-dropdown'
          },
          parent: testCaseInput
        });
      }).always(function() {
        wrapper.modal('open');
      });
    } else {
      wrapper.modal('open');
    }
  }

  /**
   * Create a test acknowledgment object.
   * @param key (String) The key associated with the acknowledgment.
   * @param test (String) The test name in the acknowledgment.
   * @param branches (list) The list of all branches in the acknowledgment.
   * @param devices (Set) The list of all devoces in the acknowledgment.
   * @param testCases (Set) The list of all test cases in the acknowledgment.
   * @param note (String) The note in the acknowledgment.
   */
  function createAcknowledgment(key, test, branches, devices, testCases, note) {
    var wrapper = $('<div class="col s12 ack-entry"></div>');
    var details = $('<div class="col card hoverable"></div>').appendTo(wrapper);
    details.addClass(_isReadOnly ? 's12' : 's11')
    var testDiv = $('<div class="col s12"><b>' + test + '</b></div>').appendTo(details);
    var infoBtn = $('<span class="info-icon right"></a>').appendTo(testDiv);
    infoBtn.append('<i class="material-icons">info_outline</i>');
    details.click(function() {
      showModal(wrapper, key, test, branches, devices, testCases, note);
    });
    var branchesSummary = 'All';
    if (!!branches && branches.length == 1) {
      branchesSummary = branches[0];
    } else if (!!branches && branches.length > 1) {
      branchesSummary = branches[0];
      branchesSummary += '<span class="count-indicator"> (+' + (branches.length - 1) + ')</span>';
    }
    $('<div class="col l4 s12"><b>Branches: </b>' + branchesSummary + '</div>').appendTo(details);
    var devicesSummary = 'All';
    if (!!devices && devices.length == 1) {
      devicesSummary = devices[0];
    } else if (!!devices && devices.length > 1) {
      devicesSummary = devices[0];
      devicesSummary += '<span class="count-indicator"> (+' + (devices.length - 1) + ')</span>';
    }
    $('<div class="col l4 s12"><b>Devices: </b>' + devicesSummary + '</div>').appendTo(details);
    var testCaseSummary = 'All';
    if (!!testCases && testCases.length == 1) {
      testCaseSummary = testCases[0];
    } else if (!!testCases && testCases.length > 1) {
      testCaseSummary = testCases[0];
      testCaseSummary += '<span class="count-indicator"> (+' + (testCases.length - 1) + ')</span>';
    }
    details.append('<div class="col l4  s12"><b>Test Cases: </b>' + testCaseSummary + '</div>');

    if (!_isReadOnly) {
      var btnContainer = $('<div class="col s1 center btn-container"></div>');

      var clear = $('<a class="col s12 btn-flat remove-button"></a>');
      clear.append('<i class="material-icons">clear</i>');
      clear.attr('title', 'Remove');
      clear.click(function() { removeAcknowledgment(wrapper, key); });
      clear.appendTo(btnContainer);

      btnContainer.appendTo(wrapper);
    }
    return wrapper;
  }

  /**
   * Create a test acknowledgments UI.
   * @param allTests (list) The list of all test names.
   * @param allBranches (list) The list of all branches.
   * @param allDevices (list) The list of all device names.
   * @param testAcknowledgments (list) JSON-serialized TestAcknowledgmentEntity object list.
   * @param readOnly (boolean) True if the acknowledgments are read-only, false if mutable.
   */
  $.fn.testAcknowledgments = function(
      allTests, allBranches, allDevices, testAcknowledgments, readOnly) {
    var self = $(this);
    _allTestsSet = new Set(allTests);
    _allBranches = allBranches;
    _allDevices = allDevices;
    _isReadOnly = readOnly;
    var searchRow = $('<div class="search-row"></div>');
    var headerRow = $('<div></div>');
    var acks = $('<div class="acknowledgments"></div>');

    if (!_isReadOnly) {
      var inputWrapper = $('<div class="input-field col s8"></div>');
      var input = $('<input type="text"></input>').appendTo(inputWrapper);
      inputWrapper.append('<label>Search for tests to add an acknowledgment</label>');
      inputWrapper.appendTo(searchRow);
      input.sizedAutocomplete({
        source: allTests,
        classes: {
          'ui-autocomplete': 'card autocomplete-dropdown'
        },
        parent: input
      });

      var btnWrapper = $('<div class="col s1"></div>');
      var btn = $('<a class="btn waves-effect waves-light red btn-floating"></a>');
      btn.append('<i class="material-icons">add</a>');
      btn.appendTo(btnWrapper);
      btnWrapper.appendTo(searchRow);
      btn.click(function() {
        if (!_allTestsSet.has(input.val())) return;
        var ack = createAcknowledgment(undefined, input.val());
        ack.hide().prependTo(acks);
        showModal(ack, undefined, input.val());
      });
      searchRow.appendTo(self);
    }

    var headerCol = $('<div class="col s12 section-header-col"></div>').appendTo(headerRow);
    if (_isReadOnly) {
      headerCol.append('<p class="acknowledgment-info">' + _readOnlySummary + '</p>');
    } else {
      headerCol.append('<p class="acknowledgment-info">' + _writableSummary + '</p>');
    }
    headerRow.appendTo(self);

    testAcknowledgments.forEach(function(ack) {
      var wrapper = createAcknowledgment(
        ack.key, ack.testName, ack.branches, ack.devices, ack.testCaseNames, ack.note);
      wrapper.appendTo(acks);
    });
    acks.appendTo(self);

    self.append('<div class="modal modal-fixed-footer acknowledgments-modal" id="modal"></div>');
  };

})(jQuery);
