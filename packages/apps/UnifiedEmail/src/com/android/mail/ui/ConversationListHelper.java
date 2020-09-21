/*
 * Copyright (C) 2013 Google Inc.
 * Licensed to The Android Open Source Project.
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
package com.android.mail.ui;

import com.google.common.collect.Lists;

import android.content.Context;
import android.view.LayoutInflater;

import com.android.mail.providers.Account;

import com.android.mail.R;

import java.util.ArrayList;

public class ConversationListHelper {
    /**
     * Creates a list of newly created special views.
     */
    public ArrayList<ConversationSpecialItemView> makeConversationListSpecialViews(
            final Context context, final ControllableActivity activity, final Account account) {
        // Note that these teasers have to be added in the order of importance because it's a stack,
        // thus the last teaser added will appear on top.

        // Sync disabled teaser view
        final ConversationSyncDisabledTipView conversationSyncDisabledTipView =
                new ConversationSyncDisabledTipView(context);
        conversationSyncDisabledTipView.bindAccount(account, activity);

        // Message in outbox teaser view
        final ConversationsInOutboxTipView conversationsInOutboxTipView =
                new ConversationsInOutboxTipView(context);
        conversationsInOutboxTipView.bind(account, activity.getFolderSelector());

        // Conversation photo teaser view
        final ConversationPhotoTeaserView conversationPhotoTeaser =
                new ConversationPhotoTeaserView(context);

        // Long press to select tip
        final ConversationLongPressTipView conversationLongPressTipView =
                new ConversationLongPressTipView(context);

        final NestedFolderTeaserView nestedFolderTeaserView =
                (NestedFolderTeaserView) LayoutInflater.from(context)
                        .inflate(R.layout.nested_folder_teaser_view, null);
        nestedFolderTeaserView.bind(account, activity.getFolderSelector());

        // Order matters.  If a and b are added in order itemViews.add(a), itemViews.add(b),
        // they will appear in conversation list as:
        // b
        // a
        final ArrayList<ConversationSpecialItemView> itemViews = Lists.newArrayList();
        itemViews.add(conversationPhotoTeaser);
        itemViews.add(conversationLongPressTipView);
        itemViews.add(conversationSyncDisabledTipView);
        itemViews.add(conversationsInOutboxTipView);
        itemViews.add(nestedFolderTeaserView);
        return itemViews;
    }
}
