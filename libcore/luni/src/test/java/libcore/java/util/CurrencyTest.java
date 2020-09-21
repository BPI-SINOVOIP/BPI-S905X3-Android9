/*
 * Copyright (C) 2010 The Android Open Source Project
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

package libcore.java.util;

import java.util.Currency;
import java.util.Locale;
import java.util.Set;

import libcore.libcore.util.SerializationTester;

public class CurrencyTest extends junit.framework.TestCase {
    // Regression test to ensure that Currency.getSymbol(Locale) returns the
    // currency code if ICU doesn't have a localization of the symbol. The
    // harmony Currency tests don't test this, and their DecimalFormat tests
    // only test it as a side-effect, and in a way that only detected my
    // specific mistake of returning null (returning "stinky" would have
    // passed).
    public void test_getSymbol_fallback() throws Exception {
        // This assumes that AED never becomes a currency important enough to
        // Canada that Canadians give it a localized (to Canada) symbol.
        assertEquals("AED", Currency.getInstance("AED").getSymbol(Locale.CANADA));
    }

    public void test_getSymbol_locale() {
        Currency currency = Currency.getInstance("DEM");
        assertEquals("DEM", currency.getSymbol(Locale.FRANCE));
        assertEquals("DM", currency.getSymbol(Locale.GERMANY));
        assertEquals("DEM", currency.getSymbol(Locale.US));
    }

    /**
     * Checks that the no-argument version of {@link Currency#getSymbol()} uses the
     * default DISPLAY locale as opposed to the default locale or the default FORMAT
     * locale.
     */
    public void test_getSymbol_noLocaleArgument() {
        Currency currency = Currency.getInstance("DEM");
        Locales locales = Locales.getAndSetDefaultForTest(Locale.US, Locale.GERMANY, Locale.FRANCE);
        try {
            // getAndSetDefaultForTest(uncategorizedLocale, displayLocale, formatLocale)
            assertEquals("DM", currency.getSymbol());
        } finally {
            locales.setAsDefault();
        }
    }

    // Regression test to ensure that Currency.getInstance(String) throws if
    // given an invalid ISO currency code.
    public void test_getInstance_illegal_currency_code() throws Exception {
        Currency.getInstance("USD");
        try {
            Currency.getInstance("BOGO-DOLLARS");
            fail("expected IllegalArgumentException for invalid ISO currency code");
        } catch (IllegalArgumentException expected) {
        }
    }

    public void testGetAvailableCurrencies() throws Exception {
        Set<Currency> all = Currency.getAvailableCurrencies();
        // Confirm that a few well-known stable currencies are present.
        assertTrue(all.toString(), all.contains(Currency.getInstance("CHF")));
        assertTrue(all.toString(), all.contains(Currency.getInstance("EUR")));
        assertTrue(all.toString(), all.contains(Currency.getInstance("GBP")));
        assertTrue(all.toString(), all.contains(Currency.getInstance("JPY")));
        assertTrue(all.toString(), all.contains(Currency.getInstance("USD")));
    }

    public void test_getDisplayName_locale_chf() throws Exception {
        Currency currency = Currency.getInstance("CHF");
        assertEquals("Swiss Franc", currency.getDisplayName(Locale.US));
        assertEquals("Schweizer Franken", currency.getDisplayName(new Locale("de", "CH")));
        assertEquals("franc suisse", currency.getDisplayName(new Locale("fr", "CH")));
        assertEquals("franco svizzero", currency.getDisplayName(new Locale("it", "CH")));
    }

    public void test_getDisplayName_locale_dem() throws Exception {
        Currency currency = Currency.getInstance("DEM");
        assertEquals("Deutsche Mark", currency.getDisplayName(Locale.GERMANY));
        assertEquals("German Mark", currency.getDisplayName(Locale.US));
        assertEquals("mark allemand", currency.getDisplayName(Locale.FRANCE));
    }

    public void test_getDisplayName_null() {
        Currency currency = Currency.getInstance("CHF");
        try {
            currency.getDisplayName(null);
            fail();
        } catch (NullPointerException expected) {
        }
    }

    /**
     * Checks that the no-argument version of {@link Currency#getDisplayName()} uses
     * the default DISPLAY locale, as opposed to the default locale or the default
     * FORMAT locale.
     */
    public void test_getDisplayName_noLocaleArgument() {
        Currency currency = Currency.getInstance("DEM");
        // getAndSetDefaultForTest(uncategorizedLocale, displayLocale, formatLocale)
        Locales locales = Locales.getAndSetDefaultForTest(Locale.US, Locale.GERMANY, Locale.FRANCE);
        try {
            assertEquals("Deutsche Mark", currency.getDisplayName());
        } finally {
            locales.setAsDefault();
        }
    }

    public void test_getDefaultFractionDigits() throws Exception {
        assertEquals(2, Currency.getInstance("USD").getDefaultFractionDigits());
        assertEquals(0, Currency.getInstance("JPY").getDefaultFractionDigits());
        assertEquals(-1, Currency.getInstance("XXX").getDefaultFractionDigits());
    }

    // http://code.google.com/p/android/issues/detail?id=38622
    public void test_getSymbol_38622() throws Exception {
        // The CLDR data had the Portuguese symbol for "EUR" in pt, not in pt_PT.
        // We weren't falling back from pt_PT to pt, so we didn't find it and would
        // default to U+00A4 CURRENCY SIGN (¤) rather than €.
        Locale pt_BR = new Locale("pt", "BR");
        Locale pt_PT = new Locale("pt", "PT");
        assertEquals("R$", Currency.getInstance(pt_BR).getSymbol(pt_BR));
        assertEquals("R$", Currency.getInstance(pt_BR).getSymbol(pt_PT));
        assertEquals("€", Currency.getInstance(pt_PT).getSymbol(pt_BR));
        assertEquals("€", Currency.getInstance(pt_PT).getSymbol(pt_PT));
    }

    public void test_nullLocales() {
        Currency currency = Currency.getInstance(Locale.getDefault());
        try {
            currency.getSymbol(null);
            fail();
        } catch (NullPointerException expected) {}
    }

    public void testSerialization() throws Exception {
        Currency usd = Currency.getInstance("USD");
        String actual = SerializationTester.serializeHex(usd);
        String expected = "ACED0005737200126A6176612E7574696C2E43757272656E6379FDCD934A5911A91F02" +
                "00014C000C63757272656E6379436F64657400124C6A6176612F6C616E672F537472696E673B7870" +
                "740003555344";
        assertEquals(expected, actual);

        Currency deserializedUsd = (Currency) SerializationTester.deserializeHex(expected);
        assertSame("Currency objects should be singletons", usd, deserializedUsd);
    }

    public void test_getNumericCode() throws Exception {
        assertEquals(840, Currency.getInstance("USD").getNumericCode());
        assertEquals(826, Currency.getInstance("GBP").getNumericCode());
        assertEquals(999, Currency.getInstance("XXX").getNumericCode());
        assertEquals(0, Currency.getInstance("XFU").getNumericCode());
    }

}
