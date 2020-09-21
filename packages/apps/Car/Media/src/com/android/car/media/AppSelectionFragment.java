package com.android.car.media;

import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v7.widget.GridLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import com.android.car.media.common.MediaSource;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;

import androidx.car.widget.PagedListView;

/**
 * A {@link Fragment} that implements the app selection UI
 */
public class AppSelectionFragment extends Fragment {
    private AppGridAdapter mGridAdapter;
    @Nullable
    private Callbacks mCallbacks;

    private class AppGridAdapter extends RecyclerView.Adapter<AppItemViewHolder> {
        private final LayoutInflater mInflater;
        private List<MediaSource> mMediaSources;

        AppGridAdapter() {
            mInflater = LayoutInflater.from(getContext());
        }

        /**
         * Triggers a refresh of media sources
         */
        void updateSources(List<MediaSource> mediaSources) {
            mMediaSources = new ArrayList<>(mediaSources);
            notifyDataSetChanged();
        }

        @Override
        public AppItemViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
            View view = mInflater.inflate(R.layout.app_selection_item, parent, false);
            return new AppItemViewHolder(view, getContext());

        }

        @Override
        public void onBindViewHolder(AppItemViewHolder vh, int position) {
            vh.bind(mMediaSources.get(position));
        }

        @Override
        public int getItemCount() {
            return mMediaSources.size();
        }
    }

    private class AppItemViewHolder extends RecyclerView.ViewHolder {
        private final Context mContext;
        public View mAppItem;
        public ImageView mAppIconView;
        public TextView mAppNameView;

        public AppItemViewHolder(View view, Context context) {
            super(view);
            mContext = context;
            mAppItem = view.findViewById(R.id.app_item);
            mAppIconView = mAppItem.findViewById(R.id.app_icon);
            mAppNameView = mAppItem.findViewById(R.id.app_name);
        }

        /**
         * Binds a media source to a view
         */
        public void bind(MediaSource mediaSource) {
            // Empty out the view
            mAppItem.setOnClickListener(v -> {
                if (mCallbacks != null) {
                    mCallbacks.onMediaSourceSelected(mediaSource);
                }
            });
            mAppIconView.setImageDrawable(mediaSource.getPackageIcon());
            mAppNameView.setText(mediaSource.getName());;
        }
    }

    /**
     * Fragment callbacks (implemented by the hosting Activity)
     */
    public interface Callbacks {
        /**
         * Invoked whenever this fragment requires to obtain the list of media source to select
         * from.
         */
        List<MediaSource> getMediaSources();

        /**
         * Invoked when the user makes a selection
         */
        void onMediaSourceSelected(MediaSource mediaSource);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, final ViewGroup container,
            Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.fragment_app_selection, container, false);
        int columnNumber = getResources().getInteger(R.integer.num_app_selector_columns);
        mGridAdapter = new AppGridAdapter();
        PagedListView gridView = view.findViewById(R.id.apps_grid);

        GridLayoutManager gridLayoutManager = new GridLayoutManager(getContext(), columnNumber);
        gridView.getRecyclerView().setLayoutManager(gridLayoutManager);
        gridView.setAdapter(mGridAdapter);
        return view;
    }

    @Override
    public void onAttach(Context context) {
        super.onAttach(context);
        mCallbacks = (Callbacks) context;
    }

    @Override
    public void onDetach() {
        super.onDetach();
        mCallbacks = null;
    }

    @Override
    public void onResume() {
        super.onResume();
        refresh();
    }

    /**
     * Refreshes the list of media sources.
     */
    public void refresh() {
        if (mCallbacks != null) {
            mGridAdapter.updateSources(mCallbacks.getMediaSources());
        }
    }
}
