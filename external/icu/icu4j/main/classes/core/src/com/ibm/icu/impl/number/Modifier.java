// © 2017 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html#License
package com.ibm.icu.impl.number;

/**
 * A Modifier is an object that can be passed through the formatting pipeline until it is finally applied to the string
 * builder. A Modifier usually contains a prefix and a suffix that are applied, but it could contain something else,
 * like a {@link com.ibm.icu.text.SimpleFormatter} pattern.
 *
 * A Modifier is usually immutable, except in cases such as {@link MutablePatternModifier}, which are mutable for performance
 * reasons.
 */
public interface Modifier {

    /**
     * Apply this Modifier to the string builder.
     *
     * @param output
     *            The string builder to which to apply this modifier.
     * @param leftIndex
     *            The left index of the string within the builder. Equal to 0 when only one number is being formatted.
     * @param rightIndex
     *            The right index of the string within the string builder. Equal to length when only one number is being
     *            formatted.
     * @return The number of characters (UTF-16 code units) that were added to the string builder.
     */
    public int apply(NumberStringBuilder output, int leftIndex, int rightIndex);

    /**
     * Gets the length of the prefix. This information can be used in combination with {@link #apply} to extract the
     * prefix and suffix strings.
     *
     * @return The number of characters (UTF-16 code units) in the prefix.
     */
    public int getPrefixLength();

    /**
     * Returns the number of code points in the modifier, prefix plus suffix.
     */
    public int getCodePointCount();

    /**
     * Whether this modifier is strong. If a modifier is strong, it should always be applied immediately and not allowed
     * to bubble up. With regard to padding, strong modifiers are considered to be on the inside of the prefix and
     * suffix.
     *
     * @return Whether the modifier is strong.
     */
    public boolean isStrong();
}
