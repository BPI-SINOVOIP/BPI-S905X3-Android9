// © 2017 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html#License
package com.ibm.icu.number;

import com.ibm.icu.impl.number.RoundingUtils;

/**
 * A class that defines a rounding strategy based on a number of fraction places and optionally significant digits to be
 * used when formatting numbers in NumberFormatter.
 *
 * <p>
 * To create a FractionRounder, use one of the factory methods on Rounder.
 *
 * @draft ICU 60
 * @provisional This API might change or be removed in a future release.
 * @see NumberFormatter
 */
public abstract class FractionRounder extends Rounder {

    /* package-private */ FractionRounder() {
    }

    /**
     * Ensure that no less than this number of significant digits are retained when rounding according to fraction
     * rules.
     *
     * <p>
     * For example, with integer rounding, the number 3.141 becomes "3". However, with minimum figures set to 2, 3.141
     * becomes "3.1" instead.
     *
     * <p>
     * This setting does not affect the number of trailing zeros. For example, 3.01 would print as "3", not "3.0".
     *
     * @param minSignificantDigits
     *            The number of significant figures to guarantee.
     * @return A Rounder for chaining or passing to the NumberFormatter rounding() setter.
     * @draft ICU 60
     * @provisional This API might change or be removed in a future release.
     * @see NumberFormatter
     */
    public Rounder withMinDigits(int minSignificantDigits) {
        if (minSignificantDigits > 0 && minSignificantDigits <= RoundingUtils.MAX_INT_FRAC_SIG) {
            return constructFractionSignificant(this, minSignificantDigits, -1);
        } else {
            throw new IllegalArgumentException(
                    "Significant digits must be between 0 and " + RoundingUtils.MAX_INT_FRAC_SIG);
        }
    }

    /**
     * Ensure that no more than this number of significant digits are retained when rounding according to fraction
     * rules.
     *
     * <p>
     * For example, with integer rounding, the number 123.4 becomes "123". However, with maximum figures set to 2, 123.4
     * becomes "120" instead.
     *
     * <p>
     * This setting does not affect the number of trailing zeros. For example, with fixed fraction of 2, 123.4 would
     * become "120.00".
     *
     * @param maxSignificantDigits
     *            Round the number to no more than this number of significant figures.
     * @return A Rounder for chaining or passing to the NumberFormatter rounding() setter.
     * @draft ICU 60
     * @provisional This API might change or be removed in a future release.
     * @see NumberFormatter
     */
    public Rounder withMaxDigits(int maxSignificantDigits) {
        if (maxSignificantDigits > 0 && maxSignificantDigits <= RoundingUtils.MAX_INT_FRAC_SIG) {
            return constructFractionSignificant(this, -1, maxSignificantDigits);
        } else {
            throw new IllegalArgumentException(
                    "Significant digits must be between 0 and " + RoundingUtils.MAX_INT_FRAC_SIG);
        }
    }
}