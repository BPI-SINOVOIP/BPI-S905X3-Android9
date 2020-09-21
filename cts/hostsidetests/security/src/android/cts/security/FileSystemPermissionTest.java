package android.cts.security;

import static android.security.cts.SELinuxHostTest.copyResourceToTempFile;
import static android.security.cts.SELinuxHostTest.getDevicePolicyFile;
import static android.security.cts.SELinuxHostTest.isMac;

import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.testtype.DeviceTestCase;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.InputStream;
import java.io.IOException;
import java.nio.file.Files;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Executors;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Future;

public class FileSystemPermissionTest extends DeviceTestCase {

   /**
    * A reference to the device under test.
    */
    private ITestDevice mDevice;

    /**
     * Used to build the find command for finding insecure file system components
     */
    private static final String INSECURE_DEVICE_ADB_COMMAND = "find %s -type %s -perm /o=rwx 2>/dev/null";

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mDevice = getDevice();
    }

    public void testAllBlockDevicesAreSecure() throws Exception {
        Set<String> insecure = getAllInsecureDevicesInDirAndSubdir("/dev", "b");
        assertTrue("Found insecure block devices: " + insecure.toString(),
                insecure.isEmpty());
    }

    /**
     * Searches for all world accessable files, note this may need sepolicy to search the desired
     * location and stat files.
     * @path The path to search, must be a directory.
     * @type The type of file to search for, must be a valid find command argument to the type
     *       option.
     * @returns The set of insecure fs objects found.
     */
    private Set<String> getAllInsecureDevicesInDirAndSubdir(String path, String type) throws DeviceNotAvailableException {

        String cmd = getInsecureDeviceAdbCommand(path, type);
        String output = mDevice.executeShellCommand(cmd);
        // Splitting an empty string results in an array of an empty string.
        String [] found = output.length() > 0 ? output.split("\\s") : new String[0];
        return new HashSet<String>(Arrays.asList(found));
    }

    private static String getInsecureDeviceAdbCommand(String path, String type) {
        return String.format(INSECURE_DEVICE_ADB_COMMAND, path, type);
    }

    private static String HW_RNG_DEVICE = "/dev/hw_random";

    public void testDevHwRandomPermissions() throws Exception {
        // This test asserts that, if present, /dev/hw_random must:
        // 1. Be owned by UID root
        // 2. Not allow any world read, write, or execute permissions. The reason
        //    for being not readable by all/other is to avoid apps reading from this device.
        //    Firstly, /dev/hw_random is not public API for apps. Secondly, apps might erroneously
        //    use the output of Hardware RNG as trusted random output. Android does not trust output
        //    of /dev/hw_random. HW RNG output is only used for mixing into Linux RNG as untrusted
        //    input.
        // 3. Be a character device with major:minor 10:183 -- hwrng kernel driver is using MAJOR 10
        //    and MINOR 183
        // 4. Be openable and readable by system_server according to SELinux policy

        if (!mDevice.doesFileExist(HW_RNG_DEVICE)) {
            // Hardware RNG device is missing. This is OK because it is not required to be exposed
            // on all devices.
            return;
        }

        String command = "ls -l " + HW_RNG_DEVICE;
        String output = mDevice.executeShellCommand(command).trim();
        if (!output.endsWith(" " + HW_RNG_DEVICE)) {
            fail("Unexpected output from " + command + ": \"" + output + "\"");
        }
        String[] outputWords = output.split("\\s");
        assertEquals("Wrong device type on " + HW_RNG_DEVICE, "c", outputWords[0].substring(0, 1));
        assertEquals("Wrong world file mode on " + HW_RNG_DEVICE, "---", outputWords[0].substring(7));
        assertEquals("Wrong owner of " + HW_RNG_DEVICE, "root", outputWords[2]);
        assertEquals("Wrong device major on " + HW_RNG_DEVICE, "10,", outputWords[4]);
        assertEquals("Wrong device minor on " + HW_RNG_DEVICE, "183", outputWords[5]);

        command = "ls -Z " + HW_RNG_DEVICE;
        output = mDevice.executeShellCommand(command).trim();
        assertEquals(
                "Wrong SELinux label on " + HW_RNG_DEVICE,
                "u:object_r:hw_random_device:s0 " + HW_RNG_DEVICE,
                output);

        File sepolicy = getDevicePolicyFile(mDevice);
        output =
                new String(
                        execSearchPolicy(
                                "--allow",
                                "-s", "system_server",
                                "-t", "hw_random_device",
                                "-c", "chr_file",
                                "-p", "open",
                                sepolicy.getPath()));
        if (output.trim().isEmpty()) {
            fail("SELinux policy does not permit system_server to open " + HW_RNG_DEVICE);
        }
        output =
                new String(
                        execSearchPolicy(
                                "--allow",
                                "-s", "system_server",
                                "-t", "hw_random_device",
                                "-c", "chr_file",
                                "-p", "read",
                                sepolicy.getPath()));
        if (output.trim().isEmpty()) {
            fail("SELinux policy does not permit system_server to read " + HW_RNG_DEVICE);
        }
    }

    /**
     * Executes {@code searchpolicy} executable with the provided parameters and returns the
     * contents of standard output.
     *
     * @throws IOException if execution of searchpolicy fails, returns non-zero error code, or
     *         non-empty stderr
     */
    private static byte[] execSearchPolicy(String... args)
            throws InterruptedException, IOException {
        File tmpDir = Files.createTempDirectory("searchpolicy").toFile();
        try {
            String[] envp;
            File libsepolwrap;
            if (isMac()) {
                libsepolwrap = copyResourceToTempFile("/libsepolwrap.dylib");
                libsepolwrap =
                        Files.move(
                                libsepolwrap.toPath(),
                                new File(tmpDir, "libsepolwrap.dylib").toPath()).toFile();
                File libcpp = copyResourceToTempFile("/libc++.dylib");
                Files.move(libcpp.toPath(), new File(tmpDir, "libc++.dylib").toPath());
                envp = new String[] {"DYLD_LIBRARY_PATH=" + tmpDir.getAbsolutePath()};
            } else {
                libsepolwrap = copyResourceToTempFile("/libsepolwrap.so");
                libsepolwrap =
                        Files.move(
                                libsepolwrap.toPath(),
                                new File(tmpDir, "libsepolwrap.so").toPath()).toFile();
                File libcpp = copyResourceToTempFile("/libc++.so");
                Files.move(libcpp.toPath(), new File(tmpDir, "libc++.so").toPath());
                envp = new String[] {"LD_LIBRARY_PATH=" + tmpDir.getAbsolutePath()};
            }
            File searchpolicy = copyResourceToTempFile("/searchpolicy");
            searchpolicy =
                    Files.move(
                        searchpolicy.toPath(),
                        new File(tmpDir, "searchpolicy").toPath()).toFile();
            searchpolicy.setExecutable(true);
            libsepolwrap.setExecutable(true);
            List<String> cmd = new ArrayList<>(3 + args.length);
            cmd.add(searchpolicy.getPath());
            cmd.add("--libpath");
            cmd.add(libsepolwrap.getPath());
            for (String arg : args) {
                cmd.add(arg);
            }
            return execAndCaptureOutput(cmd.toArray(new String[0]), envp);
        } finally {
            // Delete tmpDir
            File[] files = tmpDir.listFiles();
            if (files == null) {
                files = new File[0];
            }
            for (File f : files) {
                f.delete();
            }
            tmpDir.delete();
        }
    }

    /**
     * Executes the provided command and returns the contents of standard output.
     *
     * @throws IOException if execution fails, returns a non-zero error code, or non-empty stderr
     */
    private static byte[] execAndCaptureOutput(String[] cmd, String[] envp)
            throws InterruptedException, IOException {
        // Start process, read its stdout and stderr in two corresponding background threads, wait
        // for process to terminate, throw if stderr is not empty or if return code != 0.
        final Process p = Runtime.getRuntime().exec(cmd, envp);
        ExecutorService executorService = null;
        try {
            executorService = Executors.newFixedThreadPool(2);
            Future<byte[]> stdoutContentsFuture =
                    executorService.submit(new DrainCallable(p.getInputStream()));
            Future<byte[]> stderrContentsFuture =
                    executorService.submit(new DrainCallable(p.getErrorStream()));
            int errorCode = p.waitFor();
            byte[] stderrContents = stderrContentsFuture.get();
            if ((errorCode != 0)  || (stderrContents.length > 0)) {
                throw new IOException(
                        cmd[0] + " failed with error code " + errorCode
                            + ": " + new String(stderrContents));
            }
            return stdoutContentsFuture.get();
        } catch (ExecutionException e) {
            throw new IOException("Failed to read stdout or stderr of " + cmd[0], e);
        } finally {
            if (executorService != null) {
                executorService.shutdownNow();
            }
        }
    }

    private static class DrainCallable implements Callable<byte[]> {
        private final InputStream mIn;

        private DrainCallable(InputStream in) {
            mIn = in;
        }

        @Override
        public byte[] call() throws IOException {
            ByteArrayOutputStream result = new ByteArrayOutputStream();
            byte[] buf = new byte[16384];
            int chunkSize;
            while ((chunkSize = mIn.read(buf)) != -1) {
                result.write(buf, 0, chunkSize);
            }
            return result.toByteArray();
        }
    }
}
