package android.security;

import android.test.AndroidTestCase;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.Scanner;
import java.io.File;
import java.io.IOException;
import java.util.regex.Pattern;
import java.util.regex.Matcher;

abstract class SELinuxTargetSdkTestBase extends AndroidTestCase
{
    static {
        System.loadLibrary("ctsselinux_jni");
    }

    protected static String getFile(String filename) throws IOException {
        BufferedReader in = null;
        try {
            in = new BufferedReader(new FileReader(filename));
            return in.readLine().trim();
        } finally {
            if (in != null) {
                in.close();
            }
        }
    }

    protected static String getProperty(String property)
            throws IOException {
        Process process = new ProcessBuilder("getprop", property).start();
        Scanner scanner = null;
        String line = "";
        try {
            scanner = new Scanner(process.getInputStream());
            line = scanner.nextLine();
        } finally {
            if (scanner != null) {
                scanner.close();
            }
        }
        return line;
    }

    /**
     * Verify that net.dns properties may not be read
     */
    protected static void noDns() throws IOException {
        String[] dnsProps = {"net.dns1", "net.dns2", "net.dns3", "net.dns4"};
        for(int i = 0; i < dnsProps.length; i++) {
            String dns = getProperty(dnsProps[i]);
            assertEquals("DNS properties may not be readable by apps past " +
                    "targetSdkVersion 26", dns, "");
        }
    }

    /**
     * Verify that selinux context is the expected domain based on
     * targetSdkVersion,
     */
    protected void appDomainContext(String contextRegex, String errorMsg) throws IOException {
        Pattern p = Pattern.compile(contextRegex);
        Matcher m = p.matcher(getFile("/proc/self/attr/current"));
        String context = getFile("/proc/self/attr/current");
        String msg = errorMsg + context;
        assertTrue(msg, m.matches());
    }

    /**
     * Verify that selinux context is the expected type based on
     * targetSdkVersion,
     */
    protected void appDataContext(String contextRegex, String errorMsg) throws Exception {
        Pattern p = Pattern.compile(contextRegex);
        File appDataDir = getContext().getFilesDir();
        Matcher m = p.matcher(getFileContext(appDataDir.getAbsolutePath()));
        String context = getFileContext(appDataDir.getAbsolutePath());
        String msg = errorMsg + context;
        assertTrue(msg, m.matches());
    }

    private static final native String getFileContext(String path);
}
