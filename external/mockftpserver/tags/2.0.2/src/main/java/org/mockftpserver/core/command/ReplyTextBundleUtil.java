/*
 * Copyright 2007 the original author or authors.
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
package org.mockftpserver.core.command;

import org.mockftpserver.core.util.Assert;

import java.util.ResourceBundle;

/**
 * Contains common utility method to conditionally set the reply text ResourceBundle on a
 * CommandHandler instance.
 * 
 * @version $Revision$ - $Date$
 *
 * @author Chris Mair
 */
public final class ReplyTextBundleUtil {

    /**
     * Set the <code>replyTextBundle</code> property of the specified CommandHandler to the 
     * <code>ResourceBundle</code> if and only if the <code>commandHandler</code> implements the 
     * {@link ReplyTextBundleAware} interface AND its <code>replyTextBundle</code> property 
     * has not been set (is null).
     * 
     * @param commandHandler - the CommandHandler instance
     * @param replyTextBundle - the ResourceBundle to use for localizing reply text
     * 
     * @throws org.mockftpserver.core.util.AssertFailedException - if the commandHandler is null
     */
    public static void setReplyTextBundleIfAppropriate(CommandHandler commandHandler, ResourceBundle replyTextBundle) {
        Assert.notNull(commandHandler, "commandHandler");
        if (commandHandler instanceof ReplyTextBundleAware) {
            ReplyTextBundleAware replyTextBundleAware = (ReplyTextBundleAware) commandHandler;
            if (replyTextBundleAware.getReplyTextBundle() == null) {
                replyTextBundleAware.setReplyTextBundle(replyTextBundle);
            }
        }
    }
    
}
