// © 2017 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html#License
package com.ibm.icu.number;

import java.math.BigDecimal;
import java.math.MathContext;
import java.math.RoundingMode;

import com.ibm.icu.impl.number.DecimalQuantity;
import com.ibm.icu.impl.number.MultiplierProducer;
import com.ibm.icu.impl.number.RoundingUtils;
import com.ibm.icu.util.Currency;
import com.ibm.icu.util.Currency.CurrencyUsage;

/**
 * A class that defines the rounding strategy to be used when formatting numbers in NumberFormatter.
 *
 * <p>
 * To create a Rounder, use one of the factory methods.
 *
 * @draft ICU 60
 * @provisional This API might change or be removed in a future release.
 * @see NumberFormatter
 */
public abstract class Rounder implements Cloneable {

    /* package-private final */ MathContext mathContext;

    /* package-private */ Rounder() {
        mathContext = RoundingUtils.mathContextUnlimited(RoundingUtils.DEFAULT_ROUNDING_MODE);
    }

    /**
     * Show all available digits to full precision.
     *
     * <p>
     * <strong>NOTE:</strong> When formatting a <em>double</em>, this method, along with {@link #minFraction} and
     * {@link #minDigits}, will trigger complex algorithm similar to <em>Dragon4</em> to determine the low-order digits
     * and the number of digits to display based on the value of the double. If the number of fraction places or
     * significant digits can be bounded, consider using {@link #maxFraction} or {@link #maxDigits} instead to maximize
     * performance. For more information, read the following blog post.
     *
     * <p>
     * http://www.serpentine.com/blog/2011/06/29/here-be-dragons-advances-in-problems-you-didnt-even-know-you-had/
     *
     * @return A Rounder for chaining or passing to the NumberFormatter rounding() setter.
     * @draft ICU 60
     * @provisional This API might change or be removed in a future release.
     * @see NumberFormatter
     */
    public static Rounder unlimited() {
        return constructInfinite();
    }

    /**
     * Show numbers rounded if necessary to the nearest integer.
     *
     * @return A FractionRounder for chaining or passing to the NumberFormatter rounding() setter.
     * @draft ICU 60
     * @provisional This API might change or be removed in a future release.
     * @see NumberFormatter
     */
    public static FractionRounder integer() {
        return constructFraction(0, 0);
    }

    /**
     * Show numbers rounded if necessary to a certain number of fraction places (numerals after the decimal separator).
     * Additionally, pad with zeros to ensure that this number of places are always shown.
     *
     * <p>
     * Example output with minMaxFractionPlaces = 3:
     *
     * <p>
     * 87,650.000<br>
     * 8,765.000<br>
     * 876.500<br>
     * 87.650<br>
     * 8.765<br>
     * 0.876<br>
     * 0.088<br>
     * 0.009<br>
     * 0.000 (zero)
     *
     * <p>
     * This method is equivalent to {@link #minMaxFraction} with both arguments equal.
     *
     * @param minMaxFractionPlaces
     *            The minimum and maximum number of numerals to display after the decimal separator (rounding if too
     *            long or padding with zeros if too short).
     * @return A FractionRounder for chaining or passing to the NumberFormatter rounding() setter.
     * @draft ICU 60
     * @provisional This API might change or be removed in a future release.
     * @see NumberFormatter
     */
    public static FractionRounder fixedFraction(int minMaxFractionPlaces) {
        if (minMaxFractionPlaces >= 0 && minMaxFractionPlaces <= RoundingUtils.MAX_INT_FRAC_SIG) {
            return constructFraction(minMaxFractionPlaces, minMaxFractionPlaces);
        } else {
            throw new IllegalArgumentException(
                    "Fraction length must be between 0 and " + RoundingUtils.MAX_INT_FRAC_SIG);
        }
    }

    /**
     * Always show at least a certain number of fraction places after the decimal separator, padding with zeros if
     * necessary. Do not perform rounding (display numbers to their full precision).
     *
     * <p>
     * <strong>NOTE:</strong> If you are formatting <em>doubles</em>, see the performance note in {@link #unlimited}.
     *
     * @param minFractionPlaces
     *            The minimum number of numerals to display after the decimal separator (padding with zeros if
     *            necessary).
     * @return A FractionRounder for chaining or passing to the NumberFormatter rounding() setter.
     * @draft ICU 60
     * @provisional This API might change or be removed in a future release.
     * @see NumberFormatter
     */
    public static FractionRounder minFraction(int minFractionPlaces) {
        if (minFractionPlaces >= 0 && minFractionPlaces < RoundingUtils.MAX_INT_FRAC_SIG) {
            return constructFraction(minFractionPlaces, -1);
        } else {
            throw new IllegalArgumentException(
                    "Fraction length must be between 0 and " + RoundingUtils.MAX_INT_FRAC_SIG);
        }
    }

    /**
     * Show numbers rounded if necessary to a certain number of fraction places (numerals after the decimal separator).
     * Unlike the other fraction rounding strategies, this strategy does <em>not</em> pad zeros to the end of the
     * number.
     *
     * @param maxFractionPlaces
     *            The maximum number of numerals to display after the decimal mark (rounding if necessary).
     * @return A FractionRounder for chaining or passing to the NumberFormatter rounding() setter.
     * @draft ICU 60
     * @provisional This API might change or be removed in a future release.
     * @see NumberFormatter
     */
    public static FractionRounder maxFraction(int maxFractionPlaces) {
        if (maxFractionPlaces >= 0 && maxFractionPlaces < RoundingUtils.MAX_INT_FRAC_SIG) {
            return constructFraction(0, maxFractionPlaces);
        } else {
            throw new IllegalArgumentException(
                    "Fraction length must be between 0 and " + RoundingUtils.MAX_INT_FRAC_SIG);
        }
    }

    /**
     * Show numbers rounded if necessary to a certain number of fraction places (numerals after the decimal separator);
     * in addition, always show at least a certain number of places after the decimal separator, padding with zeros if
     * necessary.
     *
     * @param minFractionPlaces
     *            The minimum number of numerals to display after the decimal separator (padding with zeros if
     *            necessary).
     * @param maxFractionPlaces
     *            The maximum number of numerals to display after the decimal separator (rounding if necessary).
     * @return A FractionRounder for chaining or passing to the NumberFormatter rounding() setter.
     * @draft ICU 60
     * @provisional This API might change or be removed in a future release.
     * @see NumberFormatter
     */
    public static FractionRounder minMaxFraction(int minFractionPlaces, int maxFractionPlaces) {
        if (minFractionPlaces >= 0 && maxFractionPlaces <= RoundingUtils.MAX_INT_FRAC_SIG
                && minFractionPlaces <= maxFractionPlaces) {
            return constructFraction(minFractionPlaces, maxFractionPlaces);
        } else {
            throw new IllegalArgumentException(
                    "Fraction length must be between 0 and " + RoundingUtils.MAX_INT_FRAC_SIG);
        }
    }

    /**
     * Show numbers rounded if necessary to a certain number of significant digits or significant figures. Additionally,
     * pad with zeros to ensure that this number of significant digits/figures are always shown.
     *
     * <p>
     * This method is equivalent to {@link #minMaxDigits} with both arguments equal.
     *
     * @param minMaxSignificantDigits
     *            The minimum and maximum number of significant digits to display (rounding if too long or padding with
     *            zeros if too short).
     * @return A Rounder for chaining or passing to the NumberFormatter rounding() setter.
     * @draft ICU 60
     * @provisional This API might change or be removed in a future release.
     * @see NumberFormatter
     */
    public static Rounder fixedDigits(int minMaxSignificantDigits) {
        if (minMaxSignificantDigits > 0 && minMaxSignificantDigits <= RoundingUtils.MAX_INT_FRAC_SIG) {
            return constructSignificant(minMaxSignificantDigits, minMaxSignificantDigits);
        } else {
            throw new IllegalArgumentException(
                    "Significant digits must be between 0 and " + RoundingUtils.MAX_INT_FRAC_SIG);
        }
    }

    /**
     * Always show at least a certain number of significant digits/figures, padding with zeros if necessary. Do not
     * perform rounding (display numbers to their full precision).
     *
     * <p>
     * <strong>NOTE:</strong> If you are formatting <em>doubles</em>, see the performance note in {@link #unlimited}.
     *
     * @param minSignificantDigits
     *            The minimum number of significant digits to display (padding with zeros if too short).
     * @return A Rounder for chaining or passing to the NumberFormatter rounding() setter.
     * @draft ICU 60
     * @provisional This API might change or be removed in a future release.
     * @see NumberFormatter
     */
    public static Rounder minDigits(int minSignificantDigits) {
        if (minSignificantDigits > 0 && minSignificantDigits <= RoundingUtils.MAX_INT_FRAC_SIG) {
            return constructSignificant(minSignificantDigits, -1);
        } else {
            throw new IllegalArgumentException(
                    "Significant digits must be between 0 and " + RoundingUtils.MAX_INT_FRAC_SIG);
        }
    }

    /**
     * Show numbers rounded if necessary to a certain number of significant digits/figures.
     *
     * @param maxSignificantDigits
     *            The maximum number of significant digits to display (rounding if too long).
     * @return A Rounder for chaining or passing to the NumberFormatter rounding() setter.
     * @draft ICU 60
     * @provisional This API might change or be removed in a future release.
     * @see NumberFormatter
     */
    public static Rounder maxDigits(int maxSignificantDigits) {
        if (maxSignificantDigits > 0 && maxSignificantDigits <= RoundingUtils.MAX_INT_FRAC_SIG) {
            return constructSignificant(0, maxSignificantDigits);
        } else {
            throw new IllegalArgumentException(
                    "Significant digits must be between 0 and " + RoundingUtils.MAX_INT_FRAC_SIG);
        }
    }

    /**
     * Show numbers rounded if necessary to a certain number of significant digits/figures; in addition, always show at
     * least a certain number of significant digits, padding with zeros if necessary.
     *
     * @param minSignificantDigits
     *            The minimum number of significant digits to display (padding with zeros if necessary).
     * @param maxSignificantDigits
     *            The maximum number of significant digits to display (rounding if necessary).
     * @return A Rounder for chaining or passing to the NumberFormatter rounding() setter.
     * @draft ICU 60
     * @provisional This API might change or be removed in a future release.
     * @see NumberFormatter
     */
    public static Rounder minMaxDigits(int minSignificantDigits, int maxSignificantDigits) {
        if (minSignificantDigits > 0 && maxSignificantDigits <= RoundingUtils.MAX_INT_FRAC_SIG
                && minSignificantDigits <= maxSignificantDigits) {
            return constructSignificant(minSignificantDigits, maxSignificantDigits);
        } else {
            throw new IllegalArgumentException(
                    "Significant digits must be between 0 and " + RoundingUtils.MAX_INT_FRAC_SIG);
        }
    }

    /**
     * Show numbers rounded if necessary to the closest multiple of a certain rounding increment. For example, if the
     * rounding increment is 0.5, then round 1.2 to 1 and round 1.3 to 1.5.
     *
     * <p>
     * In order to ensure that numbers are padded to the appropriate number of fraction places, set the scale on the
     * rounding increment BigDecimal. For example, to round to the nearest 0.5 and always display 2 numerals after the
     * decimal separator (to display 1.2 as "1.00" and 1.3 as "1.50"), you can run:
     *
     * <pre>
     * Rounder.increment(new BigDecimal("0.50"))
     * </pre>
     *
     * <p>
     * For more information on the scale of Java BigDecimal, see {@link java.math.BigDecimal#scale()}.
     *
     * @param roundingIncrement
     *            The increment to which to round numbers.
     * @return A Rounder for chaining or passing to the NumberFormatter rounding() setter.
     * @draft ICU 60
     * @provisional This API might change or be removed in a future release.
     * @see NumberFormatter
     */
    public static Rounder increment(BigDecimal roundingIncrement) {
        if (roundingIncrement != null && roundingIncrement.compareTo(BigDecimal.ZERO) > 0) {
            return constructIncrement(roundingIncrement);
        } else {
            throw new IllegalArgumentException("Rounding increment must be positive and non-null");
        }
    }

    /**
     * Show numbers rounded and padded according to the rules for the currency unit. The most common rounding settings
     * for currencies include <code>Rounder.fixedFraction(2)</code>, <code>Rounder.integer()</code>, and
     * <code>Rounder.increment(0.05)</code> for cash transactions ("nickel rounding").
     *
     * <p>
     * The exact rounding details will be resolved at runtime based on the currency unit specified in the
     * NumberFormatter chain. To round according to the rules for one currency while displaying the symbol for another
     * currency, the withCurrency() method can be called on the return value of this method.
     *
     * @param currencyUsage
     *            Either STANDARD (for digital transactions) or CASH (for transactions where the rounding increment may
     *            be limited by the available denominations of cash or coins).
     * @return A CurrencyRounder for chaining or passing to the NumberFormatter rounding() setter.
     * @draft ICU 60
     * @provisional This API might change or be removed in a future release.
     * @see NumberFormatter
     */
    public static CurrencyRounder currency(CurrencyUsage currencyUsage) {
        if (currencyUsage != null) {
            return constructCurrency(currencyUsage);
        } else {
            throw new IllegalArgumentException("CurrencyUsage must be non-null");
        }
    }

    /**
     * Sets the {@link java.math.RoundingMode} to use when picking the direction to round (up or down). Common values
     * include HALF_EVEN, HALF_UP, and FLOOR. The default is HALF_EVEN.
     *
     * @param roundingMode
     *            The RoundingMode to use.
     * @return A Rounder for chaining.
     * @draft ICU 60
     * @provisional This API might change or be removed in a future release.
     * @see NumberFormatter
     */
    public Rounder withMode(RoundingMode roundingMode) {
        return withMode(RoundingUtils.mathContextUnlimited(roundingMode));
    }

    /**
     * Sets a MathContext directly instead of RoundingMode.
     *
     * @internal
     * @deprecated This API is ICU internal only.
     */
    @Deprecated
    public Rounder withMode(MathContext mathContext) {
        if (this.mathContext.equals(mathContext)) {
            return this;
        }
        Rounder other = (Rounder) this.clone();
        other.mathContext = mathContext;
        return other;
    }

    /**
     * @internal
     * @deprecated This API is ICU internal only.
     */
    @Deprecated
    @Override
    public Object clone() {
        try {
            return super.clone();
        } catch (CloneNotSupportedException e) {
            // Should not happen since parent is Object
            throw new AssertionError(e);
        }
    }

    /**
     * @internal
     * @deprecated ICU 60 This API is ICU internal only.
     */
    @Deprecated
    public abstract void apply(DecimalQuantity value);

    //////////////////////////
    // PACKAGE-PRIVATE APIS //
    //////////////////////////

    static final InfiniteRounderImpl NONE = new InfiniteRounderImpl();

    static final FractionRounderImpl FIXED_FRAC_0 = new FractionRounderImpl(0, 0);
    static final FractionRounderImpl FIXED_FRAC_2 = new FractionRounderImpl(2, 2);
    static final FractionRounderImpl MAX_FRAC_6 = new FractionRounderImpl(0, 6);

    static final SignificantRounderImpl FIXED_SIG_2 = new SignificantRounderImpl(2, 2);
    static final SignificantRounderImpl FIXED_SIG_3 = new SignificantRounderImpl(3, 3);
    static final SignificantRounderImpl RANGE_SIG_2_3 = new SignificantRounderImpl(2, 3);

    static final FracSigRounderImpl COMPACT_STRATEGY = new FracSigRounderImpl(0, 0, 2, -1);

    static final IncrementRounderImpl NICKEL = new IncrementRounderImpl(BigDecimal.valueOf(0.05));

    static final CurrencyRounderImpl MONETARY_STANDARD = new CurrencyRounderImpl(CurrencyUsage.STANDARD);
    static final CurrencyRounderImpl MONETARY_CASH = new CurrencyRounderImpl(CurrencyUsage.CASH);

    static final PassThroughRounderImpl PASS_THROUGH = new PassThroughRounderImpl();

    static Rounder constructInfinite() {
        return NONE;
    }

    static FractionRounder constructFraction(int minFrac, int maxFrac) {
        if (minFrac == 0 && maxFrac == 0) {
            return FIXED_FRAC_0;
        } else if (minFrac == 2 && maxFrac == 2) {
            return FIXED_FRAC_2;
        } else if (minFrac == 0 && maxFrac == 6) {
            return MAX_FRAC_6;
        } else {
            return new FractionRounderImpl(minFrac, maxFrac);
        }
    }

    /** Assumes that minSig <= maxSig. */
    static Rounder constructSignificant(int minSig, int maxSig) {
        if (minSig == 2 && maxSig == 2) {
            return FIXED_SIG_2;
        } else if (minSig == 3 && maxSig == 3) {
            return FIXED_SIG_3;
        } else if (minSig == 2 && maxSig == 3) {
            return RANGE_SIG_2_3;
        } else {
            return new SignificantRounderImpl(minSig, maxSig);
        }
    }

    static Rounder constructFractionSignificant(FractionRounder base_, int minSig, int maxSig) {
        assert base_ instanceof FractionRounderImpl;
        FractionRounderImpl base = (FractionRounderImpl) base_;
        if (base.minFrac == 0 && base.maxFrac == 0 && minSig == 2 /* && maxSig == -1 */) {
            return COMPACT_STRATEGY;
        } else {
            return new FracSigRounderImpl(base.minFrac, base.maxFrac, minSig, maxSig);
        }
    }

    static Rounder constructIncrement(BigDecimal increment) {
        // NOTE: .equals() is what we want, not .compareTo()
        if (increment.equals(NICKEL.increment)) {
            return NICKEL;
        } else {
            return new IncrementRounderImpl(increment);
        }
    }

    static CurrencyRounder constructCurrency(CurrencyUsage usage) {
        if (usage == CurrencyUsage.STANDARD) {
            return MONETARY_STANDARD;
        } else if (usage == CurrencyUsage.CASH) {
            return MONETARY_CASH;
        } else {
            throw new AssertionError();
        }
    }

    static Rounder constructFromCurrency(CurrencyRounder base_, Currency currency) {
        assert base_ instanceof CurrencyRounderImpl;
        CurrencyRounderImpl base = (CurrencyRounderImpl) base_;
        double incrementDouble = currency.getRoundingIncrement(base.usage);
        if (incrementDouble != 0.0) {
            BigDecimal increment = BigDecimal.valueOf(incrementDouble);
            return constructIncrement(increment);
        } else {
            int minMaxFrac = currency.getDefaultFractionDigits(base.usage);
            return constructFraction(minMaxFrac, minMaxFrac);
        }
    }

    static Rounder constructPassThrough() {
        return PASS_THROUGH;
    }

    /**
     * Returns a valid working Rounder. If the Rounder is a CurrencyRounder, applies the given currency. Otherwise,
     * simply passes through the argument.
     *
     * @param currency
     *            A currency object to use in case the input object needs it.
     * @return A Rounder object ready for use.
     */
    Rounder withLocaleData(Currency currency) {
        if (this instanceof CurrencyRounder) {
            return ((CurrencyRounder) this).withCurrency(currency);
        } else {
            return this;
        }
    }

    int chooseMultiplierAndApply(DecimalQuantity input, MultiplierProducer producer) {
        // TODO: Make a better and more efficient implementation.
        // TODO: Avoid the object creation here.
        DecimalQuantity copy = input.createCopy();

        assert !input.isZero();
        int magnitude = input.getMagnitude();
        int multiplier = producer.getMultiplier(magnitude);
        input.adjustMagnitude(multiplier);
        apply(input);

        // If the number turned to zero when rounding, do not re-attempt the rounding.
        if (!input.isZero() && input.getMagnitude() == magnitude + multiplier + 1) {
            magnitude += 1;
            input.copyFrom(copy);
            multiplier = producer.getMultiplier(magnitude);
            input.adjustMagnitude(multiplier);
            assert input.getMagnitude() == magnitude + multiplier - 1;
            apply(input);
            assert input.getMagnitude() == magnitude + multiplier;
        }

        return multiplier;
    }

    ///////////////
    // INTERNALS //
    ///////////////

    static class InfiniteRounderImpl extends Rounder {

        public InfiniteRounderImpl() {
        }

        @Override
        public void apply(DecimalQuantity value) {
            value.roundToInfinity();
            value.setFractionLength(0, Integer.MAX_VALUE);
        }
    }

    static class FractionRounderImpl extends FractionRounder {
        final int minFrac;
        final int maxFrac;

        public FractionRounderImpl(int minFrac, int maxFrac) {
            this.minFrac = minFrac;
            this.maxFrac = maxFrac;
        }

        @Override
        public void apply(DecimalQuantity value) {
            value.roundToMagnitude(getRoundingMagnitudeFraction(maxFrac), mathContext);
            value.setFractionLength(Math.max(0, -getDisplayMagnitudeFraction(minFrac)), Integer.MAX_VALUE);
        }
    }

    static class SignificantRounderImpl extends Rounder {
        final int minSig;
        final int maxSig;

        public SignificantRounderImpl(int minSig, int maxSig) {
            this.minSig = minSig;
            this.maxSig = maxSig;
        }

        @Override
        public void apply(DecimalQuantity value) {
            value.roundToMagnitude(getRoundingMagnitudeSignificant(value, maxSig), mathContext);
            value.setFractionLength(Math.max(0, -getDisplayMagnitudeSignificant(value, minSig)), Integer.MAX_VALUE);
        }

        /** Version of {@link #apply} that obeys minInt constraints. Used for scientific notation compatibility mode. */
        public void apply(DecimalQuantity quantity, int minInt) {
            assert quantity.isZero();
            quantity.setFractionLength(minSig - minInt, Integer.MAX_VALUE);
        }
    }

    static class FracSigRounderImpl extends Rounder {
        final int minFrac;
        final int maxFrac;
        final int minSig;
        final int maxSig;

        public FracSigRounderImpl(int minFrac, int maxFrac, int minSig, int maxSig) {
            this.minFrac = minFrac;
            this.maxFrac = maxFrac;
            this.minSig = minSig;
            this.maxSig = maxSig;
        }

        @Override
        public void apply(DecimalQuantity value) {
            int displayMag = getDisplayMagnitudeFraction(minFrac);
            int roundingMag = getRoundingMagnitudeFraction(maxFrac);
            if (minSig == -1) {
                // Max Sig override
                int candidate = getRoundingMagnitudeSignificant(value, maxSig);
                roundingMag = Math.max(roundingMag, candidate);
            } else {
                // Min Sig override
                int candidate = getDisplayMagnitudeSignificant(value, minSig);
                roundingMag = Math.min(roundingMag, candidate);
            }
            value.roundToMagnitude(roundingMag, mathContext);
            value.setFractionLength(Math.max(0, -displayMag), Integer.MAX_VALUE);
        }
    }

    static class IncrementRounderImpl extends Rounder {
        final BigDecimal increment;

        public IncrementRounderImpl(BigDecimal increment) {
            this.increment = increment;
        }

        @Override
        public void apply(DecimalQuantity value) {
            value.roundToIncrement(increment, mathContext);
            value.setFractionLength(increment.scale(), increment.scale());
        }
    }

    static class CurrencyRounderImpl extends CurrencyRounder {
        final CurrencyUsage usage;

        public CurrencyRounderImpl(CurrencyUsage usage) {
            this.usage = usage;
        }

        @Override
        public void apply(DecimalQuantity value) {
            // Call .withCurrency() before .apply()!
            throw new AssertionError();
        }
    }

    static class PassThroughRounderImpl extends Rounder {

        public PassThroughRounderImpl() {
        }

        @Override
        public void apply(DecimalQuantity value) {
            // TODO: Assert that value has already been rounded
        }
    }

    private static int getRoundingMagnitudeFraction(int maxFrac) {
        if (maxFrac == -1) {
            return Integer.MIN_VALUE;
        }
        return -maxFrac;
    }

    private static int getRoundingMagnitudeSignificant(DecimalQuantity value, int maxSig) {
        if (maxSig == -1) {
            return Integer.MIN_VALUE;
        }
        int magnitude = value.isZero() ? 0 : value.getMagnitude();
        return magnitude - maxSig + 1;
    }

    private static int getDisplayMagnitudeFraction(int minFrac) {
        if (minFrac == 0) {
            return Integer.MAX_VALUE;
        }
        return -minFrac;
    }

    private static int getDisplayMagnitudeSignificant(DecimalQuantity value, int minSig) {
        int magnitude = value.isZero() ? 0 : value.getMagnitude();
        return magnitude - minSig + 1;
    }
}
