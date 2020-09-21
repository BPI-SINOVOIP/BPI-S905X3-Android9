/*
*Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
*This source code is subject to the terms and conditions defined in the
*file 'LICENSE' which is part of this source code package.
*/

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <syslog.h>
#include <netdb.h>
#include <utmp.h>
#include <pwd.h>
#include <setjmp.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cutils/properties.h>
#include <sys/un.h>

#include <android/log.h>
#include <netutils/ifc.h>

#include "netwrapper.h"

#include "pppoe_status.h"

#define LOCAL_TAG "PPPOE_WRAPPER"

static pid_t read_pid(const char *pidfile)
{
	FILE *fp;
	pid_t pid;

	if ((fp = fopen(pidfile, "r")) == NULL) {
        __android_log_print(ANDROID_LOG_ERROR, LOCAL_TAG,
            "failed to open %s (%s)\n", pidfile, strerror(errno));
		errno = ENOENT;
		return 0;
	}

	if (fscanf(fp, "%d", &pid) != 1) {
        __android_log_print(ANDROID_LOG_ERROR, LOCAL_TAG,
            "failed to read pid, make pid as 0\n");
		pid = 0;
	}
	fclose(fp);

    __android_log_print(ANDROID_LOG_INFO, LOCAL_TAG,
        "read_pid: %d\n", pid);
    
	return pid;
}

static int pppoe_disconnect_handler(char *request)
{
	pid_t pid;
    int ret;
    
    pid = read_pid(PPPOE_PIDFILE);
    if ( 0 == pid ) {
        __android_log_print(ANDROID_LOG_ERROR, LOCAL_TAG,
            "failed to stop ppp for no pid got\n" );
        return -1;
    }

    ret = kill(pid, 0);
    if ( 0 != ret ) {
        __android_log_print(ANDROID_LOG_ERROR, LOCAL_TAG,
            "process(#%d) died already???\n", pid );
        return -1;
    }

    /*
    The signals SIGKILL and SIGSTOP cannot 
    be caught, blocked, or ignored.
    So send SIGUSR1 to notify pppoe to send PADT.
    */
    ret = kill(pid, SIGUSR1);
    __android_log_print(ANDROID_LOG_INFO, LOCAL_TAG,
        "Send SIGUSR1 to pid(#%d), ret = %d\n", pid, ret );

    /*
    If no sleep before send SIGKILL, pppoe will just being killed
    rather than sending PADT.
    */
    __android_log_print(ANDROID_LOG_INFO, LOCAL_TAG,
        "sleep before send SIGKILL to pid(#%d)\n", pid );
    
    sleep(5);

    ret = kill(pid, SIGKILL);
    __android_log_print(ANDROID_LOG_INFO, LOCAL_TAG,
        "Send SIGKILL to pid(#%d), ret = %d\n", pid, ret );

	unlink(PPPOE_PIDFILE);
    __android_log_print(ANDROID_LOG_INFO, LOCAL_TAG,
        "removed %s\n", PPPOE_PIDFILE );

    return 0;    
}

extern void sendSavedPADT(char *padt_file);
static int pppoe_terminate_handler(char *request)
{
    int i;
    
    for ( i = 0; i < 3; i++ ) {
        sendSavedPADT(_ROOT_PATH "/etc/ppp/padt_bin");
        sleep(1);
    }

    return 0;    
}


static int pppoe_connect_handler(char *request)
{
    if (strstr(request, "eth0")) {
        __android_log_print(ANDROID_LOG_INFO, LOCAL_TAG,
            "FIXME!!! SHOULD NOT clear eth0 ip address. Reference to bug#98924.\n");
        ifc_init();
        ifc_set_addr("eth0",0);
        ifc_close();
    }

    return system(request);
}


int main(int argc, char* argv[])
{
    netwrapper_register_handler("ppp-stop", pppoe_disconnect_handler);
    netwrapper_register_handler("ppp-terminate", pppoe_terminate_handler);
    netwrapper_register_handler("pppd pty", pppoe_connect_handler);
    netwrapper_main(PPPOE_WRAPPER_SERVER_PATH);

    return 0;
}





