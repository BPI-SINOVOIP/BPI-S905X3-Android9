/*
 * Copyright (C) 2013 The Android Open Source Project
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
package com.android.tradefed.log;

import com.android.tradefed.util.IEmail;
import com.android.tradefed.util.IEmail.Message;

import junit.framework.TestCase;

import org.easymock.EasyMock;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Collection;

/**
 * Unit tests for {@link TerribleFailureEmailHandler}.
 */
public class TerribleFailureEmailHandlerTest extends TestCase {
    private IEmail mMockEmail;
    private TerribleFailureEmailHandler mWtfEmailHandler;
    private final static String MOCK_HOST_NAME = "myhostname.mydomain.com";
    private long mCurrentTimeMillis;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mMockEmail = EasyMock.createMock(IEmail.class);
        mWtfEmailHandler = new TerribleFailureEmailHandler(mMockEmail) {
            @Override
            protected String getLocalHostName() {
                return MOCK_HOST_NAME;
            }

            @Override
            protected long getCurrentTimeMillis() {
                return mCurrentTimeMillis;
            }
        };
        mCurrentTimeMillis = System.currentTimeMillis();
    }

    /**
     * Test normal success case for {@link TerribleFailureEmailHandler#onTerribleFailure(String, Throwable)}.
     * @throws IOException
     */
    public void testOnTerribleFailure() throws IllegalArgumentException, IOException {
        mMockEmail.send(EasyMock.<Message>anyObject());
        EasyMock.replay(mMockEmail);
        mWtfEmailHandler.addDestination("user@domain.com");
        boolean retValue = mWtfEmailHandler.onTerribleFailure("something terrible happened", null);
        EasyMock.verify(mMockEmail);
        assertTrue(retValue);
    }

    /**
     * Test that onTerribleFailure catches IllegalArgumentException when Mailer
     * state is incorrect
     */
    public void testOnTerribleFailure_catchesIllegalArgumentException() {
        try {
            mMockEmail.send(EasyMock.<Message> anyObject());
        } catch (IOException e) {
            fail("IOException escaped the method under test - should never happen");
        }
        EasyMock.expectLastCall().andThrow(new IllegalArgumentException("Mailer state illegal"));
        EasyMock.replay(mMockEmail);

        mWtfEmailHandler.addDestination("user@domain.com");
        boolean retValue = mWtfEmailHandler.onTerribleFailure("something terrible happened", null);
        assertFalse(retValue);
    }

    /**
     * Test that onTerribleFailure catches IOException
     */
    public void testOnTerribleFailure_catchesIOException() {
        try {
            mMockEmail.send(EasyMock.<Message> anyObject());
        } catch (IOException e) {
            fail("IOException escaped the method under test - should never happen");
        }
        EasyMock.expectLastCall().andThrow(new IOException("Mailer had an IO Exception"));
        EasyMock.replay(mMockEmail);

        mWtfEmailHandler.addDestination("user@domain.com");
        boolean retValue = mWtfEmailHandler.onTerribleFailure("something terrible happened", null);
        assertFalse(retValue);
    }

    /**
     * Test that no email is attempted to be sent when there is no destination set
     */
    public void testOnTerribleFailure_emptyDestinations() {
        boolean retValue = mWtfEmailHandler.onTerribleFailure("something terrible happened", null);
        assertFalse(retValue);
    }

    /**
     * Test that no email is attempted to be sent if it is too adjacent to the previous failure.
     */
    public void testOnTerribleFailure_adjacentFailures() throws IllegalArgumentException,
            IOException {
        mMockEmail.send(EasyMock.<Message>anyObject());
        mWtfEmailHandler.setMinEmailInterval(60000);

        EasyMock.replay(mMockEmail);
        mWtfEmailHandler.addDestination("user@domain.com");
        boolean retValue = mWtfEmailHandler.onTerribleFailure("something terrible happened", null);
        assertTrue(retValue);
        mCurrentTimeMillis += 30000;
        retValue = mWtfEmailHandler.onTerribleFailure("something terrible happened again", null);
        assertFalse(retValue);
        EasyMock.verify(mMockEmail);
    }

    /**
     * Test that the second email is attempted to be sent if it is not adjacent to the previous
     * failure.
     */
    public void testOnTerribleFailure_notAdjacentFailures() throws IllegalArgumentException,
            IOException {
        mMockEmail.send(EasyMock.<Message>anyObject());
        mMockEmail.send(EasyMock.<Message>anyObject());
        mWtfEmailHandler.setMinEmailInterval(60000);

        EasyMock.replay(mMockEmail);
        mWtfEmailHandler.addDestination("user@domain.com");
        boolean retValue = mWtfEmailHandler.onTerribleFailure("something terrible happened", null);
        assertTrue(retValue);
        mCurrentTimeMillis += 90000;
        retValue = mWtfEmailHandler.onTerribleFailure("something terrible happened again", null);
        assertTrue(retValue);
        EasyMock.verify(mMockEmail);
    }

    /**
     * Test that the generated email message actually contains the sender and
     * destination email addresses.
     */
    public void testGenerateEmailMessage() {
        Collection<String> destinations = new ArrayList<String>();
        String sender = "alerter@email.address.com";
        String destA = "a@email.address.com";
        String destB = "b@email.address.com";
        destinations.add(destB);
        destinations.add(destA);

        mWtfEmailHandler.setSender(sender);
        mWtfEmailHandler.addDestination(destA);
        mWtfEmailHandler.addDestination(destB);
        Message msg = mWtfEmailHandler.generateEmailMessage("something terrible happened",
                new Throwable("hello"));
        assertEquals(msg.getSender(), sender);
        assertTrue(msg.getTo().equals(destinations));
    }

    /**
     * Test normal success case for
     * {@link TerribleFailureEmailHandler#generateEmailSubject()}.
     */
    public void testGenerateEmailSubject() {
        assertEquals("WTF happened to tradefed on " + MOCK_HOST_NAME,
                mWtfEmailHandler.generateEmailSubject());
    }
}
