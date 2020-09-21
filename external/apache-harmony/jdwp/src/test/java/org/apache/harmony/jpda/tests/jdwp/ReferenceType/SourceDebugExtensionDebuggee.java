/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/**
 * @author Anatoly F. Bondarenko
 */

/**
 * Created on 24.02.2005
 */
package org.apache.harmony.jpda.tests.jdwp.ReferenceType;

import java.lang.reflect.Constructor;
import java.lang.reflect.Proxy;
import java.net.URLClassLoader;
import java.net.URL;
import java.nio.ByteBuffer;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Base64;

import org.apache.harmony.jpda.tests.share.JPDADebuggeeSynchronizer;
import org.apache.harmony.jpda.tests.share.SyncDebuggee;

public class SourceDebugExtensionDebuggee extends SyncDebuggee {

    private final static String classWithSourceDebugExtension =
        "org.apache.harmony.jpda.tests.jdwp.Events.SourceDebugExtensionMockClass";

    private final static String inMemoryDexClassLoaderClass =
        "dalvik.system.InMemoryDexClassLoader";

    /*
     * A base64 string of a dex file built from the
     * class packaged with apache-harmony for JSR45 testing:
     *
     *  cd apache-harmony/jdwp/src/test/resources
     *  dx --dex --output=classes.dex \
     *      org/apache/harmony/jpda/tests/jdwp/Events/SourceDebugExtensionMockClass.class
     *
     * This simplifies dealing with multiple dex files in the Android
     * build system and with Jack which discards the JSR45 metadata.
     */
    private final static String base64DexWithExtensionClass =
"ZGV4CjAzNQAktKYbHXK+eDSBH4IRfDw2pS8X3+CKeds0BAAAcAAAAHhWNBIAAAAAAAAAAHADAAAT" +
"AAAAcAAAAAgAAAC8AAAAAwAAANwAAAABAAAAAAEAAAQAAAAIAQAAAQAAACgBAADsAgAASAEAAKYB" +
"AACuAQAAuwEAAOUBAAD8AQAAEAIAACQCAAA4AgAAgwIAAOkCAAANAwAAEAMAABQDAAApAwAALwMA" +
"ADUDAAA6AwAAQwMAAEkDAAACAAAAAwAAAAQAAAAFAAAABgAAAAcAAAAKAAAADAAAAAoAAAAGAAAA" +
"AAAAAAsAAAAGAAAAmAEAAAsAAAAGAAAAoAEAAAQAAQAPAAAAAQABABAAAAACAAAAAAAAAAUAAAAA" +
"AAAABQACAA4AAAAFAAAAAQAAAAIAAAAAAAAACQAAAIgBAABiAwAAAAAAAAEAAABcAwAAAQABAAEA" +
"AABQAwAABAAAAHAQAQAAAA4AAwABAAIAAABVAwAACAAAAGIAAAAaAQEAbiAAABAADgBIAQAAAAAA" +
"AAAAAAAAAAAAAQAAAAMAAAABAAAABwAGPGluaXQ+AAtIZWxsbyBXb3JsZAAoTGRhbHZpay9hbm5v" +
"dGF0aW9uL1NvdXJjZURlYnVnRXh0ZW5zaW9uOwAVTGphdmEvaW8vUHJpbnRTdHJlYW07ABJMamF2" +
"YS9sYW5nL09iamVjdDsAEkxqYXZhL2xhbmcvU3RyaW5nOwASTGphdmEvbGFuZy9TeXN0ZW07AElM" +
"b3JnL2FwYWNoZS9oYXJtb255L2pwZGEvdGVzdHMvamR3cC9FdmVudHMvU291cmNlRGVidWdFeHRl" +
"bnNpb25Nb2NrQ2xhc3M7AGRTTUFQCmhlbGxvd29ybGRfanNwLmphdmEKSlNQCipTIEpTUAoqRgor" +
"IDAgaGVsbG93b3JsZC5qc3AKaGVsbG93b3JsZC5qc3AKKkwKMSw1OjUzCjY6NTgsMwo3LDQ6NjEK" +
"KkUKACJTb3VyY2VEZWJ1Z0V4dGVuc2lvbk1vY2tDbGFzcy5qYXZhAAFWAAJWTAATW0xqYXZhL2xh" +
"bmcvU3RyaW5nOwAEYXJncwAEbWFpbgADb3V0AAdwcmludGxuAAR0aGlzAAV2YWx1ZQADAAcOAAYB" +
"DgcOeAACAAESFwgAAAIAAoGABNACAQnoAhAAAAAAAAAAAQAAAAAAAAABAAAAEwAAAHAAAAACAAAA" +
"CAAAALwAAAADAAAAAwAAANwAAAAEAAAAAQAAAAABAAAFAAAABAAAAAgBAAAGAAAAAQAAACgBAAAD" +
"EAAAAQAAAEgBAAABIAAAAgAAAFABAAAGIAAAAQAAAIgBAAABEAAAAgAAAJgBAAACIAAAEwAAAKYB" +
"AAADIAAAAgAAAFADAAAEIAAAAQAAAFwDAAAAIAAAAQAAAGIDAAAAEAAAAQAAAHADAAA=";

    /*
     * A base64 string of a jar file containing:
     * org/apache/harmony/jpda/tests/jdwp/Events/SourceDebugExtensionMockClass.class
     */
    private final static String base64JarWithExtensionClass =
"UEsDBBQACAgIAAZwmUoAAAAAAAAAAAAAAAAJAAQATUVUQS1JTkYv/soAAAMAUEsHCAAAAAACAAAA" +
"AAAAAFBLAwQUAAgICAAGcJlKAAAAAAAAAAAAAAAAFAAAAE1FVEEtSU5GL01BTklGRVNULk1G803M" +
"y0xLLS7RDUstKs7Mz7NSMNQz4OVyLkpNLElN0XWqBAlY6BnEm5jqZuaVpBblJeYoaPgXJSbnpCo4" +
"5xcV5BcllgD1afJy8XIBAFBLBwiUuAa1TAAAAE0AAABQSwMECgAACAAATYIRSQAAAAAAAAAAAAAA" +
"AAQAAABvcmcvUEsDBAoAAAgAAE2CEUkAAAAAAAAAAAAAAAALAAAAb3JnL2FwYWNoZS9QSwMECgAA" +
"CAAATYIRSQAAAAAAAAAAAAAAABMAAABvcmcvYXBhY2hlL2hhcm1vbnkvUEsDBAoAAAgAAE2CEUkA" +
"AAAAAAAAAAAAAAAYAAAAb3JnL2FwYWNoZS9oYXJtb255L2pwZGEvUEsDBAoAAAgAAE2CEUkAAAAA" +
"AAAAAAAAAAAeAAAAb3JnL2FwYWNoZS9oYXJtb255L2pwZGEvdGVzdHMvUEsDBAoAAAgAAE2CEUkA" +
"AAAAAAAAAAAAAAAjAAAAb3JnL2FwYWNoZS9oYXJtb255L2pwZGEvdGVzdHMvamR3cC9QSwMECgAA" +
"CAAAHUmYSgAAAAAAAAAAAAAAACoAAABvcmcvYXBhY2hlL2hhcm1vbnkvanBkYS90ZXN0cy9qZHdw" +
"L0V2ZW50cy9QSwMEFAAICAgATYIRSQAAAAAAAAAAAAAAAE0AAABvcmcvYXBhY2hlL2hhcm1vbnkv" +
"anBkYS90ZXN0cy9qZHdwL0V2ZW50cy9Tb3VyY2VEZWJ1Z0V4dGVuc2lvbk1vY2tDbGFzcy5jbGFz" +
"c61RTW/TQBB926Rxsg20JE35Brdc0hDVRCUtShASKikfciGSUThwQBt75TjYXst2Wvqz4AASB34A" +
"PwoxdipFoIgTe5i3M/tm9N7sz1/ffwDo4J6GFYbnKnYNEQl7Io2JiAMVnhvTyBFGKpM0MabOWWQM" +
"TmVId0vNYls+k+OZO/iUyjDxVHii7I9HvkgSDUWGjak4FYYvQtd4M55KO2UoPfZCL33CUGjujhiK" +
"R8qRHAVUqlhFiWHd9EL5ehaMZfxWjH3JUDOVLfyRiL0svygW04mXMLw0/5PcPo0MhBcybDXfmwvZ" +
"Vhp7odvfHVVwBXUNtT9MWedJKoMqNtEgQ2pG/hrzZk8ZQ+pMqV+KoF/GVYa1F9L3lf5Oxb7DcR03" +
"NdxgqC/hV3ELtxm0KCv5JKrRXKaJNIvYpTXUl0hm4HPHx162sJ1/2t/L2hk2l5GwDTKH7KzQjX6J" +
"okaZQcgIV1vfUP6cP3OKpbxYwBrF6pxAeImwgstYv2g+zIdR7Qs2altfcW0xgBNmY8pEXAyp4A7u" +
"5hw9j9vYIXSsk6dDPskWe5bt9cM0iXIz/JU15C1Lz+GY39cf6AvWHrH4X2nL5J12t9fd5we97qP2" +
"Pj9sP+wddHhrwH8DUEsHCFc9MsLTAQAAIwMAAFBLAQIUABQACAgIAAZwmUoAAAAAAgAAAAAAAAAJ" +
"AAQAAAAAAAAAAAAAAAAAAABNRVRBLUlORi/+ygAAUEsBAhQAFAAICAgABnCZSpS4BrVMAAAATQAA" +
"ABQAAAAAAAAAAAAAAAAAPQAAAE1FVEEtSU5GL01BTklGRVNULk1GUEsBAgoACgAACAAATYIRSQAA" +
"AAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAywAAAG9yZy9QSwECCgAKAAAIAABNghFJAAAAAAAAAAAA" +
"AAAACwAAAAAAAAAAAAAAAADtAAAAb3JnL2FwYWNoZS9QSwECCgAKAAAIAABNghFJAAAAAAAAAAAA" +
"AAAAEwAAAAAAAAAAAAAAAAAWAQAAb3JnL2FwYWNoZS9oYXJtb255L1BLAQIKAAoAAAgAAE2CEUkA" +
"AAAAAAAAAAAAAAAYAAAAAAAAAAAAAAAAAEcBAABvcmcvYXBhY2hlL2hhcm1vbnkvanBkYS9QSwEC" +
"CgAKAAAIAABNghFJAAAAAAAAAAAAAAAAHgAAAAAAAAAAAAAAAAB9AQAAb3JnL2FwYWNoZS9oYXJt" +
"b255L2pwZGEvdGVzdHMvUEsBAgoACgAACAAATYIRSQAAAAAAAAAAAAAAACMAAAAAAAAAAAAAAAAA" +
"uQEAAG9yZy9hcGFjaGUvaGFybW9ueS9qcGRhL3Rlc3RzL2pkd3AvUEsBAgoACgAACAAAHUmYSgAA" +
"AAAAAAAAAAAAACoAAAAAAAAAAAAAAAAA+gEAAG9yZy9hcGFjaGUvaGFybW9ueS9qcGRhL3Rlc3Rz" +
"L2pkd3AvRXZlbnRzL1BLAQIUABQACAgIAE2CEUlXPTLC0wEAACMDAABNAAAAAAAAAAAAAAAAAEIC" +
"AABvcmcvYXBhY2hlL2hhcm1vbnkvanBkYS90ZXN0cy9qZHdwL0V2ZW50cy9Tb3VyY2VEZWJ1Z0V4" +
"dGVuc2lvbk1vY2tDbGFzcy5jbGFzc1BLBQYAAAAACgAKAN8CAACQBAAAAAA=";

    private ClassLoader getClassLoaderInitializedWithDexFile() {
        try {
            byte[] dexBytes = Base64.getDecoder().decode(base64DexWithExtensionClass);
            ByteBuffer dexBuffer = ByteBuffer.wrap(dexBytes);
            Class<?> klass = Class.forName(inMemoryDexClassLoaderClass);
            Constructor<?> constructor = klass.getConstructor(ByteBuffer.class, ClassLoader.class);
            return (ClassLoader) constructor.newInstance(dexBuffer,
                                                         ClassLoader.getSystemClassLoader());
        } catch (Exception e) {
            logWriter.println("--> Debuggee: Failed to instantiate " + inMemoryDexClassLoaderClass
                              + " " + e);
            return null;
        }
    }

    private ClassLoader getClassLoaderInitializedWithClassFile() {
        try {
            byte[] jarBytes = Base64.getDecoder().decode(base64JarWithExtensionClass);
            Path jarPath = Files.createTempFile(null, "jar");
            jarPath.toFile().deleteOnExit();
            Files.write(jarPath, jarBytes);
            return new URLClassLoader(new URL[] { jarPath.toUri().toURL() });
        } catch (Exception e) {
            logWriter.println("--> Debuggee: Failed to instantiate URLClassLoader: " + e);
            return null;
        }
    }

    @Override
    public void run() {
        ClassLoader classLoader = null;
        if (System.getProperty("java.vendor").contains("Android")) {
            classLoader = getClassLoaderInitializedWithDexFile();
        } else {
            classLoader = getClassLoaderInitializedWithClassFile();
        }

        Class<?> klass = null;
        try {
            klass = classLoader.loadClass(classWithSourceDebugExtension);
        } catch (ClassNotFoundException e) {
            logWriter.println("--> Debuggee: Could not find class " +
                              classWithSourceDebugExtension);
        }

        // Create an instance of classWithSourceDebugExtension so the
        // SourceDebugExtension metadata can be reported back to the debugger.
        Object o = null;
        if (klass != null) {
            try {
                o = klass.getConstructor().newInstance();
            } catch (Exception e) {
                logWriter.println("--> Debuggee: Failed to instantiate " +
                                  classWithSourceDebugExtension + ": " + e);
            }
        }

        // Instantiate a proxy whose name should contain "$Proxy".
        Class proxy = Proxy.getProxyClass(SomeInterface.class.getClassLoader(),
                                          new Class[] { SomeInterface.class });

        synchronizer.sendMessage(JPDADebuggeeSynchronizer.SGNL_READY);
        logWriter.println("--> Debuggee: SourceDebugExtensionDebuggee...");
        synchronizer.receiveMessage(JPDADebuggeeSynchronizer.SGNL_CONTINUE);
    }

    interface SomeInterface {}

    public static void main(String [] args) {
        runDebuggee(SourceDebugExtensionDebuggee.class);
    }

}
