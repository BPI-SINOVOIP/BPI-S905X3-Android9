/*
 * Copyright (C) 2015 The Android Open Source Project
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
package android.databinding.adapters;

import com.android.databinding.library.baseAdapters.R;

import android.databinding.BindingAdapter;
import android.databinding.BindingMethod;
import android.databinding.BindingMethods;
import android.databinding.InverseBindingAdapter;
import android.databinding.InverseBindingListener;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.text.Editable;
import android.text.InputFilter;
import android.text.InputType;
import android.text.Spanned;
import android.text.SpannableString;
import android.text.SpannableStringBuilder;
import android.text.TextWatcher;
import android.text.method.DialerKeyListener;
import android.text.method.DigitsKeyListener;
import android.text.method.KeyListener;
import android.text.method.PasswordTransformationMethod;
import android.text.method.TextKeyListener;
import android.util.Log;
import android.util.TypedValue;
import android.widget.TextView;

@BindingMethods({
        @BindingMethod(type = TextView.class, attribute = "android:autoLink", method = "setAutoLinkMask"),
        @BindingMethod(type = TextView.class, attribute = "android:drawablePadding", method = "setCompoundDrawablePadding"),
        @BindingMethod(type = TextView.class, attribute = "android:editorExtras", method = "setInputExtras"),
        @BindingMethod(type = TextView.class, attribute = "android:inputType", method = "setRawInputType"),
        @BindingMethod(type = TextView.class, attribute = "android:scrollHorizontally", method = "setHorizontallyScrolling"),
        @BindingMethod(type = TextView.class, attribute = "android:textAllCaps", method = "setAllCaps"),
        @BindingMethod(type = TextView.class, attribute = "android:textColorHighlight", method = "setHighlightColor"),
        @BindingMethod(type = TextView.class, attribute = "android:textColorHint", method = "setHintTextColor"),
        @BindingMethod(type = TextView.class, attribute = "android:textColorLink", method = "setLinkTextColor"),
        @BindingMethod(type = TextView.class, attribute = "android:onEditorAction", method = "setOnEditorActionListener"),
})
public class TextViewBindingAdapter {

    private static final String TAG = "TextViewBindingAdapters";
    public static final int INTEGER = 0x01;
    public static final int SIGNED = 0x03;
    public static final int DECIMAL = 0x05;

    @BindingAdapter("android:text")
    public static void setText(TextView view, CharSequence text) {
        final CharSequence oldText = view.getText();
        if (text == oldText || (text == null && oldText.length() == 0)) {
            return;
        }
        if (text instanceof Spanned) {
            if (text.equals(oldText)) {
                return; // No change in the spans, so don't set anything.
            }
        } else if (!haveContentsChanged(text, oldText)) {
            return; // No content changes, so don't set anything.
        }
        view.setText(text);
    }

    @InverseBindingAdapter(attribute = "android:text", event = "android:textAttrChanged")
    public static String getTextString(TextView view) {
        return view.getText().toString();
    }

    @BindingAdapter({"android:autoText"})
    public static void setAutoText(TextView view, boolean autoText) {
        KeyListener listener = view.getKeyListener();

        TextKeyListener.Capitalize capitalize = TextKeyListener.Capitalize.NONE;

        int inputType = listener != null ? listener.getInputType() : 0;
        if ((inputType & InputType.TYPE_TEXT_FLAG_CAP_CHARACTERS) != 0) {
            capitalize = TextKeyListener.Capitalize.CHARACTERS;
        } else if ((inputType & InputType.TYPE_TEXT_FLAG_CAP_WORDS) != 0) {
            capitalize = TextKeyListener.Capitalize.WORDS;
        } else if ((inputType & InputType.TYPE_TEXT_FLAG_CAP_SENTENCES) != 0) {
            capitalize = TextKeyListener.Capitalize.SENTENCES;
        }
        view.setKeyListener(TextKeyListener.getInstance(autoText, capitalize));
    }

    @BindingAdapter({"android:capitalize"})
    public static void setCapitalize(TextView view, TextKeyListener.Capitalize capitalize) {
        KeyListener listener = view.getKeyListener();

        int inputType = listener.getInputType();
        boolean autoText = (inputType & InputType.TYPE_TEXT_FLAG_AUTO_CORRECT) != 0;
        view.setKeyListener(TextKeyListener.getInstance(autoText, capitalize));
    }

    @BindingAdapter({"android:bufferType"})
    public static void setBufferType(TextView view, TextView.BufferType bufferType) {
        view.setText(view.getText(), bufferType);
    }

    @BindingAdapter({"android:digits"})
    public static void setDigits(TextView view, CharSequence digits) {
        if (digits != null) {
            view.setKeyListener(DigitsKeyListener.getInstance(digits.toString()));
        } else if (view.getKeyListener() instanceof DigitsKeyListener) {
            view.setKeyListener(null);
        }
    }

    @BindingAdapter({"android:numeric"})
    public static void setNumeric(TextView view, int numeric) {
        view.setKeyListener(DigitsKeyListener.getInstance((numeric & SIGNED) != 0,
                (numeric & DECIMAL) != 0));
    }

    @BindingAdapter({"android:phoneNumber"})
    public static void setPhoneNumber(TextView view, boolean phoneNumber) {
        if (phoneNumber) {
            view.setKeyListener(DialerKeyListener.getInstance());
        } else if (view.getKeyListener() instanceof DialerKeyListener) {
            view.setKeyListener(null);
        }
    }

    private static void setIntrinsicBounds(Drawable drawable) {
        if (drawable != null) {
            drawable.setBounds(0, 0, drawable.getIntrinsicWidth(), drawable.getIntrinsicHeight());
        }
    }

    @BindingAdapter({"android:drawableBottom"})
    public static void setDrawableBottom(TextView view, Drawable drawable) {
        setIntrinsicBounds(drawable);
        Drawable[] drawables = view.getCompoundDrawables();
        view.setCompoundDrawables(drawables[0], drawables[1], drawables[2], drawable);
    }

    @BindingAdapter({"android:drawableLeft"})
    public static void setDrawableLeft(TextView view, Drawable drawable) {
        setIntrinsicBounds(drawable);
        Drawable[] drawables = view.getCompoundDrawables();
        view.setCompoundDrawables(drawable, drawables[1], drawables[2], drawables[3]);
    }

    @BindingAdapter({"android:drawableRight"})
    public static void setDrawableRight(TextView view, Drawable drawable) {
        setIntrinsicBounds(drawable);
        Drawable[] drawables = view.getCompoundDrawables();
        view.setCompoundDrawables(drawables[0], drawables[1], drawable,
                drawables[3]);
    }

    @BindingAdapter({"android:drawableTop"})
    public static void setDrawableTop(TextView view, Drawable drawable) {
        setIntrinsicBounds(drawable);
        Drawable[] drawables = view.getCompoundDrawables();
        view.setCompoundDrawables(drawables[0], drawable, drawables[2],
                drawables[3]);
    }

    @BindingAdapter({"android:drawableStart"})
    public static void setDrawableStart(TextView view, Drawable drawable) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.JELLY_BEAN_MR1) {
            setDrawableLeft(view, drawable);
        } else {
            setIntrinsicBounds(drawable);
            Drawable[] drawables = view.getCompoundDrawablesRelative();
            view.setCompoundDrawablesRelative(drawable, drawables[1], drawables[2], drawables[3]);
        }
    }

    @BindingAdapter({"android:drawableEnd"})
    public static void setDrawableEnd(TextView view, Drawable drawable) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.JELLY_BEAN_MR1) {
            setDrawableRight(view, drawable);
        } else {
            setIntrinsicBounds(drawable);
            Drawable[] drawables = view.getCompoundDrawablesRelative();
            view.setCompoundDrawablesRelative(drawables[0], drawables[1], drawable, drawables[3]);
        }
    }

    @BindingAdapter({"android:imeActionLabel"})
    public static void setImeActionLabel(TextView view, CharSequence value) {
        view.setImeActionLabel(value, view.getImeActionId());
    }

    @BindingAdapter({"android:imeActionId"})
    public static void setImeActionLabel(TextView view, int value) {
        view.setImeActionLabel(view.getImeActionLabel(), value);
    }

    @BindingAdapter({"android:inputMethod"})
    public static void setInputMethod(TextView view, CharSequence inputMethod) {
        try {
            Class<?> c = Class.forName(inputMethod.toString());
            view.setKeyListener((KeyListener) c.newInstance());
        } catch (ClassNotFoundException e) {
            Log.e(TAG, "Could not create input method: " + inputMethod, e);
        } catch (InstantiationException e) {
            Log.e(TAG, "Could not create input method: " + inputMethod, e);
        } catch (IllegalAccessException e) {
            Log.e(TAG, "Could not create input method: " + inputMethod, e);
        }
    }

    @BindingAdapter({"android:lineSpacingExtra"})
    public static void setLineSpacingExtra(TextView view, float value) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
            view.setLineSpacing(value, view.getLineSpacingMultiplier());
        } else {
            view.setLineSpacing(value, 1);
        }
    }

    @BindingAdapter({"android:lineSpacingMultiplier"})
    public static void setLineSpacingMultiplier(TextView view, float value) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
            view.setLineSpacing(view.getLineSpacingExtra(), value);
        } else {
            view.setLineSpacing(0, value);
        }
    }

    @BindingAdapter({"android:maxLength"})
    public static void setMaxLength(TextView view, int value) {
        InputFilter[] filters = view.getFilters();
        if (filters == null) {
            filters = new InputFilter[]{
                    new InputFilter.LengthFilter(value)
            };
        } else {
            boolean foundMaxLength = false;
            for (int i = 0; i < filters.length; i++) {
                InputFilter filter = filters[i];
                if (filter instanceof InputFilter.LengthFilter) {
                    foundMaxLength = true;
                    boolean replace = true;
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                        replace = ((InputFilter.LengthFilter) filter).getMax() != value;
                    }
                    if (replace) {
                        filters[i] = new InputFilter.LengthFilter(value);
                    }
                    break;
                }
            }
            if (!foundMaxLength) {
                // can't use Arrays.copyOf -- it shows up in API 9
                InputFilter[] oldFilters = filters;
                filters = new InputFilter[oldFilters.length + 1];
                System.arraycopy(oldFilters, 0, filters, 0, oldFilters.length);
                filters[filters.length - 1] = new InputFilter.LengthFilter(value);
            }
        }
        view.setFilters(filters);
    }

    @BindingAdapter({"android:password"})
    public static void setPassword(TextView view, boolean password) {
        if (password) {
            view.setTransformationMethod(PasswordTransformationMethod.getInstance());
        } else if (view.getTransformationMethod() instanceof PasswordTransformationMethod) {
            view.setTransformationMethod(null);
        }
    }

    @BindingAdapter({"android:shadowColor"})
    public static void setShadowColor(TextView view, int color) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
            float dx = view.getShadowDx();
            float dy = view.getShadowDy();
            float r = view.getShadowRadius();
            view.setShadowLayer(r, dx, dy, color);
        }
    }

    @BindingAdapter({"android:shadowDx"})
    public static void setShadowDx(TextView view, float dx) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
            int color = view.getShadowColor();
            float dy = view.getShadowDy();
            float r = view.getShadowRadius();
            view.setShadowLayer(r, dx, dy, color);
        }
    }

    @BindingAdapter({"android:shadowDy"})
    public static void setShadowDy(TextView view, float dy) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
            int color = view.getShadowColor();
            float dx = view.getShadowDx();
            float r = view.getShadowRadius();
            view.setShadowLayer(r, dx, dy, color);
        }
    }

    @BindingAdapter({"android:shadowRadius"})
    public static void setShadowRadius(TextView view, float r) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
            int color = view.getShadowColor();
            float dx = view.getShadowDx();
            float dy = view.getShadowDy();
            view.setShadowLayer(r, dx, dy, color);
        }
    }

    @BindingAdapter({"android:textSize"})
    public static void setTextSize(TextView view, float size) {
        view.setTextSize(TypedValue.COMPLEX_UNIT_PX, size);
    }

    private static boolean haveContentsChanged(CharSequence str1, CharSequence str2) {
        if ((str1 == null) != (str2 == null)) {
            return true;
        } else if (str1 == null) {
            return false;
        }
        final int length = str1.length();
        if (length != str2.length()) {
            return true;
        }
        for (int i = 0; i < length; i++) {
            if (str1.charAt(i) != str2.charAt(i)) {
                return true;
            }
        }
        return false;
    }

    @BindingAdapter(value = {"android:beforeTextChanged", "android:onTextChanged",
            "android:afterTextChanged", "android:textAttrChanged"}, requireAll = false)
    public static void setTextWatcher(TextView view, final BeforeTextChanged before,
            final OnTextChanged on, final AfterTextChanged after,
            final InverseBindingListener textAttrChanged) {
        final TextWatcher newValue;
        if (before == null && after == null && on == null && textAttrChanged == null) {
            newValue = null;
        } else {
            newValue = new TextWatcher() {
                @Override
                public void beforeTextChanged(CharSequence s, int start, int count, int after) {
                    if (before != null) {
                        before.beforeTextChanged(s, start, count, after);
                    }
                }

                @Override
                public void onTextChanged(CharSequence s, int start, int before, int count) {
                    if (on != null) {
                        on.onTextChanged(s, start, before, count);
                    }
                    if (textAttrChanged != null) {
                        textAttrChanged.onChange();
                    }
                }

                @Override
                public void afterTextChanged(Editable s) {
                    if (after != null) {
                        after.afterTextChanged(s);
                    }
                }
            };
        }
        final TextWatcher oldValue = ListenerUtil.trackListener(view, newValue, R.id.textWatcher);
        if (oldValue != null) {
            view.removeTextChangedListener(oldValue);
        }
        if (newValue != null) {
            view.addTextChangedListener(newValue);
        }
    }

    public interface AfterTextChanged {
        void afterTextChanged(Editable s);
    }

    public interface BeforeTextChanged {
        void beforeTextChanged(CharSequence s, int start, int count, int after);
    }

    public interface OnTextChanged {
        void onTextChanged(CharSequence s, int start, int before, int count);
    }
}
