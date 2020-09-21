/*
 * Copyright (C) 2018 The Android Open Source Project
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

/* Contributed by Orange */

package android.omapi.accesscontrol1.cts;

import static org.junit.Assert.*;
import static org.junit.Assume.assumeTrue;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.Executor;
import java.util.concurrent.TimeoutException;

import android.content.pm.PackageManager;
import android.os.RemoteException;
import android.se.omapi.Channel;
import android.se.omapi.Reader;
import android.se.omapi.SEService;
import android.se.omapi.SEService.OnConnectedListener;
import android.se.omapi.Session;
import android.support.test.InstrumentationRegistry;

import com.android.compatibility.common.util.PropertyUtil;

public class AccessControlTest {
    private final static String UICC_READER_PREFIX = "SIM";
    private final static String ESE_READER_PREFIX = "eSE";
    private final static String SD_READER_PREFIX = "SD";

    private final static byte[] AID_40 = new byte[] { (byte) 0xA0, 0x00, 0x00,
        0x04, 0x76, 0x41, 0x6E, 0x64, 0x72, 0x6F, 0x69, 0x64, 0x43, 0x54,
        0x53, 0x40 };
    private final static byte[] AID_41 = new byte[] { (byte) 0xA0, 0x00, 0x00,
        0x04, 0x76, 0x41, 0x6E, 0x64, 0x72, 0x6F, 0x69, 0x64, 0x43, 0x54,
        0x53, 0x41 };
    private final static byte[] AID_42 = new byte[] { (byte) 0xA0, 0x00, 0x00,
        0x04, 0x76, 0x41, 0x6E, 0x64, 0x72, 0x6F, 0x69, 0x64, 0x43, 0x54,
        0x53, 0x42 };
    private final static byte[] AID_43 = new byte[] { (byte) 0xA0, 0x00, 0x00,
        0x04, 0x76, 0x41, 0x6E, 0x64, 0x72, 0x6F, 0x69, 0x64, 0x43, 0x54,
        0x53, 0x43 };
    private final static byte[] AID_44 = new byte[] { (byte) 0xA0, 0x00, 0x00,
        0x04, 0x76, 0x41, 0x6E, 0x64, 0x72, 0x6F, 0x69, 0x64, 0x43, 0x54,
        0x53, 0x44 };
    private final static byte[] AID_45 = new byte[] { (byte) 0xA0, 0x00, 0x00,
        0x04, 0x76, 0x41, 0x6E, 0x64, 0x72, 0x6F, 0x69, 0x64, 0x43, 0x54,
        0x53, 0x45 };
    private final static byte[] AID_46 = new byte[] { (byte) 0xA0, 0x00, 0x00,
        0x04, 0x76, 0x41, 0x6E, 0x64, 0x72, 0x6F, 0x69, 0x64, 0x43, 0x54,
        0x53, 0x46 };
    private final static byte[] AID_47 = new byte[] { (byte) 0xA0, 0x00, 0x00,
        0x04, 0x76, 0x41, 0x6E, 0x64, 0x72, 0x6F, 0x69, 0x64, 0x43, 0x54,
        0x53, 0x47 };
    private final static byte[] AID_48 = new byte[] { (byte) 0xA0, 0x00, 0x00,
        0x04, 0x76, 0x41, 0x6E, 0x64, 0x72, 0x6F, 0x69, 0x64, 0x43, 0x54,
        0x53, 0x48 };
    private final static byte[] AID_49 = new byte[] { (byte) 0xA0, 0x00, 0x00,
        0x04, 0x76, 0x41, 0x6E, 0x64, 0x72, 0x6F, 0x69, 0x64, 0x43, 0x54,
        0x53, 0x49 };
    private final static byte[] AID_4A = new byte[] { (byte) 0xA0, 0x00, 0x00,
        0x04, 0x76, 0x41, 0x6E, 0x64, 0x72, 0x6F, 0x69, 0x64, 0x43, 0x54,
        0x53, (byte) 0x4A };
    private final static byte[] AID_4B = new byte[] { (byte) 0xA0, 0x00, 0x00,
        0x04, 0x76, 0x41, 0x6E, 0x64, 0x72, 0x6F, 0x69, 0x64, 0x43, 0x54,
        0x53, (byte) 0x4B };
    private final static byte[] AID_4C = new byte[] { (byte) 0xA0, 0x00, 0x00,
        0x04, 0x76, 0x41, 0x6E, 0x64, 0x72, 0x6F, 0x69, 0x64, 0x43, 0x54,
        0x53, (byte) 0x4C };
    private final static byte[] AID_4D = new byte[] { (byte) 0xA0, 0x00, 0x00,
        0x04, 0x76, 0x41, 0x6E, 0x64, 0x72, 0x6F, 0x69, 0x64, 0x43, 0x54,
        0x53, (byte) 0x4D };
    private final static byte[] AID_4E = new byte[] { (byte) 0xA0, 0x00, 0x00,
        0x04, 0x76, 0x41, 0x6E, 0x64, 0x72, 0x6F, 0x69, 0x64, 0x43, 0x54,
        0x53, (byte) 0x4E };
    private final static byte[] AID_4F = new byte[] { (byte) 0xA0, 0x00, 0x00,
        0x04, 0x76, 0x41, 0x6E, 0x64, 0x72, 0x6F, 0x69, 0x64, 0x43, 0x54,
        0x53, (byte) 0x4F };

    private final static byte[][] AUTHORIZED_AID = new byte[][] { AID_40,
        AID_41, AID_42, AID_44, AID_45, AID_47, AID_48, AID_49, AID_4A,
        AID_4B, AID_4C, AID_4D, AID_4E, AID_4F };
    private final static byte[][] UNAUTHORIZED_AID = new byte[][] { AID_43,
        AID_46 };

    /* Authorized APDU for AID_40 */
    private final static byte[][] AUTHORIZED_APDU_AID_40 = new byte[][] {
        { 0x00, 0x06, 0x00, 0x00 }, { (byte) 0xA0, 0x06, 0x00, 0x00 },};
    /* Unauthorized APDU for AID_40 */
    private final static byte[][] UNAUTHORIZED_APDU_AID_40 = new byte[][] {
        { 0x00, 0x08, 0x00, 0x00, 0x00 },
        { (byte) 0x80, 0x06, 0x00, 0x00 },
        { (byte) 0xA0, 0x08, 0x00, 0x00, 0x00 },
        { (byte) 0x94, 0x06, 0x00, 0x00, 0x00 }, };

    /* Authorized APDU for AID_41 */
    private final static byte[][] AUTHORIZED_APDU_AID_41 = new byte[][] {
        { (byte) 0x94, 0x06, 0x00, 0x00 },
        { (byte) 0x94, 0x08, 0x00, 0x00, 0x00 },
        { (byte) 0x94, (byte) 0xC0, 0x00, 0x00, 0x01, (byte) 0xAA, 0x00 },
        { (byte) 0x94, 0x0A, 0x00, 0x00, 0x01, (byte) 0xAA } };
    /* Unauthorized APDU for AID_41 */
    private final static byte[][] UNAUTHORIZED_APDU_AID_41 = new byte[][] {
        { 0x00, 0x06, 0x00, 0x00 }, { (byte) 0x80, 0x06, 0x00, 0x00 },
        { (byte) 0xA0, 0x06, 0x00, 0x00 },
        { 0x00, 0x08, 0x00, 0x00, 0x00 },
        { (byte) 0x00, 0x0A, 0x00, 0x00, 0x01, (byte) 0xAA },
        { (byte) 0x80, 0x0A, 0x00, 0x00, 0x01, (byte) 0xAA },
        { (byte) 0xA0, 0x0A, 0x00, 0x00, 0x01, (byte) 0xAA },
        { (byte) 0x80, 0x08, 0x00, 0x00, 0x00 },
        { (byte) 0xA0, 0x08, 0x00, 0x00, 0x00 },
        { 0x00, (byte) 0xC0, 0x00, 0x00, 0x01, (byte) 0xAA, 0x00 },
        { (byte) 0x80, (byte) 0xC0, 0x00, 0x00, 0x01, (byte) 0xAA, 0x00 },
        { (byte) 0xA0, (byte) 0xC0, 0x00, 0x00, 0x01, (byte) 0xAA, 0x00 },
    };

    private final long SERVICE_CONNECTION_TIME_OUT = 3000;
    private SEService seService;
    private Object serviceMutex = new Object();
    private Timer connectionTimer;
    private ServiceConnectionTimerTask mTimerTask = new ServiceConnectionTimerTask();
    private boolean connected = false;

    private final OnConnectedListener mListener = new OnConnectedListener() {
        public void onConnected() {
            synchronized (serviceMutex) {
                connected = true;
                serviceMutex.notify();
            }
        }
    };

    class SynchronousExecutor implements Executor {
        public void execute(Runnable r) {
            r.run();
        }
    }

    private boolean supportsHardware() {
        final PackageManager pm = InstrumentationRegistry.getContext().getPackageManager();
        boolean lowRamDevice = PropertyUtil.propertyEquals("ro.config.low_ram", "true");
        return !lowRamDevice || (lowRamDevice && pm.hasSystemFeature("android.hardware.type.watch"));
    }

    @Before
    public void setUp() throws Exception {
        assumeTrue(supportsHardware());
        seService = new SEService(InstrumentationRegistry.getContext(), new SynchronousExecutor(), mListener);
        connectionTimer = new Timer();
        connectionTimer.schedule(mTimerTask, SERVICE_CONNECTION_TIME_OUT);
    }

    @After
    public void tearDown() throws Exception {
        if (seService != null && seService.isConnected()) {
            seService.shutdown();
            connected = false;
        }
    }

    private void waitForConnection() throws TimeoutException {
        synchronized (serviceMutex) {
            if (!connected) {
                try {
                    serviceMutex.wait();
                 } catch (InterruptedException e) {
                    e.printStackTrace();
                 }
            }
            if (!connected) {
                throw new TimeoutException(
                    "Service could not be connected after "
                    + SERVICE_CONNECTION_TIME_OUT + " ms");
            }
            if (connectionTimer != null) {
                connectionTimer.cancel();
            }
        }
    }

    @Test
    public void testAuthorizedAID() {
        for (byte[] aid : AUTHORIZED_AID) {
            testSelectableAid(aid);
        }
    }

    @Test
    public void testUnauthorizedAID() {
        for (byte[] aid : UNAUTHORIZED_AID) {
            testUnauthorisedAid(aid);
        }
    }

    @Test
    public void testAuthorizedAPDUAID40() {
        for (byte[] apdu : AUTHORIZED_APDU_AID_40) {
            testTransmitAPDU(AID_40, apdu);
        }
    }

    @Test
    public void testUnauthorisedAPDUAID40() {
        for (byte[] apdu : UNAUTHORIZED_APDU_AID_40) {
            testUnauthorisedAPDU(AID_40, apdu);
        }
    }

    @Test
    public void testAuthorizedAPDUAID41() {
        for (byte[] apdu : AUTHORIZED_APDU_AID_41) {
            testTransmitAPDU(AID_41, apdu);
        }
    }

    @Test
    public void testUnauthorisedAPDUAID41() {
        for (byte[] apdu : UNAUTHORIZED_APDU_AID_41) {
            testUnauthorisedAPDU(AID_41, apdu);
        }
    }

    private void testSelectableAid(byte[] aid) {
        Session session = null;
        Channel channel = null;
        try {
            waitForConnection();
            Reader[] readers = seService.getReaders();
            for (Reader reader : readers) {
                assertTrue(reader.isSecureElementPresent());
                session = reader.openSession();
                assertNotNull("Null Session", session);
                channel = session.openLogicalChannel(aid, (byte) 0x00);
                assertNotNull("Null Channel", channel);
                byte[] selectResponse = channel.getSelectResponse();
                assertNotNull("Null Select Response", selectResponse);
                assertEquals(selectResponse[selectResponse.length - 1] & 0xFF, 0x00);
                assertEquals(selectResponse[selectResponse.length - 2] & 0xFF, 0x90);
                assertTrue("Select Response is not complete", verifyBerTlvData(selectResponse));
            }
        } catch (Exception e) {
            fail("Unexpected Exception " + e);
        } finally{
            if (channel != null)
                channel.close();
            if (session != null)
                session.close();
        }
    }

    private void testUnauthorisedAid(byte[] aid) {
        Session session = null;
        Channel channel = null;
        try {
            waitForConnection();
            Reader[] readers = seService.getReaders();

            for (Reader reader : readers) {
                assertTrue(reader.isSecureElementPresent());
                session = reader.openSession();
                assertNotNull("Null Session", session);
                channel = session.openLogicalChannel(aid, (byte) 0x00);
                fail("SecurityException Expected ");
            }
        } catch(SecurityException ex){ }
        catch (Exception e) {
            fail("Unexpected Exception " + e);
        }
        if (channel != null)
            channel.close();
        if (session != null)
            session.close();
    }

    private void testTransmitAPDU(byte[] aid, byte[] apdu) {
        Session session = null;
        Channel channel = null;
        try {
            waitForConnection();
            Reader[] readers = seService.getReaders();

            for (Reader reader : readers) {
                assertTrue(reader.isSecureElementPresent());
                session = reader.openSession();
                assertNotNull("Null Session", session);
                channel = session.openLogicalChannel(aid, (byte) 0x00);
                assertNotNull("Null Channel", channel);
                byte[] selectResponse = channel.getSelectResponse();
                assertNotNull("Null Select Response", selectResponse);
                assertEquals(selectResponse[selectResponse.length - 1] & 0xFF, 0x00);
                assertEquals(selectResponse[selectResponse.length - 2] & 0xFF, 0x90);
                assertTrue("Select Response is not complete", verifyBerTlvData(selectResponse));
                byte[] apduResponse = channel.transmit(apdu);
                assertNotNull("Null Channel", apduResponse);
            }
        } catch (Exception e) {
            fail("Unexpected Exception " + e);
        }
        if (channel != null)
            channel.close();
        if (session != null)
            session.close();
    }

    private void testUnauthorisedAPDU(byte[] aid, byte[] apdu) {
        Session session = null;
        Channel channel = null;
        boolean exceptionOnTransmit = false;
        try {
            waitForConnection();
            Reader[] readers = seService.getReaders();

            for (Reader reader : readers) {
                assertTrue(reader.isSecureElementPresent());
                session = reader.openSession();
                assertNotNull("Null Session", session);
                channel = session.openLogicalChannel(aid, (byte) 0x00);
                assertNotNull("Null Channel", channel);
                byte[] selectResponse = channel.getSelectResponse();
                assertNotNull("Null Select Response", selectResponse);
                assertEquals(selectResponse[selectResponse.length - 1] & 0xFF, 0x00);
                assertEquals(selectResponse[selectResponse.length - 2] & 0xFF, 0x90);
                assertTrue("Select Response is not complete", verifyBerTlvData(selectResponse));
                exceptionOnTransmit = true;
                channel.transmit(apdu);
                fail("Security Exception is expected");
            }
        } catch (SecurityException ex) {
          if (!exceptionOnTransmit) {
            fail("Unexpected SecurityException onSelect" + ex);
          }
        } catch (Exception e) {
          fail("Unexpected Exception " + e);
        } finally {
            if(channel != null)
                channel.close();
            if (session != null)
                session.close();
        }
    }

    /**
     * Verifies TLV data
     *
     * @param tlv
     * @return true if the data is tlv formatted, false otherwise
     */
    private static boolean verifyBerTlvData(byte[] tlv) {
        if (tlv == null || tlv.length == 0) {
            throw new RuntimeException("Invalid tlv, null");
        }

        int i = 0;
        byte[] key = new byte[2];
        key[0] = tlv[i];
        if ((key[0] & 0x1F) == 0x1F) {
            // extra byte for TAG field
            key[1] = tlv[i = i + 1];
        }

        int len = tlv[i = i + 1] & 0xFF;
        if (len > 127) {
            // more than 1 byte for length
            int bytesLength = len - 128;
            len = 0;
            for (int j = bytesLength - 1; j >= 0; j--) {
              len += (tlv[i = i + 1] & 0xFF) * Math.pow(10, j);
            }
        }
        return tlv.length == (i + len + 3);
    }

    class ServiceConnectionTimerTask extends TimerTask {
        @Override
        public void run() {
            synchronized (serviceMutex) {
                serviceMutex.notifyAll();
            }
        }
    }
}
