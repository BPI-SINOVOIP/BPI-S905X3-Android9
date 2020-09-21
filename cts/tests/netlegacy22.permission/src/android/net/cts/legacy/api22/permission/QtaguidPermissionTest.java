package android.net.cts.legacy.api22.permission;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.net.TrafficStats;
import android.support.test.filters.MediumTest;
import android.test.AndroidTestCase;

import java.io.File;
import java.net.ServerSocket;
import java.net.Socket;

import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;



public class QtaguidPermissionTest extends AndroidTestCase {

    private static final String QTAGUID_STATS_FILE = "/proc/net/xt_qtaguid/stats";

    @MediumTest
    public void testDevQtaguidSane() throws Exception {
        File f = new File("/dev/xt_qtaguid");
        assertTrue(f.canRead());
        assertFalse(f.canWrite());
        assertFalse(f.canExecute());
    }

    public void testAccessPrivateTrafficStats() throws IOException {

        final int ownAppUid = getContext().getApplicationInfo().uid;
        try {
            BufferedReader qtaguidReader = new BufferedReader(new FileReader(QTAGUID_STATS_FILE));
            String line;
            // Skip the header line;
            qtaguidReader.readLine();
            while ((line = qtaguidReader.readLine()) != null) {
                String tokens[] = line.split(" ");
                // Go through all the entries we find the qtaguid stats and fail if we find a stats
                // with different uid.
                if (tokens.length > 3 && !tokens[3].equals(String.valueOf(ownAppUid))) {
                    fail("Other apps detailed traffic stats leaked, self uid: "
                         + String.valueOf(ownAppUid) + " find uid: " + tokens[3]);
                }
            }
            qtaguidReader.close();
        } catch (FileNotFoundException e) {
            fail("Was not able to access qtaguid/stats: " + e);
        }
    }

    private void accessOwnTrafficStats() throws IOException {

        final int ownAppUid = getContext().getApplicationInfo().uid;

        boolean foundOwnDetailedStats = false;
        try {
            BufferedReader qtaguidReader = new BufferedReader(new FileReader(QTAGUID_STATS_FILE));
            String line;
            while ((line = qtaguidReader.readLine()) != null) {
                String tokens[] = line.split(" ");
                if (tokens.length > 3 && tokens[3].equals(String.valueOf(ownAppUid))) {
                    if (!tokens[2].equals("0x0")) {
                      foundOwnDetailedStats = true;
                    }
                }
            }
            qtaguidReader.close();
        } catch (FileNotFoundException e) {
            fail("Was not able to access qtaguid/stats: " + e);
        }
        assertTrue("Was expecting to find own traffic stats", foundOwnDetailedStats);
    }

    public void testAccessOwnQtaguidTrafficStats() throws IOException {

        // Transfer 1MB of data across an explicitly localhost socket.
        final int byteCount = 1024;
        final int packetCount = 1024;

        final ServerSocket server = new ServerSocket(0);
        new Thread("CreatePrivateDataTest.createTrafficStatsWithTags") {
            @Override
            public void run() {
                try {
                    Socket socket = new Socket("localhost", server.getLocalPort());
                    // Make sure that each write()+flush() turns into a packet:
                    // disable Nagle.
                    socket.setTcpNoDelay(true);
                    OutputStream out = socket.getOutputStream();
                    byte[] buf = new byte[byteCount];
                    for (int i = 0; i < packetCount; i++) {
                        TrafficStats.setThreadStatsTag(i % 10);
                        TrafficStats.tagSocket(socket);
                        out.write(buf);
                        out.flush();
                    }
                    out.close();
                    socket.close();
                } catch (IOException e) {
                  assertTrue("io exception" + e, false);
                }
            }
        }.start();

        try {
            Socket socket = server.accept();
            InputStream in = socket.getInputStream();
            byte[] buf = new byte[byteCount];
            int read = 0;
            while (read < byteCount * packetCount) {
                int n = in.read(buf);
                assertTrue("Unexpected EOF", n > 0);
                read += n;
            }
        } finally {
            server.close();
        }

        accessOwnTrafficStats();
    }
}
