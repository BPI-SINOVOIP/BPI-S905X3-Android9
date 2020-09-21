#include "RenderArea.h"

#include <gui/LayerState.h>

namespace android {

float RenderArea::getCaptureFillValue(CaptureFill captureFill) {
    switch(captureFill) {
        case CaptureFill::CLEAR:
            return 0.0f;
        case CaptureFill::OPAQUE:
        default:
            return 1.0f;
    }
}
/*
 * Checks that the requested width and height are valid and updates them to the render area
 * dimensions if they are set to 0
 */
status_t RenderArea::updateDimensions(int displayRotation) {
    // get screen geometry

    uint32_t width = getWidth();
    uint32_t height = getHeight();

    if (mRotationFlags & Transform::ROT_90) {
        std::swap(width, height);
    }

    if (displayRotation & DisplayState::eOrientationSwapMask) {
        std::swap(width, height);
    }

    if ((mReqWidth > width) || (mReqHeight > height)) {
        ALOGE("size mismatch (%d, %d) > (%d, %d)", mReqWidth, mReqHeight, width, height);
        return BAD_VALUE;
    }

    if (mReqWidth == 0) {
        mReqWidth = width;
    }
    if (mReqHeight == 0) {
        mReqHeight = height;
    }

    return NO_ERROR;
}

} // namespace android
