// © 2017 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING && !UPRV_INCOMPLETE_CPP11_SUPPORT
#pragma once

#include "number_stringbuilder.h"
#include "intltest.h"
#include "number_affixutils.h"

using namespace icu::number;
using namespace icu::number::impl;

////////////////////////////////////////////////////////////////////////////////////////
// INSTRUCTIONS:                                                                      //
// To add new NumberFormat unit test classes, create a new class like the ones below, //
// and then add it as a switch statement in NumberTest at the bottom of this file.    /////////
// To add new methods to existing unit test classes, add the method to the class declaration //
// below, and also add it to the class's implementation of runIndexedTest().                 //
///////////////////////////////////////////////////////////////////////////////////////////////

class AffixUtilsTest : public IntlTest {
  public:
    void testEscape();
    void testUnescape();
    void testContainsReplaceType();
    void testInvalid();
    void testUnescapeWithSymbolProvider();

    void runIndexedTest(int32_t index, UBool exec, const char *&name, char *par = 0);

  private:
    UnicodeString unescapeWithDefaults(const SymbolProvider &defaultProvider, UnicodeString input,
                                       UErrorCode &status);
};

class NumberFormatterApiTest : public IntlTest {
  public:
    NumberFormatterApiTest();
    NumberFormatterApiTest(UErrorCode &status);

    void notationSimple();
    void notationScientific();
    void notationCompact();
    void unitMeasure();
    void unitCurrency();
    void unitPercent();
    void roundingFraction();
    void roundingFigures();
    void roundingFractionFigures();
    void roundingOther();
    void grouping();
    void padding();
    void integerWidth();
    void symbols();
    // TODO: Add this method if currency symbols override support is added.
    //void symbolsOverride();
    void sign();
    void decimal();
    void locale();
    void formatTypes();
    void errors();

    void runIndexedTest(int32_t index, UBool exec, const char *&name, char *par = 0);

  private:
    CurrencyUnit USD;
    CurrencyUnit GBP;
    CurrencyUnit CZK;
    CurrencyUnit CAD;

    MeasureUnit METER;
    MeasureUnit DAY;
    MeasureUnit SQUARE_METER;
    MeasureUnit FAHRENHEIT;

    NumberingSystem MATHSANB;
    NumberingSystem LATN;

    DecimalFormatSymbols FRENCH_SYMBOLS;
    DecimalFormatSymbols SWISS_SYMBOLS;
    DecimalFormatSymbols MYANMAR_SYMBOLS;

    void assertFormatDescending(const UnicodeString &message, const UnlocalizedNumberFormatter &f,
                                Locale locale, ...);

    void assertFormatDescendingBig(const UnicodeString &message, const UnlocalizedNumberFormatter &f,
                                   Locale locale, ...);

    void assertFormatSingle(const UnicodeString &message, const UnlocalizedNumberFormatter &f,
                            Locale locale, double input, const UnicodeString &expected);
};

class DecimalQuantityTest : public IntlTest {
  public:
    void testDecimalQuantityBehaviorStandalone();
    void testSwitchStorage();
    void testAppend();
    void testConvertToAccurateDouble();
    void testUseApproximateDoubleWhenAble();

    void runIndexedTest(int32_t index, UBool exec, const char *&name, char *par = 0);

  private:
    void assertDoubleEquals(UnicodeString message, double a, double b);
    void assertHealth(const DecimalQuantity &fq);
    void assertToStringAndHealth(const DecimalQuantity &fq, const UnicodeString &expected);
    void checkDoubleBehavior(double d, bool explicitRequired);
};

class ModifiersTest : public IntlTest {
  public:
    void testConstantAffixModifier();
    void testConstantMultiFieldModifier();
    void testSimpleModifier();
    void testCurrencySpacingEnabledModifier();

    void runIndexedTest(int32_t index, UBool exec, const char *&name, char *par = 0);

  private:
    void assertModifierEquals(const Modifier &mod, int32_t expectedPrefixLength, bool expectedStrong,
                              UnicodeString expectedChars, UnicodeString expectedFields,
                              UErrorCode &status);

    void assertModifierEquals(const Modifier &mod, NumberStringBuilder &sb, int32_t expectedPrefixLength,
                              bool expectedStrong, UnicodeString expectedChars,
                              UnicodeString expectedFields, UErrorCode &status);
};

class PatternModifierTest : public IntlTest {
  public:
    void testBasic();
    void testMutableEqualsImmutable();

    void runIndexedTest(int32_t index, UBool exec, const char *&name, char *par = 0);

  private:
    UnicodeString getPrefix(const MutablePatternModifier &mod, UErrorCode &status);
    UnicodeString getSuffix(const MutablePatternModifier &mod, UErrorCode &status);
};

class PatternStringTest : public IntlTest {
  public:
    void testToPatternSimple();
    void testExceptionOnInvalid();
    void testBug13117();

    void runIndexedTest(int32_t index, UBool exec, const char *&name, char *par = 0);

  private:
};

class NumberStringBuilderTest : public IntlTest {
  public:
    void testInsertAppendUnicodeString();
    void testInsertAppendCodePoint();
    void testCopy();
    void testFields();
    void testUnlimitedCapacity();
    void testCodePoints();

    void runIndexedTest(int32_t index, UBool exec, const char *&name, char *par = 0);

  private:
    void assertEqualsImpl(const UnicodeString &a, const NumberStringBuilder &b);
};


// NOTE: This macro is identical to the one in itformat.cpp
#define TESTCLASS(id, TestClass)          \
    case id:                              \
        name = #TestClass;                \
        if (exec) {                       \
            logln(#TestClass " test---"); \
            logln((UnicodeString)"");     \
            TestClass test;               \
            callTest(test, par);          \
        }                                 \
        break

class NumberTest : public IntlTest {
  public:
    void runIndexedTest(int32_t index, UBool exec, const char *&name, char *par = 0) {
        if (exec) {
            logln("TestSuite NumberTest: ");
        }

        switch (index) {
        TESTCLASS(0, AffixUtilsTest);
        TESTCLASS(1, NumberFormatterApiTest);
        TESTCLASS(2, DecimalQuantityTest);
        TESTCLASS(3, ModifiersTest);
        TESTCLASS(4, PatternModifierTest);
        TESTCLASS(5, PatternStringTest);
        TESTCLASS(6, NumberStringBuilderTest);
        default: name = ""; break; // needed to end loop
        }
    }
};

#endif /* #if !UCONFIG_NO_FORMATTING */
