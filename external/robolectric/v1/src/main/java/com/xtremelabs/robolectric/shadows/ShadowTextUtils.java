package com.xtremelabs.robolectric.shadows;

import android.text.TextUtils;
import com.xtremelabs.robolectric.internal.Implementation;
import com.xtremelabs.robolectric.internal.Implements;
import com.xtremelabs.robolectric.util.Join;

import java.util.Collection;

@SuppressWarnings({"UnusedDeclaration"})
@Implements(TextUtils.class)
public class ShadowTextUtils {
    @Implementation
    public static CharSequence expandTemplate(CharSequence template,
                                              CharSequence... values) {
        String s = template.toString();
        for (int i = 0, valuesLength = values.length; i < valuesLength; i++) {
            CharSequence value = values[i];
            s = s.replace("^" + (i + 1), value);
        }
        return s;
    }

    @Implementation
    public static boolean isEmpty(CharSequence s) {
      return (s == null || s.length() == 0);
    }

    @Implementation
    public static int getTrimmedLength(CharSequence s) {
        int len = s.length();

        int start = 0;
        while (start < len && s.charAt(start) <= ' ') {
            start++;
        }

        int end = len;
        while (end > start && s.charAt(end - 1) <= ' ') {
            end--;
        }

        return end - start;
    }

    @Implementation
    public static String join(CharSequence delimiter, Iterable tokens) {
        return Join.join((String) delimiter, (Collection) tokens);
    }

    @Implementation
    public static String join(CharSequence delimiter, Object[] tokens) {
        return Join.join((String) delimiter, tokens);
    }

    @Implementation
    public static boolean isDigitsOnly(CharSequence str) {
        final int len = str.length();
        for (int i = 0; i < len; i++) {
          if (!Character.isDigit(str.charAt(i))) {
            return false;
          }
        }
        return true;
    }
    
    @Implementation
    public static String[] split(String text, String expression) {
    	if(text.length() == 0) {
    		return new String[]{};
    	}
    	
    	return text.split(expression);
    }
}
