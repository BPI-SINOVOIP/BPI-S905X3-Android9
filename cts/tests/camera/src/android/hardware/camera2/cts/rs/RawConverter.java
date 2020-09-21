/*
 * Copyright 2015 The Android Open Source Project
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

package android.hardware.camera2.cts.rs;

import android.graphics.Bitmap;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraMetadata;
import android.hardware.camera2.CaptureResult;
import android.hardware.camera2.params.ColorSpaceTransform;
import android.hardware.camera2.params.LensShadingMap;
import android.renderscript.Allocation;
import android.renderscript.Element;
import android.renderscript.Float3;
import android.renderscript.Float4;
import android.renderscript.Int4;
import android.renderscript.Matrix3f;
import android.renderscript.RenderScript;
import android.renderscript.Type;

import android.hardware.camera2.cts.ScriptC_raw_converter;
import android.util.Log;
import android.util.Rational;
import android.util.SparseIntArray;

import java.util.Arrays;

/**
 * Utility class providing methods for rendering RAW16 images into other colorspaces.
 */
public class RawConverter {
    private static final String TAG = "RawConverter";
    private static final boolean DEBUG = Log.isLoggable(TAG, Log.DEBUG);

    /**
     * Matrix to convert from CIE XYZ colorspace to sRGB, Bradford-adapted to D65.
     */
    private static final float[] sXYZtoRGBBradford = new float[] {
            3.1338561f, -1.6168667f, -0.4906146f,
            -0.9787684f, 1.9161415f, 0.0334540f,
            0.0719453f, -0.2289914f, 1.4052427f
    };

    /**
     * Matrix to convert from the ProPhoto RGB colorspace to CIE XYZ colorspace.
     */
    private static final float[] sProPhotoToXYZ = new float[] {
            0.797779f, 0.135213f, 0.031303f,
            0.288000f, 0.711900f, 0.000100f,
            0.000000f, 0.000000f, 0.825105f
    };

    /**
     * Matrix to convert from CIE XYZ colorspace to ProPhoto RGB colorspace.
     */
    private static final float[] sXYZtoProPhoto = new float[] {
            1.345753f, -0.255603f, -0.051025f,
            -0.544426f, 1.508096f, 0.020472f,
            0.000000f, 0.000000f, 1.211968f
    };

    /**
     * Coefficients for a 3rd order polynomial, ordered from highest to lowest power.  This
     * polynomial approximates the default tonemapping curve used for ACR3.
     */
    private static final float[] DEFAULT_ACR3_TONEMAP_CURVE_COEFFS = new float[] {
            -0.7836f, 0.8469f, 0.943f, 0.0209f
    };

    /**
     * The D50 whitepoint coordinates in CIE XYZ colorspace.
     */
    private static final float[] D50_XYZ = new float[] { 0.9642f, 1, 0.8249f };

    /**
     * An array containing the color temperatures for standard reference illuminants.
     */
    private static final SparseIntArray sStandardIlluminants = new SparseIntArray();
    private static final int NO_ILLUMINANT = -1;
    static {
        sStandardIlluminants.append(CameraMetadata.SENSOR_REFERENCE_ILLUMINANT1_CLOUDY_WEATHER,
                6504);
        sStandardIlluminants.append(CameraMetadata.SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT, 6504);
        sStandardIlluminants.append(CameraMetadata.SENSOR_REFERENCE_ILLUMINANT1_D65, 6504);
        sStandardIlluminants.append(CameraMetadata.SENSOR_REFERENCE_ILLUMINANT1_FINE_WEATHER, 5003);
        sStandardIlluminants.append(CameraMetadata.SENSOR_REFERENCE_ILLUMINANT1_D50, 5003);
        sStandardIlluminants.append(CameraMetadata.SENSOR_REFERENCE_ILLUMINANT1_FLASH, 5503);
        sStandardIlluminants.append(CameraMetadata.SENSOR_REFERENCE_ILLUMINANT1_D55, 5503);
        sStandardIlluminants.append(CameraMetadata.SENSOR_REFERENCE_ILLUMINANT1_SHADE, 7504);
        sStandardIlluminants.append(CameraMetadata.SENSOR_REFERENCE_ILLUMINANT1_D75, 7504);
        sStandardIlluminants.append(CameraMetadata.SENSOR_REFERENCE_ILLUMINANT1_ISO_STUDIO_TUNGSTEN,
                2856);
        sStandardIlluminants.append(CameraMetadata.SENSOR_REFERENCE_ILLUMINANT1_TUNGSTEN, 2856);
        sStandardIlluminants.append(CameraMetadata.SENSOR_REFERENCE_ILLUMINANT1_STANDARD_A, 2856);
        sStandardIlluminants.append(
                CameraMetadata.SENSOR_REFERENCE_ILLUMINANT1_DAY_WHITE_FLUORESCENT, 4874);
        sStandardIlluminants.append(CameraMetadata.SENSOR_REFERENCE_ILLUMINANT1_STANDARD_B, 4874);
        sStandardIlluminants.append(CameraMetadata.SENSOR_REFERENCE_ILLUMINANT1_STANDARD_C, 6774);
        sStandardIlluminants.append(
                CameraMetadata.SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT_FLUORESCENT, 6430);
        sStandardIlluminants.append(
                CameraMetadata.SENSOR_REFERENCE_ILLUMINANT1_COOL_WHITE_FLUORESCENT, 4230);
        sStandardIlluminants.append(
                CameraMetadata.SENSOR_REFERENCE_ILLUMINANT1_WHITE_FLUORESCENT, 3450);
        sStandardIlluminants.append(CameraMetadata.SENSOR_REFERENCE_ILLUMINANT1_FLUORESCENT, 2940);
    }

    /**
     * Convert a RAW16 buffer into an sRGB buffer, and write the result into a bitmap.
     *
     * <p> This function applies the operations roughly outlined in the Adobe DNG specification
     * using the provided metadata about the image sensor.  Sensor data for Android devices is
     * assumed to be relatively linear, and no extra linearization step is applied here.  The
     * following operations are applied in the given order:</p>
     *
     * <ul>
     *     <li>
     *         Black level subtraction - the black levels given in the SENSOR_BLACK_LEVEL_PATTERN
     *         tag are subtracted from the corresponding raw pixels.
     *     </li>
     *     <li>
     *         Rescaling - each raw pixel is scaled by 1/(white level - black level).
     *     </li>
     *     <li>
     *         Lens shading correction - the interpolated gains from the gain map defined in the
     *         STATISTICS_LENS_SHADING_CORRECTION_MAP are applied to each raw pixel.
     *     </li>
     *     <li>
     *         Clipping - each raw pixel is clipped to a range of [0.0, 1.0].
     *     </li>
     *     <li>
     *         Demosaic - the RGB channels for each pixel are retrieved from the Bayer mosaic
     *         of raw pixels using a simple bilinear-interpolation demosaicing algorithm.
     *     </li>
     *     <li>
     *         Colorspace transform to wide-gamut RGB - each pixel is mapped into a
     *         wide-gamut colorspace (in this case ProPhoto RGB is used) from the sensor
     *         colorspace.
     *     </li>
     *     <li>
     *         Tonemapping - A basic tonemapping curve using the default from ACR3 is applied
     *         (no further exposure compensation is applied here, though this could be improved).
     *     </li>
     *     <li>
     *         Colorspace transform to final RGB - each pixel is mapped into linear sRGB colorspace.
     *     </li>
     *     <li>
     *         Gamma correction - each pixel is gamma corrected using γ=2.2 to map into sRGB
     *         colorspace for viewing.
     *     </li>
     *     <li>
     *         Packing - each pixel is scaled so that each color channel has a range of [0, 255],
     *         and is packed into an Android bitmap.
     *     </li>
     * </ul>
     *
     * <p> Arguments given here are assumed to come from the values for the corresponding
     * {@link CameraCharacteristics.Key}s defined for the camera that produced this RAW16 buffer.
     * </p>
     * @param rs a {@link RenderScript} context to use.
     * @param inputWidth width of the input RAW16 image in pixels.
     * @param inputHeight height of the input RAW16 image in pixels.
     * @param inputStride stride of the input RAW16 image in bytes.
     * @param rawImageInput a byte array containing a RAW16 image.
     * @param staticMetadata the {@link CameraCharacteristics} for this RAW capture.
     * @param dynamicMetadata the {@link CaptureResult} for this RAW capture.
     * @param outputOffsetX the offset width into the raw image of the left side of the output
     *                      rectangle.
     * @param outputOffsetY the offset height into the raw image of the top side of the output
     *                      rectangle.
     * @param argbOutput a {@link Bitmap} to output the rendered RAW image into.  The height and
     *                   width of this bitmap along with the output offsets are used to determine
     *                   the dimensions and offset of the output rectangle contained in the RAW
     *                   image to be rendered.
     */
    public static void convertToSRGB(RenderScript rs, int inputWidth, int inputHeight,
            int inputStride, byte[] rawImageInput, CameraCharacteristics staticMetadata,
            CaptureResult dynamicMetadata, int outputOffsetX, int outputOffsetY,
            /*out*/Bitmap argbOutput) {
        int cfa = staticMetadata.get(CameraCharacteristics.SENSOR_INFO_COLOR_FILTER_ARRANGEMENT);
        int[] blackLevelPattern = new int[4];
        staticMetadata.get(CameraCharacteristics.SENSOR_BLACK_LEVEL_PATTERN).
                copyTo(blackLevelPattern, /*offset*/0);
        int whiteLevel = staticMetadata.get(CameraCharacteristics.SENSOR_INFO_WHITE_LEVEL);
        int ref1 = staticMetadata.get(CameraCharacteristics.SENSOR_REFERENCE_ILLUMINANT1);
        int ref2;
        if (staticMetadata.get(CameraCharacteristics.SENSOR_REFERENCE_ILLUMINANT2) != null) {
            ref2 = staticMetadata.get(CameraCharacteristics.SENSOR_REFERENCE_ILLUMINANT2);
        }
        else {
            ref2 = ref1;
        }
        float[] calib1 = new float[9];
        float[] calib2 = new float[9];
        convertColorspaceTransform(
                staticMetadata.get(CameraCharacteristics.SENSOR_CALIBRATION_TRANSFORM1), calib1);
        if (staticMetadata.get(CameraCharacteristics.SENSOR_CALIBRATION_TRANSFORM2) != null) {
            convertColorspaceTransform(
                staticMetadata.get(CameraCharacteristics.SENSOR_CALIBRATION_TRANSFORM2), calib2);
        }
        else {
            convertColorspaceTransform(
                staticMetadata.get(CameraCharacteristics.SENSOR_CALIBRATION_TRANSFORM1), calib2);
        }
        float[] color1 = new float[9];
        float[] color2 = new float[9];
        convertColorspaceTransform(
                staticMetadata.get(CameraCharacteristics.SENSOR_COLOR_TRANSFORM1), color1);
        if (staticMetadata.get(CameraCharacteristics.SENSOR_COLOR_TRANSFORM2) != null) {
            convertColorspaceTransform(
                staticMetadata.get(CameraCharacteristics.SENSOR_COLOR_TRANSFORM2), color2);
        }
        else {
            convertColorspaceTransform(
                staticMetadata.get(CameraCharacteristics.SENSOR_COLOR_TRANSFORM1), color2);
        }
        float[] forward1 = new float[9];
        float[] forward2 = new float[9];
        convertColorspaceTransform(
                staticMetadata.get(CameraCharacteristics.SENSOR_FORWARD_MATRIX1), forward1);
        if (staticMetadata.get(CameraCharacteristics.SENSOR_FORWARD_MATRIX2) != null) {
            convertColorspaceTransform(
                staticMetadata.get(CameraCharacteristics.SENSOR_FORWARD_MATRIX2), forward2);
        }
        else {
            convertColorspaceTransform(
                staticMetadata.get(CameraCharacteristics.SENSOR_FORWARD_MATRIX1), forward2);
        }

        Rational[] neutral = dynamicMetadata.get(CaptureResult.SENSOR_NEUTRAL_COLOR_POINT);

        LensShadingMap shadingMap = dynamicMetadata.get(CaptureResult.STATISTICS_LENS_SHADING_CORRECTION_MAP);

        convertToSRGB(rs, inputWidth, inputHeight, inputStride, cfa, blackLevelPattern, whiteLevel,
                rawImageInput, ref1, ref2, calib1, calib2, color1, color2,
                forward1, forward2, neutral, shadingMap, outputOffsetX, outputOffsetY, argbOutput);
    }

    /**
     * Convert a RAW16 buffer into an sRGB buffer, and write the result into a bitmap.
     *
     * @see #convertToSRGB
     */
    private static void convertToSRGB(RenderScript rs, int inputWidth, int inputHeight,
            int inputStride, int cfa, int[] blackLevelPattern, int whiteLevel, byte[] rawImageInput,
            int referenceIlluminant1, int referenceIlluminant2, float[] calibrationTransform1,
            float[] calibrationTransform2, float[] colorMatrix1, float[] colorMatrix2,
            float[] forwardTransform1, float[] forwardTransform2, Rational[/*3*/] neutralColorPoint,
            LensShadingMap lensShadingMap, int outputOffsetX, int outputOffsetY,
            /*out*/Bitmap argbOutput) {

        // Validate arguments
        if (argbOutput == null || rs == null || rawImageInput == null) {
            throw new IllegalArgumentException("Null argument to convertToSRGB");
        }
        if (argbOutput.getConfig() != Bitmap.Config.ARGB_8888) {
            throw new IllegalArgumentException(
                    "Output bitmap passed to convertToSRGB is not ARGB_8888 format");
        }
        if (outputOffsetX < 0 || outputOffsetY < 0) {
            throw new IllegalArgumentException("Negative offset passed to convertToSRGB");
        }
        if ((inputStride / 2) < inputWidth) {
            throw new IllegalArgumentException("Stride too small.");
        }
        if ((inputStride % 2) != 0) {
            throw new IllegalArgumentException("Invalid stride for RAW16 format, see graphics.h.");
        }
        int outWidth = argbOutput.getWidth();
        int outHeight = argbOutput.getHeight();
        if (outWidth + outputOffsetX > inputWidth || outHeight + outputOffsetY > inputHeight) {
            throw new IllegalArgumentException("Raw image with dimensions (w=" + inputWidth +
                    ", h=" + inputHeight + "), cannot converted into sRGB image with dimensions (w="
                    + outWidth + ", h=" + outHeight + ").");
        }
        if (cfa < 0 || cfa > 3) {
            throw new IllegalArgumentException("Unsupported cfa pattern " + cfa + " used.");
        }
        if (DEBUG) {
            Log.d(TAG, "Metadata Used:");
            Log.d(TAG, "Input width,height: " + inputWidth + "," + inputHeight);
            Log.d(TAG, "Output offset x,y: " + outputOffsetX + "," + outputOffsetY);
            Log.d(TAG, "Output width,height: " + outWidth + "," + outHeight);
            Log.d(TAG, "CFA: " + cfa);
            Log.d(TAG, "BlackLevelPattern: " + Arrays.toString(blackLevelPattern));
            Log.d(TAG, "WhiteLevel: " + whiteLevel);
            Log.d(TAG, "ReferenceIlluminant1: " + referenceIlluminant1);
            Log.d(TAG, "ReferenceIlluminant2: " + referenceIlluminant2);
            Log.d(TAG, "CalibrationTransform1: " + Arrays.toString(calibrationTransform1));
            Log.d(TAG, "CalibrationTransform2: " + Arrays.toString(calibrationTransform2));
            Log.d(TAG, "ColorMatrix1: " + Arrays.toString(colorMatrix1));
            Log.d(TAG, "ColorMatrix2: " + Arrays.toString(colorMatrix2));
            Log.d(TAG, "ForwardTransform1: " + Arrays.toString(forwardTransform1));
            Log.d(TAG, "ForwardTransform2: " + Arrays.toString(forwardTransform2));
            Log.d(TAG, "NeutralColorPoint: " + Arrays.toString(neutralColorPoint));
        }

        Allocation gainMap = null;
        if (lensShadingMap != null) {
            float[] lsm = new float[lensShadingMap.getGainFactorCount()];
            lensShadingMap.copyGainFactors(/*inout*/lsm, /*offset*/0);
            gainMap = createFloat4Allocation(rs, lsm, lensShadingMap.getColumnCount(),
                    lensShadingMap.getRowCount());
        }

        float[] normalizedForwardTransform1 = Arrays.copyOf(forwardTransform1,
                forwardTransform1.length);
        normalizeFM(normalizedForwardTransform1);
        float[] normalizedForwardTransform2 = Arrays.copyOf(forwardTransform2,
                forwardTransform2.length);
        normalizeFM(normalizedForwardTransform2);

        float[] normalizedColorMatrix1 = Arrays.copyOf(colorMatrix1, colorMatrix1.length);
        normalizeCM(normalizedColorMatrix1);
        float[] normalizedColorMatrix2 = Arrays.copyOf(colorMatrix2, colorMatrix2.length);
        normalizeCM(normalizedColorMatrix2);

        if (DEBUG) {
            Log.d(TAG, "Normalized ForwardTransform1: " + Arrays.toString(normalizedForwardTransform1));
            Log.d(TAG, "Normalized ForwardTransform2: " + Arrays.toString(normalizedForwardTransform2));
            Log.d(TAG, "Normalized ColorMatrix1: " + Arrays.toString(normalizedColorMatrix1));
            Log.d(TAG, "Normalized ColorMatrix2: " + Arrays.toString(normalizedColorMatrix2));
        }

        // Calculate full sensor colorspace to sRGB colorspace transform.
        double interpolationFactor = findDngInterpolationFactor(referenceIlluminant1,
                referenceIlluminant2, calibrationTransform1, calibrationTransform2,
                normalizedColorMatrix1, normalizedColorMatrix2, neutralColorPoint);
        if (DEBUG) Log.d(TAG, "Interpolation factor used: " + interpolationFactor);
        float[] sensorToXYZ = new float[9];
        calculateCameraToXYZD50Transform(normalizedForwardTransform1, normalizedForwardTransform2,
                calibrationTransform1, calibrationTransform2, neutralColorPoint,
                interpolationFactor, /*out*/sensorToXYZ);
        if (DEBUG) Log.d(TAG, "CameraToXYZ xform used: " + Arrays.toString(sensorToXYZ));
        float[] sensorToProPhoto = new float[9];
        multiply(sXYZtoProPhoto, sensorToXYZ, /*out*/sensorToProPhoto);
        if (DEBUG) Log.d(TAG, "CameraToIntemediate xform used: " + Arrays.toString(sensorToProPhoto));
        Allocation output = Allocation.createFromBitmap(rs, argbOutput);

        float[] proPhotoToSRGB = new float[9];
        multiply(sXYZtoRGBBradford, sProPhotoToXYZ, /*out*/proPhotoToSRGB);

        // Setup input allocation (16-bit raw pixels)
        Type.Builder typeBuilder = new Type.Builder(rs, Element.U16(rs));
        typeBuilder.setX((inputStride / 2));
        typeBuilder.setY(inputHeight);
        Type inputType = typeBuilder.create();
        Allocation input = Allocation.createTyped(rs, inputType);
        input.copyFromUnchecked(rawImageInput);

        // Setup RS kernel globals
        ScriptC_raw_converter converterKernel = new ScriptC_raw_converter(rs);
        converterKernel.set_inputRawBuffer(input);
        converterKernel.set_whiteLevel(whiteLevel);
        converterKernel.set_sensorToIntermediate(new Matrix3f(transpose(sensorToProPhoto)));
        converterKernel.set_intermediateToSRGB(new Matrix3f(transpose(proPhotoToSRGB)));
        converterKernel.set_offsetX(outputOffsetX);
        converterKernel.set_offsetY(outputOffsetY);
        converterKernel.set_rawHeight(inputHeight);
        converterKernel.set_rawWidth(inputWidth);
        converterKernel.set_neutralPoint(new Float3(neutralColorPoint[0].floatValue(),
                neutralColorPoint[1].floatValue(), neutralColorPoint[2].floatValue()));
        converterKernel.set_toneMapCoeffs(new Float4(DEFAULT_ACR3_TONEMAP_CURVE_COEFFS[0],
                DEFAULT_ACR3_TONEMAP_CURVE_COEFFS[1], DEFAULT_ACR3_TONEMAP_CURVE_COEFFS[2],
                DEFAULT_ACR3_TONEMAP_CURVE_COEFFS[3]));
        converterKernel.set_hasGainMap(gainMap != null);
        if (gainMap != null) {
            converterKernel.set_gainMap(gainMap);
            converterKernel.set_gainMapWidth(lensShadingMap.getColumnCount());
            converterKernel.set_gainMapHeight(lensShadingMap.getRowCount());
        }

        converterKernel.set_cfaPattern(cfa);
        converterKernel.set_blackLevelPattern(new Int4(blackLevelPattern[0],
                blackLevelPattern[1], blackLevelPattern[2], blackLevelPattern[3]));
        converterKernel.forEach_convert_RAW_To_ARGB(output);
        output.copyTo(argbOutput);  // Force RS sync with bitmap (does not do an extra copy).
    }

    /**
     * Create a float-backed renderscript {@link Allocation} with the given dimensions, containing
     * the contents of the given float array.
     *
     * @param rs a {@link RenderScript} context to use.
     * @param fArray the float array to copy into the {@link Allocation}.
     * @param width the width of the {@link Allocation}.
     * @param height the height of the {@link Allocation}.
     * @return an {@link Allocation} containing the given floats.
     */
    private static Allocation createFloat4Allocation(RenderScript rs, float[] fArray,
                                                    int width, int height) {
        if (fArray.length != width * height * 4) {
            throw new IllegalArgumentException("Invalid float array of length " + fArray.length +
                    ", must be correct size for Allocation of dimensions " + width + "x" + height);
        }
        Type.Builder builder = new Type.Builder(rs, Element.F32_4(rs));
        builder.setX(width);
        builder.setY(height);
        Allocation fAlloc = Allocation.createTyped(rs, builder.create());
        fAlloc.copyFrom(fArray);
        return fAlloc;
    }

    /**
     * Calculate the correlated color temperature (CCT) for a given x,y chromaticity in CIE 1931 x,y
     * chromaticity space using McCamy's cubic approximation algorithm given in:
     *
     * McCamy, Calvin S. (April 1992).
     * "Correlated color temperature as an explicit function of chromaticity coordinates".
     * Color Research & Application 17 (2): 142–144
     *
     * @param x x chromaticity component.
     * @param y y chromaticity component.
     *
     * @return the CCT associated with this chromaticity coordinate.
     */
    private static double calculateColorTemperature(double x, double y) {
        double n = (x - 0.332) / (y - 0.1858);
        return -449 * Math.pow(n, 3) + 3525 * Math.pow(n, 2) - 6823.3 * n + 5520.33;
    }

    /**
     * Calculate the x,y chromaticity coordinates in CIE 1931 x,y chromaticity space from the given
     * CIE XYZ coordinates.
     *
     * @param X the CIE XYZ X coordinate.
     * @param Y the CIE XYZ Y coordinate.
     * @param Z the CIE XYZ Z coordinate.
     *
     * @return the [x, y] chromaticity coordinates as doubles.
     */
    private static double[] calculateCIExyCoordinates(double X, double Y, double Z) {
        double[] ret = new double[] { 0, 0 };
        ret[0] = X / (X + Y + Z);
        ret[1] = Y / (X + Y + Z);
        return ret;
    }

    /**
     * Linearly interpolate between a and b given fraction f.
     *
     * @param a first term to interpolate between, a will be returned when f == 0.
     * @param b second term to interpolate between, b will be returned when f == 1.
     * @param f the fraction to interpolate by.
     *
     * @return interpolated result as double.
     */
    private static double lerp(double a, double b, double f) {
        return (a * (1.0f - f)) + (b * f);
    }

    /**
     * Linearly interpolate between 3x3 matrices a and b given fraction f.
     *
     * @param a first 3x3 matrix to interpolate between, a will be returned when f == 0.
     * @param b second 3x3 matrix to interpolate between, b will be returned when f == 1.
     * @param f the fraction to interpolate by.
     * @param result will be set to contain the interpolated matrix.
     */
    private static void lerp(float[] a, float[] b, double f, /*out*/float[] result) {
        for (int i = 0; i < 9; i++) {
            result[i] = (float) lerp(a[i], b[i], f);
        }
    }

    /**
     * Convert a 9x9 {@link ColorSpaceTransform} to a matrix and write the matrix into the
     * output.
     *
     * @param xform a {@link ColorSpaceTransform} to transform.
     * @param output the 3x3 matrix to overwrite.
     */
    private static void convertColorspaceTransform(ColorSpaceTransform xform, /*out*/float[] output) {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                output[i * 3 + j] = xform.getElement(j, i).floatValue();
            }
        }
    }

    /**
     * Find the interpolation factor to use with the RAW matrices given a neutral color point.
     *
     * @param referenceIlluminant1 first reference illuminant.
     * @param referenceIlluminant2 second reference illuminant.
     * @param calibrationTransform1 calibration matrix corresponding to the first reference
     *                              illuminant.
     * @param calibrationTransform2 calibration matrix corresponding to the second reference
     *                              illuminant.
     * @param colorMatrix1 color matrix corresponding to the first reference illuminant.
     * @param colorMatrix2 color matrix corresponding to the second reference illuminant.
     * @param neutralColorPoint the neutral color point used to calculate the interpolation factor.
     *
     * @return the interpolation factor corresponding to the given neutral color point.
     */
    private static double findDngInterpolationFactor(int referenceIlluminant1,
            int referenceIlluminant2, float[] calibrationTransform1, float[] calibrationTransform2,
            float[] colorMatrix1, float[] colorMatrix2, Rational[/*3*/] neutralColorPoint) {

        int colorTemperature1 = sStandardIlluminants.get(referenceIlluminant1, NO_ILLUMINANT);
        if (colorTemperature1 == NO_ILLUMINANT) {
            throw new IllegalArgumentException("No such illuminant for reference illuminant 1: " +
                    referenceIlluminant1);
        }

        int colorTemperature2 = sStandardIlluminants.get(referenceIlluminant2, NO_ILLUMINANT);
        if (colorTemperature2 == NO_ILLUMINANT) {
            throw new IllegalArgumentException("No such illuminant for reference illuminant 2: " +
                    referenceIlluminant2);
        }

        if (DEBUG) Log.d(TAG, "ColorTemperature1: " + colorTemperature1);
        if (DEBUG) Log.d(TAG, "ColorTemperature2: " + colorTemperature2);

        double interpFactor = 0.5; // Initial guess for interpolation factor
        double oldInterpFactor = interpFactor;

        double lastDiff = Double.MAX_VALUE;
        double tolerance = 0.0001;
        float[] XYZToCamera1 = new float[9];
        float[] XYZToCamera2 = new float[9];
        multiply(calibrationTransform1, colorMatrix1, /*out*/XYZToCamera1);
        multiply(calibrationTransform2, colorMatrix2, /*out*/XYZToCamera2);

        float[] cameraNeutral = new float[] { neutralColorPoint[0].floatValue(),
                neutralColorPoint[1].floatValue(), neutralColorPoint[2].floatValue()};

        float[] neutralGuess = new float[3];
        float[] interpXYZToCamera = new float[9];
        float[] interpXYZToCameraInverse = new float[9];


        double lower = Math.min(colorTemperature1, colorTemperature2);
        double upper = Math.max(colorTemperature1, colorTemperature2);

        if(DEBUG) {
            Log.d(TAG, "XYZtoCamera1: " + Arrays.toString(XYZToCamera1));
            Log.d(TAG, "XYZtoCamera2: " + Arrays.toString(XYZToCamera2));
            Log.d(TAG, "Finding interpolation factor, initial guess 0.5...");
        }
        // Iteratively guess xy value, find new CCT, and update interpolation factor.
        int loopLimit = 30;
        int count = 0;
        while (lastDiff > tolerance && loopLimit > 0) {
            if (DEBUG) Log.d(TAG, "Loop count " + count);
            lerp(XYZToCamera1, XYZToCamera2, interpFactor, interpXYZToCamera);
            if (!invert(interpXYZToCamera, /*out*/interpXYZToCameraInverse)) {
                throw new IllegalArgumentException(
                        "Cannot invert XYZ to Camera matrix, input matrices are invalid.");
            }

            map(interpXYZToCameraInverse, cameraNeutral, /*out*/neutralGuess);
            double[] xy = calculateCIExyCoordinates(neutralGuess[0], neutralGuess[1],
                    neutralGuess[2]);

            double colorTemperature = calculateColorTemperature(xy[0], xy[1]);

            if (colorTemperature <= lower) {
                interpFactor = 1;
            } else if (colorTemperature >= upper) {
                interpFactor = 0;
            } else {
                double invCT = 1.0 / colorTemperature;
                interpFactor = (invCT - 1.0 / upper) / ( 1.0 / lower - 1.0 / upper);
            }

            if (lower == colorTemperature1) {
                interpFactor = 1.0 - interpFactor;
            }

            interpFactor = (interpFactor + oldInterpFactor) / 2;
            lastDiff = Math.abs(oldInterpFactor - interpFactor);
            oldInterpFactor = interpFactor;
            loopLimit--;
            count++;

            if (DEBUG) {
                Log.d(TAG, "CameraToXYZ chosen: " + Arrays.toString(interpXYZToCameraInverse));
                Log.d(TAG, "XYZ neutral color guess: " + Arrays.toString(neutralGuess));
                Log.d(TAG, "xy coordinate: " + Arrays.toString(xy));
                Log.d(TAG, "xy color temperature: " + colorTemperature);
                Log.d(TAG, "New interpolation factor: " + interpFactor);
            }
        }

        if (loopLimit == 0) {
            Log.w(TAG, "Could not converge on interpolation factor, using factor " + interpFactor +
                    " with remaining error factor of " + lastDiff);
        }
        return interpFactor;
    }

    /**
     * Calculate the transform from the raw camera sensor colorspace to CIE XYZ colorspace with a
     * D50 whitepoint.
     *
     * @param forwardTransform1 forward transform matrix corresponding to the first reference
     *                          illuminant.
     * @param forwardTransform2 forward transform matrix corresponding to the second reference
     *                          illuminant.
     * @param calibrationTransform1 calibration transform matrix corresponding to the first
     *                              reference illuminant.
     * @param calibrationTransform2 calibration transform matrix corresponding to the second
     *                              reference illuminant.
     * @param neutralColorPoint the neutral color point used to calculate the interpolation factor.
     * @param interpolationFactor the interpolation factor to use for the forward and
     *                            calibration transforms.
     * @param outputTransform set to the full sensor to XYZ colorspace transform.
     */
    private static void calculateCameraToXYZD50Transform(float[] forwardTransform1,
            float[] forwardTransform2, float[] calibrationTransform1, float[] calibrationTransform2,
            Rational[/*3*/] neutralColorPoint, double interpolationFactor,
            /*out*/float[] outputTransform) {
        float[] cameraNeutral = new float[] { neutralColorPoint[0].floatValue(),
                neutralColorPoint[1].floatValue(), neutralColorPoint[2].floatValue()};
        if (DEBUG) Log.d(TAG, "Camera neutral: " + Arrays.toString(cameraNeutral));

        float[] interpolatedCC = new float[9];
        lerp(calibrationTransform1, calibrationTransform2, interpolationFactor,
                interpolatedCC);
        float[] inverseInterpolatedCC = new float[9];
        if (!invert(interpolatedCC, /*out*/inverseInterpolatedCC)) {
            throw new IllegalArgumentException( "Cannot invert interpolated calibration transform" +
                    ", input matrices are invalid.");
        }
        if (DEBUG) Log.d(TAG, "Inverted interpolated CalibrationTransform: " +
                Arrays.toString(inverseInterpolatedCC));

        float[] referenceNeutral = new float[3];
        map(inverseInterpolatedCC, cameraNeutral, /*out*/referenceNeutral);
        if (DEBUG) Log.d(TAG, "Reference neutral: " + Arrays.toString(referenceNeutral));
        float maxNeutral = Math.max(Math.max(referenceNeutral[0], referenceNeutral[1]),
                referenceNeutral[2]);
        float[] D = new float[] { maxNeutral/referenceNeutral[0], 0, 0,
                                  0, maxNeutral/referenceNeutral[1], 0,
                                  0, 0, maxNeutral/referenceNeutral[2] };
        if (DEBUG) Log.d(TAG, "Reference Neutral Diagonal: " + Arrays.toString(D));

        float[] intermediate = new float[9];
        float[] intermediate2 = new float[9];

        lerp(forwardTransform1, forwardTransform2, interpolationFactor, /*out*/intermediate);
        if (DEBUG) Log.d(TAG, "Interpolated ForwardTransform: " + Arrays.toString(intermediate));

        multiply(D, inverseInterpolatedCC, /*out*/intermediate2);
        multiply(intermediate, intermediate2, /*out*/outputTransform);
    }

    /**
     * Map a 3d column vector using the given matrix.
     *
     * @param matrix float array containing 3x3 matrix to map vector by.
     * @param input 3 dimensional vector to map.
     * @param output 3 dimensional vector result.
     */
    private static void map(float[] matrix, float[] input, /*out*/float[] output) {
        output[0] = input[0] * matrix[0] + input[1] * matrix[1] + input[2] * matrix[2];
        output[1] = input[0] * matrix[3] + input[1] * matrix[4] + input[2] * matrix[5];
        output[2] = input[0] * matrix[6] + input[1] * matrix[7] + input[2] * matrix[8];
    }

    /**
     * Multiply two 3x3 matrices together: A * B
     *
     * @param a left matrix.
     * @param b right matrix.
     */
    private static void multiply(float[] a, float[] b, /*out*/float[] output) {
        output[0] = a[0] * b[0] + a[1] * b[3] + a[2] * b[6];
        output[3] = a[3] * b[0] + a[4] * b[3] + a[5] * b[6];
        output[6] = a[6] * b[0] + a[7] * b[3] + a[8] * b[6];
        output[1] = a[0] * b[1] + a[1] * b[4] + a[2] * b[7];
        output[4] = a[3] * b[1] + a[4] * b[4] + a[5] * b[7];
        output[7] = a[6] * b[1] + a[7] * b[4] + a[8] * b[7];
        output[2] = a[0] * b[2] + a[1] * b[5] + a[2] * b[8];
        output[5] = a[3] * b[2] + a[4] * b[5] + a[5] * b[8];
        output[8] = a[6] * b[2] + a[7] * b[5] + a[8] * b[8];
    }

    /**
     * Transpose a 3x3 matrix in-place.
     *
     * @param m the matrix to transpose.
     * @return the transposed matrix.
     */
    private static float[] transpose(/*inout*/float[/*9*/] m) {
        float t = m[1];
        m[1] = m[3];
        m[3] = t;
        t = m[2];
        m[2] = m[6];
        m[6] = t;
        t = m[5];
        m[5] = m[7];
        m[7] = t;
        return m;
    }

    /**
     * Invert a 3x3 matrix, or return false if the matrix is singular.
     *
     * @param m matrix to invert.
     * @param output set the output to be the inverse of m.
     */
    private static boolean invert(float[] m, /*out*/float[] output) {
        double a00 = m[0];
        double a01 = m[1];
        double a02 = m[2];
        double a10 = m[3];
        double a11 = m[4];
        double a12 = m[5];
        double a20 = m[6];
        double a21 = m[7];
        double a22 = m[8];

        double t00 = a11 * a22 - a21 * a12;
        double t01 = a21 * a02 - a01 * a22;
        double t02 = a01 * a12 - a11 * a02;
        double t10 = a20 * a12 - a10 * a22;
        double t11 = a00 * a22 - a20 * a02;
        double t12 = a10 * a02 - a00 * a12;
        double t20 = a10 * a21 - a20 * a11;
        double t21 = a20 * a01 - a00 * a21;
        double t22 = a00 * a11 - a10 * a01;

        double det = a00 * t00 + a01 * t10 + a02 * t20;
        if (Math.abs(det) < 1e-9) {
            return false; // Inverse too close to zero, not invertible.
        }

        output[0] = (float) (t00 / det);
        output[1] = (float) (t01 / det);
        output[2] = (float) (t02 / det);
        output[3] = (float) (t10 / det);
        output[4] = (float) (t11 / det);
        output[5] = (float) (t12 / det);
        output[6] = (float) (t20 / det);
        output[7] = (float) (t21 / det);
        output[8] = (float) (t22 / det);
        return true;
    }

    /**
     * Scale each element in a matrix by the given scaling factor.
     *
     * @param factor factor to scale by.
     * @param matrix the float array containing a 3x3 matrix to scale.
     */
    private static void scale(float factor, /*inout*/float[] matrix) {
        for (int i = 0; i < 9; i++) {
            matrix[i] *= factor;
        }
    }

    /**
     * Clamp a value to a given range.
     *
     * @param low lower bound to clamp to.
     * @param high higher bound to clamp to.
     * @param value the value to clamp.
     * @return the clamped value.
     */
    private static double clamp(double low, double high, double value) {
        return Math.max(low, Math.min(high, value));
    }

    /**
     * Return the max float in the array.
     *
     * @param array array of floats to search.
     * @return max float in the array.
     */
    private static float max(float[] array) {
        float val = array[0];
        for (float f : array) {
            val = (f > val) ? f : val;
        }
        return val;
    }

    /**
     * Normalize ColorMatrix to eliminate headroom for input space scaled to [0, 1] using
     * the D50 whitepoint.  This maps the D50 whitepoint into the colorspace used by the
     * ColorMatrix, then uses the resulting whitepoint to renormalize the ColorMatrix so
     * that the channel values in the resulting whitepoint for this operation are clamped
     * to the range [0, 1].
     *
     * @param colorMatrix a 3x3 matrix containing a DNG ColorMatrix to be normalized.
     */
    private static void normalizeCM(/*inout*/float[] colorMatrix) {
        float[] tmp = new float[3];
        map(colorMatrix, D50_XYZ, /*out*/tmp);
        float maxVal = max(tmp);
        if (maxVal > 0) {
            scale(1.0f / maxVal, colorMatrix);
        }
    }

    /**
     * Normalize ForwardMatrix to ensure that sensor whitepoint [1, 1, 1] maps to D50 in CIE XYZ
     * colorspace.
     *
     * @param forwardMatrix a 3x3 matrix containing a DNG ForwardTransform to be normalized.
     */
    private static void normalizeFM(/*inout*/float[] forwardMatrix) {
        float[] tmp = new float[] {1, 1, 1};
        float[] xyz = new float[3];
        map(forwardMatrix, tmp, /*out*/xyz);

        float[] intermediate = new float[9];
        float[] m = new float[] {1.0f / xyz[0], 0, 0, 0, 1.0f / xyz[1], 0, 0, 0, 1.0f / xyz[2]};

        multiply(m, forwardMatrix, /*out*/ intermediate);
        float[] m2 = new float[] {D50_XYZ[0], 0, 0, 0, D50_XYZ[1], 0, 0, 0, D50_XYZ[2]};
        multiply(m2, intermediate, /*out*/forwardMatrix);
    }
}
