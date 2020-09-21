/* GENERATED SOURCE. DO NOT MODIFY. */
// © 2017 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html#License
package android.icu.impl.number;

/**
 * An interface used by compact notation and scientific notation to choose a multiplier while rounding.
 * @hide Only a subset of ICU is exposed in Android
 */
public interface MultiplierProducer {
    int getMultiplier(int magnitude);
}
