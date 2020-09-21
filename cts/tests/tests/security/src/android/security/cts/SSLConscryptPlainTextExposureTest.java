/*
 * Copyright (C) 2018 The AndroCid Open Source Project
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
package android.security.cts;

import android.content.res.Resources;
import android.test.AndroidTestCase;
import android.platform.test.annotations.SecurityTest;
import android.test.InstrumentationTestCase;
import android.content.Context;
import android.os.AsyncTask;
import android.os.Bundle;
import android.util.Log;

import java.io.InputStream;
import java.io.IOException;
import java.io.ByteArrayInputStream;
import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.channels.ClosedChannelException;
import java.nio.channels.SelectionKey;
import java.nio.channels.Selector;
import java.nio.channels.ServerSocketChannel;
import java.nio.channels.SocketChannel;
import java.nio.channels.spi.SelectorProvider;
import java.security.KeyManagementException;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.security.UnrecoverableKeyException;
import java.security.cert.CertificateException;
import java.util.Formatter;
import java.util.Iterator;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.regex.Pattern;

import javax.net.ssl.KeyManagerFactory;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLEngine;
import javax.net.ssl.SSLEngineResult;
import javax.net.ssl.SSLException;
import javax.net.ssl.SSLSession;
import javax.net.ssl.TrustManagerFactory;

public class SSLConscryptPlainTextExposureTest extends InstrumentationTestCase {

  private TestSSLServer mTestSSLServer;
  private TestSSLConnection mTestSSLClient;
  public static Context context;
  public static String output = "";
  private final String pattern = ".*PLAIN TEXT EXPOSED.*";

  public void test_android_CVE_2017_13309() {

    context = getInstrumentation().getContext();
    mTestSSLServer = new TestSSLServer();

    try {
      Thread.sleep(1000);
    } catch (InterruptedException e) {
      e.printStackTrace();
    }
    mTestSSLClient = new TestSSLConnection();
    mTestSSLClient.StartConnect();
    assertFalse("Pattern found",
        Pattern.compile(pattern,
          Pattern.DOTALL).matcher(output).matches());
  }

  public static InputStream getResource(final int resource){
    return SSLConscryptPlainTextExposureTest.context.getResources().openRawResource(resource);
  }

  public static void setOutput(String data){
    SSLConscryptPlainTextExposureTest.output = data;
  }
}


class TestSSLConnection {

  public SSLContext sslc;

  public SocketChannel socketChannel;
  public SSLEngine clientEngine;
  public String remoteAddress = "127.0.0.1";
  public int port = 9000;
  public ByteBuffer[] dataOutAppBuffers = new ByteBuffer[3];
  public ByteBuffer dataOutNetBuffer;
  public ByteBuffer hsInAppBuffer, hsInNetBuffer, hsOutAppBuffer, hsOutNetBuffer;
  public boolean isHandshaked = false;
  public ExecutorService executor = Executors.newSingleThreadExecutor();
  public InputStream clientKey = null;
  public InputStream trustedCert = null;

  public void StartConnect() {
    KeyStore ks = null;

    clientKey = SSLConscryptPlainTextExposureTest.getResource(R.raw.cve_2017_13309_client);
    trustedCert = SSLConscryptPlainTextExposureTest.getResource(R.raw.cve_2017_13309_trustedcert);

    try {
      ks = KeyStore.getInstance(KeyStore.getDefaultType());
    } catch (KeyStoreException e) {
      e.printStackTrace();
    }
    KeyStore ts = null;
    try {
      ts = KeyStore.getInstance(KeyStore.getDefaultType());
    } catch (KeyStoreException e) {
      e.printStackTrace();
    }

    try {
      ks.load(clientKey, "pocclient".toCharArray());
    } catch (Exception e) {
      e.printStackTrace();
    }

    try {
      ts.load(trustedCert, "trusted".toCharArray());
    } catch (Exception e) {
      e.printStackTrace();
    }

    KeyManagerFactory kmf = null;
    try {
      kmf = KeyManagerFactory.getInstance(KeyManagerFactory.getDefaultAlgorithm());
    } catch (NoSuchAlgorithmException e) {
      e.printStackTrace();
    }

    try {
      kmf.init(ks, "keypass".toCharArray());
    } catch (Exception e) {
      e.printStackTrace();
    }

    TrustManagerFactory tmf = null;
    try {
      tmf = TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm());
    } catch (NoSuchAlgorithmException e) {
      e.printStackTrace();
    }
    try {
      tmf.init(ts);
    } catch (KeyStoreException e) {
      e.printStackTrace();
    }

    SSLContext sslCtx = null;
    try {
      sslCtx = SSLContext.getInstance("TLSv1.2");
    } catch (NoSuchAlgorithmException e) {
      e.printStackTrace();
    }

    try {
      sslCtx.init(kmf.getKeyManagers(), tmf.getTrustManagers(), null);
    } catch (KeyManagementException e) {
      e.printStackTrace();
    }

    sslc = sslCtx;

    clientEngine = sslc.createSSLEngine(remoteAddress, port);
    clientEngine.setUseClientMode(true);
    SSLSession session = clientEngine.getSession();

    hsOutAppBuffer = ByteBuffer.allocate(4096);
    hsOutNetBuffer = ByteBuffer.allocate(session.getPacketBufferSize());
    hsInAppBuffer = ByteBuffer.allocate(4096);
    hsInNetBuffer = ByteBuffer.allocate(session.getPacketBufferSize());
    dataOutNetBuffer = ByteBuffer.allocate(session.getPacketBufferSize());

    try {
      socketChannel = SocketChannel.open();
    } catch (IOException e) {
      e.printStackTrace();
    }
    try {
      socketChannel.configureBlocking(false);
    } catch (IOException e) {
      e.printStackTrace();
    }
    try {
      socketChannel.connect(new InetSocketAddress(remoteAddress, port));
    } catch (IOException e) {
      e.printStackTrace();
    }

    try {
      while(!socketChannel.finishConnect()) {
      }
    } catch (IOException e) {
      e.printStackTrace();
    }

    try {
      clientEngine.beginHandshake();
    } catch (SSLException e) {
      e.printStackTrace();
    }

    try {
      isHandshaked = doHandshake(socketChannel, clientEngine);
    } catch (IOException e) {
      e.printStackTrace();
    }

    if(isHandshaked) {
      dataOutAppBuffers[0] = ByteBuffer.wrap("PLAIN TEXT EXPOSED".getBytes());
      dataOutAppBuffers[1] = ByteBuffer.wrap("PLAIN TEXT EXPOSED".getBytes());
      dataOutAppBuffers[2] = ByteBuffer.wrap("PLAIN TEXT EXPOSED".getBytes());

      while(dataOutAppBuffers[0].hasRemaining() || dataOutAppBuffers[1].hasRemaining() || dataOutAppBuffers[2].hasRemaining()) {
        dataOutNetBuffer.clear();
        SSLEngineResult result = null;
        try {
          result = clientEngine.wrap(dataOutAppBuffers, 0, 3, dataOutNetBuffer);
        } catch (SSLException e) {
          e.printStackTrace();
        }
        switch(result.getStatus()) {
          case OK:
            dataOutNetBuffer.flip();
            String outbuff = new String("");
            Formatter formatter = new Formatter();
            for(int i = 0; i < dataOutNetBuffer.limit(); i++) {
              outbuff += formatter.format("%02x ", dataOutNetBuffer.get(i)).toString();
            }
            String output = new String(dataOutNetBuffer.array());
            SSLConscryptPlainTextExposureTest.setOutput(output);
            break;
          case BUFFER_OVERFLOW:
            dataOutNetBuffer = enlargePacketBuffer(clientEngine, dataOutNetBuffer);
            break;
          case BUFFER_UNDERFLOW:
            try {
              throw new SSLException("Buffer underflow in sending data");
            } catch (SSLException e) {
              e.printStackTrace();
            }
          case CLOSED:
            try {
              closeConnection(socketChannel, clientEngine);
            } catch (IOException e) {
              e.printStackTrace();
            }
            return;
          default:
            throw new IllegalStateException("Invalid SSL status: " + result.getStatus());
        }
      }
    }

    try {
      clientKey.close();
    } catch (IOException e){
      e.printStackTrace();
    }

    try{
      trustedCert.close();
    } catch (IOException e){
      e.printStackTrace();
    }

    try {
      shutdown();
    } catch (IOException e) {
      e.printStackTrace();
    }
  }

  public void shutdown() throws IOException {
    closeConnection(socketChannel, clientEngine);
    executor.shutdown();
  }

  public void closeConnection(SocketChannel socketChannel, SSLEngine engine) throws IOException {
    engine.closeOutbound();
    doHandshake(socketChannel, engine);
    socketChannel.close();
  }

  public boolean doHandshake(SocketChannel socketChannel, SSLEngine engine) throws IOException {
    SSLEngineResult result;
    SSLEngineResult.HandshakeStatus handshakeStatus;
    int appBufferSize = engine.getSession().getApplicationBufferSize();

    ByteBuffer srcAppData = ByteBuffer.allocate(appBufferSize);
    ByteBuffer dstAppData = ByteBuffer.allocate(appBufferSize);

    srcAppData.clear();
    dstAppData.clear();

    handshakeStatus = engine.getHandshakeStatus();
    while (handshakeStatus != SSLEngineResult.HandshakeStatus.FINISHED
        && handshakeStatus != SSLEngineResult.HandshakeStatus.NOT_HANDSHAKING) {
      switch(handshakeStatus) {
        case NEED_UNWRAP:
          if(socketChannel.read(hsInNetBuffer) < 0) {
            if(engine.isInboundDone() && engine.isOutboundDone()) {
              return false;
            }
            try {
              engine.closeInbound();
            } catch (SSLException e) {
              Log.e("poc-test","Forced to close inbound. No proper SSL/TLS close notification message from peer");
            }
            engine.closeOutbound();
            handshakeStatus = engine.getHandshakeStatus();
            break;
          }
          hsInNetBuffer.flip();
          try {
            result = engine.unwrap(hsInNetBuffer, hsInAppBuffer);
            hsInNetBuffer.compact();
            handshakeStatus = result.getHandshakeStatus();
          } catch (SSLException sslException) {
            engine.closeOutbound();
            handshakeStatus = engine.getHandshakeStatus();
            break;
          }
          switch(result.getStatus()) {
            case OK:
              break;
            case BUFFER_OVERFLOW:
              hsInAppBuffer = enlargeApplicationBuffer(engine, hsInAppBuffer);
              break;
            case BUFFER_UNDERFLOW:
              hsInNetBuffer = handleBufferUnderflow(engine, hsInNetBuffer);
              break;
            case CLOSED:
              if (engine.isOutboundDone()) {
                return false;
              } else {
                engine.closeOutbound();
                handshakeStatus = engine.getHandshakeStatus();
                break;
              }
            default:
              throw new IllegalStateException("Invalid SSL status: " + result.getStatus());
          }
        case NEED_WRAP:
          hsOutNetBuffer.clear();
          try {
            result = engine.wrap(hsOutAppBuffer, hsOutNetBuffer);
            handshakeStatus = result.getHandshakeStatus();
          } catch (SSLException sslException) {
            engine.closeOutbound();
            handshakeStatus = engine.getHandshakeStatus();
            break;
          }
          switch(result.getStatus()) {
            case OK:
              hsOutNetBuffer.flip();
              while(hsOutNetBuffer.hasRemaining()) {
                socketChannel.write(hsOutNetBuffer);
              }
              break;
            case BUFFER_OVERFLOW:
              hsOutNetBuffer = enlargePacketBuffer(engine, hsOutNetBuffer);
              break;
            case BUFFER_UNDERFLOW:
              throw new SSLException("Buffer underflow in handshake and wrap");
            case CLOSED:
              try {
                hsOutNetBuffer.flip();
                while(hsOutNetBuffer.hasRemaining()) {
                  socketChannel.write(hsOutNetBuffer);
                }
                hsInNetBuffer.clear();
              } catch (Exception e) {
                handshakeStatus = engine.getHandshakeStatus();
              }
              break;
            default:
              throw new IllegalStateException("Invalid SSL status: " + result.getStatus());
          }
          break;
        case NEED_TASK:
          Runnable task;
          while((task = engine.getDelegatedTask()) != null) {
            executor.execute(task);
          }
          handshakeStatus = engine.getHandshakeStatus();
          break;
        case FINISHED:
          break;
        case NOT_HANDSHAKING:
          break;
        default:
          throw new IllegalStateException("Invalid SSL status: " + handshakeStatus);
      }
        }
    return true;
  }

  public ByteBuffer enlargePacketBuffer(SSLEngine engine, ByteBuffer buffer) {
    return enlargeBuffer(buffer, engine.getSession().getPacketBufferSize());
  }

  public ByteBuffer enlargeApplicationBuffer(SSLEngine engine, ByteBuffer buffer) {
    return enlargeBuffer(buffer, engine.getSession().getApplicationBufferSize());
  }

  public ByteBuffer enlargeBuffer(ByteBuffer buffer, int bufferSize) {
    if(bufferSize > buffer.capacity()) {
      buffer = ByteBuffer.allocate(bufferSize);
    }
    else {
      buffer = ByteBuffer.allocate(buffer.capacity() * 2);
    }
    return buffer;
  }
  public ByteBuffer handleBufferUnderflow(SSLEngine engine, ByteBuffer buffer) {
    if(engine.getSession().getPacketBufferSize() < buffer.limit()) {
      return buffer;
    }
    else {
      ByteBuffer replaceBuffer = enlargePacketBuffer(engine, buffer);
      buffer.flip();
      replaceBuffer.put(buffer);
      return replaceBuffer;
    }
  }
}

class TestSSLServer {

  public ServerRunnable serverRunnable;
  Thread server;

  public TestSSLServer() {
    serverRunnable = new ServerRunnable();
    server = new Thread(serverRunnable);
    server.start();

    try{
      Thread.sleep(1000);
    }catch(InterruptedException e){
      e.printStackTrace();
    }
  }

  protected void onCancelled(String result) {
    serverRunnable.stop();
  }
}

class ServerRunnable implements Runnable {

  SSLServer server;
  InputStream serverKey = null;
  InputStream trustedCert = null;

  public void run() {
    try {
      server = new SSLServer();
      server.runServer();
    }
    catch (Exception e) {
      e.printStackTrace();
    }
  }

  public void stop() {
    server.stopServer();
  }
}

class SSLServer {
  public SSLContext serverContext;

  public ByteBuffer hsInAppBuffer, hsInNetBuffer, hsOutAppBuffer, hsOutNetBuffer;
  public ByteBuffer dataInAppBuffer, dataInNetBuffer;

  final String hostAddress = "127.0.0.1";
  public int port = 9000;
  public boolean bActive = false;

  public Selector selector;
  InputStream serverKey = null;
  InputStream trustedCert = null;
  public ExecutorService executor = Executors.newSingleThreadExecutor();

  public void stopServer() {
    bActive = false;
    executor.shutdown();
    selector.wakeup();
  }

  public void runServer() {
    KeyStore ks = null;

    serverKey = SSLConscryptPlainTextExposureTest.getResource(R.raw.cve_2017_13309_server);
    trustedCert = SSLConscryptPlainTextExposureTest.getResource(R.raw.cve_2017_13309_trustedcert);

    try {
      ks = KeyStore.getInstance(KeyStore.getDefaultType());
    } catch (KeyStoreException e) {
      e.printStackTrace();
    }
    KeyStore ts = null;
    try {
      ts = KeyStore.getInstance(KeyStore.getDefaultType());
    } catch (KeyStoreException e) {
      e.printStackTrace();
    }

    try {
      ks.load(serverKey, "pocserver".toCharArray());
    } catch (Exception e) {
      e.printStackTrace();
    }
    try {
      ts.load(trustedCert, "trusted".toCharArray());
    } catch (Exception e) {
      e.printStackTrace();
    }

    KeyManagerFactory kmf = null;
    try {
      kmf = KeyManagerFactory.getInstance(KeyManagerFactory.getDefaultAlgorithm());
    } catch (NoSuchAlgorithmException e) {
      e.printStackTrace();
    }
    try {
      kmf.init(ks, "keypass".toCharArray());
    } catch (Exception e) {
      e.printStackTrace();
    }

    TrustManagerFactory tmf = null;
    try {
      tmf = TrustManagerFactory.getInstance(TrustManagerFactory.getDefaultAlgorithm());
    } catch (NoSuchAlgorithmException e) {
      e.printStackTrace();
    }
    try {
      tmf.init(ts);
    } catch (KeyStoreException e) {
      e.printStackTrace();
    }

    SSLContext sslCtx = null;
    try {
      sslCtx = SSLContext.getInstance("TLSv1.2");
    } catch (NoSuchAlgorithmException e) {
      e.printStackTrace();
    }

    try {
      sslCtx.init(kmf.getKeyManagers(), tmf.getTrustManagers(), new SecureRandom());
    } catch (KeyManagementException e) {
      e.printStackTrace();
    }

    serverContext = sslCtx;

    SSLSession dummySession = serverContext.createSSLEngine().getSession();

    hsInAppBuffer = ByteBuffer.allocate(4096);
    hsInNetBuffer = ByteBuffer.allocate(dummySession.getPacketBufferSize());
    hsOutAppBuffer = ByteBuffer.allocate(4096);
    hsOutNetBuffer = ByteBuffer.allocate(dummySession.getPacketBufferSize());
    dataInAppBuffer = ByteBuffer.allocate(4096);
    dataInNetBuffer = ByteBuffer.allocate(dummySession.getPacketBufferSize());
    dummySession.invalidate();

    try {
      selector = SelectorProvider.provider().openSelector();
    } catch (IOException e) {
      e.printStackTrace();
    }
    ServerSocketChannel serverSocketChannel = null;
    try {
      serverSocketChannel = ServerSocketChannel.open();
    } catch (IOException e) {
      e.printStackTrace();
    }
    try {
      serverSocketChannel.configureBlocking(false);
    } catch (IOException e) {
      e.printStackTrace();
    }
    try {
      serverSocketChannel.socket().bind(new InetSocketAddress(hostAddress, port));
    } catch (IOException e) {
      e.printStackTrace();
    }
    try {
      serverSocketChannel.register(selector, SelectionKey.OP_ACCEPT);
    } catch (ClosedChannelException e) {
      e.printStackTrace();
    }

    bActive = true;

    while(bActive) {
      try {
        selector.select();
      } catch (IOException e) {
        e.printStackTrace();
      }
      Iterator<SelectionKey> selectedKeys = selector.selectedKeys().iterator();
      while (selectedKeys.hasNext()) {
        SelectionKey key = selectedKeys.next();
        selectedKeys.remove();
        if (!key.isValid()) {
          continue;
        }
        if (key.isAcceptable()) {
          try {
            accept(key);
          } catch (Exception e) {
            e.printStackTrace();
          }
        } else if (key.isReadable()) {
          try {
            read((SocketChannel) key.channel(), (SSLEngine) key.attachment());
          } catch (IOException e) {
            e.printStackTrace();
          }
        }
      }
    }
  }

  public void accept(SelectionKey key) throws Exception{
    SocketChannel socketChannel = ((ServerSocketChannel)key.channel()).accept();
    socketChannel.configureBlocking(false);

    SSLEngine engine = serverContext.createSSLEngine();
    engine.setUseClientMode(false);
    engine.beginHandshake();

    socketChannel.register(selector, SelectionKey.OP_READ, engine);

    if(doHandshake(socketChannel, engine)) {
      socketChannel.register(selector, SelectionKey.OP_READ, engine);
    }
    else {
      socketChannel.close();
    }
  }

  public boolean doHandshake(SocketChannel socketChannel, SSLEngine engine) throws IOException {
    SSLEngineResult result = null;
    SSLEngineResult.HandshakeStatus handshakeStatus;
    int appBufferSize = engine.getSession().getApplicationBufferSize();

    ByteBuffer srcAppData = ByteBuffer.allocate(appBufferSize);
    ByteBuffer dstAppData = ByteBuffer.allocate(appBufferSize);

    srcAppData.clear();
    dstAppData.clear();

    handshakeStatus = engine.getHandshakeStatus();
    while (handshakeStatus != SSLEngineResult.HandshakeStatus.FINISHED
        && handshakeStatus != SSLEngineResult.HandshakeStatus.NOT_HANDSHAKING) {

      switch(handshakeStatus) {
        case NEED_UNWRAP:
          if(socketChannel.read(hsInNetBuffer) < 0) {
            if(engine.isInboundDone() && engine.isOutboundDone()) {
              return false;
            }
            try {
              engine.closeInbound();
            } catch (SSLException e) {
              Log.e("server-poc-test","Forced to close inbound. No proper SSL/TLS close notification message from peer");
            }
            engine.closeOutbound();
            handshakeStatus = engine.getHandshakeStatus();
            break;
          }
          hsInNetBuffer.flip();
          try {
            result = engine.unwrap(hsInNetBuffer, hsInAppBuffer);
            hsInNetBuffer.compact();
            handshakeStatus = result.getHandshakeStatus();
          } catch (SSLException sslException) {
            engine.closeOutbound();
            handshakeStatus = engine.getHandshakeStatus();
            break;
          }
          switch(result.getStatus()) {
            case OK:
              break;
            case BUFFER_OVERFLOW:
              hsInAppBuffer = enlargeApplicationBuffer(engine, hsInAppBuffer);
              break;
            case BUFFER_UNDERFLOW:
              hsInNetBuffer = handleBufferUnderflow(engine, hsInNetBuffer);
              break;
            case CLOSED:
              if (engine.isOutboundDone()) {
                return false;
              } else {
                engine.closeOutbound();
                handshakeStatus = engine.getHandshakeStatus();
                break;
              }
            default:
              throw new IllegalStateException("Invalid SSL status: " + result.getStatus());
          }
        case NEED_WRAP:
          hsOutNetBuffer.clear();
          try {
            result = engine.wrap(hsOutAppBuffer, hsOutNetBuffer);
            handshakeStatus = result.getHandshakeStatus();
          } catch (SSLException sslException) {
            engine.closeOutbound();
            handshakeStatus = engine.getHandshakeStatus();
            break;
          }
          switch(result.getStatus()) {
            case OK:
              hsOutNetBuffer.flip();
              while(hsOutNetBuffer.hasRemaining()) {
                socketChannel.write(hsOutNetBuffer);
              }
              break;
            case BUFFER_OVERFLOW:
              hsOutNetBuffer = enlargePacketBuffer(engine, hsOutNetBuffer);
              break;
            case BUFFER_UNDERFLOW:
              throw new SSLException("Buffer underflow in handshake and wrap");
            case CLOSED:
              try {
                hsOutNetBuffer.flip();
                while(hsOutNetBuffer.hasRemaining()) {
                  socketChannel.write(hsOutNetBuffer);
                }
                hsInNetBuffer.clear();
              } catch (Exception e) {
                handshakeStatus = engine.getHandshakeStatus();
              }
              break;
            default:
              throw new IllegalStateException("Invalid SSL status: " + result.getStatus());
          }
          break;
        case NEED_TASK:
          Runnable task;
          while((task = engine.getDelegatedTask()) != null) {
            executor.execute(task);
          }
          handshakeStatus = engine.getHandshakeStatus();
          break;
        case FINISHED:
          break;
        case NOT_HANDSHAKING:
          break;
        default:
          throw new IllegalStateException("Invalid SSL status: " + handshakeStatus);
      }
        }
    return true;
  }

  public ByteBuffer enlargePacketBuffer(SSLEngine engine, ByteBuffer buffer) {
    return enlargeBuffer(buffer, engine.getSession().getPacketBufferSize());
  }

  public ByteBuffer enlargeApplicationBuffer(SSLEngine engine, ByteBuffer buffer) {
    return enlargeBuffer(buffer, engine.getSession().getApplicationBufferSize());
  }

  public ByteBuffer enlargeBuffer(ByteBuffer buffer, int bufferSize) {
    if(bufferSize > buffer.capacity()) {
      buffer = ByteBuffer.allocate(bufferSize);
    }
    else {
      buffer = ByteBuffer.allocate(buffer.capacity() * 2);
    }
    return buffer;
  }
  public ByteBuffer handleBufferUnderflow(SSLEngine engine, ByteBuffer buffer) {
    if(engine.getSession().getPacketBufferSize() < buffer.limit()) {
      return buffer;
    }
    else {
      ByteBuffer replaceBuffer = enlargePacketBuffer(engine, buffer);
      buffer.flip();
      replaceBuffer.put(buffer);
      return replaceBuffer;
    }
  }

  public void read(SocketChannel socketChannel, SSLEngine engine) throws IOException {
    dataInNetBuffer.clear();
    int bytesRead = socketChannel.read(dataInNetBuffer);
    if(bytesRead > 0) {
      dataInNetBuffer.flip();
      while(dataInNetBuffer.hasRemaining()) {
        dataInAppBuffer.clear();
        SSLEngineResult result = engine.unwrap(dataInNetBuffer, dataInAppBuffer);
        switch(result.getStatus()) {
          case OK:
            dataInAppBuffer.flip();
            break;
          case BUFFER_OVERFLOW:
            dataInAppBuffer = enlargeApplicationBuffer(engine, dataInAppBuffer);
            break;
          case BUFFER_UNDERFLOW:
            dataInNetBuffer = handleBufferUnderflow(engine, dataInNetBuffer);
            break;
          case CLOSED:
            closeConnection(socketChannel, engine);
            return;
          default:
            throw new IllegalStateException("invalid SSL status: " + result.getStatus());
        }
      }
    }
    else if(bytesRead < 0) {
      handleEndOfStream(socketChannel, engine);
    }
  }

  public void handleEndOfStream(SocketChannel socketChannel, SSLEngine engine) throws IOException {
    try {
      engine.closeInbound();
    }
    catch (Exception e) {
      Log.e("server-poc-test", "Close inbound forced");
    }
    closeConnection(socketChannel, engine);
  }

  public void closeConnection(SocketChannel socketChannel, SSLEngine engine) throws IOException {
    try{
      serverKey.close();
    } catch (IOException e){
      e.printStackTrace();
    }

    try {
      trustedCert.close();
    } catch (IOException e){
      e.printStackTrace();
    }

    engine.closeOutbound();
    doHandshake(socketChannel, engine);
    socketChannel.close();
  }
}
