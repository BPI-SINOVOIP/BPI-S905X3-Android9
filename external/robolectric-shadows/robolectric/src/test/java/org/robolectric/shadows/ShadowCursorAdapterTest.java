package org.robolectric.shadows;

import static org.assertj.core.api.Assertions.assertThat;

import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CursorAdapter;
import java.util.ArrayList;
import java.util.List;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.Shadows;

@RunWith(RobolectricTestRunner.class)
public class ShadowCursorAdapterTest {

  private Cursor curs;
  private CursorAdapter adapter;
  private SQLiteDatabase database;

  @Before
  public void setUp() throws Exception {
    database = SQLiteDatabase.create(null);
    database.execSQL("CREATE TABLE table_name(_id INT PRIMARY KEY, name VARCHAR(255));");
    String[] inserts = {
        "INSERT INTO table_name (_id, name) VALUES(1234, 'Chuck');",
        "INSERT INTO table_name (_id, name) VALUES(1235, 'Julie');",
        "INSERT INTO table_name (_id, name) VALUES(1236, 'Chris');",
        "INSERT INTO table_name (_id, name) VALUES(1237, 'Brenda');",
        "INSERT INTO table_name (_id, name) VALUES(1238, 'Jane');"
    };

    for (String insert : inserts) {
      database.execSQL(insert);
    }

    String sql = "SELECT * FROM table_name;";
    curs = database.rawQuery(sql, null);

    adapter = new TestAdapter(curs);
  }

  @Test
  public void testChangeCursor() {
    assertThat(adapter.getCursor()).isNotNull();
    assertThat(adapter.getCursor()).isSameAs(curs);

    adapter.changeCursor(null);

    assertThat(curs.isClosed()).isTrue();
    assertThat(adapter.getCursor()).isNull();
  }

  @Test
  public void testSwapCursor() {
    assertThat(adapter.getCursor()).isNotNull();
    assertThat(adapter.getCursor()).isSameAs(curs);

    Cursor oldCursor = adapter.swapCursor(null);

    assertThat(oldCursor).isSameAs(curs);
    assertThat(curs.isClosed()).isFalse();
    assertThat(adapter.getCursor()).isNull();
  }

  @Test
  public void testCount() {
    assertThat(adapter.getCount()).isEqualTo(curs.getCount());
    adapter.changeCursor(null);
    assertThat(adapter.getCount()).isEqualTo(0);
  }

  @Test
  public void testGetItemId() {
    for (int i = 0; i < 5; i++) {
      assertThat(adapter.getItemId(i)).isEqualTo((long) 1234 + i);
    }
  }

  @Test
  public void testGetView() {
    List<View> views = new ArrayList<>();
    for (int i = 0; i < 5; i++) {
      views.add(new View(RuntimeEnvironment.application));
    }

    Shadows.shadowOf(adapter).setViews(views);

    for (int i = 0; i < 5; i++) {
      assertThat(adapter.getView(i, null, null)).isSameAs(views.get(i));
    }
  }

  @Test public void shouldNotErrorOnCursorChangeWhenNoFlagsAreSet() throws Exception {
    adapter = new TestAdapterWithFlags(curs, 0);
    adapter.changeCursor(database.rawQuery("SELECT * FROM table_name;", null));
    assertThat(adapter.getCursor()).isNotSameAs(curs);
  }

  private static class TestAdapter extends CursorAdapter {

    public TestAdapter(Cursor curs) {
      super(RuntimeEnvironment.application, curs, false);
    }

    @Override
    public void bindView(View view, Context context, Cursor cursor) {
    }

    @Override
    public View newView(Context context, Cursor cursor, ViewGroup parent) {
      return null;
    }
  }

  private static class TestAdapterWithFlags extends CursorAdapter {
    public TestAdapterWithFlags(Cursor c, int flags) {
      super(RuntimeEnvironment.application, c, flags);
    }

    @Override public View newView(Context context, Cursor cursor, ViewGroup parent) {
      return null;
    }

    @Override public void bindView(View view, Context context, Cursor cursor) {
    }
  }
}
