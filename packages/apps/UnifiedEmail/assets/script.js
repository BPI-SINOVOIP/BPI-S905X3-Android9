/*
 * Copyright (C) 2012 Google Inc.
 * Licensed to The Android Open Source Project.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

var BLOCKED_SRC_ATTR = "blocked-src";

// the set of Elements currently scheduled for processing in handleAllImageLoads
// this is an Array, but we treat it like a Set and only insert unique items
var gImageLoadElements = [];

var gScaleInfo;

/**
 * Only revert transforms that do an imperfect job of shrinking content if they fail
 * to shrink by this much. Expressed as a ratio of:
 * (original width difference : width difference after transforms);
 */
var TRANSFORM_MINIMUM_EFFECTIVE_RATIO = 0.7;

// Don't ship with this on.
var DEBUG_DISPLAY_TRANSFORMS = false;

var gTransformText = {};

/**
 * Returns the page offset of an element.
 *
 * @param {Element} element The element to return the page offset for.
 * @return {left: number, top: number} A tuple including a left and top value representing
 *     the page offset of the element.
 */
function getTotalOffset(el) {
    var result = {
        left: 0,
        top: 0
    };
    var parent = el;

    while (parent) {
        result.left += parent.offsetLeft;
        result.top += parent.offsetTop;
        parent = parent.offsetParent;
    }

    return result;
}

/**
 * Walks up the DOM starting at a given element, and returns an element that has the
 * specified class name or null.
 */
function up(el, className) {
    var parent = el;
    while (parent) {
        if (parent.classList && parent.classList.contains(className)) {
            break;
        }
        parent = parent.parentNode;
    }
    return parent || null;
}

function getCachedValue(div, property, attrName) {
    var value;
    if (div.hasAttribute(attrName)) {
        value = div.getAttribute(attrName);
    } else {
        value = div[property];
        div.setAttribute(attrName, value);
    }
    return value;
}

function onToggleClick(e) {
    toggleQuotedText(e.target);
    measurePositions();
}

function toggleQuotedText(toggleElement) {
    var elidedTextElement = toggleElement.nextSibling;
    var isHidden = getComputedStyle(elidedTextElement).display == 'none';
    toggleElement.innerHTML = isHidden ? MSG_HIDE_ELIDED : MSG_SHOW_ELIDED;
    elidedTextElement.style.display = isHidden ? 'block' : 'none';

    // Revealing the elided text should normalize it to fit-width to prevent
    // this message from blowing out the conversation width.
    if (isHidden) {
        normalizeElementWidths([elidedTextElement]);
    }
}

function collapseAllQuotedText() {
    processQuotedText(document.documentElement, false /* showElided */);
}

function processQuotedText(elt, showElided) {
    var i;
    var elements = elt.getElementsByClassName("elided-text");
    var elidedElement, toggleElement;
    for (i = 0; i < elements.length; i++) {
        elidedElement = elements[i];
        toggleElement = document.createElement("div");
        toggleElement.className = "mail-elided-text";
        toggleElement.innerHTML = MSG_SHOW_ELIDED;
        toggleElement.setAttribute("dir", "auto");
        toggleElement.onclick = onToggleClick;
        elidedElement.style.display = 'none';
        elidedElement.parentNode.insertBefore(toggleElement, elidedElement);
        if (showElided) {
            toggleQuotedText(toggleElement);
        }
    }
}

function isConversationEmpty(bodyDivs) {
    var i, len;
    var msgBody;
    var text;

    // Check if given divs are empty (in appearance), and disable zoom if so.
    for (i = 0, len = bodyDivs.length; i < len; i++) {
        msgBody = bodyDivs[i];
        // use 'textContent' to exclude markup when determining whether bodies are empty
        // (fall back to more expensive 'innerText' if 'textContent' isn't implemented)
        text = msgBody.textContent || msgBody.innerText;
        if (text.trim().length > 0) {
            return false;
        }
    }
    return true;
}

function normalizeAllMessageWidths() {
    var expandedBodyDivs;
    var metaViewport;
    var contentValues;
    var isEmpty;

    expandedBodyDivs = document.querySelectorAll(".expanded > .mail-message-content");

    isEmpty = isConversationEmpty(expandedBodyDivs);

    normalizeElementWidths(expandedBodyDivs);

    // assemble a working <meta> viewport "content" value from the base value in the
    // document, plus any dynamically determined options
    metaViewport = document.getElementById("meta-viewport");
    contentValues = [metaViewport.getAttribute("content")];
    if (isEmpty) {
        contentValues.push(metaViewport.getAttribute("data-zoom-off"));
    } else {
        contentValues.push(metaViewport.getAttribute("data-zoom-on"));
    }
    metaViewport.setAttribute("content", contentValues.join(","));
}

/*
 * Normalizes the width of all elements supplied to the document body's overall width.
 * Narrower elements are zoomed in, and wider elements are zoomed out.
 * This method is idempotent.
 */
function normalizeElementWidths(elements) {
    var i;
    var el;
    var documentWidth;
    var goalWidth;
    var origWidth;
    var newZoom, oldZoom;
    var outerZoom;
    var outerDiv;

    documentWidth = document.body.offsetWidth;
    goalWidth = WEBVIEW_WIDTH;

    for (i = 0; i < elements.length; i++) {
        el = elements[i];
        oldZoom = el.style.zoom;
        // reset any existing normalization
        if (oldZoom) {
            el.style.zoom = 1;
        }
        origWidth = el.style.width;
        el.style.width = goalWidth + "px";
        transformContent(el, goalWidth, el.scrollWidth);
        newZoom = documentWidth / el.scrollWidth;
        if (NORMALIZE_MESSAGE_WIDTHS) {
            if (el.classList.contains("mail-message-content")) {
                outerZoom = 1;
            } else {
                outerDiv = up(el, "mail-message-content");
                outerZoom = outerDiv ? outerDiv.style.zoom : 1;
            }
            el.style.zoom = newZoom / outerZoom;
        }
        el.style.width = origWidth;
    }
}

function transformContent(el, docWidth, elWidth) {
    var nodes;
    var i, len;
    var index;
    var newWidth = elWidth;
    var wStr;
    var touched;
    // the format of entries in this array is:
    // entry := [ undoFunction, undoFunctionThis, undoFunctionParamArray ]
    var actionLog = [];
    var node;
    var done = false;
    var msgId;
    var transformText;
    var existingText;
    var textElement;
    var start;
    var beforeWidth;
    var tmpActionLog = [];
    if (elWidth <= docWidth) {
        return;
    }

    start = Date.now();

    if (el.parentElement.classList.contains("mail-message")) {
        msgId = el.parentElement.id;
        transformText = "[origW=" + elWidth + "/" + docWidth;
    }

    // Try munging all divs or textareas with inline styles where the width
    // is wider than docWidth, and change it to be a max-width.
    touched = false;
    nodes = ENABLE_MUNGE_TABLES ? el.querySelectorAll("div[style], textarea[style]") : [];
    touched = transformBlockElements(nodes, docWidth, actionLog);
    if (touched) {
        newWidth = el.scrollWidth;
        console.log("ran div-width munger on el=" + el + " oldW=" + elWidth + " newW=" + newWidth
            + " docW=" + docWidth);
        if (msgId) {
            transformText += " DIV:newW=" + newWidth;
        }
        if (newWidth <= docWidth) {
            done = true;
        }
    }

    if (!done) {
        // OK, that wasn't enough. Find images with widths and override their widths.
        nodes = ENABLE_MUNGE_IMAGES ? el.querySelectorAll("img") : [];
        touched = transformImages(nodes, docWidth, actionLog);
        if (touched) {
            newWidth = el.scrollWidth;
            console.log("ran img munger on el=" + el + " oldW=" + elWidth + " newW=" + newWidth
                + " docW=" + docWidth);
            if (msgId) {
                transformText += " IMG:newW=" + newWidth;
            }
            if (newWidth <= docWidth) {
                done = true;
            }
        }
    }

    if (!done) {
        // OK, that wasn't enough. Find tables with widths and override their widths.
        // Also ensure that any use of 'table-layout: fixed' is negated, since using
        // that with 'width: auto' causes erratic table width.
        nodes = ENABLE_MUNGE_TABLES ? el.querySelectorAll("table") : [];
        touched = addClassToElements(nodes, shouldMungeTable, "munged",
            actionLog);
        if (touched) {
            newWidth = el.scrollWidth;
            console.log("ran table munger on el=" + el + " oldW=" + elWidth + " newW=" + newWidth
                + " docW=" + docWidth);
            if (msgId) {
                transformText += " TABLE:newW=" + newWidth;
            }
            if (newWidth <= docWidth) {
                done = true;
            }
        }
    }

    if (!done) {
        // OK, that wasn't enough. Try munging all <td> to override any width and nowrap set.
        beforeWidth = newWidth;
        nodes = ENABLE_MUNGE_TABLES ? el.querySelectorAll("td") : [];
        touched = addClassToElements(nodes, null /* mungeAll */, "munged",
            tmpActionLog);
        if (touched) {
            newWidth = el.scrollWidth;
            console.log("ran td munger on el=" + el + " oldW=" + elWidth + " newW=" + newWidth
                + " docW=" + docWidth);
            if (msgId) {
                transformText += " TD:newW=" + newWidth;
            }
            if (newWidth <= docWidth) {
                done = true;
            } else if (newWidth == beforeWidth) {
                // this transform did not improve things, and it is somewhat risky.
                // back it out, since it's the last transform and we gained nothing.
                undoActions(tmpActionLog);
            } else {
                // the transform WAS effective (although not 100%)
                // copy the temporary action log entries over as normal
                for (i = 0, len = tmpActionLog.length; i < len; i++) {
                    actionLog.push(tmpActionLog[i]);
                }
            }
        }
    }

    // If the transformations shrank the width significantly enough, leave them in place.
    // We figure that in those cases, the benefits outweight the risk of rendering artifacts.
    if (!done && (elWidth - newWidth) / (elWidth - docWidth) >
            TRANSFORM_MINIMUM_EFFECTIVE_RATIO) {
        console.log("transform(s) deemed effective enough");
        done = true;
    }

    if (done) {
        if (msgId) {
            transformText += "]";
            existingText = gTransformText[msgId];
            if (!existingText) {
                transformText = "Message transforms: " + transformText;
            } else {
                transformText = existingText + " " + transformText;
            }
            gTransformText[msgId] = transformText;
            window.mail.onMessageTransform(msgId, transformText);
            if (DEBUG_DISPLAY_TRANSFORMS) {
                textElement = el.firstChild;
                if (!textElement.classList || !textElement.classList.contains("transform-text")) {
                    textElement = document.createElement("div");
                    textElement.classList.add("transform-text");
                    textElement.style.fontSize = "10px";
                    textElement.style.color = "#ccc";
                    el.insertBefore(textElement, el.firstChild);
                }
                textElement.innerHTML = transformText + "<br>";
            }
        }
        console.log("munger(s) succeeded, elapsed time=" + (Date.now() - start));
        return;
    }

    // reverse all changes if the width is STILL not narrow enough
    // (except the width->maxWidth change, which is not particularly destructive)
    undoActions(actionLog);
    if (actionLog.length > 0) {
        console.log("all mungers failed, changes reversed. elapsed time=" + (Date.now() - start));
    }
}

function undoActions(actionLog) {
    for (i = 0, len = actionLog.length; i < len; i++) {
        actionLog[i][0].apply(actionLog[i][1], actionLog[i][2]);
    }
}

function addClassToElements(nodes, conditionFn, classToAdd, actionLog) {
    var i, len;
    var node;
    var added = false;
    for (i = 0, len = nodes.length; i < len; i++) {
        node = nodes[i];
        if (!conditionFn || conditionFn(node)) {
            if (node.classList.contains(classToAdd)) {
                continue;
            }
            node.classList.add(classToAdd);
            added = true;
            actionLog.push([node.classList.remove, node.classList, [classToAdd]]);
        }
    }
    return added;
}

function transformBlockElements(nodes, docWidth, actionLog) {
    var i, len;
    var node;
    var wStr;
    var index;
    var touched = false;

    for (i = 0, len = nodes.length; i < len; i++) {
        node = nodes[i];
        wStr = node.style.width || node.style.minWidth;
        index = wStr ? wStr.indexOf("px") : -1;
        if (index >= 0 && wStr.slice(0, index) > docWidth) {
            saveStyleProperty(node, "width", actionLog);
            saveStyleProperty(node, "minWidth", actionLog);
            saveStyleProperty(node, "maxWidth", actionLog);
            node.style.width = "100%";
            node.style.minWidth = "";
            node.style.maxWidth = wStr;
            touched = true;
        }
    }
    return touched;
}

function transformImages(nodes, docWidth, actionLog) {
    var i, len;
    var node;
    var w, h;
    var touched = false;

    for (i = 0, len = nodes.length; i < len; i++) {
        node = nodes[i];
        w = node.offsetWidth;
        h = node.offsetHeight;
        // shrink w/h proportionally if the img is wider than available width
        if (w > docWidth) {
            saveStyleProperty(node, "maxWidth", actionLog);
            saveStyleProperty(node, "width", actionLog);
            saveStyleProperty(node, "height", actionLog);
            node.style.maxWidth = docWidth + "px";
            node.style.width = "100%";
            node.style.height = "auto";
            touched = true;
        }
    }
    return touched;
}

function saveStyleProperty(node, property, actionLog) {
    var savedName = "data-" + property;
    node.setAttribute(savedName, node.style[property]);
    actionLog.push([undoSetProperty, node, [property, savedName]]);
}

function undoSetProperty(property, savedProperty) {
    this.style[property] = savedProperty ? this.getAttribute(savedProperty) : "";
}

function shouldMungeTable(table) {
    return table.hasAttribute("width") || table.style.width;
}

function hideAllUnsafeImages() {
    hideUnsafeImages(document.getElementsByClassName("mail-message-content"));
}

function hideUnsafeImages(msgContentDivs) {
    var i, msgContentCount;
    var j, imgCount;
    var msgContentDiv, image;
    var images;
    var showImages;
    var k = 0;
    var urls = new Array();
    var messageIds = new Array();
    for (i = 0, msgContentCount = msgContentDivs.length; i < msgContentCount; i++) {
        msgContentDiv = msgContentDivs[i];
        showImages = msgContentDiv.classList.contains("mail-show-images");

        images = msgContentDiv.getElementsByTagName("img");
        for (j = 0, imgCount = images.length; j < imgCount; j++) {
            image = images[j];
            var src = rewriteRelativeImageSrc(image);
            if (src) {
                urls[k] = src;
                messageIds[k] = msgContentDiv.parentNode.id;
                k++;
            }
            attachImageLoadListener(image);
            // TODO: handle inline image attachments for all supported protocols
            if (!showImages) {
                blockImage(image);
            }
        }
    }

    window.mail.onInlineAttachmentsParsed(urls, messageIds);
}

/**
 * Changes relative paths to absolute path by pre-pending the account uri.
 * @param {Element} imgElement Image for which the src path will be updated.
 * @returns the rewritten image src string or null if the imgElement was not rewritten.
 */
function rewriteRelativeImageSrc(imgElement) {
    var src = imgElement.src;

    // DOC_BASE_URI will always be a unique x-thread:// uri for this particular conversation
    if (src.indexOf(DOC_BASE_URI) == 0 && (DOC_BASE_URI != CONVERSATION_BASE_URI)) {
        // The conversation specifies a different base uri than the document
        src = CONVERSATION_BASE_URI + src.substring(DOC_BASE_URI.length);
        imgElement.src = src;
        return src;
    }

    // preserve cid urls as is
    if (src.substring(0, 4) == "cid:") {
        return src;
    }

    return null;
};


function attachImageLoadListener(imageElement) {
    // Reset the src attribute to the empty string because onload will only fire if the src
    // attribute is set after the onload listener.
    var originalSrc = imageElement.src;
    imageElement.src = '';
    imageElement.onload = imageOnLoad;
    imageElement.src = originalSrc;
}

/**
 * Handle an onload event for an <img> tag.
 * The image could be within an elided-text block, or at the top level of a message.
 * When a new image loads, its new bounds may affect message or elided-text geometry,
 * so we need to inspect and adjust the enclosing element's zoom level where necessary.
 *
 * Because this method can be called really often, and zoom-level adjustment is slow,
 * we collect the elements to be processed and do them all later in a single deferred pass.
 */
function imageOnLoad(e) {
    // normalize the quoted text parent if we're in a quoted text block, or else
    // normalize the parent message content element
    var parent = up(e.target, "elided-text") || up(e.target, "mail-message-content");
    if (!parent) {
        // sanity check. shouldn't really happen.
        return;
    }

    // if there was no previous work, schedule a new deferred job
    if (gImageLoadElements.length == 0) {
        window.setTimeout(handleAllImageOnLoads, 0);
    }

    // enqueue the work if it wasn't already enqueued
    if (gImageLoadElements.indexOf(parent) == -1) {
        gImageLoadElements.push(parent);
    }
}

// handle all deferred work from image onload events
function handleAllImageOnLoads() {
    normalizeElementWidths(gImageLoadElements);
    measurePositions();
    // clear the queue so the next onload event starts a new job
    gImageLoadElements = [];
}

function blockImage(imageElement) {
    var src = imageElement.src;
    if (src.indexOf("http://") == 0 || src.indexOf("https://") == 0 ||
            src.indexOf("content://") == 0 || src.indexOf("cid:") == 0) {
        imageElement.setAttribute(BLOCKED_SRC_ATTR, src);
        imageElement.src = "data:";
    }
}

function setWideViewport() {
    var metaViewport = document.getElementById('meta-viewport');
    metaViewport.setAttribute('content', 'width=' + WIDE_VIEWPORT_WIDTH);
}

function restoreScrollPosition() {
    var scrollYPercent = window.mail.getScrollYPercent();
    if (scrollYPercent && document.body.offsetHeight > window.innerHeight) {
        document.body.scrollTop = Math.floor(scrollYPercent * document.body.offsetHeight);
    }
}

function onContentReady(event) {
    // hack for b/1333356
    if (RUNNING_KITKAT_OR_LATER) {
        restoreScrollPosition();
    }
    window.mail.onContentReady();
}

function setupContentReady() {
    var signalDiv;

    // PAGE READINESS SIGNAL FOR JELLYBEAN AND NEWER
    // Notify the app on 'webkitAnimationStart' of a simple dummy element with a simple no-op
    // animation that immediately runs on page load. The app uses this as a signal that the
    // content is loaded and ready to draw, since WebView delays firing this event until the
    // layers are composited and everything is ready to draw.
    //
    // This code is conditionally enabled on JB+ by setting the ENABLE_CONTENT_READY flag.
    if (ENABLE_CONTENT_READY) {
        signalDiv = document.getElementById("initial-load-signal");
        signalDiv.addEventListener("webkitAnimationStart", onContentReady, false);
    }
}

// BEGIN Java->JavaScript handlers
function measurePositions() {
    var overlayTops, overlayBottoms;
    var i;
    var len;

    var expandedBody, headerSpacer;
    var prevBodyBottom = 0;
    var expandedBodyDivs = document.querySelectorAll(".expanded > .mail-message-content");

    // N.B. offsetTop and offsetHeight of an element with the "zoom:" style applied cannot be
    // trusted.

    overlayTops = new Array(expandedBodyDivs.length + 1);
    overlayBottoms = new Array(expandedBodyDivs.length + 1);
    for (i = 0, len = expandedBodyDivs.length; i < len; i++) {
        expandedBody = expandedBodyDivs[i];
        headerSpacer = expandedBody.previousElementSibling;
        // addJavascriptInterface handler only supports string arrays
        overlayTops[i] = prevBodyBottom;
        overlayBottoms[i] = (getTotalOffset(headerSpacer).top + headerSpacer.offsetHeight);
        prevBodyBottom = getTotalOffset(expandedBody.nextElementSibling).top;
    }
    // add an extra one to mark the top/bottom of the last message footer spacer
    overlayTops[i] = prevBodyBottom;
    overlayBottoms[i] = document.documentElement.scrollHeight;

    window.mail.onWebContentGeometryChange(overlayTops, overlayBottoms);
}

function unblockImages(messageDomIds) {
    var i, j, images, imgCount, image, blockedSrc;
    for (j = 0, len = messageDomIds.length; j < len; j++) {
        var messageDomId = messageDomIds[j];
        var msg = document.getElementById(messageDomId);
        if (!msg) {
            console.log("can't unblock, no matching message for id: " + messageDomId);
            continue;
        }
        images = msg.getElementsByTagName("img");
        for (i = 0, imgCount = images.length; i < imgCount; i++) {
            image = images[i];
            blockedSrc = image.getAttribute(BLOCKED_SRC_ATTR);
            if (blockedSrc) {
                image.src = blockedSrc;
                image.removeAttribute(BLOCKED_SRC_ATTR);
            }
        }
    }
}

function setConversationHeaderSpacerHeight(spacerHeight) {
    var spacer = document.getElementById("conversation-header");
    if (!spacer) {
        console.log("can't set spacer for conversation header");
        return;
    }
    spacer.style.height = spacerHeight + "px";
    measurePositions();
}

function setConversationFooterSpacerHeight(spacerHeight) {
    var spacer = document.getElementById("conversation-footer");
    if (!spacer) {
        console.log("can't set spacer for conversation footer");
        return;
    }
    spacer.style.height = spacerHeight + "px";
    measurePositions();
}

function setMessageHeaderSpacerHeight(messageDomId, spacerHeight) {
    var spacer = document.querySelector("#" + messageDomId + " > .mail-message-header");
    setSpacerHeight(spacer, spacerHeight);
}

function setSpacerHeight(spacer, spacerHeight) {
    if (!spacer) {
        console.log("can't set spacer for message with id: " + messageDomId);
        return;
    }
    spacer.style.height = spacerHeight + "px";
    measurePositions();
}

function setMessageBodyVisible(messageDomId, isVisible, spacerHeight) {
    var i, len;
    var visibility = isVisible ? "block" : "none";
    var messageDiv = document.querySelector("#" + messageDomId);
    var collapsibleDivs = document.querySelectorAll("#" + messageDomId + " > .collapsible");
    if (!messageDiv || collapsibleDivs.length == 0) {
        console.log("can't set body visibility for message with id: " + messageDomId);
        return;
    }

    messageDiv.classList.toggle("expanded");
    for (i = 0, len = collapsibleDivs.length; i < len; i++) {
        collapsibleDivs[i].style.display = visibility;
    }

    // revealing new content should trigger width normalization, since the initial render
    // skips collapsed and super-collapsed messages
    if (isVisible) {
        normalizeElementWidths(messageDiv.getElementsByClassName("mail-message-content"));
    }

    setMessageHeaderSpacerHeight(messageDomId, spacerHeight);
}

function replaceSuperCollapsedBlock(startIndex) {
    var parent, block, msg;

    block = document.querySelector(".mail-super-collapsed-block[index='" + startIndex + "']");
    if (!block) {
        console.log("can't expand super collapsed block at index: " + startIndex);
        return;
    }
    parent = block.parentNode;
    block.innerHTML = window.mail.getTempMessageBodies();

    // process the new block contents in one go before we pluck them out of the common ancestor
    processQuotedText(block, false /* showElided */);
    hideUnsafeImages(block.getElementsByClassName("mail-message-content"));

    msg = block.firstChild;
    while (msg) {
        parent.insertBefore(msg, block);
        msg = block.firstChild;
    }
    parent.removeChild(block);
    disablePostForms();
    measurePositions();
}

function processNewMessageBody(msgContentDiv) {
    processQuotedText(msgContentDiv, true /* showElided */);
    hideUnsafeImages([msgContentDiv]);
    if (up(msgContentDiv, "mail-message").classList.contains("expanded")) {
        normalizeElementWidths([msgContentDiv]);
    }
}

function replaceMessageBodies(messageIds) {
    var i;
    var id;
    var msgContentDiv;

    for (i = 0, len = messageIds.length; i < len; i++) {
        id = messageIds[i];
        msgContentDiv = document.querySelector("#" + id + " > .mail-message-content");
        // Check if we actually have a div before trying to replace this message body.
        if (msgContentDiv) {
            msgContentDiv.innerHTML = window.mail.getMessageBody(id);
            processNewMessageBody(msgContentDiv);
        } else {
            // There's no message div, just skip it. We're in a really busted state.
            console.log("Mail message content for msg " + id + " to replace not found.");
        }
    }
    disablePostForms();
    measurePositions();
}

// handle the special case of adding a single new message at the end of a conversation
function appendMessageHtml() {
    var msg = document.createElement("div");
    msg.innerHTML = window.mail.getTempMessageBodies();
    var body = msg.children[0];  // toss the outer div, it was just to render innerHTML into
    document.body.insertBefore(body, document.getElementById("conversation-footer"));
    processNewMessageBody(body.querySelector(".mail-message-content"));
    disablePostForms();
    measurePositions();
}

function disablePostForms() {
    var forms = document.getElementsByTagName('FORM');
    var i;
    var j;
    var elements;

    for (i = 0; i < forms.length; ++i) {
        if (forms[i].method.toUpperCase() === 'POST') {
            forms[i].onsubmit = function() {
                alert(MSG_FORMS_ARE_DISABLED);
                return false;
            }
            elements = forms[i].elements;
            for (j = 0; j < elements.length; ++j) {
                if (elements[j].type != 'submit') {
                    elements[j].disabled = true;
                }
            }
        }
    }
}
// END Java->JavaScript handlers

// Do this first to ensure that the readiness signal comes through,
// even if a stray exception later occurs.
setupContentReady();

collapseAllQuotedText();
hideAllUnsafeImages();
normalizeAllMessageWidths();
//setWideViewport();
// hack for b/1333356
if (!RUNNING_KITKAT_OR_LATER) {
    restoreScrollPosition();
}
disablePostForms();
measurePositions();
