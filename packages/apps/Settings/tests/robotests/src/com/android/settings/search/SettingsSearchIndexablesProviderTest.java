package com.android.settings.search;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;

import android.Manifest;
import android.content.pm.ProviderInfo;
import android.database.Cursor;
import android.net.Uri;
import android.provider.SearchIndexablesContract;

import com.android.settings.R;
import com.android.settings.search.indexing.FakeSettingsFragment;
import com.android.settings.testutils.FakeFeatureFactory;
import com.android.settings.testutils.SettingsRobolectricTestRunner;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

@RunWith(SettingsRobolectricTestRunner.class)
public class SettingsSearchIndexablesProviderTest {

    private static final String BASE_AUTHORITY = "com.android.settings";

    private SettingsSearchIndexablesProvider mProvider;
    private FakeFeatureFactory mFakeFeatureFactory;

    @Before
    public void setUp() {
        mProvider = spy(new SettingsSearchIndexablesProvider());
        ProviderInfo info = new ProviderInfo();
        info.exported = true;
        info.grantUriPermissions = true;
        info.authority = BASE_AUTHORITY;
        info.readPermission = Manifest.permission.READ_SEARCH_INDEXABLES;
        mProvider.attachInfo(RuntimeEnvironment.application, info);

        final SearchFeatureProvider featureProvider = new SearchFeatureProviderImpl();
        featureProvider.getSearchIndexableResources().getProviderValues().clear();
        featureProvider.getSearchIndexableResources().getProviderValues()
                .add(FakeSettingsFragment.class);
        mFakeFeatureFactory = FakeFeatureFactory.setupForTest();
        mFakeFeatureFactory.searchFeatureProvider = featureProvider;
    }

    @After
    public void cleanUp() {
        mFakeFeatureFactory.searchFeatureProvider = mock(SearchFeatureProvider.class);
    }

    @Test
    public void testRawColumnFetched() {
        Uri rawUri = Uri.parse("content://" + BASE_AUTHORITY + "/" +
                SearchIndexablesContract.INDEXABLES_RAW_PATH);

        final Cursor cursor = mProvider.query(rawUri,
                SearchIndexablesContract.INDEXABLES_RAW_COLUMNS, null, null, null);

        cursor.moveToFirst();
        assertThat(cursor.getString(1)).isEqualTo(FakeSettingsFragment.TITLE);
        assertThat(cursor.getString(2)).isEqualTo(FakeSettingsFragment.SUMMARY_ON);
        assertThat(cursor.getString(3)).isEqualTo(FakeSettingsFragment.SUMMARY_OFF);
        assertThat(cursor.getString(4)).isEqualTo(FakeSettingsFragment.ENTRIES);
        assertThat(cursor.getString(5)).isEqualTo(FakeSettingsFragment.KEYWORDS);
        assertThat(cursor.getString(6)).isEqualTo(FakeSettingsFragment.SCREEN_TITLE);
        assertThat(cursor.getString(7)).isEqualTo(FakeSettingsFragment.CLASS_NAME);
        assertThat(cursor.getInt(8)).isEqualTo(FakeSettingsFragment.ICON);
        assertThat(cursor.getString(9)).isEqualTo(FakeSettingsFragment.INTENT_ACTION);
        assertThat(cursor.getString(10)).isEqualTo(FakeSettingsFragment.TARGET_PACKAGE);
        assertThat(cursor.getString(11)).isEqualTo(FakeSettingsFragment.TARGET_CLASS);
        assertThat(cursor.getString(12)).isEqualTo(FakeSettingsFragment.KEY);
    }

    @Test
    public void testResourcesColumnFetched() {
        Uri rawUri = Uri.parse("content://" + BASE_AUTHORITY + "/" +
                SearchIndexablesContract.INDEXABLES_XML_RES_PATH);

        final Cursor cursor = mProvider.query(rawUri,
                SearchIndexablesContract.INDEXABLES_XML_RES_COLUMNS, null, null, null);

        cursor.moveToFirst();
        assertThat(cursor.getCount()).isEqualTo(1);
        assertThat(cursor.getInt(1)).isEqualTo(R.xml.display_settings);
        assertThat(cursor.getString(2)).isEqualTo(FakeSettingsFragment.CLASS_NAME);
        assertThat(cursor.getInt(3)).isEqualTo(0);
        assertThat(cursor.getString(4)).isNull();
        assertThat(cursor.getString(5)).isNull();
        assertThat(cursor.getString(6)).isNull();
    }

    @Test
    @Config(qualifiers = "mcc999")
    public void testNonIndexablesColumnFetched() {
        Uri rawUri = Uri.parse("content://" + BASE_AUTHORITY + "/" +
                SearchIndexablesContract.NON_INDEXABLES_KEYS_PATH);

        final Cursor cursor = mProvider.query(rawUri,
                SearchIndexablesContract.NON_INDEXABLES_KEYS_COLUMNS, null, null, null);

        cursor.moveToFirst();
        assertThat(cursor.getCount()).isEqualTo(2);
        assertThat(cursor.getString(0)).isEqualTo("pref_key_1");
        cursor.moveToNext();
        assertThat(cursor.getString(0)).isEqualTo("pref_key_3");
    }
}
