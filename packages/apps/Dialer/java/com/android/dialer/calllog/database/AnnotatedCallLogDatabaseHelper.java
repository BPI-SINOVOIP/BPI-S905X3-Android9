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
 * limitations under the License
 */

package com.android.dialer.calllog.database;

import android.content.Context;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.provider.CallLog.Calls;
import com.android.dialer.calllog.database.contract.AnnotatedCallLogContract.AnnotatedCallLog;
import com.android.dialer.common.LogUtil;
import java.util.Locale;

/** {@link SQLiteOpenHelper} for the AnnotatedCallLog database. */
class AnnotatedCallLogDatabaseHelper extends SQLiteOpenHelper {
  private final int maxRows;

  AnnotatedCallLogDatabaseHelper(Context appContext, int maxRows) {
    super(appContext, "annotated_call_log.db", null, 1);
    this.maxRows = maxRows;
  }

  private static final String CREATE_TABLE_SQL =
      "create table if not exists "
          + AnnotatedCallLog.TABLE
          + " ("
          + (AnnotatedCallLog._ID + " integer primary key, ")
          + (AnnotatedCallLog.TIMESTAMP + " integer, ")
          + (AnnotatedCallLog.NUMBER + " blob, ")
          + (AnnotatedCallLog.FORMATTED_NUMBER + " text, ")
          + (AnnotatedCallLog.NUMBER_PRESENTATION + " integer, ")
          + (AnnotatedCallLog.DURATION + " integer, ")
          + (AnnotatedCallLog.DATA_USAGE + " integer, ")
          + (AnnotatedCallLog.IS_READ + " integer, ")
          + (AnnotatedCallLog.NEW + " integer, ")
          + (AnnotatedCallLog.GEOCODED_LOCATION + " text, ")
          + (AnnotatedCallLog.PHONE_ACCOUNT_COMPONENT_NAME + " text, ")
          + (AnnotatedCallLog.PHONE_ACCOUNT_ID + " text, ")
          + (AnnotatedCallLog.PHONE_ACCOUNT_LABEL + " text, ")
          + (AnnotatedCallLog.PHONE_ACCOUNT_COLOR + " integer, ")
          + (AnnotatedCallLog.FEATURES + " integer, ")
          + (AnnotatedCallLog.TRANSCRIPTION + " integer, ")
          + (AnnotatedCallLog.VOICEMAIL_URI + " text, ")
          + (AnnotatedCallLog.CALL_TYPE + " integer not null, ")
          + (AnnotatedCallLog.NUMBER_ATTRIBUTES + " blob, ")
          + (AnnotatedCallLog.TRANSCRIPTION_STATE + " integer")
          + ");";

  /**
   * Deletes all but the first maxRows rows (by timestamp, excluding voicemails) to keep the table a
   * manageable size.
   */
  private static final String CREATE_TRIGGER_SQL =
      "create trigger delete_old_rows after insert on "
          + AnnotatedCallLog.TABLE
          + " when (select count(*) from "
          + AnnotatedCallLog.TABLE
          + " where "
          + AnnotatedCallLog.CALL_TYPE
          + " != "
          + Calls.VOICEMAIL_TYPE
          + ") > %d"
          + " begin delete from "
          + AnnotatedCallLog.TABLE
          + " where "
          + AnnotatedCallLog._ID
          + " in (select "
          + AnnotatedCallLog._ID
          + " from "
          + AnnotatedCallLog.TABLE
          + " where "
          + AnnotatedCallLog.CALL_TYPE
          + " != "
          + Calls.VOICEMAIL_TYPE
          + " order by timestamp limit (select count(*)-%d"
          + " from "
          + AnnotatedCallLog.TABLE
          + " where "
          + AnnotatedCallLog.CALL_TYPE
          + " != "
          + Calls.VOICEMAIL_TYPE
          + ")); end;";

  private static final String CREATE_INDEX_ON_CALL_TYPE_SQL =
      "create index call_type_index on "
          + AnnotatedCallLog.TABLE
          + " ("
          + AnnotatedCallLog.CALL_TYPE
          + ");";

  private static final String CREATE_INDEX_ON_NUMBER_SQL =
      "create index number_index on "
          + AnnotatedCallLog.TABLE
          + " ("
          + AnnotatedCallLog.NUMBER
          + ");";

  @Override
  public void onCreate(SQLiteDatabase db) {
    LogUtil.enterBlock("AnnotatedCallLogDatabaseHelper.onCreate");
    long startTime = System.currentTimeMillis();
    db.execSQL(CREATE_TABLE_SQL);
    db.execSQL(String.format(Locale.US, CREATE_TRIGGER_SQL, maxRows, maxRows));
    db.execSQL(CREATE_INDEX_ON_CALL_TYPE_SQL);
    db.execSQL(CREATE_INDEX_ON_NUMBER_SQL);
    // TODO(zachh): Consider logging impression.
    LogUtil.i(
        "AnnotatedCallLogDatabaseHelper.onCreate",
        "took: %dms",
        System.currentTimeMillis() - startTime);
  }

  @Override
  public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {}
}
