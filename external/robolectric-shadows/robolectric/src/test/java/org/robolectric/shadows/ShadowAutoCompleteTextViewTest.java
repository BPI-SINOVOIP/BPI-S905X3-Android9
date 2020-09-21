package org.robolectric.shadows;

import static org.assertj.core.api.Assertions.assertThat;

import android.content.Context;
import android.widget.ArrayAdapter;
import android.widget.AutoCompleteTextView;
import android.widget.Filter;
import android.widget.Filterable;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;

@RunWith(RobolectricTestRunner.class)
public class ShadowAutoCompleteTextViewTest {
  private final AutoCompleteAdapter adapter = new AutoCompleteAdapter(RuntimeEnvironment.application);

  @Test
  public void shouldInvokeFilter() throws Exception {
    Robolectric.getForegroundThreadScheduler().pause();
    AutoCompleteTextView view = new AutoCompleteTextView(RuntimeEnvironment.application);
    view.setAdapter(adapter);

    view.setText("Foo");
    assertThat(adapter.getCount()).isEqualTo(2);
  }

  private class AutoCompleteAdapter extends ArrayAdapter<String> implements Filterable {
    public AutoCompleteAdapter(Context context) {
      super(context, android.R.layout.simple_list_item_1);
    }

    @Override
    public Filter getFilter() {
      return new AutoCompleteFilter();
    }
  }

  private class AutoCompleteFilter extends Filter {
    @Override
    protected FilterResults performFiltering(CharSequence text) {
      FilterResults results = new FilterResults();
      if (text != null) {
        results.count = 2;
        results.values = new ArrayList<>(Arrays.asList("Foo", "Bar"));
      }
      return results;
    }

    @Override
    protected void publishResults(CharSequence text, FilterResults results) {
      if (results != null) {
        adapter.clear();
        adapter.addAll((List<String>) results.values);
        adapter.notifyDataSetChanged();
      }
    }
  }
}
