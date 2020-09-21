package com.android.car.media.widgets;

import android.annotation.Nullable;
import android.content.Context;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.widget.RelativeLayout;
import android.widget.SeekBar;
import android.widget.TextView;

import com.android.car.media.MetadataController;
import com.android.car.media.R;
import com.android.car.media.common.PlaybackModel;

/**
 * A view that can be used to display the metadata and playback progress of a media item.
 * This view can be styled using the "MetadataView" styleable attributes.
 */
public class MetadataView extends RelativeLayout {
    private final MetadataController mMetadataController;

    public MetadataView(Context context) {
        this(context, null);
    }

    public MetadataView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public MetadataView(Context context, AttributeSet attrs, int defStyleAttr) {
        this(context, attrs, defStyleAttr, 0);
    }

    public MetadataView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        LayoutInflater.from(context).inflate(R.layout.metadata_compact, this, true);
        TextView title = findViewById(R.id.title);
        TextView subtitle = findViewById(R.id.subtitle);
        SeekBar seekBar = findViewById(R.id.seek_bar);
        mMetadataController = new MetadataController(title, subtitle, null, seekBar, null);
    }

    /**
     * Registers the {@link PlaybackModel} this widget will use to follow playback state.
     * Consumers of this class must unregister the {@link PlaybackModel} by calling this method with
     * null.
     *
     * @param model {@link PlaybackModel} to subscribe, or null to unsubscribe.
     */
    public void setModel(@Nullable PlaybackModel model) {
        mMetadataController.setModel(model);
    }

}
