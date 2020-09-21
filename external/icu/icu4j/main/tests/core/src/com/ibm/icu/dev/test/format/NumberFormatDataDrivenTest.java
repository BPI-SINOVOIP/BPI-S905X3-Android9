// © 2017 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html#License
package com.ibm.icu.dev.test.format;

import java.math.BigDecimal;
import java.math.RoundingMode;
import java.text.ParseException;
import java.text.ParsePosition;

import org.junit.Test;

import com.ibm.icu.dev.test.TestUtil;
import com.ibm.icu.impl.number.DecimalFormatProperties;
import com.ibm.icu.impl.number.Padder.PadPosition;
import com.ibm.icu.impl.number.Parse;
import com.ibm.icu.impl.number.Parse.ParseMode;
import com.ibm.icu.impl.number.PatternStringParser;
import com.ibm.icu.impl.number.PatternStringUtils;
import com.ibm.icu.number.LocalizedNumberFormatter;
import com.ibm.icu.number.NumberFormatter;
import com.ibm.icu.text.DecimalFormat;
import com.ibm.icu.text.DecimalFormat.PropertySetter;
import com.ibm.icu.text.DecimalFormatSymbols;
import com.ibm.icu.util.CurrencyAmount;
import com.ibm.icu.util.ULocale;

public class NumberFormatDataDrivenTest {

  private static ULocale EN = new ULocale("en");

  private static Number toNumber(String s) {
    if (s.equals("NaN")) {
      return Double.NaN;
    } else if (s.equals("-Inf")) {
      return Double.NEGATIVE_INFINITY;
    } else if (s.equals("Inf")) {
      return Double.POSITIVE_INFINITY;
    }
    return new BigDecimal(s);
  }

  // Android patch: Android can't access DecimalFormat_ICU58 for testing (b/33448125).
  // That class lived in a package under test and relied on package access, but
  // 1.) Android Compatibility Test Suite (CTS) run tests with a different ClassLoader,
  //     preventing package access, and
  // 2.) By default, the OpenJDK 9 toolchain won't compile non-libcore code that in
  //     libcore packages (see http://b/68224249).
  /*
  private DataDrivenNumberFormatTestUtility.CodeUnderTest ICU58 =
      new DataDrivenNumberFormatTestUtility.CodeUnderTest() {
        @Override
        public Character Id() {
          return 'J';
        }

        @Override
        public String format(DataDrivenNumberFormatTestData tuple) {
          DecimalFormat_ICU58 fmt = createDecimalFormat(tuple);
          String actual = fmt.format(toNumber(tuple.format));
          String expected = tuple.output;
          if (!expected.equals(actual)) {
            return "Expected " + expected + ", got " + actual;
          }
          return null;
        }

        @Override
        public String toPattern(DataDrivenNumberFormatTestData tuple) {
          DecimalFormat_ICU58 fmt = createDecimalFormat(tuple);
          StringBuilder result = new StringBuilder();
          if (tuple.toPattern != null) {
            String expected = tuple.toPattern;
            String actual = fmt.toPattern();
            if (!expected.equals(actual)) {
              result.append("Expected toPattern=" + expected + ", got " + actual);
            }
          }
          if (tuple.toLocalizedPattern != null) {
            String expected = tuple.toLocalizedPattern;
            String actual = fmt.toLocalizedPattern();
            if (!expected.equals(actual)) {
              result.append("Expected toLocalizedPattern=" + expected + ", got " + actual);
            }
          }
          return result.length() == 0 ? null : result.toString();
        }

        @Override
        public String parse(DataDrivenNumberFormatTestData tuple) {
          DecimalFormat_ICU58 fmt = createDecimalFormat(tuple);
          ParsePosition ppos = new ParsePosition(0);
          Number actual = fmt.parse(tuple.parse, ppos);
          if (ppos.getIndex() == 0) {
            return "Parse failed; got " + actual + ", but expected " + tuple.output;
          }
          if (tuple.output.equals("fail")) {
            return null;
          }
          Number expected = toNumber(tuple.output);
          // number types cannot be compared, this is the best we can do.
          if (expected.doubleValue() != actual.doubleValue()
              && !Double.isNaN(expected.doubleValue())
              && !Double.isNaN(expected.doubleValue())) {
            return "Expected: " + expected + ", got: " + actual;
          }
          return null;
        }

        @Override
        public String parseCurrency(DataDrivenNumberFormatTestData tuple) {
          DecimalFormat_ICU58 fmt = createDecimalFormat(tuple);
          ParsePosition ppos = new ParsePosition(0);
          CurrencyAmount currAmt = fmt.parseCurrency(tuple.parse, ppos);
          if (ppos.getIndex() == 0) {
            return "Parse failed; got " + currAmt + ", but expected " + tuple.output;
          }
          if (tuple.output.equals("fail")) {
            return null;
          }
          Number expected = toNumber(tuple.output);
          Number actual = currAmt.getNumber();
          // number types cannot be compared, this is the best we can do.
          if (expected.doubleValue() != actual.doubleValue()
              && !Double.isNaN(expected.doubleValue())
              && !Double.isNaN(expected.doubleValue())) {
            return "Expected: " + expected + ", got: " + actual;
          }

          if (!tuple.outputCurrency.equals(currAmt.getCurrency().toString())) {
            return "Expected currency: " + tuple.outputCurrency + ", got: " + currAmt.getCurrency();
          }
          return null;
        }

        /**
         * @param tuple
         * @return
         *
        private DecimalFormat_ICU58 createDecimalFormat(DataDrivenNumberFormatTestData tuple) {

          DecimalFormat_ICU58 fmt =
              new DecimalFormat_ICU58(
                  tuple.pattern == null ? "0" : tuple.pattern,
                  new DecimalFormatSymbols(tuple.locale == null ? EN : tuple.locale));
          adjustDecimalFormat(tuple, fmt);
          return fmt;
        }
        /**
         * @param tuple
         * @param fmt
         *
        private void adjustDecimalFormat(
            DataDrivenNumberFormatTestData tuple, DecimalFormat_ICU58 fmt) {
          if (tuple.minIntegerDigits != null) {
            fmt.setMinimumIntegerDigits(tuple.minIntegerDigits);
          }
          if (tuple.maxIntegerDigits != null) {
            fmt.setMaximumIntegerDigits(tuple.maxIntegerDigits);
          }
          if (tuple.minFractionDigits != null) {
            fmt.setMinimumFractionDigits(tuple.minFractionDigits);
          }
          if (tuple.maxFractionDigits != null) {
            fmt.setMaximumFractionDigits(tuple.maxFractionDigits);
          }
          if (tuple.currency != null) {
            fmt.setCurrency(tuple.currency);
          }
          if (tuple.minGroupingDigits != null) {
            // Oops we don't support this.
          }
          if (tuple.useSigDigits != null) {
            fmt.setSignificantDigitsUsed(tuple.useSigDigits != 0);
          }
          if (tuple.minSigDigits != null) {
            fmt.setMinimumSignificantDigits(tuple.minSigDigits);
          }
          if (tuple.maxSigDigits != null) {
            fmt.setMaximumSignificantDigits(tuple.maxSigDigits);
          }
          if (tuple.useGrouping != null) {
            fmt.setGroupingUsed(tuple.useGrouping != 0);
          }
          if (tuple.multiplier != null) {
            fmt.setMultiplier(tuple.multiplier);
          }
          if (tuple.roundingIncrement != null) {
            fmt.setRoundingIncrement(tuple.roundingIncrement.doubleValue());
          }
          if (tuple.formatWidth != null) {
            fmt.setFormatWidth(tuple.formatWidth);
          }
          if (tuple.padCharacter != null && tuple.padCharacter.length() > 0) {
            fmt.setPadCharacter(tuple.padCharacter.charAt(0));
          }
          if (tuple.useScientific != null) {
            fmt.setScientificNotation(tuple.useScientific != 0);
          }
          if (tuple.grouping != null) {
            fmt.setGroupingSize(tuple.grouping);
          }
          if (tuple.grouping2 != null) {
            fmt.setSecondaryGroupingSize(tuple.grouping2);
          }
          if (tuple.roundingMode != null) {
            fmt.setRoundingMode(tuple.roundingMode);
          }
          if (tuple.currencyUsage != null) {
            fmt.setCurrencyUsage(tuple.currencyUsage);
          }
          if (tuple.minimumExponentDigits != null) {
            fmt.setMinimumExponentDigits(tuple.minimumExponentDigits.byteValue());
          }
          if (tuple.exponentSignAlwaysShown != null) {
            fmt.setExponentSignAlwaysShown(tuple.exponentSignAlwaysShown != 0);
          }
          if (tuple.decimalSeparatorAlwaysShown != null) {
            fmt.setDecimalSeparatorAlwaysShown(tuple.decimalSeparatorAlwaysShown != 0);
          }
          if (tuple.padPosition != null) {
            fmt.setPadPosition(tuple.padPosition);
          }
          if (tuple.positivePrefix != null) {
            fmt.setPositivePrefix(tuple.positivePrefix);
          }
          if (tuple.positiveSuffix != null) {
            fmt.setPositiveSuffix(tuple.positiveSuffix);
          }
          if (tuple.negativePrefix != null) {
            fmt.setNegativePrefix(tuple.negativePrefix);
          }
          if (tuple.negativeSuffix != null) {
            fmt.setNegativeSuffix(tuple.negativeSuffix);
          }
          if (tuple.localizedPattern != null) {
            fmt.applyLocalizedPattern(tuple.localizedPattern);
          }
          int lenient = tuple.lenient == null ? 1 : tuple.lenient.intValue();
          fmt.setParseStrict(lenient == 0);
          if (tuple.parseIntegerOnly != null) {
            fmt.setParseIntegerOnly(tuple.parseIntegerOnly != 0);
          }
          if (tuple.parseCaseSensitive != null) {
            // Not supported.
          }
          if (tuple.decimalPatternMatchRequired != null) {
            fmt.setDecimalPatternMatchRequired(tuple.decimalPatternMatchRequired != 0);
          }
          if (tuple.parseNoExponent != null) {
            // Oops, not supported for now
          }
        }
      };
  */
  // Android patch end.

  private DataDrivenNumberFormatTestUtility.CodeUnderTest JDK =
      new DataDrivenNumberFormatTestUtility.CodeUnderTest() {
        @Override
        public Character Id() {
          return 'K';
        }

        @Override
        public String format(DataDrivenNumberFormatTestData tuple) {
          java.text.DecimalFormat fmt = createDecimalFormat(tuple);
          String actual = fmt.format(toNumber(tuple.format));
          String expected = tuple.output;
          if (!expected.equals(actual)) {
            return "Expected " + expected + ", got " + actual;
          }
          return null;
        }

        @Override
        public String toPattern(DataDrivenNumberFormatTestData tuple) {
          java.text.DecimalFormat fmt = createDecimalFormat(tuple);
          StringBuilder result = new StringBuilder();
          if (tuple.toPattern != null) {
            String expected = tuple.toPattern;
            String actual = fmt.toPattern();
            if (!expected.equals(actual)) {
              result.append("Expected toPattern=" + expected + ", got " + actual);
            }
          }
          if (tuple.toLocalizedPattern != null) {
            String expected = tuple.toLocalizedPattern;
            String actual = fmt.toLocalizedPattern();
            if (!expected.equals(actual)) {
              result.append("Expected toLocalizedPattern=" + expected + ", got " + actual);
            }
          }
          return result.length() == 0 ? null : result.toString();
        }

        @Override
        public String parse(DataDrivenNumberFormatTestData tuple) {
          java.text.DecimalFormat fmt = createDecimalFormat(tuple);
          ParsePosition ppos = new ParsePosition(0);
          Number actual = fmt.parse(tuple.parse, ppos);
          if (ppos.getIndex() == 0) {
            return "Parse failed; got " + actual + ", but expected " + tuple.output;
          }
          if (tuple.output.equals("fail")) {
            return null;
          }
          Number expected = toNumber(tuple.output);
          // number types cannot be compared, this is the best we can do.
          if (expected.doubleValue() != actual.doubleValue()
              && !Double.isNaN(expected.doubleValue())
              && !Double.isNaN(expected.doubleValue())) {
            return "Expected: " + expected + ", got: " + actual;
          }
          return null;
        }

        /**
         * @param tuple
         * @return
         */
        private java.text.DecimalFormat createDecimalFormat(DataDrivenNumberFormatTestData tuple) {
          java.text.DecimalFormat fmt =
              new java.text.DecimalFormat(
                  tuple.pattern == null ? "0" : tuple.pattern,
                  new java.text.DecimalFormatSymbols(
                      (tuple.locale == null ? EN : tuple.locale).toLocale()));
          adjustDecimalFormat(tuple, fmt);
          return fmt;
        }

        /**
         * @param tuple
         * @param fmt
         */
        private void adjustDecimalFormat(
            DataDrivenNumberFormatTestData tuple, java.text.DecimalFormat fmt) {
          if (tuple.minIntegerDigits != null) {
            fmt.setMinimumIntegerDigits(tuple.minIntegerDigits);
          }
          if (tuple.maxIntegerDigits != null) {
            fmt.setMaximumIntegerDigits(tuple.maxIntegerDigits);
          }
          if (tuple.minFractionDigits != null) {
            fmt.setMinimumFractionDigits(tuple.minFractionDigits);
          }
          if (tuple.maxFractionDigits != null) {
            fmt.setMaximumFractionDigits(tuple.maxFractionDigits);
          }
          if (tuple.currency != null) {
            fmt.setCurrency(java.util.Currency.getInstance(tuple.currency.toString()));
          }
          if (tuple.minGroupingDigits != null) {
            // Oops we don't support this.
          }
          if (tuple.useSigDigits != null) {
            // Oops we don't support this
          }
          if (tuple.minSigDigits != null) {
            // Oops we don't support this
          }
          if (tuple.maxSigDigits != null) {
            // Oops we don't support this
          }
          if (tuple.useGrouping != null) {
            fmt.setGroupingUsed(tuple.useGrouping != 0);
          }
          if (tuple.multiplier != null) {
            fmt.setMultiplier(tuple.multiplier);
          }
          if (tuple.roundingIncrement != null) {
            // Not supported
          }
          if (tuple.formatWidth != null) {
            // Not supported
          }
          if (tuple.padCharacter != null && tuple.padCharacter.length() > 0) {
            // Not supported
          }
          if (tuple.useScientific != null) {
            // Not supported
          }
          if (tuple.grouping != null) {
            fmt.setGroupingSize(tuple.grouping);
          }
          if (tuple.grouping2 != null) {
            // Not supported
          }
          if (tuple.roundingMode != null) {
            // Not supported
          }
          if (tuple.currencyUsage != null) {
            // Not supported
          }
          if (tuple.minimumExponentDigits != null) {
            // Not supported
          }
          if (tuple.exponentSignAlwaysShown != null) {
            // Not supported
          }
          if (tuple.decimalSeparatorAlwaysShown != null) {
            fmt.setDecimalSeparatorAlwaysShown(tuple.decimalSeparatorAlwaysShown != 0);
          }
          if (tuple.padPosition != null) {
            // Not supported
          }
          if (tuple.positivePrefix != null) {
            fmt.setPositivePrefix(tuple.positivePrefix);
          }
          if (tuple.positiveSuffix != null) {
            fmt.setPositiveSuffix(tuple.positiveSuffix);
          }
          if (tuple.negativePrefix != null) {
            fmt.setNegativePrefix(tuple.negativePrefix);
          }
          if (tuple.negativeSuffix != null) {
            fmt.setNegativeSuffix(tuple.negativeSuffix);
          }
          if (tuple.localizedPattern != null) {
            fmt.applyLocalizedPattern(tuple.localizedPattern);
          }

          // lenient parsing not supported by JDK
          if (tuple.parseIntegerOnly != null) {
            fmt.setParseIntegerOnly(tuple.parseIntegerOnly != 0);
          }
          if (tuple.parseCaseSensitive != null) {
            // Not supported.
          }
          if (tuple.decimalPatternMatchRequired != null) {
            // Oops, not supported
          }
          if (tuple.parseNoExponent != null) {
            // Oops, not supported for now
          }
        }
      };

  static void propertiesFromTuple(DataDrivenNumberFormatTestData tuple, DecimalFormatProperties properties) {
    if (tuple.minIntegerDigits != null) {
      properties.setMinimumIntegerDigits(tuple.minIntegerDigits);
    }
    if (tuple.maxIntegerDigits != null) {
      properties.setMaximumIntegerDigits(tuple.maxIntegerDigits);
    }
    if (tuple.minFractionDigits != null) {
      properties.setMinimumFractionDigits(tuple.minFractionDigits);
    }
    if (tuple.maxFractionDigits != null) {
      properties.setMaximumFractionDigits(tuple.maxFractionDigits);
    }
    if (tuple.currency != null) {
      properties.setCurrency(tuple.currency);
    }
    if (tuple.minGroupingDigits != null) {
      properties.setMinimumGroupingDigits(tuple.minGroupingDigits);
    }
    if (tuple.useSigDigits != null) {
      // TODO
    }
    if (tuple.minSigDigits != null) {
      properties.setMinimumSignificantDigits(tuple.minSigDigits);
    }
    if (tuple.maxSigDigits != null) {
      properties.setMaximumSignificantDigits(tuple.maxSigDigits);
    }
    if (tuple.useGrouping != null && tuple.useGrouping == 0) {
      properties.setGroupingSize(-1);
      properties.setSecondaryGroupingSize(-1);
    }
    if (tuple.multiplier != null) {
      properties.setMultiplier(new BigDecimal(tuple.multiplier));
    }
    if (tuple.roundingIncrement != null) {
      properties.setRoundingIncrement(new BigDecimal(tuple.roundingIncrement.toString()));
    }
    if (tuple.formatWidth != null) {
      properties.setFormatWidth(tuple.formatWidth);
    }
    if (tuple.padCharacter != null && tuple.padCharacter.length() > 0) {
      properties.setPadString(tuple.padCharacter.toString());
    }
    if (tuple.useScientific != null) {
      properties.setMinimumExponentDigits(
          tuple.useScientific != 0 ? 1 : -1);
    }
    if (tuple.grouping != null) {
      properties.setGroupingSize(tuple.grouping);
    }
    if (tuple.grouping2 != null) {
      properties.setSecondaryGroupingSize(tuple.grouping2);
    }
    if (tuple.roundingMode != null) {
      properties.setRoundingMode(RoundingMode.valueOf(tuple.roundingMode));
    }
    if (tuple.currencyUsage != null) {
      properties.setCurrencyUsage(tuple.currencyUsage);
    }
    if (tuple.minimumExponentDigits != null) {
      properties.setMinimumExponentDigits(tuple.minimumExponentDigits.byteValue());
    }
    if (tuple.exponentSignAlwaysShown != null) {
      properties.setExponentSignAlwaysShown(tuple.exponentSignAlwaysShown != 0);
    }
    if (tuple.decimalSeparatorAlwaysShown != null) {
      properties.setDecimalSeparatorAlwaysShown(tuple.decimalSeparatorAlwaysShown != 0);
    }
    if (tuple.padPosition != null) {
      properties.setPadPosition(PadPosition.fromOld(tuple.padPosition));
    }
    if (tuple.positivePrefix != null) {
      properties.setPositivePrefix(tuple.positivePrefix);
    }
    if (tuple.positiveSuffix != null) {
      properties.setPositiveSuffix(tuple.positiveSuffix);
    }
    if (tuple.negativePrefix != null) {
      properties.setNegativePrefix(tuple.negativePrefix);
    }
    if (tuple.negativeSuffix != null) {
      properties.setNegativeSuffix(tuple.negativeSuffix);
    }
    if (tuple.localizedPattern != null) {
      DecimalFormatSymbols symbols = DecimalFormatSymbols.getInstance(tuple.locale);
      String converted = PatternStringUtils.convertLocalized(tuple.localizedPattern, symbols, false);
      PatternStringParser.parseToExistingProperties(converted, properties);
    }
    if (tuple.lenient != null) {
      properties.setParseMode(tuple.lenient == 0 ? ParseMode.STRICT : ParseMode.LENIENT);
    }
    if (tuple.parseIntegerOnly != null) {
      properties.setParseIntegerOnly(tuple.parseIntegerOnly != 0);
    }
    if (tuple.parseCaseSensitive != null) {
      properties.setParseCaseSensitive(tuple.parseCaseSensitive != 0);
    }
    if (tuple.decimalPatternMatchRequired != null) {
      properties.setDecimalPatternMatchRequired(tuple.decimalPatternMatchRequired != 0);
    }
    if (tuple.parseNoExponent != null) {
      properties.setParseNoExponent(tuple.parseNoExponent != 0);
    }
  }

  /**
   * Formatting, but no other features.
   */
  private DataDrivenNumberFormatTestUtility.CodeUnderTest ICU60 =
      new DataDrivenNumberFormatTestUtility.CodeUnderTest() {

        @Override
        public Character Id() {
          return 'Q';
        }

        /**
         * Runs a single formatting test. On success, returns null. On failure, returns the error.
         * This implementation just returns null. Subclasses should override.
         *
         * @param tuple contains the parameters of the format test.
         */
        @Override
        public String format(DataDrivenNumberFormatTestData tuple) {
          String pattern = (tuple.pattern == null) ? "0" : tuple.pattern;
          ULocale locale = (tuple.locale == null) ? ULocale.ENGLISH : tuple.locale;
          DecimalFormatProperties properties =
              PatternStringParser.parseToProperties(
                  pattern,
                  tuple.currency != null
                      ? PatternStringParser.IGNORE_ROUNDING_ALWAYS
                      : PatternStringParser.IGNORE_ROUNDING_NEVER);
          propertiesFromTuple(tuple, properties);
          DecimalFormatSymbols symbols = DecimalFormatSymbols.getInstance(locale);
          LocalizedNumberFormatter fmt = NumberFormatter.fromDecimalFormat(properties, symbols, null).locale(locale);
          Number number = toNumber(tuple.format);
          String expected = tuple.output;
          String actual = fmt.format(number).toString();
          if (!expected.equals(actual)) {
            return "Expected \"" + expected + "\", got \"" + actual + "\"";
          }
          return null;
        }
      };

    /**
     * All features except formatting.
     */
    private DataDrivenNumberFormatTestUtility.CodeUnderTest ICU60_Other =
            new DataDrivenNumberFormatTestUtility.CodeUnderTest() {

        @Override
        public Character Id() {
          return 'S';
        }

        /**
         * Runs a single toPattern test. On success, returns null. On failure, returns the error. This implementation
         * just returns null. Subclasses should override.
         *
         * @param tuple
         *            contains the parameters of the format test.
         */
        @Override
        public String toPattern(DataDrivenNumberFormatTestData tuple) {
            String pattern = (tuple.pattern == null) ? "0" : tuple.pattern;
            final DecimalFormatProperties properties;
            DecimalFormat df;
            try {
                properties = PatternStringParser.parseToProperties(
                        pattern,
                        tuple.currency != null ? PatternStringParser.IGNORE_ROUNDING_ALWAYS
                                : PatternStringParser.IGNORE_ROUNDING_NEVER);
                propertiesFromTuple(tuple, properties);
                // TODO: Use PatternString.propertiesToString() directly. (How to deal with CurrencyUsage?)
                df = new DecimalFormat();
                df.setProperties(new PropertySetter() {
                    @Override
                    public void set(DecimalFormatProperties props) {
                        props.copyFrom(properties);
                    }
                });
            } catch (IllegalArgumentException e) {
                e.printStackTrace();
                return e.getLocalizedMessage();
            }

            if (tuple.toPattern != null) {
                String expected = tuple.toPattern;
                String actual = df.toPattern();
                if (!expected.equals(actual)) {
                    return "Expected toPattern='" + expected + "'; got '" + actual + "'";
                }
            }
            if (tuple.toLocalizedPattern != null) {
                String expected = tuple.toLocalizedPattern;
                String actual = PatternStringUtils.propertiesToPatternString(properties);
                if (!expected.equals(actual)) {
                    return "Expected toLocalizedPattern='" + expected + "'; got '" + actual + "'";
                }
            }
            return null;
        }

        /**
         * Runs a single parse test. On success, returns null. On failure, returns the error. This implementation just
         * returns null. Subclasses should override.
         *
         * @param tuple
         *            contains the parameters of the format test.
         */
        @Override
        public String parse(DataDrivenNumberFormatTestData tuple) {
            String pattern = (tuple.pattern == null) ? "0" : tuple.pattern;
            DecimalFormatProperties properties;
            ParsePosition ppos = new ParsePosition(0);
            Number actual;
            try {
                properties = PatternStringParser.parseToProperties(
                        pattern,
                        tuple.currency != null ? PatternStringParser.IGNORE_ROUNDING_ALWAYS
                                : PatternStringParser.IGNORE_ROUNDING_NEVER);
                propertiesFromTuple(tuple, properties);
                actual = Parse.parse(tuple.parse, ppos, properties, DecimalFormatSymbols.getInstance(tuple.locale));
            } catch (IllegalArgumentException e) {
                return "parse exception: " + e.getMessage();
            }
            if (actual == null && ppos.getIndex() != 0) {
                throw new AssertionError("Error: value is null but parse position is not zero");
            }
            if (ppos.getIndex() == 0) {
                return "Parse failed; got " + actual + ", but expected " + tuple.output;
            }
            if (tuple.output.equals("NaN")) {
                if (!Double.isNaN(actual.doubleValue())) {
                    return "Expected NaN, but got: " + actual;
                }
                return null;
            } else if (tuple.output.equals("Inf")) {
                if (!Double.isInfinite(actual.doubleValue()) || Double.compare(actual.doubleValue(), 0.0) < 0) {
                    return "Expected Inf, but got: " + actual;
                }
                return null;
            } else if (tuple.output.equals("-Inf")) {
                if (!Double.isInfinite(actual.doubleValue()) || Double.compare(actual.doubleValue(), 0.0) > 0) {
                    return "Expected -Inf, but got: " + actual;
                }
                return null;
            } else if (tuple.output.equals("fail")) {
                return null;
            } else if (new BigDecimal(tuple.output).compareTo(new BigDecimal(actual.toString())) != 0) {
                return "Expected: " + tuple.output + ", got: " + actual;
            } else {
                return null;
            }
        }

        /**
         * Runs a single parse currency test. On success, returns null. On failure, returns the error. This
         * implementation just returns null. Subclasses should override.
         *
         * @param tuple
         *            contains the parameters of the format test.
         */
        @Override
        public String parseCurrency(DataDrivenNumberFormatTestData tuple) {
            String pattern = (tuple.pattern == null) ? "0" : tuple.pattern;
            DecimalFormatProperties properties;
            ParsePosition ppos = new ParsePosition(0);
            CurrencyAmount actual;
            try {
                properties = PatternStringParser.parseToProperties(
                        pattern,
                        tuple.currency != null ? PatternStringParser.IGNORE_ROUNDING_ALWAYS
                                : PatternStringParser.IGNORE_ROUNDING_NEVER);
                propertiesFromTuple(tuple, properties);
                actual = Parse
                        .parseCurrency(tuple.parse, ppos, properties, DecimalFormatSymbols.getInstance(tuple.locale));
            } catch (ParseException e) {
                e.printStackTrace();
                return "parse exception: " + e.getMessage();
            }
            if (ppos.getIndex() == 0 || actual.getCurrency().getCurrencyCode().equals("XXX")) {
                return "Parse failed; got " + actual + ", but expected " + tuple.output;
            }
            BigDecimal expectedNumber = new BigDecimal(tuple.output);
            if (expectedNumber.compareTo(new BigDecimal(actual.getNumber().toString())) != 0) {
                return "Wrong number: Expected: " + expectedNumber + ", got: " + actual;
            }
            String expectedCurrency = tuple.outputCurrency;
            if (!expectedCurrency.equals(actual.getCurrency().toString())) {
                return "Wrong currency: Expected: " + expectedCurrency + ", got: " + actual;
            }
            return null;
        }

        /**
         * Runs a single select test. On success, returns null. On failure, returns the error. This implementation just
         * returns null. Subclasses should override.
         *
         * @param tuple
         *            contains the parameters of the format test.
         */
        @Override
        public String select(DataDrivenNumberFormatTestData tuple) {
            return null;
        }
    };

  // Android patch: Android can't access DecimalFormat_ICU58 for testing (b/33448125).
  /*
  @Test
  public void TestDataDrivenICU58() {
    DataDrivenNumberFormatTestUtility.runFormatSuiteIncludingKnownFailures(
        "numberformattestspecification.txt", ICU58);
  }
  */
  // Android patch end.

  // Note: This test case is really questionable. Depending on Java version,
  // something may or may not work. However the test data assumes a specific
  // Java runtime version. We should probably disable this test case - #13372
  @Test
  public void TestDataDrivenJDK() {
    // Android implements java.text.DecimalFormat with ICU4J (ticket #13322).
    // Oracle/OpenJDK 9's behavior is not exactly same with Oracle/OpenJDK 8.
    // Some test cases failed on 8 work well, while some other test cases
    // fail on 9, but worked on 8. Skip this test case if Java version is not 8.
    org.junit.Assume.assumeTrue(
            TestUtil.getJavaVendor() != TestUtil.JavaVendor.Android
            && TestUtil.getJavaVersion() < 9);

    DataDrivenNumberFormatTestUtility.runFormatSuiteIncludingKnownFailures(
            "numberformattestspecification.txt", JDK);
  }

  @Test
  public void TestDataDrivenICULatest_Format() {
    DataDrivenNumberFormatTestUtility.runFormatSuiteIncludingKnownFailures(
        "numberformattestspecification.txt", ICU60);
  }

  @Test
  public void TestDataDrivenICULatest_Other() {
    DataDrivenNumberFormatTestUtility.runFormatSuiteIncludingKnownFailures(
        "numberformattestspecification.txt", ICU60_Other);
  }
}
