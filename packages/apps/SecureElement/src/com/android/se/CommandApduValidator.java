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
/*
 * Contributed by: Giesecke & Devrient GmbH.
 */

package com.android.se;

/**
 * Validates APDU command format and throw IllegalArgumentException, if anything is wrong.
 */
public class CommandApduValidator {

    private static final int CMD_APDU_LENGTH_CASE1 = 4;
    private static final int CMD_APDU_LENGTH_CASE2 = 5;
    private static final int CMD_APDU_LENGTH_CASE2_EXTENDED = 7;
    private static final int CMD_APDU_LENGTH_CASE3_WITHOUT_DATA = 5;
    private static final int CMD_APDU_LENGTH_CASE3_WITHOUT_DATA_EXTENDED = 7;
    private static final int CMD_APDU_LENGTH_CASE4_WITHOUT_DATA = 6;
    private static final int CMD_APDU_LENGTH_CASE4_WITHOUT_DATA_EXTENDED = 9;

    private static final int MAX_EXPECTED_DATA_LENGTH = 65536;

    private static final int OFFSET_CLA = 0;
    private static final int OFFSET_INS = 1;
    private static final int OFFSET_P3 = 4;
    private static final int OFFSET_DATA = 5;
    private static final int OFFSET_DATA_EXTENDED = 7;

    private CommandApduValidator() {
    }

    /**
     * Executes the validation for the specified APDU command.
     *
     * @param apdu a command APDU as byte array.
     *
     * @throws IllegalArgumentException If the command does not follow the APDU command format.
     */
    public static void execute(byte[] apdu) throws IllegalArgumentException {
        if (apdu.length < CMD_APDU_LENGTH_CASE1) {
            throw new IllegalArgumentException("Invalid length for command (" + apdu.length + ").");
        }
        checkCla(apdu[OFFSET_CLA]);
        checkIns(apdu[OFFSET_INS]);

        if (apdu.length == CMD_APDU_LENGTH_CASE1) {
            return; // Case 1
        }

        if (apdu.length == CMD_APDU_LENGTH_CASE2) {
            checkLe((int) 0x0FF & apdu[OFFSET_P3]);
            return; // Case 2S
        }

        if (apdu[OFFSET_P3] != (byte) 0x00) {
            int lc = ((int) 0x0FF & apdu[OFFSET_P3]);
            if (apdu.length == CMD_APDU_LENGTH_CASE3_WITHOUT_DATA + lc) {
                return; // Case 3S
            }
            if (apdu.length == CMD_APDU_LENGTH_CASE4_WITHOUT_DATA + lc) {
                checkLe((int) 0x0FF & apdu[apdu.length - 1]);
                return; // Case 4S
            }
            throw new IllegalArgumentException("Unexpected value of Lc (" + lc + ")");
        }

        if (apdu.length == CMD_APDU_LENGTH_CASE2_EXTENDED) {
            checkLe((((int) 0x0FF & apdu[OFFSET_DATA]) << 8)
                    + ((int) 0x0FF & apdu[OFFSET_DATA + 1]));
            return; // Case 2E
        }

        if (apdu.length <= OFFSET_DATA_EXTENDED) {
            throw new IllegalArgumentException("Unexpected value of Lc or Le" + apdu.length);
        }

        int lc = (((int) 0x0FF & apdu[OFFSET_DATA]) << 8) + ((int) 0x0FF & apdu[OFFSET_DATA + 1]);
        if (lc == 0) {
            throw new IllegalArgumentException("Lc can't be 0");
        }

        if (apdu.length == CMD_APDU_LENGTH_CASE3_WITHOUT_DATA_EXTENDED
                + lc) {
            return; // Case 3E
        }

        if (apdu.length == CMD_APDU_LENGTH_CASE4_WITHOUT_DATA_EXTENDED + lc) {
            checkLe((((int) 0x0FF & apdu[apdu.length - 2]) << 8)
                    + ((int) 0x0FF & apdu[apdu.length - 1]));
            return; // Case 4E
        }
        throw new IllegalArgumentException("Unexpected value of Lc (" + lc + ")");
    }

    private static void checkCla(byte cla) throws IllegalArgumentException {
        if (cla == (byte) 0xFF) {
            throw new IllegalArgumentException(
                    "Invalid value of CLA (" + Integer.toHexString(cla) + ")");
        }
    }

    private static void checkIns(byte ins) throws IllegalArgumentException {
        if ((ins & 0x0F0) == 0x60 || ((ins & 0x0F0) == 0x90)) {
            throw new IllegalArgumentException(
                    "Invalid value of INS (" + Integer.toHexString(ins) + "). "
                            + "0x6X and 0x9X are not valid values");
        }
    }

    private static void checkLe(int le) throws IllegalArgumentException {
        if (le < 0 || le > MAX_EXPECTED_DATA_LENGTH) {
            throw new IllegalArgumentException(
                    "Invalid value for le parameter (" + le + ").");
        }
    }
}
