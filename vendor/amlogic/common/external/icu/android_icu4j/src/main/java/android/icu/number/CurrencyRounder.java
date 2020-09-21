/* GENERATED SOURCE. DO NOT MODIFY. */
// © 2017 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html#License
package android.icu.number;

import android.icu.util.Currency;

/**
 * A class that defines a rounding strategy parameterized by a currency to be used when formatting numbers in
 * NumberFormatter.
 *
 * <p>
 * To create a CurrencyRounder, use one of the factory methods on Rounder.
 *
 * @see NumberFormatter
 * @hide Only a subset of ICU is exposed in Android
 * @hide draft / provisional / internal are hidden on Android
 */
public abstract class CurrencyRounder extends Rounder {

    /* package-private */ CurrencyRounder() {
    }

    /**
     * Associates a currency with this rounding strategy.
     *
     * <p>
     * <strong>Calling this method is <em>not required</em></strong>, because the currency specified in unit() or via a
     * CurrencyAmount passed into format(Measure) is automatically applied to currency rounding strategies. However,
     * this method enables you to override that automatic association.
     *
     * <p>
     * This method also enables numbers to be formatted using currency rounding rules without explicitly using a
     * currency format.
     *
     * @param currency
     *            The currency to associate with this rounding strategy.
     * @return A Rounder for chaining or passing to the NumberFormatter rounding() setter.
     * @see NumberFormatter
     * @hide draft / provisional / internal are hidden on Android
     */
    public Rounder withCurrency(Currency currency) {
        if (currency != null) {
            return constructFromCurrency(this, currency);
        } else {
            throw new IllegalArgumentException("Currency must not be null");
        }
    };
}