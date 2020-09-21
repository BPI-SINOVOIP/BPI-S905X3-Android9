/*
 * Copyright 2017 The Chromium Authors.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#ifndef _RUNTIME_ARC_INPUT_BRIDGE_PROTOCOL_H
#define _RUNTIME_ARC_INPUT_BRIDGE_PROTOCOL_H

#include <cstdint>

// Hack: major/minor may be defined by stdlib in error.
// see https://sourceware.org/bugzilla/show_bug.cgi?id=19239
#undef major
#undef minor

namespace arc {

// Location of the named pipe for communicating events
static const char* kArcInputBridgePipe = "/var/run/inputbridge/inputbridge";

enum class InputEventType {
    RESET = 0,

    POINTER_ENTER,
    POINTER_MOVE,
    POINTER_LEAVE,
    POINTER_BUTTON,
    POINTER_SCROLL_X,
    POINTER_SCROLL_Y,
    POINTER_SCROLL_DISCRETE,
    POINTER_SCROLL_STOP,
    POINTER_FRAME,

    TOUCH_DOWN,
    TOUCH_MOVE,
    TOUCH_UP,
    TOUCH_CANCEL,
    TOUCH_SHAPE,
    TOUCH_TOOL_TYPE,
    TOUCH_FORCE,
    TOUCH_TILT,
    TOUCH_FRAME,

    GESTURE_PINCH_BEGIN,
    GESTURE_PINCH_UPDATE,
    GESTURE_PINCH_END,

    GESTURE_SWIPE_BEGIN,
    GESTURE_SWIPE_UPDATE,
    GESTURE_SWIPE_END,

    KEY,
    KEY_MODIFIERS,
    KEY_RESET,

    GAMEPAD_CONNECTED,
    GAMEPAD_DISCONNECTED,
    GAMEPAD_AXIS,
    GAMEPAD_BUTTON,
    GAMEPAD_FRAME,

    // event for reporting the state of a switch, such as a lid switch
    SWITCH,

    // event for reporting dimensions and properties of the display
    DISPLAY_METRICS
} __attribute__((packed));

struct PointerArgs {
    float x;
    float y;
    bool discrete;
} __attribute__((packed));

struct KeyArgs {
    // Android AKEYCODE_ code after application of keyboard layout.
    uint32_t keyCode;
    // Linux KEY_ scan code as it was received from the kernel.
    uint32_t scanCode;
    // 0 = released, 1 = pressed.
    uint32_t state;
    // serial number of this key events in wayland.
    uint32_t serial;
} __attribute__((packed));

struct MetaArgs {
    // Android AMETA_ bitmask of meta state.
    uint32_t metaState;
};

struct ButtonArgs {
    // Android AMOTION_EVENT_BUTTON_ code
    uint32_t code;
    // 0 = released, 1 = pressed.
    uint32_t state;
};

enum class TouchToolType { TOUCH = 0, PEN, ERASER } __attribute__((packed));

struct TouchArgs {
    int32_t id;
    union {
        float x;
        float major;
        float force;
        TouchToolType tool;
    };
    union {
        float y;
        float minor;
    };
} __attribute__((packed));

struct GestureArgs {
    uint32_t fingers;
    bool cancelled;
} __attribute__((packed));

struct GesturePinchArgs {
    float scale;
} __attribute__((packed));

struct GestureSwipeArgs {
    float dx;
    float dy;
} __attribute__((packed));

struct GamepadArgs {
    int32_t id;
    union {
        // Used by GAMEPAD_BUTTON and GAMEPAD_AXIS respectively
        int32_t button;
        int32_t axis;
    };
    bool pressed;
    float value;
} __attribute__((packed));

struct SwitchArgs {
    // Switch ID defined at bionic/libc/kernel/uapi/linux/input.h.
    int32_t switchCode;
    // Switch value defined at frameworks/native/include/android/input.h.
    int32_t state;
} __attribute__((packed));

struct DisplayMetricsArgs {
    int32_t width;
    int32_t height;
    float ui_scale;
} __attribute__((packed));


// Union-like class describing an event. The InputEventType describes which
// of member of the union contains the data of this event.
struct BridgeInputEvent {
    uint64_t timestamp;
    int32_t taskId;
    int32_t windowId;

    InputEventType type;

    union {
        PointerArgs pointer;
        KeyArgs key;
        TouchArgs touch;
        MetaArgs meta;
        ButtonArgs button;
        GestureArgs gesture;
        GesturePinchArgs gesture_pinch;
        GestureSwipeArgs gesture_swipe;
        GamepadArgs gamepad;
        SwitchArgs switches;
        DisplayMetricsArgs display_metrics;
    };

    static BridgeInputEvent ResetEvent(uint64_t timestamp) {
        return {timestamp, -1, 0, InputEventType::RESET, {}};
    }

    static BridgeInputEvent KeyEvent(uint64_t timestamp, uint32_t keyCode, uint32_t scanCode,
                                     uint32_t state, uint32_t serial) {
        BridgeInputEvent event{timestamp, -1, 0, InputEventType::KEY, {}};
        event.key = {keyCode, scanCode, state, serial};
        return event;
    }

    static BridgeInputEvent KeyModifiersEvent(uint64_t timestamp, uint32_t modifiers) {
        BridgeInputEvent event{timestamp, -1, 0, InputEventType::KEY_MODIFIERS, {}};
        event.meta = {modifiers};
        return event;
    }

    static BridgeInputEvent PointerEvent(uint64_t timestamp, InputEventType type, float x = 0,
                                         float y = 0, bool discrete = false) {
        BridgeInputEvent event{timestamp, -1, 0, type, {}};
        event.pointer = {x, y, discrete};
        return event;
    }

    static BridgeInputEvent PointerButtonEvent(uint64_t timestamp, uint32_t code, uint32_t state) {
        BridgeInputEvent event{timestamp, -1, 0, InputEventType::POINTER_BUTTON, {}};
        event.button = {code, state};
        return event;
    }

    static BridgeInputEvent PinchBeginEvent(uint64_t timestamp) {
        return {timestamp, -1, 0, InputEventType::GESTURE_PINCH_BEGIN, {}};
    }

    static BridgeInputEvent PinchUpdateEvent(uint64_t timestamp, float scale) {
        BridgeInputEvent event{timestamp, -1, 0, InputEventType::GESTURE_PINCH_UPDATE, {}};
        event.gesture_pinch.scale = scale;
        return event;
    }

    static BridgeInputEvent PinchEndEvent(uint64_t timestamp, bool cancelled) {
        BridgeInputEvent event{timestamp, -1, 0, InputEventType::GESTURE_PINCH_END, {}};
        event.gesture.cancelled = cancelled;
        return event;
    }

    static BridgeInputEvent SwipeBeginEvent(uint64_t timestamp, uint32_t fingers) {
        BridgeInputEvent event{timestamp, -1, 0, InputEventType::GESTURE_SWIPE_BEGIN, {}};
        event.gesture.fingers = fingers;
        return event;
    }

    static BridgeInputEvent SwipeUpdateEvent(uint64_t timestamp, float dx, float dy) {
        BridgeInputEvent event{timestamp, -1, 0, InputEventType::GESTURE_SWIPE_UPDATE, {}};
        event.gesture_swipe.dx = dx;
        event.gesture_swipe.dy = dy;
        return event;
    }

    static BridgeInputEvent SwipeEndEvent(uint64_t timestamp, bool cancelled) {
        BridgeInputEvent event{timestamp, -1, 0, InputEventType::GESTURE_SWIPE_END, {}};
        event.gesture.cancelled = cancelled;
        return event;
    }

    static BridgeInputEvent GamepadConnectedEvent(int32_t id) {
        BridgeInputEvent event{0, -1, 0, InputEventType::GAMEPAD_CONNECTED, {}};
        event.gamepad.id = id;
        return event;
    }

    static BridgeInputEvent GamepadDisconnectedEvent(int32_t id) {
        BridgeInputEvent event{0, -1, 0, InputEventType::GAMEPAD_DISCONNECTED, {}};
        event.gamepad.id = id;
        return event;
    }

    static BridgeInputEvent GamepadAxisEvent(uint64_t timestamp, int32_t id, int32_t axis,
                                             float value) {
        BridgeInputEvent event{timestamp, -1, 0, InputEventType::GAMEPAD_AXIS, {}};
        event.gamepad.id = id;
        event.gamepad.axis = axis;
        event.gamepad.value = value;
        return event;
    }

    static BridgeInputEvent GamepadButtonEvent(uint64_t timestamp, int32_t id, int32_t button,
                                               bool pressed, float value) {
        BridgeInputEvent event{timestamp, -1, 0, InputEventType::GAMEPAD_BUTTON, {}};
        event.gamepad.id = id;
        event.gamepad.button = button;
        event.gamepad.pressed = pressed;
        event.gamepad.value = value;
        return event;
    }

    static BridgeInputEvent GamepadFrameEvent(uint64_t timestamp, int32_t id) {
        BridgeInputEvent event{timestamp, -1, 0, InputEventType::GAMEPAD_FRAME, {}};
        event.gamepad.id = id;
        return event;
    }

    static BridgeInputEvent SwitchEvent(uint64_t timestamp, int32_t switchCode, int32_t state) {
        BridgeInputEvent event{timestamp, -1, 0, InputEventType::SWITCH, {}};
        event.switches.switchCode = switchCode;
        event.switches.state = state;
        return event;
    }
} __attribute__((packed));

}  // namespace arc

#endif  // _RUNTIME_ARC_INPUT_BRIDGE_PROTOCOL_H
