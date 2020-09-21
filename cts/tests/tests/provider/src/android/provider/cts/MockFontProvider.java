/*
 * Copyright (C) 2017 The Android Open Source Project
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
package android.provider.cts;

import static android.provider.FontsContract.Columns;

import android.content.ContentProvider;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.content.res.AssetManager;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.os.ParcelFileDescriptor;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.StandardCopyOption;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

public class MockFontProvider extends ContentProvider {
    final static String AUTHORITY = "android.provider.fonts.cts.font";

    final static String[] FONT_FILES = {
        "samplefont1.ttf", "samplefont2.ttf", "samplefont.ttc",
    };
    private static final int SAMPLE_FONT_FILE_0_ID = 0;
    private static final int SAMPLE_FONT_FILE_1_ID = 1;
    private static final int SAMPLE_TTC_FONT_FILE_ID = 2;

    static final String SINGLE_FONT_FAMILY_QUERY = "singleFontFamily";
    static final String ALL_ATTRIBUTE_VALUES_QUERY = "allAttributeValues";
    static final String MULTIPLE_FAMILY_QUERY = "multipleFontFamily";
    static final String NOT_FOUND_QUERY = "notFound";
    static final String UNAVAILABLE_QUERY = "unavailable";
    static final String MALFORMED_QUERY = "malformed";
    static final String NOT_FOUND_SECOND_QUERY = "notFoundSecond";
    static final String NOT_FOUND_THIRD_QUERY = "notFoundThird";
    static final String NEGATIVE_ERROR_CODE_QUERY = "negativeCode";
    static final String MANDATORY_FIELDS_ONLY_QUERY = "mandatoryFields";

    static class Font {
        public Font(int id, int fileId, int ttcIndex, String varSettings, int weight, int italic,
                int resultCode, boolean returnAllFields) {
            mId = id;
            mFileId = fileId;
            mTtcIndex = ttcIndex;
            mVarSettings = varSettings;
            mWeight = weight;
            mItalic = italic;
            mResultCode = resultCode;
            mReturnAllFields = returnAllFields;
        }

        public int getId() {
            return mId;
        }

        public int getTtcIndex() {
            return mTtcIndex;
        }

        public String getVarSettings() {
            return mVarSettings;
        }

        public int getWeight() {
            return mWeight;
        }

        public int getItalic() {
            return mItalic;
        }

        public int getResultCode() {
            return mResultCode;
        }

        public int getFileId() {
            return mFileId;
        }

        public boolean isReturnAllFields() {
            return mReturnAllFields;
        }

        private int mId;
        private int mFileId;
        private int mTtcIndex;
        private String mVarSettings;
        private int mWeight;
        private int mItalic;
        private int mResultCode;
        private final boolean mReturnAllFields;
    };

    private static Map<String, Font[]> QUERY_MAP;
    static {
        HashMap<String, Font[]> map = new HashMap<>();
        int id = 0;

        map.put(SINGLE_FONT_FAMILY_QUERY, new Font[] {
            new Font(id++, SAMPLE_FONT_FILE_0_ID, 0, null, 400, 0, Columns.RESULT_CODE_OK, true),
        });

        map.put(MULTIPLE_FAMILY_QUERY, new Font[] {
            new Font(id++, SAMPLE_FONT_FILE_0_ID, 0, null, 400, 0, Columns.RESULT_CODE_OK, true),
            new Font(id++, SAMPLE_FONT_FILE_1_ID, 0, null, 400, 0, Columns.RESULT_CODE_OK, true),
            new Font(id++, SAMPLE_FONT_FILE_0_ID, 0, null, 700, 1, Columns.RESULT_CODE_OK, true),
            new Font(id++, SAMPLE_FONT_FILE_1_ID, 0, null, 700, 1, Columns.RESULT_CODE_OK, true),
        });

        map.put(ALL_ATTRIBUTE_VALUES_QUERY, new Font[] {
                new Font(id++, SAMPLE_TTC_FONT_FILE_ID, 0, "'wght' 100", 400, 0,
                        Columns.RESULT_CODE_OK, true),
                new Font(id++, SAMPLE_TTC_FONT_FILE_ID, 1, "'wght' 100", 700, 1,
                        Columns.RESULT_CODE_OK, true),
        });

        map.put(NOT_FOUND_QUERY, new Font[] {
                new Font(0, 0, 0, null, 400, 0, Columns.RESULT_CODE_FONT_NOT_FOUND, true),
        });

        map.put(UNAVAILABLE_QUERY, new Font[] {
                new Font(0, 0, 0, null, 400, 0, Columns.RESULT_CODE_FONT_UNAVAILABLE, true),
        });

        map.put(MALFORMED_QUERY, new Font[] {
                new Font(0, 0, 0, null, 400, 0, Columns.RESULT_CODE_MALFORMED_QUERY, true),
        });

        map.put(NOT_FOUND_SECOND_QUERY, new Font[] {
                new Font(id++, SAMPLE_FONT_FILE_0_ID, 0, null, 700, 0, Columns.RESULT_CODE_OK,
                        true),
                new Font(0, 0, 0, null, 400, 0, Columns.RESULT_CODE_FONT_NOT_FOUND, true),
        });

        map.put(NOT_FOUND_THIRD_QUERY, new Font[] {
                new Font(id++, SAMPLE_FONT_FILE_0_ID, 0, null, 700, 0, Columns.RESULT_CODE_OK,
                        true),
                new Font(0, 0, 0, null, 400, 0, Columns.RESULT_CODE_FONT_NOT_FOUND, true),
                new Font(id++, SAMPLE_FONT_FILE_0_ID, 0, null, 700, 0, Columns.RESULT_CODE_OK,
                        true),
        });

        map.put(NEGATIVE_ERROR_CODE_QUERY, new Font[] {
                new Font(id++, SAMPLE_FONT_FILE_0_ID, 0, null, 700, 0, -5, true),
        });

        map.put(MANDATORY_FIELDS_ONLY_QUERY, new Font[] {
                new Font(id++, SAMPLE_FONT_FILE_0_ID, 0, null, 400, 0,
                        Columns.RESULT_CODE_OK, false),
        });


        QUERY_MAP = Collections.unmodifiableMap(map);
    }

    private static Cursor buildCursor(Font[] in) {
        if (!in[0].mReturnAllFields) {
            MatrixCursor cursor = new MatrixCursor(new String[] { Columns._ID, Columns.FILE_ID });
            for (Font font : in) {
                MatrixCursor.RowBuilder builder = cursor.newRow();
                builder.add(Columns._ID, font.getId());
                builder.add(Columns.FILE_ID, font.getFileId());
            }
            return cursor;
        }
        MatrixCursor cursor = new MatrixCursor(new String[] {
                Columns._ID, Columns.TTC_INDEX, Columns.VARIATION_SETTINGS, Columns.WEIGHT,
                Columns.ITALIC, Columns.RESULT_CODE, Columns.FILE_ID});
        for (Font font : in) {
            MatrixCursor.RowBuilder builder = cursor.newRow();
            builder.add(Columns._ID, font.getId());
            builder.add(Columns.FILE_ID, font.getFileId());
            builder.add(Columns.TTC_INDEX, font.getTtcIndex());
            builder.add(Columns.VARIATION_SETTINGS, font.getVarSettings());
            builder.add(Columns.WEIGHT, font.getWeight());
            builder.add(Columns.ITALIC, font.getItalic());
            builder.add(Columns.RESULT_CODE, font.getResultCode());
        }
        return cursor;
    }

    public static void prepareFontFiles(Context context) {
        final AssetManager mgr = context.getAssets();
        for (String path : FONT_FILES) {
            try (InputStream is = mgr.open(path)) {
                Files.copy(is, getCopiedFile(context, path).toPath(),
                        StandardCopyOption.REPLACE_EXISTING);
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        }
    }

    public static void cleanUpFontFiles(Context context) {
        for (String file : FONT_FILES) {
            getCopiedFile(context, file).delete();
        }
    }

    public static File getCopiedFile(Context context, String path) {
        return new File(context.getFilesDir(), path);
    }

    @Override
    public ParcelFileDescriptor openFile(Uri uri, String mode) {
        final int id = (int) ContentUris.parseId(uri);
        final File targetFile = getCopiedFile(getContext(), FONT_FILES[id]);
        try {
            return ParcelFileDescriptor.open(targetFile, ParcelFileDescriptor.MODE_READ_ONLY);
        } catch (FileNotFoundException e) {
            throw new RuntimeException(
                    "Failed to find font file. Did you forget to call prepareFontFiles in setUp?");
        }
    }

    @Override
    public boolean onCreate() {
        return true;
    }

    @Override
    public String getType(Uri uri) {
        return "vnd.android.cursor.dir/vnd.android.provider.cts.font";
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs,
            String sortOrder) {
        return buildCursor(QUERY_MAP.get(selectionArgs[0]));
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        throw new UnsupportedOperationException("insert is not supported.");
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        throw new UnsupportedOperationException("delete is not supported.");
    }

    @Override
    public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
        throw new UnsupportedOperationException("update is not supported.");
    }
}
