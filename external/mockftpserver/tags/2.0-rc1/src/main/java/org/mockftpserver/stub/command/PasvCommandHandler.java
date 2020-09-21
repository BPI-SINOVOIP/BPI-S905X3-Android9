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
package org.mockftpserver.stub.command;

import org.apache.log4j.Logger;
import org.mockftpserver.core.command.Command;
import org.mockftpserver.core.command.CommandHandler;
import org.mockftpserver.core.command.InvocationRecord;
import org.mockftpserver.core.command.ReplyCodes;
import org.mockftpserver.core.session.Session;
import org.mockftpserver.core.util.Assert;
import org.mockftpserver.core.util.PortParser;

import java.io.IOException;
import java.net.InetAddress;

/**
 * CommandHandler for the PASV (Passove Mode) command. Request the Session to switch to passive
 * data connection mode. Return a reply code of 227, along with response text of the form:
 * "<i>Entering Passive Mode. (h1,h2,h3,h4,p1,p2)</i>", where <i>h1..h4</i> are the 4
 * bytes of the 32-bit internet host address of the server, and <i>p1..p2</i> are the 2
 * bytes of the 16-bit TCP port address of the data connection on the server to which
 * the client must connect. See RFC959 for more information.
 * <p/>
 * Each invocation record stored by this CommandHandler contains no data elements.
 *
 * @author Chris Mair
 * @version $Revision$ - $Date$
 */
public final class PasvCommandHandler extends AbstractStubCommandHandler implements CommandHandler {

    private static final Logger LOG = Logger.getLogger(PasvCommandHandler.class);

    /**
     * Constructor. Initialize the replyCode.
     */
    public PasvCommandHandler() {
        setReplyCode(ReplyCodes.PASV_OK);
    }

    /**
     * @throws IOException
     * @see org.mockftpserver.core.command.CommandHandler#handleCommand(org.mockftpserver.core.command.Command, org.mockftpserver.core.session.Session)
     */
    public void handleCommand(Command command, Session session, InvocationRecord invocationRecord)
            throws IOException {

        int port = session.switchToPassiveMode();
        InetAddress server = session.getServerHost();

        Assert.isTrue(port > -1, "The server-side port is invalid: " + port);
        LOG.debug("server=" + server + " port=" + port);
        String hostAndPort = "(" + PortParser.convertHostAndPortToCommaDelimitedBytes(server, port) + ")";

        sendReply(session, hostAndPort);
    }

}
