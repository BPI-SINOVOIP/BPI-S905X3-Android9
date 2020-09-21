/*
 * Copyright (C) 2008 The Android Open Source Project
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

#include <errno.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "android-base/stringprintf.h"

#include "base/logging.h"  // For VLOG.
#include "jdwp/jdwp_priv.h"
#include "thread-current-inl.h"

#ifdef ART_TARGET_ANDROID
#include "cutils/sockets.h"
#endif

/*
 * The JDWP <-> ADB transport protocol is explained in detail
 * in system/core/adb/jdwp_service.c. Here's a summary.
 *
 * 1/ when the JDWP thread starts, it tries to connect to a Unix
 *    domain stream socket (@jdwp-control) that is opened by the
 *    ADB daemon.
 *
 * 2/ it then sends the current process PID as an int32_t.
 *
 * 3/ then, it uses recvmsg to receive file descriptors from the
 *    daemon. each incoming file descriptor is a pass-through to
 *    a given JDWP debugger, that can be used to read the usual
 *    JDWP-handshake, etc...
 */

static constexpr char kJdwpControlName[] = "\0jdwp-control";
static constexpr size_t kJdwpControlNameLen = sizeof(kJdwpControlName) - 1;
/* This timeout is for connect/send with control socket. In practice, the
 * connect should never timeout since it's just connect to a local unix domain
 * socket. But in case adb is buggy and doesn't respond to any connection, the
 * connect will block. For send, actually it would never block since we only send
 * several bytes and the kernel buffer is big enough to accept it. 10 seconds
 * should be far enough.
 */
static constexpr int kControlSockSendTimeout = 10;

namespace art {

namespace JDWP {

using android::base::StringPrintf;

struct JdwpAdbState : public JdwpNetStateBase {
 public:
  explicit JdwpAdbState(JdwpState* state)
      : JdwpNetStateBase(state),
        state_lock_("JdwpAdbState lock", kJdwpAdbStateLock) {
    control_sock_ = -1;
    shutting_down_ = false;

    control_addr_.controlAddrUn.sun_family = AF_UNIX;
    control_addr_len_ = sizeof(control_addr_.controlAddrUn.sun_family) + kJdwpControlNameLen;
    memcpy(control_addr_.controlAddrUn.sun_path, kJdwpControlName, kJdwpControlNameLen);
  }

  ~JdwpAdbState() {
    if (clientSock != -1) {
      shutdown(clientSock, SHUT_RDWR);
      close(clientSock);
    }
    if (control_sock_ != -1) {
      shutdown(control_sock_, SHUT_RDWR);
      close(control_sock_);
    }
  }

  virtual bool Accept() REQUIRES(!state_lock_);

  virtual bool Establish(const JdwpOptions*) {
    return false;
  }

  virtual void Shutdown() REQUIRES(!state_lock_) {
    int control_sock;
    int local_clientSock;
    {
      MutexLock mu(Thread::Current(), state_lock_);
      shutting_down_ = true;
      control_sock = this->control_sock_;
      local_clientSock = this->clientSock;
      /* clear these out so it doesn't wake up and try to reuse them */
      this->control_sock_ = this->clientSock = -1;
    }

    if (local_clientSock != -1) {
      shutdown(local_clientSock, SHUT_RDWR);
    }

    if (control_sock != -1) {
      shutdown(control_sock, SHUT_RDWR);
    }

    WakePipe();
  }

  virtual bool ProcessIncoming() REQUIRES(!state_lock_);

 private:
  int ReceiveClientFd() REQUIRES(!state_lock_);

  bool IsDown() REQUIRES(!state_lock_) {
    MutexLock mu(Thread::Current(), state_lock_);
    return shutting_down_;
  }

  int ControlSock() REQUIRES(!state_lock_) {
    MutexLock mu(Thread::Current(), state_lock_);
    if (shutting_down_) {
      CHECK_EQ(control_sock_, -1);
    }
    return control_sock_;
  }

  int control_sock_ GUARDED_BY(state_lock_);
  bool shutting_down_ GUARDED_BY(state_lock_);
  Mutex state_lock_;

  socklen_t control_addr_len_;
  union {
    sockaddr_un controlAddrUn;
    sockaddr controlAddrPlain;
  } control_addr_;
};

/*
 * Do initial prep work, e.g. binding to ports and opening files.  This
 * runs in the main thread, before the JDWP thread starts, so it shouldn't
 * do anything that might block forever.
 */
bool InitAdbTransport(JdwpState* state, const JdwpOptions*) {
  VLOG(jdwp) << "ADB transport startup";
  state->netState = new JdwpAdbState(state);
  return (state->netState != nullptr);
}

/*
 * Receive a file descriptor from ADB.  The fd can be used to communicate
 * directly with a debugger or DDMS.
 *
 * Returns the file descriptor on success.  On failure, returns -1 and
 * closes netState->control_sock_.
 */
int JdwpAdbState::ReceiveClientFd() {
  char dummy = '!';
  union {
    cmsghdr cm;
    char buffer[CMSG_SPACE(sizeof(int))];
  } cm_un;

  iovec iov;
  iov.iov_base       = &dummy;
  iov.iov_len        = 1;

  msghdr msg;
  msg.msg_name       = nullptr;
  msg.msg_namelen    = 0;
  msg.msg_iov        = &iov;
  msg.msg_iovlen     = 1;
  msg.msg_flags      = 0;
  msg.msg_control    = cm_un.buffer;
  msg.msg_controllen = sizeof(cm_un.buffer);

  cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
  cmsg->cmsg_len   = msg.msg_controllen;
  cmsg->cmsg_level = SOL_SOCKET;
  cmsg->cmsg_type  = SCM_RIGHTS;
  (reinterpret_cast<int*>(CMSG_DATA(cmsg)))[0] = -1;

  int rc = TEMP_FAILURE_RETRY(recvmsg(ControlSock(), &msg, 0));

  if (rc <= 0) {
    if (rc == -1) {
      PLOG(WARNING) << "Receiving file descriptor from ADB failed (socket " << ControlSock() << ")";
    }
    MutexLock mu(Thread::Current(), state_lock_);
    close(control_sock_);
    control_sock_ = -1;
    return -1;
  }

  return (reinterpret_cast<int*>(CMSG_DATA(cmsg)))[0];
}

/*
 * Block forever, waiting for a debugger to connect to us.  Called from the
 * JDWP thread.
 *
 * This needs to un-block and return "false" if the VM is shutting down.  It
 * should return "true" when it successfully accepts a connection.
 */
bool JdwpAdbState::Accept() {
  int retryCount = 0;

  /* first, ensure that we get a connection to the ADB daemon */

 retry:
  if (IsDown()) {
    return false;
  }

  if (ControlSock() == -1) {
    int        sleep_ms     = 500;
    const int  sleep_max_ms = 2*1000;

    int sock = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (sock < 0) {
      PLOG(ERROR) << "Could not create ADB control socket";
      return false;
    }
    struct timeval timeout;
    timeout.tv_sec = kControlSockSendTimeout;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    {
      MutexLock mu(Thread::Current(), state_lock_);
      control_sock_ = sock;
      if (shutting_down_) {
        return false;
      }
      if (!MakePipe()) {
        return false;
      }
    }

    int32_t pid = getpid();

    for (;;) {
      /*
       * If adbd isn't running, because USB debugging was disabled or
       * perhaps the system is restarting it for "adb root", the
       * connect() will fail.  We loop here forever waiting for it
       * to come back.
       *
       * Waking up and polling every couple of seconds is generally a
       * bad thing to do, but we only do this if the application is
       * debuggable *and* adbd isn't running.  Still, for the sake
       * of battery life, we should consider timing out and giving
       * up after a few minutes in case somebody ships an app with
       * the debuggable flag set.
       */
      int ret = connect(ControlSock(), &control_addr_.controlAddrPlain, control_addr_len_);
      if (!ret) {
        int control_sock = ControlSock();
#ifdef ART_TARGET_ANDROID
        if (control_sock < 0 || !socket_peer_is_trusted(control_sock)) {
          if (control_sock >= 0 && shutdown(control_sock, SHUT_RDWR)) {
            PLOG(ERROR) << "trouble shutting down socket";
          }
          return false;
        }
#endif

        /* now try to send our pid to the ADB daemon */
        ret = TEMP_FAILURE_RETRY(send(control_sock, &pid, sizeof(pid), 0));
        if (ret == sizeof(pid)) {
          VLOG(jdwp) << "PID " << pid << " sent to ADB";
          break;
        }

        PLOG(ERROR) << "Weird, can't send JDWP process pid to ADB";
        return false;
      }
      if (VLOG_IS_ON(jdwp)) {
        PLOG(ERROR) << "Can't connect to ADB control socket";
      }

      usleep(sleep_ms * 1000);

      sleep_ms += (sleep_ms >> 1);
      if (sleep_ms > sleep_max_ms) {
        sleep_ms = sleep_max_ms;
      }
      if (IsDown()) {
        return false;
      }
    }
  }

  VLOG(jdwp) << "trying to receive file descriptor from ADB";
  /* now we can receive a client file descriptor */
  int sock = ReceiveClientFd();
  {
    MutexLock mu(Thread::Current(), state_lock_);
    clientSock = sock;
    if (shutting_down_) {
      return false;       // suppress logs and additional activity
    }
  }
  if (clientSock == -1) {
    if (++retryCount > 5) {
      LOG(ERROR) << "adb connection max retries exceeded";
      return false;
    }
    goto retry;
  } else {
    VLOG(jdwp) << "received file descriptor " << clientSock << " from ADB";
    SetAwaitingHandshake(true);
    input_count_ = 0;
    return true;
  }
}

/*
 * Process incoming data.  If no data is available, this will block until
 * some arrives.
 *
 * If we get a full packet, handle it.
 *
 * To take some of the mystery out of life, we want to reject incoming
 * connections if we already have a debugger attached.  If we don't, the
 * debugger will just mysteriously hang until it times out.  We could just
 * close the listen socket, but there's a good chance we won't be able to
 * bind to the same port again, which would confuse utilities.
 *
 * Returns "false" on error (indicating that the connection has been severed),
 * "true" if things are still okay.
 */
bool JdwpAdbState::ProcessIncoming() {
  int readCount;

  CHECK_NE(clientSock, -1);

  if (!HaveFullPacket()) {
    /* read some more, looping until we have data */
    errno = 0;
    while (1) {
      int selCount;
      fd_set readfds;
      int maxfd = -1;
      int fd;

      FD_ZERO(&readfds);

      /* configure fds; note these may get zapped by another thread */
      fd = ControlSock();
      if (fd >= 0) {
        FD_SET(fd, &readfds);
        if (maxfd < fd) {
          maxfd = fd;
        }
      }
      fd = clientSock;
      if (fd >= 0) {
        FD_SET(fd, &readfds);
        if (maxfd < fd) {
          maxfd = fd;
        }
      }
      fd = wake_pipe_[0];
      if (fd >= 0) {
        FD_SET(fd, &readfds);
        if (maxfd < fd) {
          maxfd = fd;
        }
      } else {
        LOG(INFO) << "NOTE: entering select w/o wakepipe";
      }

      if (maxfd < 0) {
        VLOG(jdwp) << "+++ all fds are closed";
        return false;
      }

      /*
       * Select blocks until it sees activity on the file descriptors.
       * Closing the local file descriptor does not count as activity,
       * so we can't rely on that to wake us up (it works for read()
       * and accept(), but not select()).
       *
       * We can do one of three things: (1) send a signal and catch
       * EINTR, (2) open an additional fd ("wake pipe") and write to
       * it when it's time to exit, or (3) time out periodically and
       * re-issue the select.  We're currently using #2, as it's more
       * reliable than #1 and generally better than #3.  Wastes two fds.
       */
      selCount = select(maxfd + 1, &readfds, nullptr, nullptr, nullptr);
      if (selCount < 0) {
        if (errno == EINTR) {
          continue;
        }
        PLOG(ERROR) << "select failed";
        goto fail;
      }

      if (wake_pipe_[0] >= 0 && FD_ISSET(wake_pipe_[0], &readfds)) {
        VLOG(jdwp) << "Got wake-up signal, bailing out of select";
        goto fail;
      }
      int control_sock = ControlSock();
      if (control_sock >= 0 && FD_ISSET(control_sock, &readfds)) {
        int  sock = ReceiveClientFd();
        if (sock >= 0) {
          LOG(INFO) << "Ignoring second debugger -- accepting and dropping";
          close(sock);
        } else {
          CHECK_EQ(ControlSock(), -1);
          /*
           * Remote side most likely went away, so our next read
           * on clientSock will fail and throw us out of the loop.
           */
        }
      }
      if (clientSock >= 0 && FD_ISSET(clientSock, &readfds)) {
        readCount = read(clientSock, input_buffer_ + input_count_, sizeof(input_buffer_) - input_count_);
        if (readCount < 0) {
          /* read failed */
          if (errno != EINTR) {
            goto fail;
          }
          VLOG(jdwp) << "+++ EINTR hit";
          return true;
        } else if (readCount == 0) {
          /* EOF hit -- far end went away */
          VLOG(jdwp) << "+++ peer disconnected";
          goto fail;
        } else {
          break;
        }
      }
    }

    input_count_ += readCount;
    if (!HaveFullPacket()) {
      return true;        /* still not there yet */
    }
  }

  /*
   * Special-case the initial handshake.  For some bizarre reason we're
   * expected to emulate bad tty settings by echoing the request back
   * exactly as it was sent.  Note the handshake is always initiated by
   * the debugger, no matter who connects to whom.
   *
   * Other than this one case, the protocol [claims to be] stateless.
   */
  if (IsAwaitingHandshake()) {
    if (memcmp(input_buffer_, kMagicHandshake, kMagicHandshakeLen) != 0) {
      LOG(ERROR) << StringPrintf("ERROR: bad handshake '%.14s'", input_buffer_);
      goto fail;
    }

    errno = 0;
    int cc = TEMP_FAILURE_RETRY(write(clientSock, input_buffer_, kMagicHandshakeLen));
    if (cc != kMagicHandshakeLen) {
      PLOG(ERROR) << "Failed writing handshake bytes (" << cc << " of " << kMagicHandshakeLen << ")";
      goto fail;
    }

    ConsumeBytes(kMagicHandshakeLen);
    SetAwaitingHandshake(false);
    VLOG(jdwp) << "+++ handshake complete";
    return true;
  }

  /*
   * Handle this packet.
   */
  return state_->HandlePacket();

 fail:
  Close();
  return false;
}

}  // namespace JDWP

}  // namespace art
