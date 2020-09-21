Window.prototype.orginalEventListener = Window.prototype.addEventListener;
Window.prototype.addEventListener = function(type, listener, useCapture) {
    if (type !== "beforeunload") {
        orginalEventListener(type, listener, useCapture);
    }
}
