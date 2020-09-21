/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
* *
This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
* *
Description: java file
*/
package com.droidlogic.mboxlauncher;


import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.security.Key;

import javax.crypto.Cipher;

public class DesUtils {
	public static final String STRING_KEY = "gjaoun";
	private static final String LOG_TAG = "ChipCheck";

	private Cipher encryptCipher = null;
	private Cipher decryptCipher = null;

	public static boolean isAmlogicChip() {
		// following code check if chip is amlogic
		String cupinfo = "7c0f13b6d5986e65";
		try {
			DesUtils des = new DesUtils(STRING_KEY);
			if (des.decrypt(GetCpuInfo(des)).indexOf(des.decrypt(cupinfo)) != -1) {
				return true;
			} else {
					//	" Sorry! Your cpu is not Amlogic,u cant use the Jar");
				return false;
			}
		} catch (Exception e) {
		}

		return false;
	}

	public static String byteArr2HexStr(byte[] arrB) throws Exception {
		int iLen = arrB.length;
		StringBuffer sb = new StringBuffer(iLen * 2);

		for (int i = 0; i < iLen; i++) {
			int intTmp = arrB[i];
			while (intTmp < 0) {
				intTmp = intTmp + 256;
			}
			if (intTmp < 16) {
				sb.append("0");
			}

			sb.append(Integer.toString(intTmp, 16));
		}
		return sb.toString();
	}

	public static byte[] hexStr2ByteArr(String strIn) throws Exception {
		byte[] arrB = strIn.getBytes();
		int iLen = arrB.length;
		byte[] arrOut = new byte[iLen / 2];

		for (int i = 0; i < iLen; i = i + 2) {
			String strTmp = new String(arrB, i, 2);
			arrOut[i / 2] = (byte) Integer.parseInt(strTmp, 16);
		}
		return arrOut;
	}

	public DesUtils() throws Exception {
		this(STRING_KEY);
	}

	public DesUtils(String strKey) throws Exception {
		// Security.addProvider(new com.sun.crypto.provider.SunJCE());
		Key key = getKey(strKey.getBytes());
		encryptCipher = Cipher.getInstance("DES");
		encryptCipher.init(Cipher.ENCRYPT_MODE, key);

		decryptCipher = Cipher.getInstance("DES");
		decryptCipher.init(Cipher.DECRYPT_MODE, key);
	}

	public byte[] encrypt(byte[] arrB) throws Exception {
		return encryptCipher.doFinal(arrB);
	}

	public String encrypt(String strIn) throws Exception {
		return byteArr2HexStr(encrypt(strIn.getBytes()));
	}

	public byte[] decrypt(byte[] arrB) throws Exception {
		return decryptCipher.doFinal(arrB);
	}

	public String decrypt(String strIn) throws Exception {
		return new String(decrypt(hexStr2ByteArr(strIn)));
	}

	private Key getKey(byte[] arrBTmp) throws Exception {
		byte[] arrB = new byte[8];
		for (int i = 0; i < arrBTmp.length && i < arrB.length; i++) {
			arrB[i] = arrBTmp[i];
		}

		Key key = new javax.crypto.spec.SecretKeySpec(arrB, "DES");
		return key;
	}

	// get cpu info
	public static String GetCpuInfo(DesUtils des) {
		String result = null;
		CommandRun cmdexe = new CommandRun();
		try {
			String[] args = { "/system/bin/cat", "/proc/cpuinfo" };
			result = cmdexe.run(args, "/system/bin/");
			result = result.toLowerCase();
			try {
				result = des.encrypt(result);
			} catch (Exception e) {
				// TODO Auto-generated catch block
			}
		} catch (IOException ex) {
		}
		return result;
	}

	static class CommandRun {
		public synchronized String run(String[] cmd, String workdirectory)
				throws IOException {
			String result = "";
			try {
				ProcessBuilder builder = new ProcessBuilder(cmd);
				InputStream in = null;

				if (workdirectory != null) {
					builder.directory(new File(workdirectory));
					builder.redirectErrorStream(true);
					Process process = builder.start();
					in = process.getInputStream();
					byte[] re = new byte[1024];
					while (in.read(re) != -1)
						result = result + new String(re);
				}

				if (in != null) {
					in.close();
				}
			} catch (Exception ex) {
			}
			return result;
		}
	}
}
