/**
 * Copyright 2016 Google Inc. All Rights Reserved.
 *
 * <p>Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of the License at
 *
 * <p>http://www.apache.org/licenses/LICENSE-2.0
 *
 * <p>Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.vts.util;

import com.android.vts.entity.DeviceInfoEntity;
import com.android.vts.entity.TestRunEntity;
import com.android.vts.entity.UserFavoriteEntity;
import com.google.appengine.api.datastore.DatastoreService;
import com.google.appengine.api.datastore.DatastoreServiceFactory;
import com.google.appengine.api.datastore.Entity;
import com.google.appengine.api.datastore.Key;
import com.google.appengine.api.datastore.Query;
import com.google.appengine.api.datastore.Query.Filter;
import com.google.appengine.api.datastore.Query.FilterOperator;
import com.google.appengine.api.datastore.Query.FilterPredicate;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Properties;
import java.util.Set;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.mail.Message;
import javax.mail.MessagingException;
import javax.mail.Session;
import javax.mail.Transport;
import javax.mail.internet.InternetAddress;
import javax.mail.internet.MimeMessage;
import org.apache.commons.lang.StringUtils;

/** EmailHelper, a helper class for building and sending emails. */
public class EmailHelper {
    protected static final Logger logger = Logger.getLogger(EmailHelper.class.getName());
    protected static final String DEFAULT_EMAIL = System.getProperty("DEFAULT_EMAIL");
    protected static final String EMAIL_DOMAIN = System.getProperty("EMAIL_DOMAIN");
    protected static final String SENDER_EMAIL = System.getProperty("SENDER_EMAIL");
    private static final String VTS_EMAIL_NAME = "VTS Alert Bot";

    /**
     * Create an email footer with the information from the test run.
     *
     * @param testRun The TestRunEntity containing test run metadata, or null.
     * @param devices The list of devices whose fingerprints to include in the email, or null.
     * @param link A link to the Dashboard page containing more information.
     * @return The String email footer.
     */
    public static String getEmailFooter(
            TestRunEntity testRun, List<DeviceInfoEntity> devices, String link) {
        StringBuilder sb = new StringBuilder();
        sb.append("<br><br>");
        if (devices != null) {
            for (DeviceInfoEntity device : devices) {
                sb.append("Device: " + device.getFingerprint() + "<br>");
            }
        }

        if (testRun != null) {
            sb.append("VTS Build ID: " + testRun.testBuildId + "<br>");
            sb.append("Start Time: " + TimeUtil.getDateTimeString(testRun.startTimestamp));
            sb.append("<br>End Time: " + TimeUtil.getDateTimeString(testRun.endTimestamp));
        }
        sb.append(
                "<br><br>For details, visit the"
                        + " <a href='"
                        + link
                        + "'>"
                        + "VTS dashboard.</a>");
        return sb.toString();
    }

    /**
     * Fetches the list of subscriber email addresses for a test.
     *
     * @param testKey The key for the test for which to fetch the email addresses.
     * @returns List of email addresses (String).
     * @throws IOException
     */
    public static List<String> getSubscriberEmails(Key testKey) throws IOException {
        DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();
        Filter testFilter =
                new FilterPredicate(UserFavoriteEntity.TEST_KEY, FilterOperator.EQUAL, testKey);
        Query favoritesQuery = new Query(UserFavoriteEntity.KIND).setFilter(testFilter);
        Set<String> emailSet = new HashSet<>();
        if (!StringUtils.isBlank(DEFAULT_EMAIL)) {
            emailSet.add(DEFAULT_EMAIL);
        }
        for (Entity favorite : datastore.prepare(favoritesQuery).asIterable()) {
            UserFavoriteEntity favoriteEntity = UserFavoriteEntity.fromEntity(favorite);
            if (favoriteEntity != null
                    && favoriteEntity.user != null
                    && favoriteEntity.user.getEmail().endsWith(EMAIL_DOMAIN)
                    && !favoriteEntity.muteNotifications) {
                emailSet.add(favoriteEntity.user.getEmail());
            }
        }
        return new ArrayList<>(emailSet);
    }

    /**
     * Sends an email to the specified email address to notify of a test status change.
     *
     * @param emails List of subscriber email addresses (byte[]) to which the email should be sent.
     * @param subject The email subject field, string.
     * @param body The html (string) body to send in the email.
     * @returns The Message object to be sent.
     * @throws MessagingException, UnsupportedEncodingException
     */
    public static Message composeEmail(List<String> emails, String subject, String body)
            throws MessagingException, UnsupportedEncodingException {
        if (emails.size() == 0) {
            throw new MessagingException("No subscriber email addresses provided");
        }
        Properties props = new Properties();
        Session session = Session.getDefaultInstance(props, null);

        Message msg = new MimeMessage(session);
        for (String email : emails) {
            try {
                msg.addRecipient(Message.RecipientType.TO, new InternetAddress(email, email));
            } catch (MessagingException | UnsupportedEncodingException e) {
                // Gracefully continue when a subscriber email is invalid.
                logger.log(Level.WARNING, "Error sending email to recipient " + email + " : ", e);
            }
        }
        msg.setFrom(new InternetAddress(SENDER_EMAIL, VTS_EMAIL_NAME));
        msg.setSubject(subject);
        msg.setContent(body, "text/html; charset=utf-8");
        return msg;
    }

    /**
     * Sends an email.
     *
     * @param msg Message object to send.
     * @returns true if the message sends successfully, false otherwise
     */
    public static boolean send(Message msg) {
        try {
            Transport.send(msg);
        } catch (MessagingException e) {
            logger.log(Level.WARNING, "Error sending email : ", e);
            return false;
        }
        return true;
    }

    /**
     * Sends a list of emails and logs any failures.
     *
     * @param messages List of Message objects to be sent.
     */
    public static void sendAll(List<Message> messages) {
        for (Message msg : messages) {
            send(msg);
        }
    }

    /**
     * Sends an email.
     *
     * @param recipients List of email address strings to which an email will be sent.
     * @param subject The subject of the email.
     * @param body The body of the email.
     * @returns true if the message sends successfully, false otherwise
     */
    public static boolean send(List<String> recipients, String subject, String body) {
        try {
            Message msg = composeEmail(recipients, subject, body);
            return send(msg);
        } catch (MessagingException | UnsupportedEncodingException e) {
            logger.log(Level.WARNING, "Error composing email : ", e);
            return false;
        }
    }
}
