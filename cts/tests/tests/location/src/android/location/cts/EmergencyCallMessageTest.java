package android.location.cts;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.SystemClock;
import android.telephony.SmsManager;
import android.telephony.TelephonyManager;
import android.test.AndroidTestCase;
import android.text.TextUtils;
import android.util.Log;

import com.google.android.mms.ContentType;
import com.google.android.mms.InvalidHeaderValueException;
import com.google.android.mms.pdu.CharacterSets;
import com.google.android.mms.pdu.EncodedStringValue;
import com.google.android.mms.pdu.GenericPdu;
import com.google.android.mms.pdu.PduBody;
import com.google.android.mms.pdu.PduComposer;
import com.google.android.mms.pdu.PduHeaders;
import com.google.android.mms.pdu.PduParser;
import com.google.android.mms.pdu.PduPart;
import com.google.android.mms.pdu.SendConf;
import com.google.android.mms.pdu.SendReq;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Random;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
 * Test sending SMS and MMS using {@link android.telephony.SmsManager}.
 */
public class EmergencyCallMessageTest extends GnssTestCase {

    private static final String TAG = "EmergencyCallMSGTest";

    private static final String ACTION_MMS_SENT = "CTS_MMS_SENT_ACTION";
    private static final long DEFAULT_EXPIRY_TIME_SECS = TimeUnit.DAYS.toSeconds(7);
    private static final long MMS_CONFIG_DELAY_MILLIS = TimeUnit.SECONDS.toMillis(1);
    private static final int DEFAULT_PRIORITY = PduHeaders.PRIORITY_NORMAL;
    private static final short DEFAULT_DATA_SMS_PORT = 8091;
    private static final String PHONE_NUMBER_KEY = "android.cts.emergencycall.phonenumber";
    private static final String SUBJECT = "CTS Emergency Call MMS Test";
    private static final String MMS_MESSAGE_BODY = "CTS Emergency Call MMS test message body";
    private static final String SMS_MESSAGE_BODY = "CTS Emergency Call Sms test message body";
    private static final String SMS_DATA_MESSAGE_BODY =
        "CTS Emergency Call Sms data test message body";
    private static final String TEXT_PART_FILENAME = "text_0.txt";
    private static final String SMIL_TEXT =
            "<smil>" +
                    "<head>" +
                        "<layout>" +
                            "<root-layout/>" +
                            "<region height=\"100%%\" id=\"Text\" left=\"0%%\" top=\"0%%\" width=\"100%%\"/>" +
                        "</layout>" +
                    "</head>" +
                    "<body>" +
                        "<par dur=\"8000ms\">" +
                            "<text src=\"%s\" region=\"Text\"/>" +
                        "</par>" +
                    "</body>" +
            "</smil>";

    private static final long SENT_TIMEOUT_MILLIS = TimeUnit.MINUTES.toMillis(5); // 5 minutes

    private static final String PROVIDER_AUTHORITY = "emergencycallverifier";

    private Random mRandom;
    private SentReceiver mSentReceiver;
    private TelephonyManager mTelephonyManager;
    private PackageManager mPackageManager;

    private static class SentReceiver extends BroadcastReceiver {
        private boolean mSuccess;
        private boolean mDone;
        private final CountDownLatch mLatch;
        public SentReceiver() {
            mLatch =  new CountDownLatch(1);
            mSuccess = false;
            mDone = false;
        }

        @Override
        public void onReceive(Context context, Intent intent) {
            Log.i(TAG, "Action " + intent.getAction());
            if (!ACTION_MMS_SENT.equals(intent.getAction())) {
                return;
            }
            final int resultCode = getResultCode();
            if (resultCode == Activity.RESULT_OK) {
                final byte[] response = intent.getByteArrayExtra(SmsManager.EXTRA_MMS_DATA);
                if (response != null) {
                    final GenericPdu pdu = new PduParser(
                            response, shouldParseContentDisposition()).parse();
                    if (pdu != null && pdu instanceof SendConf) {
                        final SendConf sendConf = (SendConf) pdu;
                        if (sendConf.getResponseStatus() == PduHeaders.RESPONSE_STATUS_OK) {
                            mSuccess = true;
                        } else {
                            Log.e(TAG, "SendConf response status=" + sendConf.getResponseStatus());
                        }
                    } else {
                        Log.e(TAG, "Not a SendConf: " +
                                (pdu != null ? pdu.getClass().getCanonicalName() : "NULL"));
                    }
                } else {
                    Log.e(TAG, "Empty response");
                }
            } else {
                Log.e(TAG, "Failure result=" + resultCode);
                if (resultCode == SmsManager.MMS_ERROR_HTTP_FAILURE) {
                    final int httpError = intent.getIntExtra(SmsManager.EXTRA_MMS_HTTP_STATUS, 0);
                    Log.e(TAG, "HTTP failure=" + httpError);
                }
            }
            mDone = true;
            mLatch.countDown();
        }

        public boolean waitForSuccess(long timeoutMs) throws Exception {
            mLatch.await(timeoutMs, TimeUnit.MILLISECONDS);
            Log.i(TAG, "Wait for sent: done=" + mDone + ", success=" + mSuccess);
            return mDone && mSuccess;
        }
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        mRandom = new Random(System.currentTimeMillis());
        mTelephonyManager =
                (TelephonyManager) mContext.getSystemService(Context.TELEPHONY_SERVICE);
        mPackageManager = mContext.getPackageManager();
    }

    public void testSendSmsMessage() {
        // this test is only for cts verifier
        if (!isCtsVerifierTest()) {
            return;
        }
        SmsManager smsManager = SmsManager.getDefault();
        final String selfNumber = getPhoneNumber(mContext);
        smsManager.sendTextMessage(selfNumber, null, SMS_MESSAGE_BODY, null, null);
    }

    public void testSendSmsDataMessage() {
        // this test is only for cts verifier
        if (!isCtsVerifierTest()) {
            return;
        }
        SmsManager smsManager = SmsManager.getDefault();
        final String selfNumber = getPhoneNumber(mContext);
        smsManager.sendDataMessage(selfNumber, null, DEFAULT_DATA_SMS_PORT,
            SMS_DATA_MESSAGE_BODY.getBytes(), null, null);
    }

    public void testSendMmsMessage() throws Exception {
        // this test is only for cts verifier
        if (!isCtsVerifierTest()) {
            return;
        }
        if (!mPackageManager.hasSystemFeature(PackageManager.FEATURE_TELEPHONY)
             || !doesSupportMMS()) {
            Log.i(TAG, "testSendMmsMessage skipped: no telephony available or MMS not supported");
            return;
        }

        Log.i(TAG, "testSendMmsMessage");
        // Prime the MmsService so that MMS config is loaded
        final SmsManager smsManager = SmsManager.getDefault();
        // MMS config is loaded asynchronously. Wait a bit so it will be loaded.
        try {
            Thread.sleep(MMS_CONFIG_DELAY_MILLIS);
        } catch (InterruptedException e) {
            // Ignore
        }

        final Context context = getContext();
        // Register sent receiver
        mSentReceiver = new SentReceiver();
        context.registerReceiver(mSentReceiver, new IntentFilter(ACTION_MMS_SENT));
        // Create local provider file for sending PDU
        final String fileName = "send." + String.valueOf(Math.abs(mRandom.nextLong())) + ".dat";
        final File sendFile = new File(context.getCacheDir(), fileName);
        final String selfNumber = getPhoneNumber(context);
        assertTrue(!TextUtils.isEmpty(selfNumber));
        final byte[] pdu = buildPdu(context, selfNumber, SUBJECT, MMS_MESSAGE_BODY);
        assertNotNull(pdu);
        assertTrue(writePdu(sendFile, pdu));
        final Uri contentUri = (new Uri.Builder())
                .authority(PROVIDER_AUTHORITY)
                .path(fileName)
                .scheme(ContentResolver.SCHEME_CONTENT)
                .build();
        // Send
        final PendingIntent pendingIntent = PendingIntent.getBroadcast(
                context, 0, new Intent(ACTION_MMS_SENT), 0);
        smsManager.sendMultimediaMessage(context,
                contentUri, null/*locationUrl*/, null/*configOverrides*/, pendingIntent);
        assertTrue(mSentReceiver.waitForSuccess(SENT_TIMEOUT_MILLIS));
        sendFile.delete();
    }

    private static boolean writePdu(File file, byte[] pdu) {
        FileOutputStream writer = null;
        try {
            writer = new FileOutputStream(file);
            writer.write(pdu);
            return true;
        } catch (final IOException e) {
            String stackTrace = Log.getStackTraceString(e);
            Log.i(TAG, stackTrace);
            return false;
        } finally {
            if (writer != null) {
                try {
                    writer.close();
                } catch (IOException e) {
                }
            }
        }
    }

    private static byte[] buildPdu(Context context, String selfNumber, String subject,
       String text) {
        final SendReq req = new SendReq();
        // From, per spec
        req.setFrom(new EncodedStringValue(selfNumber));
        // To
        final String[] recipients = new String[1];
        recipients[0] = selfNumber;
        final EncodedStringValue[] encodedNumbers = EncodedStringValue.encodeStrings(recipients);
        if (encodedNumbers != null) {
            req.setTo(encodedNumbers);
        }
        // Subject
        if (!TextUtils.isEmpty(subject)) {
            req.setSubject(new EncodedStringValue(subject));
        }
        // Date
        req.setDate(System.currentTimeMillis() / 1000);
        // Body
        final PduBody body = new PduBody();
        // Add text part. Always add a smil part for compatibility, without it there
        // may be issues on some carriers/client apps
        final int size = addTextPart(body, text, true/* add text smil */);
        req.setBody(body);
        // Message size
        req.setMessageSize(size);
        // Message class
        req.setMessageClass(PduHeaders.MESSAGE_CLASS_PERSONAL_STR.getBytes());
        // Expiry
        req.setExpiry(DEFAULT_EXPIRY_TIME_SECS);
        // The following set methods throw InvalidHeaderValueException
        try {
            // Priority
            req.setPriority(DEFAULT_PRIORITY);
            // Delivery report
            req.setDeliveryReport(PduHeaders.VALUE_NO);
            // Read report
            req.setReadReport(PduHeaders.VALUE_NO);
        } catch (InvalidHeaderValueException e) {
            return null;
        }

        return new PduComposer(context, req).make();
    }

    private static int addTextPart(PduBody pb, String message, boolean addTextSmil) {
        final PduPart part = new PduPart();
        // Set Charset if it's a text media.
        part.setCharset(CharacterSets.UTF_8);
        // Set Content-Type.
        part.setContentType(ContentType.TEXT_PLAIN.getBytes());
        // Set Content-Location.
        part.setContentLocation(TEXT_PART_FILENAME.getBytes());
        int index = TEXT_PART_FILENAME.lastIndexOf(".");
        String contentId = (index == -1) ? TEXT_PART_FILENAME
                : TEXT_PART_FILENAME.substring(0, index);
        part.setContentId(contentId.getBytes());
        part.setData(message.getBytes());
        pb.addPart(part);
        if (addTextSmil) {
            final String smil = String.format(SMIL_TEXT, TEXT_PART_FILENAME);
            addSmilPart(pb, smil);
        }
        return part.getData().length;
    }

    private static void addSmilPart(PduBody pb, String smil) {
        final PduPart smilPart = new PduPart();
        smilPart.setContentId("smil".getBytes());
        smilPart.setContentLocation("smil.xml".getBytes());
        smilPart.setContentType(ContentType.APP_SMIL.getBytes());
        smilPart.setData(smil.getBytes());
        pb.addPart(0, smilPart);
    }

    private static String getPhoneNumber(Context context) {
        final TelephonyManager telephonyManager = (TelephonyManager) context.getSystemService(
                Context.TELEPHONY_SERVICE);
        String phoneNumber = telephonyManager.getLine1Number();
        if (phoneNumber.trim().isEmpty()) {
            phoneNumber = System.getProperty(PHONE_NUMBER_KEY);
        }
        return phoneNumber;
    }

    private static boolean shouldParseContentDisposition() {
        return SmsManager
                .getDefault()
                .getCarrierConfigValues()
                .getBoolean(SmsManager.MMS_CONFIG_SUPPORT_MMS_CONTENT_DISPOSITION, true);
    }

    private static boolean doesSupportMMS() {
        return SmsManager
                .getDefault()
                .getCarrierConfigValues()
                .getBoolean(SmsManager.MMS_CONFIG_MMS_ENABLED, true);
    }

}