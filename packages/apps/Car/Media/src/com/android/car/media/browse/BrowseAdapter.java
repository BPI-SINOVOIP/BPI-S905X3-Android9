/*
 * Copyright 2018 The Android Open Source Project
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

package com.android.car.media.browse;

import android.content.Context;
import android.media.browse.MediaBrowser;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.util.DiffUtil;
import android.support.v7.widget.GridLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.android.car.media.common.MediaItemMetadata;
import com.android.car.media.common.MediaSource;

import java.util.ArrayList;
import java.util.Collection;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Objects;
import java.util.function.Consumer;
import java.util.stream.Collectors;

import androidx.car.widget.PagedListView;

/**
 * A {@link RecyclerView.Adapter} that can be used to display a single level of a
 * {@link android.service.media.MediaBrowserService} media tree into a
 * {@link androidx.car.widget.PagedListView} or any other {@link RecyclerView}.
 *
 * <p>This adapter assumes that the attached {@link RecyclerView} uses a {@link GridLayoutManager},
 * as it can use both grid and list elements to produce the desired representation.
 *
 * <p> The actual strategy to group and expand media items has to be supplied by providing an
 * instance of {@link ContentForwardStrategy}.
 *
 * <p> The adapter will only start updating once {@link #start()} is invoked. At this point, the
 * provided {@link MediaBrowser} must be already in connected state.
 *
 * <p>Resources and asynchronous data loading must be released by callign {@link #stop()}.
 *
 * <p>No views will be actually updated until {@link #update()} is invoked (normally as a result of
 * the {@link Observer#onDirty()} event. This way, the consumer of this adapter has the opportunity
 * to decide whether updates should be displayd immediately, or if they should be delayed to
 * prevent flickering.
 *
 * <p>Consumers of this adapter should use {@link #registerObserver(Observer)} to receive updates.
 */
public class BrowseAdapter extends RecyclerView.Adapter<BrowseViewHolder> implements
        PagedListView.DividerVisibilityManager {
    private static final String TAG = "BrowseAdapter";
    @NonNull
    private final Context mContext;
    private final MediaSource mMediaSource;
    private final ContentForwardStrategy mCFBStrategy;
    private MediaItemMetadata mParentMediaItem;
    private LinkedHashMap<String, MediaItemState> mItemStates = new LinkedHashMap<>();
    private List<BrowseViewData> mViewData = new ArrayList<>();
    private String mParentMediaItemId;
    private List<Observer> mObservers = new ArrayList<>();
    private List<MediaItemMetadata> mQueue;
    private CharSequence mQueueTitle;
    private int mMaxSpanSize = 1;
    private State mState = State.IDLE;

    /**
     * Possible states of the adapter
     */
    public enum State {
        /** Loading of this item hasn't started yet */
        IDLE,
        /** There is pending information before this item can be displayed */
        LOADING,
        /** It was not possible to load metadata for this item */
        ERROR,
        /** Metadata for this items has been correctly loaded */
        LOADED
    }

    /**
     * An {@link BrowseAdapter} observer.
     */
    public static abstract class Observer {
        /**
         * Callback invoked anytime there is more information to be displayed, or if there is a
         * change in the overall state of the adapter.
         */
        protected void onDirty() {};

        /**
         * Callback invoked when a user clicks on a playable item.
         */
        protected void onPlayableItemClicked(MediaItemMetadata item) {};

        /**
         * Callback invoked when a user clicks on a browsable item.
         */
        protected void onBrowseableItemClicked(MediaItemMetadata item) {};

        /**
         * Callback invoked when a user clicks on a the "more items" button on a section.
         */
        protected void onMoreButtonClicked(MediaItemMetadata item) {};

        /**
         * Callback invoked when the user clicks on the title of the queue.
         */
        protected void onQueueTitleClicked() {};

        /**
         * Callback invoked when the user clicks on a queue item.
         */
        protected void onQueueItemClicked(MediaItemMetadata item) {};
    }

    private MediaSource.ItemsSubscription mSubscriptionCallback =
            (mediaSource, parentId, items) -> {
                if (items != null) {
                    onItemsLoaded(parentId, items);
                } else {
                    onLoadingError(parentId);
                }
            };


    /**
     * Represents the loading state of children of a single {@link MediaItemMetadata} in the
     * {@link BrowseAdapter}
     */
    private class MediaItemState {
        /**
         * {@link com.android.car.media.common.MediaItemMetadata} whose children are being loaded
         */
        final MediaItemMetadata mItem;
        /** Current loading state for this item */
        State mState = State.LOADING;
        /** Playable children of the given item */
        List<MediaItemMetadata> mPlayableChildren = new ArrayList<>();
        /** Browsable children of the given item */
        List<MediaItemMetadata> mBrowsableChildren = new ArrayList<>();
        /** Whether we are subscribed to updates for this item or not */
        boolean mIsSubscribed;

        MediaItemState(MediaItemMetadata item) {
            mItem = item;
        }

        void setChildren(List<MediaItemMetadata> children) {
            mPlayableChildren.clear();
            mBrowsableChildren.clear();
            for (MediaItemMetadata child : children) {
                if (child.isBrowsable()) {
                    // Browsable items could also be playable
                    mBrowsableChildren.add(child);
                } else if (child.isPlayable()) {
                    mPlayableChildren.add(child);
                }
            }
        }
    }

    /**
     * Creates a {@link BrowseAdapter} that displays the children of the given media tree node.
     *
     * @param mediaSource the {@link MediaSource} to get data from.
     * @param parentItem the node to display children of, or NULL if the
     * @param strategy a {@link ContentForwardStrategy} that would determine which items would be
     *                 expanded and how.
     */
    public BrowseAdapter(Context context, @NonNull MediaSource mediaSource,
            @Nullable MediaItemMetadata parentItem, @NonNull ContentForwardStrategy strategy) {
        mContext = context;
        mMediaSource = mediaSource;
        mParentMediaItem = parentItem;
        mCFBStrategy = strategy;
    }

    /**
     * Initiates or resumes the data loading process and subscribes to updates. The client can use
     * {@link #registerObserver(Observer)} to receive updates on the progress.
     */
    public void start() {
        mParentMediaItemId = mParentMediaItem != null ? mParentMediaItem.getId() :
                mMediaSource.getRoot();
        mMediaSource.subscribeChildren(mParentMediaItemId, mSubscriptionCallback);
        for (MediaItemState itemState : mItemStates.values()) {
            subscribe(itemState);
        }
    }

    /**
     * Stops the data loading and releases any subscriptions.
     */
    public void stop() {
        if (mParentMediaItemId == null) {
            // Not started
            return;
        }
        mMediaSource.unsubscribeChildren(mParentMediaItemId, mSubscriptionCallback);
        for (MediaItemState itemState : mItemStates.values()) {
            unsubscribe(itemState);
        }
        mParentMediaItemId = null;
    }

    /**
     * Replaces the media item whose children are being displayed in this adapter. The content of
     * the adapter will be replaced once the children of the new item are loaded.
     *
     * @param parentItem new media item to expand.
     */
    public void setParentMediaItemId(@Nullable MediaItemMetadata parentItem) {
        String newParentMediaItemId = parentItem != null ? parentItem.getId() :
                mMediaSource.getRoot();
        if (Objects.equals(newParentMediaItemId, mParentMediaItemId)) {
            return;
        }
        stop();
        mParentMediaItem = parentItem;
        mParentMediaItemId = newParentMediaItemId;
        mMediaSource.subscribeChildren(mParentMediaItemId, mSubscriptionCallback);
    }

    /**
     * Sets media queue items into this adapter.
     */
    public void setQueue(List<MediaItemMetadata> items, CharSequence queueTitle) {
        mQueue = items;
        mQueueTitle = queueTitle;
        notify(Observer::onDirty);
    }

    /**
     * Registers an {@link Observer}
     */
    public void registerObserver(Observer observer) {
        mObservers.add(observer);
    }

    /**
     * Unregisters an {@link Observer}
     */
    public void unregisterObserver(Observer observer) {
        mObservers.remove(observer);
    }

    /**
     * @return the global loading state. Consumers can use this state to determine if more
     * information is still pending to arrive or not. This method will report
     * {@link State#ERROR} only if the list of immediate children fails to load.
     */
    public State getState() {
        return mState;
    }

    /**
     * Sets the number of columns that items can take. This method only needs to be used if the
     * attached {@link RecyclerView} is NOT using a {@link GridLayoutManager}. This class will
     * automatically determine this value on {@link #onAttachedToRecyclerView(RecyclerView)}
     * otherwise.
     */
    public void setMaxSpanSize(int maxSpanSize) {
        mMaxSpanSize = maxSpanSize;
    }

    /**
     * @return a {@link GridLayoutManager.SpanSizeLookup} that can be used to obtain the span size
     * of each item in this adapter. This method is only needed if the {@link RecyclerView} is NOT
     * using a {@link GridLayoutManager}. This class will automatically use it on\
     * {@link #onAttachedToRecyclerView(RecyclerView)} otherwise.
     */
    public GridLayoutManager.SpanSizeLookup getSpanSizeLookup() {
        return new GridLayoutManager.SpanSizeLookup() {
            @Override
            public int getSpanSize(int position) {
                BrowseItemViewType viewType = mViewData.get(position).mViewType;
                return viewType.getSpanSize(mMaxSpanSize);
            }
        };
    }

    /**
     * Updates the {@link RecyclerView} with newly loaded information. This normally should be
     * invoked as a result of a {@link Observer#onDirty()} callback.
     *
     * This method is idempotent and can be used at any time (even delayed if needed). Additions,
     * removals and insertions would be notified to the {@link RecyclerView} so it can be
     * animated appropriately.
     */
    public void update() {
        List<BrowseViewData> newItems = generateViewData(mItemStates.values());
        List<BrowseViewData> oldItems = mViewData;
        mViewData = newItems;
        DiffUtil.DiffResult result = DiffUtil.calculateDiff(createDiffUtil(oldItems, newItems));
        result.dispatchUpdatesTo(this);
    }

    private void subscribe(MediaItemState state) {
        if (!state.mIsSubscribed && state.mItem.isBrowsable()) {
            mMediaSource.subscribeChildren(state.mItem.getId(), mSubscriptionCallback);
            state.mIsSubscribed = true;
        } else {
            state.mState = State.LOADED;
        }
    }

    private void unsubscribe(MediaItemState state) {
        if (state.mIsSubscribed) {
            mMediaSource.unsubscribeChildren(state.mItem.getId(), mSubscriptionCallback);
            state.mIsSubscribed = false;
        }
    }

    @NonNull
    @Override
    public BrowseViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        int layoutId = BrowseItemViewType.values()[viewType].getLayoutId();
        View view = LayoutInflater.from(mContext).inflate(layoutId, parent, false);
        return new BrowseViewHolder(view);
    }

    @Override
    public void onBindViewHolder(@NonNull BrowseViewHolder holder, int position) {
        BrowseViewData viewData = mViewData.get(position);
        holder.bind(mContext, viewData);
    }

    @Override
    public int getItemCount() {
        return mViewData.size();
    }

    @Override
    public int getItemViewType(int position) {
        return mViewData.get(position).mViewType.ordinal();
    }

    private void onItemsLoaded(String parentId, List<MediaItemMetadata> children) {
        if (parentId.equals(mParentMediaItemId)) {
            // Direct children from the requested media item id. Update subscription list.
            LinkedHashMap<String, MediaItemState> newItemStates = new LinkedHashMap<>();
            List<MediaItemState> itemsToSubscribe = new ArrayList<>();
            for (MediaItemMetadata item : children) {
                MediaItemState itemState = mItemStates.get(item.getId());
                if (itemState != null) {
                    // Reuse existing section.
                    newItemStates.put(item.getId(), itemState);
                    mItemStates.remove(item.getId());
                } else {
                    // New section, subscribe to it.
                    itemState = new MediaItemState(item);
                    newItemStates.put(item.getId(), itemState);
                    itemsToSubscribe.add(itemState);
                }
            }
            // Remove unused sections
            for (MediaItemState itemState : mItemStates.values()) {
                unsubscribe(itemState);
            }
            mItemStates = newItemStates;
            // Subscribe items once we have updated the map (updates might happen synchronously
            // if data is already available).
            for (MediaItemState itemState : itemsToSubscribe) {
                subscribe(itemState);
            }
        } else {
            MediaItemState itemState = mItemStates.get(parentId);
            if (itemState == null) {
                Log.w(TAG, "Loaded children for a section we don't have: " + parentId);
                return;
            }
            itemState.setChildren(children);
            itemState.mState = State.LOADED;
        }
        updateGlobalState();
        notify(Observer::onDirty);
    }

    private void notify(Consumer<Observer> notification) {
        for (Observer observer : mObservers) {
            notification.accept(observer);
        }
    }

    private void onLoadingError(String parentId) {
        if (parentId.equals(mParentMediaItemId)) {
            mState = State.ERROR;
        } else {
            MediaItemState state = mItemStates.get(parentId);
            if (state == null) {
                Log.w(TAG, "Error loading children for a section we don't have: " + parentId);
                return;
            }
            state.setChildren(new ArrayList<>());
            state.mState = State.ERROR;
            updateGlobalState();
        }
        notify(Observer::onDirty);
    }

    private void updateGlobalState() {
        for (MediaItemState state: mItemStates.values()) {
            if (state.mState == State.LOADING) {
                mState = State.LOADING;
                return;
            }
        }
        mState = State.LOADED;
    }

    private DiffUtil.Callback createDiffUtil(List<BrowseViewData> oldList,
            List<BrowseViewData> newList) {
        return new DiffUtil.Callback() {
            @Override
            public int getOldListSize() {
                return oldList.size();
            }

            @Override
            public int getNewListSize() {
                return newList.size();
            }

            @Override
            public boolean areItemsTheSame(int oldPos, int newPos) {
                BrowseViewData oldItem = oldList.get(oldPos);
                BrowseViewData newItem = newList.get(newPos);

                return Objects.equals(oldItem.mMediaItem, newItem.mMediaItem)
                        && Objects.equals(oldItem.mText, newItem.mText);
            }

            @Override
            public boolean areContentsTheSame(int oldPos, int newPos) {
                BrowseViewData oldItem = oldList.get(oldPos);
                BrowseViewData newItem = newList.get(newPos);

                return oldItem.equals(newItem);
            }
        };
    }

    @Override
    public void onAttachedToRecyclerView(RecyclerView recyclerView) {
        if (recyclerView.getLayoutManager() instanceof GridLayoutManager) {
            GridLayoutManager manager = (GridLayoutManager) recyclerView.getLayoutManager();
            mMaxSpanSize = manager.getSpanCount();
            manager.setSpanSizeLookup(getSpanSizeLookup());
        }
    }

    private class ItemsBuilder {
        private List<BrowseViewData> result = new ArrayList<>();

        void addItem(MediaItemMetadata item, State state,
                BrowseItemViewType viewType, Consumer<Observer> notification) {
            View.OnClickListener listener = notification != null ?
                    view -> BrowseAdapter.this.notify(notification) :
                    null;
            result.add(new BrowseViewData(item, viewType, state, listener));
        }

        void addItems(List<MediaItemMetadata> items, BrowseItemViewType viewType, int maxRows) {
            int spanSize = viewType.getSpanSize(mMaxSpanSize);
            int maxChildren = maxRows * (mMaxSpanSize / spanSize);
            result.addAll(items.stream()
                    .limit(maxChildren)
                    .map(item -> {
                        Consumer<Observer> notification = item.getQueueId() != null
                                ? observer -> observer.onQueueItemClicked(item)
                                : item.isBrowsable()
                                        ? observer -> observer.onBrowseableItemClicked(item)
                                        : observer -> observer.onPlayableItemClicked(item);
                        return new BrowseViewData(item, viewType, null, view ->
                                BrowseAdapter.this.notify(notification));
                    })
                    .collect(Collectors.toList()));
        }

        void addTitle(CharSequence title, Consumer<Observer> notification) {
            result.add(new BrowseViewData(title, BrowseItemViewType.HEADER,
                    view -> BrowseAdapter.this.notify(notification)));

        }

        void addBrowseBlock(MediaItemMetadata header, State state,
                List<MediaItemMetadata> items, BrowseItemViewType viewType, int maxChildren,
                boolean showHeader, boolean showMoreFooter) {
            if (showHeader) {
                addItem(header, state, BrowseItemViewType.HEADER, null);
            }
            addItems(items, viewType, maxChildren);
            if (showMoreFooter) {
                addItem(header, null, BrowseItemViewType.MORE_FOOTER,
                        observer -> observer.onMoreButtonClicked(header));
            }
        }

        List<BrowseViewData> build() {
            return result;
        }
    }

    /**
     * Flatten the given collection of item states into a list of {@link BrowseViewData}s. To avoid
     * flickering, the flatting will stop at the first "loading" section, avoiding unnecessary
     * insertion animations during the initial data load.
     */
    private List<BrowseViewData> generateViewData(Collection<MediaItemState> itemStates) {
        ItemsBuilder itemsBuilder = new ItemsBuilder();

        if (Log.isLoggable(TAG, Log.VERBOSE)) {
            Log.v(TAG, "Generating browse view from:");
            for (MediaItemState item : itemStates) {
                Log.v(TAG, String.format("[%s%s] '%s' (%s)",
                        item.mItem.isBrowsable() ? "B" : " ",
                        item.mItem.isPlayable() ? "P" : " ",
                        item.mItem.getTitle(),
                        item.mItem.getId()));
                List<MediaItemMetadata> items = new ArrayList<>();
                items.addAll(item.mBrowsableChildren);
                items.addAll(item.mPlayableChildren);
                for (MediaItemMetadata child : items) {
                    Log.v(TAG, String.format("   [%s%s] '%s' (%s)",
                            child.isBrowsable() ? "B" : " ",
                            child.isPlayable() ? "P" : " ",
                            child.getTitle(),
                            child.getId()));
                }
            }
        }

        if (mQueue != null && !mQueue.isEmpty() && mCFBStrategy.getMaxQueueRows() > 0
                && mCFBStrategy.getQueueViewType() != null) {
            if (mQueueTitle != null) {
                itemsBuilder.addTitle(mQueueTitle, Observer::onQueueTitleClicked);
            }
            itemsBuilder.addItems(mQueue, mCFBStrategy.getQueueViewType(),
                    mCFBStrategy.getMaxQueueRows());
        }

        boolean containsBrowsableItems = false;
        boolean containsPlayableItems = false;
        for (MediaItemState itemState : itemStates) {
            containsBrowsableItems |= itemState.mItem.isBrowsable();
            containsPlayableItems |= itemState.mItem.isPlayable();
        }

        for (MediaItemState itemState : itemStates) {
            MediaItemMetadata item = itemState.mItem;
            if (containsPlayableItems && containsBrowsableItems) {
                // If we have a mix of browsable and playable items: show them all in a list
                itemsBuilder.addItem(item, itemState.mState,
                        BrowseItemViewType.PANEL_ITEM,
                        item.isBrowsable()
                                ? observer -> observer.onBrowseableItemClicked(item)
                                : observer -> observer.onPlayableItemClicked(item));
            } else if (itemState.mItem.isBrowsable()) {
                // If we only have browsable items, check whether we should expand them or not.
                if (!itemState.mBrowsableChildren.isEmpty()
                        && !itemState.mPlayableChildren.isEmpty()
                        || !mCFBStrategy.shouldBeExpanded(item)) {
                    itemsBuilder.addItem(item, itemState.mState,
                            mCFBStrategy.getBrowsableViewType(mParentMediaItem), null);
                } else if (!itemState.mPlayableChildren.isEmpty()) {
                    itemsBuilder.addBrowseBlock(item,
                            itemState.mState,
                            itemState.mPlayableChildren,
                            mCFBStrategy.getPlayableViewType(item),
                            mCFBStrategy.getMaxRows(item, mCFBStrategy.getPlayableViewType(item)),
                            mCFBStrategy.includeHeader(item),
                            mCFBStrategy.showMoreButton(item));
                } else if (!itemState.mBrowsableChildren.isEmpty()) {
                    itemsBuilder.addBrowseBlock(item,
                            itemState.mState,
                            itemState.mBrowsableChildren,
                            mCFBStrategy.getBrowsableViewType(item),
                            mCFBStrategy.getMaxRows(item, mCFBStrategy.getBrowsableViewType(item)),
                            mCFBStrategy.includeHeader(item),
                            mCFBStrategy.showMoreButton(item));
                }
            } else if (item.isPlayable()) {
                // If we only have playable items: show them as so.
                itemsBuilder.addItem(item, itemState.mState,
                        mCFBStrategy.getPlayableViewType(mParentMediaItem),
                        observer -> observer.onPlayableItemClicked(item));
            }
        }

        return itemsBuilder.build();
    }

    @Override
    public boolean shouldHideDivider(int position) {
        return position >= mViewData.size() - 1
                || position < 0
                || mViewData.get(position).mViewType != BrowseItemViewType.PANEL_ITEM
                || mViewData.get(position + 1).mViewType != BrowseItemViewType.PANEL_ITEM;
    }
}
