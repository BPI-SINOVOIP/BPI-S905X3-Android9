/*
 * Copyright (c) 2015, Motorola Mobility LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     - Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     - Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     - Neither the name of Motorola Mobility nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MOTOROLA MOBILITY LLC BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

package com.android.service.ims.presence;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;

import android.content.ContentProviderOperation;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.OperationApplicationException;
import android.database.Cursor;
import android.database.sqlite.SQLiteException;
import android.os.RemoteException;
import android.provider.ContactsContract;
import android.provider.ContactsContract.Contacts;
import android.provider.ContactsContract.Data;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.text.TextUtils;

import com.android.ims.internal.ContactNumberUtils;
import com.android.ims.internal.EABContract;
import com.android.ims.internal.Logger;

public class EABDbUtil {
    static private Logger logger = Logger.getLogger("EABDbUtil");
    public static final String ACCOUNT_TYPE = "com.android.rcs.eab.account";

    public static boolean validateAndSyncFromContactsDb(Context context) {
        logger.debug("Enter validateAndSyncFromContactsDb");
        boolean response = true;
        // Get the last stored contact changed timestamp and sync only delta contacts.
        long contactLastChange = SharedPrefUtil.getLastContactChangedTimestamp(context, 0);
        logger.debug("contact last updated time before init :" + contactLastChange);
        ContentResolver contentResolver = context.getContentResolver();
        String[] projection = new String[] {
                ContactsContract.Contacts._ID,
                ContactsContract.Contacts.HAS_PHONE_NUMBER,
                ContactsContract.Contacts.DISPLAY_NAME,
                ContactsContract.Contacts.CONTACT_LAST_UPDATED_TIMESTAMP };
        String selection = ContactsContract.Contacts.HAS_PHONE_NUMBER + "> '0' AND "
                + ContactsContract.Contacts.CONTACT_LAST_UPDATED_TIMESTAMP
                + " >'" + contactLastChange + "'";
        String sortOrder = ContactsContract.Contacts.DISPLAY_NAME + " asc";
        Cursor cursor = null;
        try {
            cursor = contentResolver.query(Contacts.CONTENT_URI, projection, selection,
                    null, sortOrder);
        } catch (Exception e) {
            logger.error("validateAndSyncFromContactsDb() cursor exception:", e);
        }
        ArrayList<PresenceContact> allEligibleContacts = new ArrayList<PresenceContact>();

        if (cursor != null) {
            logger.debug("cursor count : " + cursor.getCount());
        } else {
            logger.debug("cursor = null");
        }

        if (cursor != null && cursor.moveToFirst()) {
            do {
                String id = cursor.getString(cursor.getColumnIndex(Contacts._ID));
                Long time = cursor.getLong(cursor.getColumnIndex(
                        Contacts.CONTACT_LAST_UPDATED_TIMESTAMP));
                // Update the latest contact last modified timestamp.
                if (contactLastChange < time) {
                    contactLastChange = time;
                }
                String[] commonDataKindsProjection = new String[] {
                        ContactsContract.CommonDataKinds.Phone._ID,
                        ContactsContract.CommonDataKinds.Phone.NUMBER,
                        ContactsContract.CommonDataKinds.Phone.DISPLAY_NAME,
                        ContactsContract.CommonDataKinds.Phone.RAW_CONTACT_ID,
                        ContactsContract.CommonDataKinds.Phone.CONTACT_ID };
                Cursor pCur = null;
                try {
                    pCur = contentResolver.query(
                            ContactsContract.CommonDataKinds.Phone.CONTENT_URI,
                            commonDataKindsProjection,
                            ContactsContract.CommonDataKinds.Phone.CONTACT_ID
                                + " = ?", new String[] { id }, null);
                } catch (Exception e) {
                    logger.error("validateAndSyncFromContactsDb() pCur exception:", e);
                }
                // ArrayList to avoid duplicate entries of contactNumber having same
                // contactId, rawContactId and dataId.
                ArrayList<String> phoneNumList = new ArrayList<String>();

                if (pCur != null && pCur.moveToFirst()) {
                    do {
                        String contactNumber = pCur.getString(pCur.getColumnIndex(
                                ContactsContract.CommonDataKinds.Phone.NUMBER));
                        if (validateEligibleContact(context, contactNumber)) {
                            String contactName = pCur.getString(pCur.getColumnIndex(
                                    ContactsContract.CommonDataKinds.Phone.DISPLAY_NAME));
                            String rawContactId = pCur.getString(pCur.getColumnIndex(
                                    ContactsContract.CommonDataKinds.Phone.RAW_CONTACT_ID));
                            String contactId = pCur.getString(pCur.getColumnIndex(
                                    ContactsContract.CommonDataKinds.Phone.CONTACT_ID));
                            String dataId = pCur.getString(pCur.getColumnIndex(
                                    ContactsContract.CommonDataKinds.Phone._ID));
                            String formattedNumber = formatNumber(contactNumber);
                            String uniquePhoneNum = formattedNumber + contactId
                                    + rawContactId + dataId;
                            logger.debug("uniquePhoneNum : " + uniquePhoneNum);
                            if (phoneNumList.contains(uniquePhoneNum)) continue;
                            phoneNumList.add(uniquePhoneNum);

                            allEligibleContacts.add(new PresenceContact(contactName, contactNumber,
                                    formattedNumber, rawContactId, contactId, dataId));
                            logger.debug("Eligible List Name: " + contactName +
                                    " Number:" + contactNumber + " RawContactID: " + rawContactId +
                                    " contactId: " + contactId + " Data.ID : " + dataId
                                    + " formattedNumber: " + formattedNumber);
                        }
                    } while (pCur.moveToNext());
                }
                if (pCur != null) {
                    pCur.close();
                }
            } while (cursor.moveToNext());
        }
        if (null != cursor) {
            cursor.close();
        }
        if (allEligibleContacts.size() > 0) {
            logger.debug("Adding : " + allEligibleContacts.size() +
                    " new contact numbers to EAB db.");
            addContactsToEabDb(context, allEligibleContacts);
            logger.debug("contact last updated time after init :" + contactLastChange);
            SharedPrefUtil.saveLastContactChangedTimestamp(context, contactLastChange);
            SharedPrefUtil.saveLastContactDeletedTimestamp(context, contactLastChange);
        }
        logger.debug("Exit validateAndSyncFromContactsDb contact numbers synced : " +
                allEligibleContacts.size());
        return response;
    }

    public static void addContactsToEabDb(Context context,
            ArrayList<PresenceContact> contactList) {
        ArrayList<ContentProviderOperation> operation = new ArrayList<ContentProviderOperation>();

        logger.debug("Adding Contacts to EAB DB");
        // To avoid the following exception - Too many content provider operations
        // between yield points. The maximum number of operations per yield point is
        // 500 for exceuteDB()
        int yieldPoint = 300;
        for (int j = 0; j < contactList.size(); j++) {
            addContactToEabDb(context, operation, contactList.get(j).getDisplayName(),
                    contactList.get(j).getPhoneNumber(), contactList.get(j).getFormattedNumber(),
                    contactList.get(j).getRawContactId(), contactList.get(j).getContactId(),
                    contactList.get(j).getDataId());
            if (yieldPoint == j) {
                exceuteDB(context, operation);
                operation = null;
                operation = new ArrayList<ContentProviderOperation>();
                yieldPoint += 300;
            }
        }
        exceuteDB(context, operation);
    }

    private static void addContactToEabDb(
            Context context, ArrayList<ContentProviderOperation> ops, String displayName,
            String phoneNumber, String formattedNumber, String rawContactId, String contactId,
            String dataId) {
        ops.add(ContentProviderOperation
                .newInsert(EABContract.EABColumns.CONTENT_URI)
                .withValue(EABContract.EABColumns.CONTACT_NAME, displayName)
                .withValue(EABContract.EABColumns.CONTACT_NUMBER, phoneNumber)
                .withValue(EABContract.EABColumns.FORMATTED_NUMBER, formattedNumber)
                .withValue(EABContract.EABColumns.ACCOUNT_TYPE, ACCOUNT_TYPE)
                .withValue(EABContract.EABColumns.RAW_CONTACT_ID, rawContactId)
                .withValue(EABContract.EABColumns.CONTACT_ID, contactId)
                .withValue(EABContract.EABColumns.DATA_ID, dataId).build());
    }

    public static void deleteContactsFromEabDb(Context context,
            ArrayList<PresenceContact> contactList) {
        ArrayList<ContentProviderOperation> operation = new ArrayList<ContentProviderOperation>();

        logger.debug("Deleting Contacts from EAB DB");
        String[] contactIdList = new String[contactList.size()];
        // To avoid the following exception - Too many content provider operations
        // between yield points. The maximum number of operations per yield point is
        // 500 for exceuteDB()
        int yieldPoint = 300;
        for (int j = 0; j < contactList.size(); j++) {
            contactIdList[j] = contactList.get(j).getContactId().toString();
            deleteContactFromEabDb(context, operation, contactIdList[j]);
            if (yieldPoint == j) {
                exceuteDB(context, operation);
                operation = null;
                operation = new ArrayList<ContentProviderOperation>();
                yieldPoint += 300;
            }
        }
        exceuteDB(context, operation);
    }

    private static void deleteContactFromEabDb(Context context,
            ArrayList<ContentProviderOperation> ops, String contactId) {
        // Add operation only if there is an entry in EABProvider table.
        String[] eabProjection = new String[] {
                EABContract.EABColumns.CONTACT_NUMBER,
                EABContract.EABColumns.CONTACT_ID };
        String eabWhereClause = null;
        if (ContactsContract.Profile.MIN_ID == Long.valueOf(contactId)) {
            eabWhereClause = EABContract.EABColumns.CONTACT_ID + " >='" + contactId + "'";
        } else {
            eabWhereClause = EABContract.EABColumns.CONTACT_ID + " ='" + contactId + "'";
        }
        Cursor eabDeleteCursor = context.getContentResolver().query(
                EABContract.EABColumns.CONTENT_URI, eabProjection,
                eabWhereClause, null, null);
        if (null != eabDeleteCursor) {
            int count = eabDeleteCursor.getCount();
            logger.debug("cursor count : " + count);
            if (count > 0) {
                eabDeleteCursor.moveToNext();
                long eabDeleteContactId = eabDeleteCursor.
                        getLong(eabDeleteCursor.getColumnIndex(EABContract.EABColumns.CONTACT_ID));
                logger.debug("eabDeleteContactId : " + eabDeleteContactId);
                if (ContactsContract.Profile.MIN_ID == Long.valueOf(contactId)) {
                    if (ContactsContract.isProfileId(eabDeleteContactId)) {
                        logger.debug("Deleting Profile contact.");
                        ops.add(ContentProviderOperation
                            .newDelete(EABContract.EABColumns.CONTENT_URI)
                            .withSelection(EABContract.EABColumns.CONTACT_ID + " >= ?",
                                    new String[] { contactId }).build());
                    } else {
                        logger.debug("Not a Profile contact. Do nothing.");
                    }
                } else {
                    ops.add(ContentProviderOperation
                        .newDelete(EABContract.EABColumns.CONTENT_URI)
                        .withSelection(EABContract.EABColumns.CONTACT_ID + " = ?",
                                new String[] { contactId }).build());
                }
            }
            eabDeleteCursor.close();
        }

    }

    public static void deleteNumbersFromEabDb(Context context,
            ArrayList<PresenceContact> contactList) {
        ArrayList<ContentProviderOperation> operation = new ArrayList<ContentProviderOperation>();

        logger.debug("Deleting Number from EAB DB");
        String[] rawContactIdList = new String [contactList.size()];
        String[] DataIdList = new String [contactList.size()];
        // To avoid the following exception - Too many content provider operations
        // between yield points. The maximum number of operations per yield point is
        // 500 for exceuteDB()
        int yieldPoint = 300;
        for (int j = 0; j < contactList.size(); j++) {
            rawContactIdList[j] = contactList.get(j).getRawContactId();
            DataIdList[j] = contactList.get(j).getDataId();
            deleteNumberFromEabDb(context, operation, rawContactIdList[j], DataIdList[j]);
            if (yieldPoint == j) {
                exceuteDB(context, operation);
                operation = null;
                operation = new ArrayList<ContentProviderOperation>();
                yieldPoint += 300;
            }
        }
        exceuteDB(context, operation);
    }

    private static void deleteNumberFromEabDb(Context context,
            ArrayList<ContentProviderOperation> ops, String rawContactId, String dataId) {
        // Add operation only if there is an entry in EABProvider table.
        String[] eabProjection = new String[] {
                EABContract.EABColumns.CONTACT_NUMBER };
        String eabWhereClause = EABContract.EABColumns.RAW_CONTACT_ID + " ='" + rawContactId
                + "' AND " + EABContract.EABColumns.DATA_ID + " ='" + dataId + "'";
        Cursor eabDeleteCursor = context.getContentResolver().query(
                EABContract.EABColumns.CONTENT_URI, eabProjection,
                eabWhereClause, null, null);
        if (null != eabDeleteCursor) {
            int count = eabDeleteCursor.getCount();
            logger.debug("Delete number cursor count : " + count);
            if (count > 0) {
                ops.add(ContentProviderOperation.newDelete(EABContract.EABColumns.CONTENT_URI)
                        .withSelection(EABContract.EABColumns.RAW_CONTACT_ID + " = ? AND "
                                        + EABContract.EABColumns.DATA_ID + " = ?",
                                new String[]{rawContactId, dataId}).build());
            }
            eabDeleteCursor.close();
        }
    }

    public static void updateNamesInEabDb(Context context,
            ArrayList<PresenceContact> contactList) {
        ArrayList<ContentProviderOperation> operation = new ArrayList<ContentProviderOperation>();

        logger.debug("Update name in EAB DB");
        String[] phoneNameList = new String[contactList.size()];
        String[] phoneNumberList = new String[contactList.size()];
        String[] rawContactIdList = new String [contactList.size()];
        String[] dataIdList = new String [contactList.size()];
        // To avoid the following exception - Too many content provider operations
        // between yield points. The maximum number of operations per yield point is
        // 500 for exceuteDB()
        int yieldPoint = 300;
        for (int j = 0; j < contactList.size(); j++) {
            phoneNameList[j] = contactList.get(j).getDisplayName();
            phoneNumberList[j] = contactList.get(j).getPhoneNumber();
            rawContactIdList[j] = contactList.get(j).getRawContactId();
            dataIdList[j] = contactList.get(j).getDataId();
            updateNameInEabDb(context, operation, phoneNameList[j],
                    phoneNumberList[j], rawContactIdList[j], dataIdList[j]);
            if (yieldPoint == j) {
                exceuteDB(context, operation);
                operation = null;
                operation = new ArrayList<ContentProviderOperation>();
                yieldPoint += 300;
            }
        }
        exceuteDB(context, operation);
    }

    private static void updateNameInEabDb(Context context,
            ArrayList<ContentProviderOperation> ops, String phoneName,
            String phoneNumber, String rawContactId, String dataId) {
        ContentValues values = new ContentValues();
        values.put(EABContract.EABColumns.CONTACT_NAME, phoneName);

        String where = EABContract.EABColumns.CONTACT_NUMBER + " = ? AND "
                + EABContract.EABColumns.RAW_CONTACT_ID + " = ? AND "
                + EABContract.EABColumns.DATA_ID + " = ?";
        ops.add(ContentProviderOperation
                .newUpdate(EABContract.EABColumns.CONTENT_URI)
                .withValues(values)
                .withSelection(where, new String[] { phoneNumber, rawContactId, dataId }).build());
    }

    private static void exceuteDB(Context context, ArrayList<ContentProviderOperation> ops) {
        if (ops.size() == 0) {
            logger.debug("exceuteDB return as operation size is 0.");
            return;
        }
        try {
            context.getContentResolver().applyBatch(EABContract.AUTHORITY, ops);
        } catch (RemoteException e) {
            e.printStackTrace();
        } catch (OperationApplicationException e) {
            e.printStackTrace();
        }
        ops.clear();
        logger.debug("exceuteDB return with successful operation.");
        return;
    }

    public static boolean validateEligibleContact(Context context, String mdn) {
        boolean number = false;
        if (null == mdn) {
            logger.debug("validateEligibleContact - mdn is null.");
            return number;
        }
        List<String> mdbList = new ArrayList<String>();
        mdbList.add(mdn);
        ContactNumberUtils mNumberUtils = ContactNumberUtils.getDefault();
        mNumberUtils.setContext(context);
        int numberType = mNumberUtils.validate(mdbList);
        logger.debug("ContactNumberUtils.validate response : " + numberType);
        if ( ContactNumberUtils.NUMBER_VALID == numberType) {
            number = true;
        }
        logger.debug("Exiting validateEligibleContact with value : " + number);
        return number;
    }

    public static String formatNumber(String mdn) {
        logger.debug("Enter FormatNumber - mdn : " + mdn);
        ContactNumberUtils mNumberUtils = ContactNumberUtils.getDefault();
        return mNumberUtils.format(mdn);
    }

    public static boolean isSpecialNumber(String number) {
        logger.debug("Enter isSpecialNumber - number : " + number);
        boolean result = false;
        if (null != number) {
            if (number.startsWith("*67") || number.startsWith("*82")) {
                result = true;
            }
        }
        logger.debug("Exit isSpecialNumber - result : " + result);
        return result;
    }
}
