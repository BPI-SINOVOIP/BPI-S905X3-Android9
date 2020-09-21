/* GENERATED SOURCE. DO NOT MODIFY. */
// © 2017 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html#License
package android.icu.number;

import java.io.IOException;
import java.math.BigDecimal;
import java.text.AttributedCharacterIterator;
import java.text.FieldPosition;
import java.util.Arrays;

import android.icu.impl.number.DecimalQuantity;
import android.icu.impl.number.MicroProps;
import android.icu.impl.number.NumberStringBuilder;
import android.icu.text.PluralRules.IFixedDecimal;
import android.icu.util.ICUUncheckedIOException;

/**
 * The result of a number formatting operation. This class allows the result to be exported in several data types,
 * including a String, an AttributedCharacterIterator, and a BigDecimal.
 *
 * @see NumberFormatter
 * @hide Only a subset of ICU is exposed in Android
 * @hide draft / provisional / internal are hidden on Android
 */
public class FormattedNumber {
    NumberStringBuilder nsb;
    DecimalQuantity fq;
    MicroProps micros;

    FormattedNumber(NumberStringBuilder nsb, DecimalQuantity fq, MicroProps micros) {
        this.nsb = nsb;
        this.fq = fq;
        this.micros = micros;
    }

    /**
     * Creates a String representation of the the formatted number.
     *
     * @return a String containing the localized number.
     * @see NumberFormatter
     * @hide draft / provisional / internal are hidden on Android
     */
    @Override
    public String toString() {
        return nsb.toString();
    }

    /**
     * Append the formatted number to an Appendable, such as a StringBuilder. This may be slightly more efficient than
     * creating a String.
     *
     * <p>
     * If an IOException occurs when appending to the Appendable, an unchecked {@link ICUUncheckedIOException} is thrown
     * instead.
     *
     * @param appendable
     *            The Appendable to which to append the formatted number string.
     * @return The same Appendable, for chaining.
     * @see Appendable
     * @see NumberFormatter
     * @hide draft / provisional / internal are hidden on Android
     */
    public <A extends Appendable> A appendTo(A appendable) {
        try {
            appendable.append(nsb);
        } catch (IOException e) {
            // Throw as an unchecked exception to avoid users needing try/catch
            throw new ICUUncheckedIOException(e);
        }
        return appendable;
    }

    /**
     * Determine the start and end indices of the first occurrence of the given <em>field</em> in the output string.
     * This allows you to determine the locations of the integer part, fraction part, and sign.
     *
     * <p>
     * If multiple different field attributes are needed, this method can be called repeatedly, or if <em>all</em> field
     * attributes are needed, consider using getFieldIterator().
     *
     * <p>
     * If a field occurs multiple times in an output string, such as a grouping separator, this method will only ever
     * return the first occurrence. Use getFieldIterator() to access all occurrences of an attribute.
     *
     * @param fieldPosition
     *            The FieldPosition to populate with the start and end indices of the desired field.
     * @see android.icu.text.NumberFormat.Field
     * @see NumberFormatter
     * @hide draft / provisional / internal are hidden on Android
     */
    public void populateFieldPosition(FieldPosition fieldPosition) {
        populateFieldPosition(fieldPosition, 0);
    }

    /**
     * @deprecated This API is ICU internal only.
     * @hide draft / provisional / internal are hidden on Android
     */
    @Deprecated
    public void populateFieldPosition(FieldPosition fieldPosition, int offset) {
        nsb.populateFieldPosition(fieldPosition, offset);
        fq.populateUFieldPosition(fieldPosition);
    }

    /**
     * Export the formatted number as an AttributedCharacterIterator. This allows you to determine which characters in
     * the output string correspond to which <em>fields</em>, such as the integer part, fraction part, and sign.
     *
     * <p>
     * If information on only one field is needed, consider using populateFieldPosition() instead.
     *
     * @return An AttributedCharacterIterator, containing information on the field attributes of the number string.
     * @see android.icu.text.NumberFormat.Field
     * @see AttributedCharacterIterator
     * @see NumberFormatter
     * @hide draft / provisional / internal are hidden on Android
     */
    public AttributedCharacterIterator getFieldIterator() {
        return nsb.getIterator();
    }

    /**
     * Export the formatted number as a BigDecimal. This endpoint is useful for obtaining the exact number being printed
     * after scaling and rounding have been applied by the number formatting pipeline.
     *
     * @return A BigDecimal representation of the formatted number.
     * @see NumberFormatter
     * @hide draft / provisional / internal are hidden on Android
     */
    public BigDecimal toBigDecimal() {
        return fq.toBigDecimal();
    }

    /**
     * @deprecated This API is ICU internal only.
     * @hide draft / provisional / internal are hidden on Android
     */
    @Deprecated
    public String getPrefix() {
        NumberStringBuilder temp = new NumberStringBuilder();
        int length = micros.modOuter.apply(temp, 0, 0);
        length += micros.modMiddle.apply(temp, 0, length);
        /* length += */ micros.modInner.apply(temp, 0, length);
        int prefixLength = micros.modOuter.getPrefixLength() + micros.modMiddle.getPrefixLength()
                + micros.modInner.getPrefixLength();
        return temp.subSequence(0, prefixLength).toString();
    }

    /**
     * @deprecated This API is ICU internal only.
     * @hide draft / provisional / internal are hidden on Android
     */
    @Deprecated
    public String getSuffix() {
        NumberStringBuilder temp = new NumberStringBuilder();
        int length = micros.modOuter.apply(temp, 0, 0);
        length += micros.modMiddle.apply(temp, 0, length);
        length += micros.modInner.apply(temp, 0, length);
        int prefixLength = micros.modOuter.getPrefixLength() + micros.modMiddle.getPrefixLength()
                + micros.modInner.getPrefixLength();
        return temp.subSequence(prefixLength, length).toString();
    }

    /**
     * @deprecated This API is ICU internal only.
     * @hide draft / provisional / internal are hidden on Android
     */
    @Deprecated
    public IFixedDecimal getFixedDecimal() {
        return fq;
    }

    /**
     * {@inheritDoc}
     *
     * @hide draft / provisional / internal are hidden on Android
     */
    @Override
    public int hashCode() {
        // NumberStringBuilder and BigDecimal are mutable, so we can't call
        // #equals() or #hashCode() on them directly.
        return Arrays.hashCode(nsb.toCharArray()) ^ Arrays.hashCode(nsb.toFieldArray()) ^ fq.toBigDecimal().hashCode();
    }

    /**
     * {@inheritDoc}
     *
     * @hide draft / provisional / internal are hidden on Android
     */
    @Override
    public boolean equals(Object other) {
        if (this == other)
            return true;
        if (other == null)
            return false;
        if (!(other instanceof FormattedNumber))
            return false;
        // NumberStringBuilder and BigDecimal are mutable, so we can't call
        // #equals() or #hashCode() on them directly.
        FormattedNumber _other = (FormattedNumber) other;
        return Arrays.equals(nsb.toCharArray(), _other.nsb.toCharArray())
                ^ Arrays.equals(nsb.toFieldArray(), _other.nsb.toFieldArray())
                ^ fq.toBigDecimal().equals(_other.fq.toBigDecimal());
    }
}