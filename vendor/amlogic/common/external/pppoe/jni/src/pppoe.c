/***********************************************************************
*
* pppoe.c
*
* Implementation of user-space PPPoE redirector for Linux.
*
* Copyright (C) 2000-2006 by Roaring Penguin Software Inc.
*
* This program may be distributed according to the terms of the GNU
* General Public License, version 2 or (at your option) any later version.
*
* LIC: GPL
*
***********************************************************************/

static char const RCSID[] =
"$Id$";

#include "pppoe.h"

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#include <android/log.h>
#define syslog(prio, fmt...) \
    __android_log_print(prio, "PPPOE", fmt)
#endif

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef USE_LINUX_PACKET
#include <sys/ioctl.h>
#include <fcntl.h>
#endif

#include <signal.h>

#ifdef HAVE_N_HDLC
#ifndef N_HDLC
#include <linux/termios.h>
#endif
#endif
#include "pppoe_status.h"

#include <linux/rtnetlink.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <net/if_arp.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <linux/rtnetlink.h>  //for ifinfomsg
#include <linux/netlink.h> //for nlmsghdr{}

#include <sys/stat.h>
#include <sys/file.h>

#define NL_POLL_MSG_SZ   8*1024

static int open_NETLINK_socket(int netlinkFamily, int groups)
{
    struct sockaddr_nl nl_addr;
    int ret = -1;
    int mysocket;

    memset(&nl_addr, 0, sizeof(struct sockaddr_nl));
    nl_addr.nl_family = AF_NETLINK;
    nl_addr.nl_pid = 0;
    nl_addr.nl_groups = groups;

    /*
     *Create connection to netlink socket
     */
    mysocket = socket(AF_NETLINK,SOCK_DGRAM,netlinkFamily);
    if (mysocket < 0) {
        syslog(LOG_ERR,"Can not create netlink poll socket" );
        goto error;
    }

    errno = 0;
    if(bind(mysocket, (struct sockaddr *)(&nl_addr),
            sizeof(struct sockaddr_nl))) {
        syslog(LOG_ERR, "Can not bind to netlink poll socket,%s",strerror(errno));

        goto error;
    }

    return mysocket;
error:
    syslog(LOG_ERR, "%s failed" ,__FUNCTION__);
    if (mysocket >0)
        close(mysocket);
    return ret;
}


static void parseNetlinkMessage
(char *buff, int len, char*phy_ifname) 
{
    unsigned int left;
    int nAttrLen;
    struct nlmsghdr *nh;
    struct rtattr *pstruAttr;
    char *ifname = "UNKNOWN_NAME";

    for (nh = (struct nlmsghdr *) buff; NLMSG_OK (nh, len);
         nh = NLMSG_NEXT (nh, len))
    {
        if (nh->nlmsg_type == NLMSG_DONE){
            syslog(LOG_ERR, "Did not find useful eth interface information");
            return;
        }

        if (nh->nlmsg_type == NLMSG_ERROR){
            /* Do some error handling. */
            syslog(LOG_ERR, "Read device name failed");
            return;
        }

        if (nh->nlmsg_type == RTM_DELLINK ||
            nh->nlmsg_type == RTM_NEWLINK) {
            struct ifinfomsg *einfo;
            int type = nh->nlmsg_type;
            char *desc;

            einfo = (struct ifinfomsg *)NLMSG_DATA(nh);

            pstruAttr = IFLA_RTA(einfo);
            nAttrLen = NLMSG_PAYLOAD(nh, sizeof(struct ifinfomsg));

            while(RTA_OK(pstruAttr, nAttrLen))
            {
                switch(pstruAttr->rta_type)
                {
                    case IFLA_IFNAME:
                        ifname = (char *)RTA_DATA(pstruAttr);
                        break;

                    default:
                        break;
                }

                pstruAttr = RTA_NEXT(pstruAttr, nAttrLen);
            }

            if (0 != strcmp(ifname, phy_ifname)) {
                continue;
            }

            if (type == RTM_NEWLINK &&
                (!(einfo->ifi_flags & IFF_LOWER_UP))) {
                    type = RTM_DELLINK;
            }

            desc = (char*)((type == RTM_DELLINK) ? "DELLINK" : "NEWLINK");

            syslog(LOG_INFO, "%s: %s(%d), flags=0X%X", ifname, desc, type, einfo->ifi_flags);
            if (type == RTM_DELLINK) {
                syslog(LOG_ERR, "physical interface %s down", ifname);
                exit(EXIT_FAILURE);
            }
        }
    }
}




static void waitForPhyInterfaceEvent
(int nl_socket_netlink_route, char *phy_ifname)
{
    char *buff;
    struct iovec iov;
    struct msghdr msg;
    int len;
    int result = -1;
    int maxfd;

    buff = (char *)malloc(NL_POLL_MSG_SZ);
    if (!buff) {
        syslog(LOG_ERR, "Allocate poll buffer failed");
        goto error;
    }

    iov.iov_base = buff;
    iov.iov_len = NL_POLL_MSG_SZ;
    msg.msg_name = NULL;
    msg.msg_namelen =  0;
    msg.msg_iov =  &iov;
    msg.msg_iovlen =  1;
    msg.msg_control =  NULL;
    msg.msg_controllen =  0;
    msg.msg_flags =  0;


    if((len = recvmsg(nl_socket_netlink_route, &msg, 0))>= 0) {
        parseNetlinkMessage(buff, len, phy_ifname);
    }

    return;

error:
    if(buff)
        free(buff);
    return;
}

/* Default interface if no -I option given */
#define DEFAULT_IF "eth0"

/* Global variables -- options */
int optInactivityTimeout = 0;	/* Inactivity timeout */
int optClampMSS          = 0;	/* Clamp MSS to this value */
int optSkipSession       = 0;	/* Perform discovery, print session info
				   and exit */
int optFloodDiscovery    = 0;   /* Flood server with discovery requests.
				   USED FOR STRESS-TESTING ONLY.  DO NOT
				   USE THE -F OPTION AGAINST A REAL ISP */

PPPoEConnection *Connection = NULL; /* Must be global -- used
				       in signal handler */

int persist = 0; 		/* We are not a pppd plugin */




/***********************************************************************
*%FUNCTION: sendSessionPacket
*%ARGUMENTS:
* conn -- PPPoE connection
* packet -- the packet to send
* len -- length of data to send
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Transmits a session packet to the peer.
***********************************************************************/
void
sendSessionPacket(PPPoEConnection *conn, PPPoEPacket *packet, int len)
{
    packet->length = htons(len);
    if (optClampMSS) {
	clampMSS(packet, "outgoing", optClampMSS);
    }
    if (sendPacket(conn, conn->sessionSocket, packet, len + HDR_SIZE) < 0) {
	if (errno == ENOBUFS) {
	    /* No buffer space is a transient error */
	    return;
	}
	exit(EXIT_FAILURE);
    }
#ifdef DEBUGGING_ENABLED
    if (conn->debugFile) {
	dumpPacket(conn->debugFile, packet, "SENT");
	fprintf(conn->debugFile, "\n");
	fflush(conn->debugFile);
    }
#endif

}

#ifdef USE_BPF
/**********************************************************************
*%FUNCTION: sessionDiscoveryPacket
*%ARGUMENTS:
* packet -- the discovery packet that was received
*%RETURNS:
* Nothing
*%DESCRIPTION:
* We got a discovery packet during the session stage.  This most likely
* means a PADT.
*
* The BSD version uses a single socket for both discovery and session
* packets.  When a packet comes in over the wire once we are in
* session mode, either syncReadFromEth() or asyncReadFromEth() will
* have already read the packet and determined it to be a discovery
* packet before passing it here.
***********************************************************************/
static void
sessionDiscoveryPacket(PPPoEPacket *packet)
{
    /* Sanity check */
    if (packet->code != CODE_PADT) {
	return;
    }

    /* It's a PADT, all right.  Is it for us? */
    if (packet->session != Connection->session) {
	/* Nope, ignore it */
	return;
    }
    if (memcmp(packet->ethHdr.h_dest, Connection->myEth, ETH_ALEN)) {
	return;
    }

    if (memcmp(packet->ethHdr.h_source, Connection->peerEth, ETH_ALEN)) {
	return;
    }

    syslog(LOG_INFO,
	   "Session %d terminated -- received PADT from peer",
	   (int) ntohs(packet->session));
    parsePacket(packet, parseLogErrs, NULL);
    sendPADT(Connection, "Received PADT from peer");
    exit(EXIT_SUCCESS);
}
#else
/**********************************************************************
*%FUNCTION: sessionDiscoveryPacket
*%ARGUMENTS:
* conn -- PPPoE connection
*%RETURNS:
* Nothing
*%DESCRIPTION:
* We got a discovery packet during the session stage.  This most likely
* means a PADT.
***********************************************************************/
static void
sessionDiscoveryPacket(PPPoEConnection *conn)
{
    PPPoEPacket packet;
    int len;

    if (receivePacket(conn->discoverySocket, &packet, &len) < 0) {
	return;
    }

    /* Check length */
    if (ntohs(packet.length) + HDR_SIZE > (unsigned int)len) {
	syslog(LOG_ERR, "Bogus PPPoE length field (%u)",
	       (unsigned int) ntohs(packet.length));
	return;
    }

    if (packet.code != CODE_PADT) {
	/* Not PADT; ignore it */
	return;
    }

    /* It's a PADT, all right.  Is it for us? */
    if (packet.session != conn->session) {
	/* Nope, ignore it */
	return;
    }

    if (memcmp(packet.ethHdr.h_dest, conn->myEth, ETH_ALEN)) {
	return;
    }

    if (memcmp(packet.ethHdr.h_source, conn->peerEth, ETH_ALEN)) {
	return;
    }
#ifdef DEBUGGING_ENABLED
    if (conn->debugFile) {
	dumpPacket(conn->debugFile, &packet, "RCVD");
	fprintf(conn->debugFile, "\n");
	fflush(conn->debugFile);
    }
#endif
    syslog(LOG_INFO,
	   "Session %d terminated -- received PADT from peer",
	   (int) ntohs(packet.session));
    parsePacket(&packet, parseLogErrs, NULL);
    sendPADT(conn, "Received PADT from peer");
    exit(EXIT_SUCCESS);
}
#endif /* USE_BPF */

/**********************************************************************
*%FUNCTION: session
*%ARGUMENTS:
* conn -- PPPoE connection info
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Handles the "session" phase of PPPoE
***********************************************************************/
void
session(PPPoEConnection *conn)
{
    fd_set readable;
    PPPoEPacket packet;
    struct timeval tv;
    struct timeval *tvp = NULL;
    int maxFD = 0;
    int r;

    /* Drop privileges */
    dropPrivs();

    /* Prepare for select() */
    if (conn->sessionSocket > maxFD)   maxFD = conn->sessionSocket;
    if (conn->discoverySocket > maxFD) maxFD = conn->discoverySocket;
    if (conn->netlinkSocket > maxFD) maxFD = conn->netlinkSocket;
    maxFD++;

    /* Fill in the constant fields of the packet to save time */
    memcpy(packet.ethHdr.h_dest, conn->peerEth, ETH_ALEN);
    memcpy(packet.ethHdr.h_source, conn->myEth, ETH_ALEN);
    packet.ethHdr.h_proto = htons(Eth_PPPOE_Session);
    packet.ver = 1;
    packet.type = 1;
    packet.code = CODE_SESS;
    packet.session = conn->session;

    initPPP();

#ifdef USE_BPF
    /* check for buffered session data */
    while (BPF_BUFFER_HAS_DATA) {
	if (conn->synchronous) {
	    syncReadFromEth(conn, conn->sessionSocket, optClampMSS);
	} else {
	    asyncReadFromEth(conn, conn->sessionSocket, optClampMSS);
	}
    }
#endif

    for (;;) {
	if (optInactivityTimeout > 0) {
	    tv.tv_sec = optInactivityTimeout;
	    tv.tv_usec = 0;
	    tvp = &tv;
	}
	FD_ZERO(&readable);
	FD_SET(STDIN_FILENO, &readable);     /* ppp packets come from stdin */
	if (conn->discoverySocket >= 0) {
	    FD_SET(conn->discoverySocket, &readable);
	}
	FD_SET(conn->sessionSocket, &readable);
	FD_SET(conn->netlinkSocket, &readable);
	while(1) {
	    r = select(maxFD, &readable, NULL, NULL, tvp);
	    if (r >= 0 || errno != EINTR) break;
	}
	if (r < 0) {
	    fatalSys("select (session)");
	}
	if (r == 0) { /* Inactivity timeout */
	    syslog(LOG_ERR, "Inactivity timeout... something wicked happened on session %d",
		   (int) ntohs(conn->session));
	    sendPADT(conn, "RP-PPPoE: Inactivity timeout");
	    syslog(LOG_ERR, "EXIT");
	    exit(EXIT_FAILURE);
	}

	/* Handle ready sockets */
	if (FD_ISSET(STDIN_FILENO, &readable)) {
	    if (conn->synchronous) {
		syncReadFromPPP(conn, &packet);
	    } else {
		asyncReadFromPPP(conn, &packet);
	    }
	}

	if (FD_ISSET(conn->sessionSocket, &readable)) {
	    do {
		if (conn->synchronous) {
		    syncReadFromEth(conn, conn->sessionSocket, optClampMSS);
		} else {
		    asyncReadFromEth(conn, conn->sessionSocket, optClampMSS);
		}
	    } while (BPF_BUFFER_HAS_DATA);
	}

	    if (FD_ISSET(conn->netlinkSocket, &readable)) {
            waitForPhyInterfaceEvent(conn->netlinkSocket, conn->ifName);
	    }

#ifndef USE_BPF
	/* BSD uses a single socket, see *syncReadFromEth() */
	/* for calls to sessionDiscoveryPacket() */
	if (conn->discoverySocket >= 0) {
	    if (FD_ISSET(conn->discoverySocket, &readable)) {
		sessionDiscoveryPacket(conn);
	    }
	}
#endif

    }
}


/***********************************************************************
*%FUNCTION: sigPADT
*%ARGUMENTS:
* src -- signal received
*%RETURNS:
* Nothing
*%DESCRIPTION:
* If an established session exists send PADT to terminate from session
*  from our end
***********************************************************************/
static void
sigPADT(int src)
{
  syslog(LOG_INFO,"Received signal %d on session %d.",
	 (int)src, (int) ntohs(Connection->session));
  sendPADTf(Connection, "RP-PPPoE: Received signal %d", src);
  //exit(EXIT_SUCCESS);
}



static void
sigUserAskExit(int src)
{
  int prop_val[64];
  FILE *file;
  syslog(LOG_INFO,"Received signal %d on session %d.",
	 (int)src, (int) ntohs(Connection->session));

  /*
  syslog(LOG_INFO,"set net.ppp.usreit as yes");
  property_set("net.ppp.usreit", "yes");
  property_get("net.ppp.usreit", prop_val, "unknown");
  syslog(LOG_INFO,"read net.ppp.usreit %s", prop_val);
  */
  umask(0);
  file = fopen(_ROOT_PATH "/etc/ppp/useraskquit", "w");
  if (!file) {
    syslog(LOG_INFO, "Could not open %s: %s\n",
	  "/etc/ppp/useraskquit", strerror(errno));
  }
  else
    fclose(file);

  sendPADTf(Connection, "RP-PPPoE: Received signal %d", src);
}


/**********************************************************************
*%FUNCTION: usage
*%ARGUMENTS:
* argv0 -- program name
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Prints usage information and exits.
***********************************************************************/
void
usage(char const *argv0)
{
    fprintf(stderr, "Usage: %s [options]\n", argv0);
    fprintf(stderr, "Options:\n");
#ifdef USE_BPF
    fprintf(stderr, "   -I if_name     -- Specify interface (REQUIRED)\n");
#else
    fprintf(stderr, "   -I if_name     -- Specify interface (default %s.)\n",
	    DEFAULT_IF);
#endif
#ifdef DEBUGGING_ENABLED
    fprintf(stderr, "   -D filename    -- Log debugging information in filename.\n");
#endif
    fprintf(stderr,
	    "   -T timeout     -- Specify inactivity timeout in seconds.\n"
	    "   -t timeout     -- Initial timeout for discovery packets in seconds\n"
	    "   -V             -- Print version and exit.\n"
	    "   -A             -- Print access concentrator names and exit.\n"
	    "   -S name        -- Set desired service name.\n"
	    "   -C name        -- Set desired access concentrator name.\n"
	    "   -U             -- Use Host-Unique to allow multiple PPPoE sessions.\n"
	    "   -s             -- Use synchronous PPP encapsulation.\n"
	    "   -m MSS         -- Clamp incoming and outgoing MSS options.\n"
	    "   -p pidfile     -- Write process-ID to pidfile.\n"
	    "   -e sess:mac    -- Skip discovery phase; use existing session.\n"
	    "   -n             -- Do not open discovery socket.\n"
	    "   -k             -- Kill a session with PADT (requires -e)\n"
	    "   -d             -- Perform discovery, print session info and exit.\n"
	    "   -f disc:sess   -- Set Ethernet frame types (hex).\n"
	    "   -h             -- Print usage information.\n\n"
	    "PPPoE Version %s, Copyright (C) 2001-2006 Roaring Penguin Software Inc.\n"
	    "PPPoE comes with ABSOLUTELY NO WARRANTY.\n"
	    "This is free software, and you are welcome to redistribute it under the terms\n"
	    "of the GNU General Public License, version 2 or any later version.\n"
	    "http://www.roaringpenguin.com\n", VERSION);
    exit(EXIT_SUCCESS);
}

static char *lockfile_path;
#define LOCKFILE_FORMAT            _ROOT_PATH "/etc/ppp/pppoe-%s.lock"

/**********************************************************************
*%FUNCTION: main
*%ARGUMENTS:
* argc, argv -- count and values of command-line arguments
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Main program
***********************************************************************/
int
main(int argc, char *argv[])
{
    int opt;
    int n;
    unsigned int m[6];		/* MAC address in -e option */
    unsigned int s;		/* Temporary to hold session */
    FILE *pidfile;
    int pidfd = -1;
    int len;
    unsigned int discoveryType, sessionType;
    char const *options;

    PPPoEConnection conn;

#ifdef HAVE_N_HDLC
    int disc = N_HDLC;
    long flags;
#endif

    if (getuid() != geteuid() ||
	getgid() != getegid()) {
	IsSetID = 1;
    }

    /* Initialize connection info */
    memset(&conn, 0, sizeof(conn));
    conn.discoverySocket = -1;
    conn.sessionSocket = -1;
    conn.discoveryTimeout = PADI_TIMEOUT;

    /* For signal handler */
    Connection = &conn;

    /* Initialize syslog */
    openlog("pppoe", LOG_PID, LOG_DAEMON);

#ifdef DEBUGGING_ENABLED
    options = "I:VAT:D:hS:C:Usm:np:e:kdf:F:t:";
#else
    options = "I:VAT:hS:C:Usm:np:e:kdf:F:t:";
#endif
    while((opt = getopt(argc, argv, options)) != -1) {
	switch(opt) {
	case 't':
	    if (sscanf(optarg, "%d", &conn.discoveryTimeout) != 1) {
		fprintf(stderr, "Illegal argument to -t: Should be -t timeout\n");
		exit(EXIT_FAILURE);
	    }
	    if (conn.discoveryTimeout < 1) {
		conn.discoveryTimeout = 1;
	    }
	    break;
	case 'F':
	    if (sscanf(optarg, "%d", &optFloodDiscovery) != 1) {
		fprintf(stderr, "Illegal argument to -F: Should be -F numFloods\n");
		exit(EXIT_FAILURE);
	    }
	    if (optFloodDiscovery < 1) optFloodDiscovery = 1;
	    fprintf(stderr,
		    "WARNING: DISCOVERY FLOOD IS MEANT FOR STRESS-TESTING\n"
		    "A PPPOE SERVER WHICH YOU OWN.  DO NOT USE IT AGAINST\n"
		    "A REAL ISP.  YOU HAVE 5 SECONDS TO ABORT.\n");
	    sleep(5);
	    break;
	case 'f':
	    if (sscanf(optarg, "%x:%x", &discoveryType, &sessionType) != 2) {
		fprintf(stderr, "Illegal argument to -f: Should be disc:sess in hex\n");
		exit(EXIT_FAILURE);
	    }
	    Eth_PPPOE_Discovery = (UINT16_t) discoveryType;
	    Eth_PPPOE_Session   = (UINT16_t) sessionType;
	    break;
	case 'd':
	    optSkipSession = 1;
	    break;

	case 'k':
	    conn.killSession = 1;
	    break;

	case 'n':
	    /* Do not even open a discovery socket -- used when invoked
	       by pppoe-server */
	    conn.noDiscoverySocket = 1;
	    break;

	case 'e':
	    /* Existing session: "sess:xx:yy:zz:aa:bb:cc" where "sess" is
	       session-ID, and xx:yy:zz:aa:bb:cc is MAC-address of peer */
	    n = sscanf(optarg, "%u:%2x:%2x:%2x:%2x:%2x:%2x",
		       &s, &m[0], &m[1], &m[2], &m[3], &m[4], &m[5]);
	    if (n != 7) {
		fprintf(stderr, "Illegal argument to -e: Should be sess:xx:yy:zz:aa:bb:cc\n");
		exit(EXIT_FAILURE);
	    }

	    /* Copy MAC address of peer */
	    for (n=0; n<6; n++) {
		conn.peerEth[n] = (unsigned char) m[n];
	    }

	    /* Convert session */
	    conn.session = htons(s);

	    /* Skip discovery phase! */
	    conn.skipDiscovery = 1;
	    break;

	case 'p':
	    switchToRealID();
	    pidfile = fopen(optarg, "w");
	    if (!pidfile) {
            syslog( LOG_INFO, "Could not open %s: %s\n",
            			optarg, strerror(errno));
        }
	    switchToEffectiveID();
	    break;
	case 'S':
	    SET_STRING(conn.serviceName, optarg);
	    break;
	case 'C':
	    SET_STRING(conn.acName, optarg);
	    break;
	case 's':
	    conn.synchronous = 1;
	    break;
	case 'U':
	    conn.useHostUniq = 1;
	    break;
#ifdef DEBUGGING_ENABLED
	case 'D':
	    switchToRealID();
	    conn.debugFile = fopen(optarg, "w");
	    switchToEffectiveID();
	    if (!conn.debugFile) {
		fprintf(stderr, "Could not open %s: %s\n",
			optarg, strerror(errno));
		exit(EXIT_FAILURE);
	    }
	    fprintf(conn.debugFile, "rp-pppoe-%s\n", VERSION);
	    fflush(conn.debugFile);
	    break;
#endif
	case 'T':
	    optInactivityTimeout = (int) strtol(optarg, NULL, 10);
	    if (optInactivityTimeout < 0) {
		optInactivityTimeout = 0;
	    }
	    break;
	case 'm':
	    optClampMSS = (int) strtol(optarg, NULL, 10);
	    if (optClampMSS < 536) {
		fprintf(stderr, "-m: %d is too low (min 536)\n", optClampMSS);
		exit(EXIT_FAILURE);
	    }
	    if (optClampMSS > 1452) {
		fprintf(stderr, "-m: %d is too high (max 1452)\n", optClampMSS);
		exit(EXIT_FAILURE);
	    }
	    break;
	case 'I':
	    SET_STRING(conn.ifName, optarg);
	    break;
	case 'V':
	    printf("Roaring Penguin PPPoE Version %s\n", VERSION);
	    exit(EXIT_SUCCESS);
	case 'A':
	    conn.printACNames = 1;
	    break;
	case 'h':
	    usage(argv[0]);
	    break;
	default:
	    usage(argv[0]);
	}
    }

    /* Pick a default interface name */
    if (!conn.ifName) {
#ifdef USE_BPF
	fprintf(stderr, "No interface specified (-I option)\n");
	exit(EXIT_FAILURE);
#else
	SET_STRING(conn.ifName, DEFAULT_IF);
#endif
    }

	len = strlen(LOCKFILE_FORMAT) + strlen(conn.ifName) + 2;
	lockfile_path = malloc(len);
	snprintf(lockfile_path, len, LOCKFILE_FORMAT, conn.ifName);
    
	pidfd = open(lockfile_path, O_WRONLY | O_CREAT | O_NONBLOCK, 0664);
	if (pidfd == -1) {
		syslog(LOG_ERR, "failed to open %s. EXIT", lockfile_path);
		exit(EXIT_FAILURE);
	}

    /* Lock the file so that only one instance of pppoe runs
	 * on an interface */
	if (flock(pidfd, LOCK_EX | LOCK_NB) == -1) {
		syslog(LOG_ERR, "failed to lock %s. EXIT", lockfile_path);
		exit(EXIT_FAILURE);
	}
 
    if (pidfile) {
        syslog( LOG_INFO, "open %s OK\n", optarg);
    	fprintf(pidfile, "%lu\n", (unsigned long) getpid());
    	fclose(pidfile);
    }

    if (!conn.printACNames) {

#ifdef HAVE_N_HDLC
	if (conn.synchronous) {
	    if (ioctl(0, TIOCSETD, &disc) < 0) {
		printErr("Unable to set line discipline to N_HDLC.  Make sure your kernel supports the N_HDLC line discipline, or do not use the SYNCHRONOUS option.  Quitting.");
		exit(EXIT_FAILURE);
	    } else {
		syslog(LOG_INFO,
		       "Changed pty line discipline to N_HDLC for synchronous mode");
	    }
	    /* There is a bug in Linux's select which returns a descriptor
	     * as readable if N_HDLC line discipline is on, even if
	     * it isn't really readable.  This return happens only when
	     * select() times out.  To avoid blocking forever in read(),
	     * make descriptor 0 non-blocking */
	    flags = fcntl(0, F_GETFL);
	    if (flags < 0) fatalSys("fcntl(F_GETFL)");
	    if (fcntl(0, F_SETFL, (long) flags | O_NONBLOCK) < 0) {
		fatalSys("fcntl(F_SETFL)");
	    }
	}
#endif

    }

    if (optFloodDiscovery) {
	for (n=0; n < optFloodDiscovery; n++) {
	    if (conn.printACNames) {
		fprintf(stderr, "Sending discovery flood %d\n", n+1);
	    }
            conn.discoverySocket =
           	 openInterface(conn.ifName, Eth_PPPOE_Discovery, conn.myEth);
	    discovery(&conn);
	    conn.discoveryState = STATE_SENT_PADI;
	    close(conn.discoverySocket);
	}
	exit(EXIT_SUCCESS);
    }

    /* Open session socket before discovery phase, to avoid losing session */
    /* packets sent by peer just after PADS packet (noted on some Cisco    */
    /* server equipment).                                                  */
    /* Opening this socket just before waitForPADS in the discovery()      */
    /* function would be more appropriate, but it would mess-up the code   */
    if (!optSkipSession)
        conn.sessionSocket = openInterface(conn.ifName, Eth_PPPOE_Session, conn.myEth);

    /* Skip discovery and don't open discovery socket? */
    if (conn.skipDiscovery && conn.noDiscoverySocket) {
	conn.discoveryState = STATE_SESSION;
    } else {
        conn.discoverySocket =
	    openInterface(conn.ifName, Eth_PPPOE_Discovery, conn.myEth);
    	syslog( LOG_INFO, "begin discovery conn.myEth =%p\n", conn.myEth);
        discovery(&conn);

        conn.netlinkSocket = open_NETLINK_socket(NETLINK_ROUTE, RTMGRP_LINK);
    }
    if (optSkipSession) {
	printf("%u:%02x:%02x:%02x:%02x:%02x:%02x\n",
	       ntohs(conn.session),
	       conn.peerEth[0],
	       conn.peerEth[1],
	       conn.peerEth[2],
	       conn.peerEth[3],
	       conn.peerEth[4],
	       conn.peerEth[5]);
	exit(EXIT_SUCCESS);
    }

    /* Set signal handlers: send PADT on HUP; ignore TERM and INT */
    signal(SIGTERM, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    signal(SIGHUP, sigPADT);
    signal(SIGUSR1, sigUserAskExit);

    session(&conn);
    return 0;
}

/**********************************************************************
*%FUNCTION: fatalSys
*%ARGUMENTS:
* str -- error message
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Prints a message plus the errno value to stderr and syslog and exits.
***********************************************************************/
void
fatalSys(char const *str)
{
    char buf[1024];
    sprintf(buf, "%.256s: Session %d: %.256s",
	    str, (int) ntohs(Connection->session), strerror(errno));
    printErr(buf);
    sendPADTf(Connection, "RP-PPPoE: System call error: %s",
	      strerror(errno));
    exit(EXIT_FAILURE);
}

/**********************************************************************
*%FUNCTION: sysErr
*%ARGUMENTS:
* str -- error message
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Prints a message plus the errno value to syslog.
***********************************************************************/
void
sysErr(char const *str)
{
    char buf[1024];
    sprintf(buf, "%.256s: %.256s", str, strerror(errno));
    printErr(buf);
}

/**********************************************************************
*%FUNCTION: rp_fatal
*%ARGUMENTS:
* str -- error message
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Prints a message to stderr and syslog and exits.
***********************************************************************/
void
rp_fatal(char const *str)
{
    printErr(str);
    sendPADTf(Connection, "RP-PPPoE: Session %d: %.256s",
	      (int) ntohs(Connection->session), str);
    exit(EXIT_FAILURE);
}

/**********************************************************************
*%FUNCTION: asyncReadFromEth
*%ARGUMENTS:
* conn -- PPPoE connection info
* sock -- Ethernet socket
* clampMss -- if non-zero, do MSS-clamping
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Reads a packet from the Ethernet interface and sends it to async PPP
* device.
***********************************************************************/
void
asyncReadFromEth(PPPoEConnection *conn, int sock, int clampMss)
{
    PPPoEPacket packet;
    int len;
    int plen;
    int i;
    unsigned char pppBuf[4096];
    unsigned char *ptr = pppBuf;
    unsigned char c;
    UINT16_t fcs;
    unsigned char header[2] = {FRAME_ADDR, FRAME_CTRL};
    unsigned char tail[2];
#ifdef USE_BPF
    int type;
#endif

    if (receivePacket(sock, &packet, &len) < 0) {
	return;
    }

    /* Check length */
    if (ntohs(packet.length) + HDR_SIZE > (unsigned int)len) {
	syslog(LOG_ERR, "Bogus PPPoE length field (%u)",
	       (unsigned int) ntohs(packet.length));
	return;
    }
#ifdef DEBUGGING_ENABLED
    if (conn->debugFile) {
	dumpPacket(conn->debugFile, &packet, "RCVD");
	fprintf(conn->debugFile, "\n");
	fflush(conn->debugFile);
    }
#endif

#ifdef USE_BPF
    /* Make sure this is a session packet before processing further */
    type = etherType(&packet);
    if (type == Eth_PPPOE_Discovery) {
	sessionDiscoveryPacket(&packet);
    } else if (type != Eth_PPPOE_Session) {
	return;
    }
#endif

    /* Sanity check */
    if (packet.code != CODE_SESS) {
	syslog(LOG_ERR, "Unexpected packet code %d", (int) packet.code);
	return;
    }
    if (packet.ver != 1) {
	syslog(LOG_ERR, "Unexpected packet version %d", (int) packet.ver);
	return;
    }
    if (packet.type != 1) {
	syslog(LOG_ERR, "Unexpected packet type %d", (int) packet.type);
	return;
    }
    if (memcmp(packet.ethHdr.h_dest, conn->myEth, ETH_ALEN)) {
	return;
    }
    if (memcmp(packet.ethHdr.h_source, conn->peerEth, ETH_ALEN)) {
	/* Not for us -- must be another session.  This is not an error,
	   so don't log anything.  */
	return;
    }

    if (packet.session != conn->session) {
	/* Not for us -- must be another session.  This is not an error,
	   so don't log anything.  */
	return;
    }
    plen = ntohs(packet.length);
    if (plen + HDR_SIZE > (unsigned int)len) {
	syslog(LOG_ERR, "Bogus length field in session packet %d (%d)",
	       (int) plen, (int) len);
	return;
    }

    /* Clamp MSS */
    if (clampMss) {
	clampMSS(&packet, "incoming", clampMss);
    }

    /* Compute FCS */
    fcs = pppFCS16(PPPINITFCS16, header, 2);
    fcs = pppFCS16(fcs, packet.payload, plen) ^ 0xffff;
    tail[0] = fcs & 0x00ff;
    tail[1] = (fcs >> 8) & 0x00ff;

    /* Build a buffer to send to PPP */
    *ptr++ = FRAME_FLAG;
    *ptr++ = FRAME_ADDR;
    *ptr++ = FRAME_ESC;
    *ptr++ = FRAME_CTRL ^ FRAME_ENC;

    for (i=0; i<plen; i++) {
	c = packet.payload[i];
	if (c == FRAME_FLAG || c == FRAME_ADDR || c == FRAME_ESC || c < 0x20) {
	    *ptr++ = FRAME_ESC;
	    *ptr++ = c ^ FRAME_ENC;
	} else {
	    *ptr++ = c;
	}
    }
    for (i=0; i<2; i++) {
	c = tail[i];
	if (c == FRAME_FLAG || c == FRAME_ADDR || c == FRAME_ESC || c < 0x20) {
	    *ptr++ = FRAME_ESC;
	    *ptr++ = c ^ FRAME_ENC;
	} else {
	    *ptr++ = c;
	}
    }
    *ptr++ = FRAME_FLAG;

    /* Ship it out */
    if (write(STDOUT_FILENO, pppBuf, (ptr-pppBuf)) < 0) {
	fatalSys("asyncReadFromEth: write");
    }
}

/**********************************************************************
*%FUNCTION: syncReadFromEth
*%ARGUMENTS:
* conn -- PPPoE connection info
* sock -- Ethernet socket
* clampMss -- if true, clamp MSS.
*%RETURNS:
* Nothing
*%DESCRIPTION:
* Reads a packet from the Ethernet interface and sends it to sync PPP
* device.
***********************************************************************/
void
syncReadFromEth(PPPoEConnection *conn, int sock, int clampMss)
{
    PPPoEPacket packet;
    int len;
    int plen;
    struct iovec vec[2];
    unsigned char dummy[2];
#ifdef USE_BPF
    int type;
#endif

    if (receivePacket(sock, &packet, &len) < 0) {
	return;
    }

    /* Check length */
    if (ntohs(packet.length) + HDR_SIZE > (unsigned int)len) {
	syslog(LOG_ERR, "Bogus PPPoE length field (%u)",
	       (unsigned int) ntohs(packet.length));
	return;
    }
#ifdef DEBUGGING_ENABLED
    if (conn->debugFile) {
	dumpPacket(conn->debugFile, &packet, "RCVD");
	fprintf(conn->debugFile, "\n");
	fflush(conn->debugFile);
    }
#endif

#ifdef USE_BPF
    /* Make sure this is a session packet before processing further */
    type = etherType(&packet);
    if (type == Eth_PPPOE_Discovery) {
	sessionDiscoveryPacket(&packet);
    } else if (type != Eth_PPPOE_Session) {
	return;
    }
#endif

    /* Sanity check */
    if (packet.code != CODE_SESS) {
	syslog(LOG_ERR, "Unexpected packet code %d", (int) packet.code);
	return;
    }
    if (packet.ver != 1) {
	syslog(LOG_ERR, "Unexpected packet version %d", (int) packet.ver);
	return;
    }
    if (packet.type != 1) {
	syslog(LOG_ERR, "Unexpected packet type %d", (int) packet.type);
	return;
    }
    if (memcmp(packet.ethHdr.h_dest, conn->myEth, ETH_ALEN)) {
	/* Not for us -- must be another session.  This is not an error,
	   so don't log anything.  */
	return;
    }
    if (memcmp(packet.ethHdr.h_source, conn->peerEth, ETH_ALEN)) {
	/* Not for us -- must be another session.  This is not an error,
	   so don't log anything.  */
	return;
    }
    if (packet.session != conn->session) {
	/* Not for us -- must be another session.  This is not an error,
	   so don't log anything.  */
	return;
    }
    plen = ntohs(packet.length);
    if (plen + HDR_SIZE > (unsigned int)len) {
	syslog(LOG_ERR, "Bogus length field in session packet %d (%d)",
	       (int) plen, (int) len);
	return;
    }

    /* Clamp MSS */
    if (clampMss) {
	clampMSS(&packet, "incoming", clampMss);
    }

    /* Ship it out */
    vec[0].iov_base = (void *) dummy;
    dummy[0] = FRAME_ADDR;
    dummy[1] = FRAME_CTRL;
    vec[0].iov_len = 2;
    vec[1].iov_base = (void *) packet.payload;
    vec[1].iov_len = plen;

    if (writev(1, vec, 2) < 0) {
	fatalSys("syncReadFromEth: write");
    }
}
