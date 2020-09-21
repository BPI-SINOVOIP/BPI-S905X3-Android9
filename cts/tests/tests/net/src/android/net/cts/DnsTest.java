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

package android.net.cts;

import android.content.Context;
import android.content.pm.PackageManager;
import android.net.ConnectivityManager;
import android.net.ConnectivityManager.NetworkCallback;
import android.net.LinkProperties;
import android.net.Network;
import android.net.NetworkInfo;
import android.os.SystemClock;
import android.test.AndroidTestCase;
import android.util.Log;

import java.net.Inet4Address;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public class DnsTest extends AndroidTestCase {

    static {
        System.loadLibrary("nativedns_jni");
    }

    private static final boolean DBG = false;
    private static final String TAG = "DnsTest";
    private static final String PROXY_NETWORK_TYPE = "PROXY";

    private ConnectivityManager mCm;

    public void setUp() {
        mCm = getContext().getSystemService(ConnectivityManager.class);
    }

    /**
     * @return true on success
     */
    private static native boolean testNativeDns();

    /**
     * Verify:
     * DNS works - forwards and backwards, giving ipv4 and ipv6
     * Test that DNS work on v4 and v6 networks
     * Test Native dns calls (4)
     * Todo:
     * Cache is flushed when we change networks
     * have per-network caches
     * No cache when there's no network
     * Perf - measure size of first and second tier caches and their effect
     * Assert requires network permission
     */
    public void testDnsWorks() throws Exception {
        ensureIpv6Connectivity();

        InetAddress addrs[] = {};
        try {
            addrs = InetAddress.getAllByName("www.google.com");
        } catch (UnknownHostException e) {}
        assertTrue("[RERUN] DNS could not resolve www.google.com. Check internet connection",
                addrs.length != 0);
        boolean foundV4 = false, foundV6 = false;
        for (InetAddress addr : addrs) {
            if (addr instanceof Inet4Address) foundV4 = true;
            else if (addr instanceof Inet6Address) foundV6 = true;
            if (DBG) Log.e(TAG, "www.google.com gave " + addr.toString());
        }

        // We should have at least one of the addresses to connect!
        assertTrue("www.google.com must have IPv4 and/or IPv6 address", foundV4 || foundV6);

        // Skip the rest of the test if the active network for watch is PROXY.
        // TODO: Check NetworkInfo type in addition to type name once ag/601257 is merged.
        if (getContext().getPackageManager().hasSystemFeature(PackageManager.FEATURE_WATCH)
                && activeNetworkInfoIsProxy()) {
            Log.i(TAG, "Skipping test because the active network type name is PROXY.");
            return;
        }

        // Clear test state so we don't get confused with the previous results.
        addrs = new InetAddress[0];
        foundV4 = foundV6 = false;
        try {
            addrs = InetAddress.getAllByName("ipv6.google.com");
        } catch (UnknownHostException e) {}
        String msg =
            "[RERUN] DNS could not resolve ipv6.google.com, check the network supports IPv6. lp=" +
            mCm.getActiveLinkProperties();
        assertTrue(msg, addrs.length != 0);
        for (InetAddress addr : addrs) {
            msg = "[RERUN] ipv6.google.com returned IPv4 address: " + addr.getHostAddress() +
                    ", check your network's DNS server. lp=" + mCm.getActiveLinkProperties();
            assertFalse (msg, addr instanceof Inet4Address);
            foundV6 |= (addr instanceof Inet6Address);
            if (DBG) Log.e(TAG, "ipv6.google.com gave " + addr.toString());
        }

        assertTrue(foundV6);

        assertTrue(testNativeDns());
    }

    private static final String[] URLS = { "www.google.com", "ipv6.google.com", "www.yahoo.com",
            "facebook.com", "youtube.com", "blogspot.com", "baidu.com", "wikipedia.org",
// live.com fails rev lookup.
            "twitter.com", "qq.com", "msn.com", "yahoo.co.jp", "linkedin.com",
            "taobao.com", "google.co.in", "sina.com.cn", "amazon.com", "wordpress.com",
            "google.co.uk", "ebay.com", "yandex.ru", "163.com", "google.co.jp", "google.fr",
            "microsoft.com", "paypal.com", "google.com.br", "flickr.com",
            "mail.ru", "craigslist.org", "fc2.com", "google.it",
// "apple.com", fails rev lookup
            "google.es",
            "imdb.com", "google.ru", "soho.com", "bbc.co.uk", "vkontakte.ru", "ask.com",
            "tumblr.com", "weibo.com", "go.com", "xvideos.com", "livejasmin.com", "cnn.com",
            "youku.com", "blogspot.com", "soso.com", "google.ca", "aol.com", "tudou.com",
            "xhamster.com", "megaupload.com", "ifeng.com", "zedo.com", "mediafire.com", "ameblo.jp",
            "pornhub.com", "google.co.id", "godaddy.com", "adobe.com", "rakuten.co.jp", "about.com",
            "espn.go.com", "4shared.com", "alibaba.com","ebay.de", "yieldmanager.com",
            "wordpress.org", "livejournal.com", "google.com.tr", "google.com.mx", "renren.com",
           "livedoor.com", "google.com.au", "youporn.com", "uol.com.br", "cnet.com", "conduit.com",
            "google.pl", "myspace.com", "nytimes.com", "ebay.co.uk", "chinaz.com", "hao123.com",
            "thepiratebay.org", "doubleclick.com", "alipay.com", "netflix.com", "cnzz.com",
            "huffingtonpost.com", "twitpic.com", "weather.com", "babylon.com", "amazon.de",
            "dailymotion.com", "orkut.com", "orkut.com.br", "google.com.sa", "odnoklassniki.ru",
            "amazon.co.jp", "google.nl", "goo.ne.jp", "stumbleupon.com", "tube8.com", "tmall.com",
            "imgur.com", "globo.com", "secureserver.net", "fileserve.com", "tianya.cn", "badoo.com",
            "ehow.com", "photobucket.com", "imageshack.us", "xnxx.com", "deviantart.com",
            "filestube.com", "addthis.com", "douban.com", "vimeo.com", "sogou.com",
            "stackoverflow.com", "reddit.com", "dailymail.co.uk", "redtube.com", "megavideo.com",
            "taringa.net", "pengyou.com", "amazon.co.uk", "fbcdn.net", "aweber.com", "spiegel.de",
            "rapidshare.com", "mixi.jp", "360buy.com", "google.cn", "digg.com", "answers.com",
            "bit.ly", "indiatimes.com", "skype.com", "yfrog.com", "optmd.com", "google.com.eg",
            "google.com.pk", "58.com", "hotfile.com", "google.co.th",
            "bankofamerica.com", "sourceforge.net", "maktoob.com", "warriorforum.com", "rediff.com",
            "google.co.za", "56.com", "torrentz.eu", "clicksor.com", "avg.com",
            "download.com", "ku6.com", "statcounter.com", "foxnews.com", "google.com.ar",
            "nicovideo.jp", "reference.com", "liveinternet.ru", "ucoz.ru", "xinhuanet.com",
            "xtendmedia.com", "naver.com", "youjizz.com", "domaintools.com", "sparkstudios.com",
            "rambler.ru", "scribd.com", "kaixin001.com", "mashable.com", "adultfirendfinder.com",
            "files.wordpress.com", "guardian.co.uk", "bild.de", "yelp.com", "wikimedia.org",
            "chase.com", "onet.pl", "ameba.jp", "pconline.com.cn", "free.fr", "etsy.com",
            "typepad.com", "youdao.com", "megaclick.com", "digitalpoint.com", "blogfa.com",
            "salesforce.com", "adf.ly", "ganji.com", "wikia.com", "archive.org", "terra.com.br",
            "w3schools.com", "ezinearticles.com", "wjs.com", "google.com.my", "clickbank.com",
            "squidoo.com", "hulu.com", "repubblica.it", "google.be", "allegro.pl", "comcast.net",
            "narod.ru", "zol.com.cn", "orange.fr", "soufun.com", "hatena.ne.jp", "google.gr",
            "in.com", "techcrunch.com", "orkut.co.in", "xunlei.com",
            "reuters.com", "google.com.vn", "hostgator.com", "kaskus.us", "espncricinfo.com",
            "hootsuite.com", "qiyi.com", "gmx.net", "xing.com", "php.net", "soku.com", "web.de",
            "libero.it", "groupon.com", "51.la", "slideshare.net", "booking.com", "seesaa.net",
            "126.com", "telegraph.co.uk", "wretch.cc", "twimg.com", "rutracker.org", "angege.com",
            "nba.com", "dell.com", "leboncoin.fr", "people.com", "google.com.tw", "walmart.com",
            "daum.net", "2ch.net", "constantcontact.com", "nifty.com", "mywebsearch.com",
            "tripadvisor.com", "google.se", "paipai.com", "google.com.ua", "ning.com", "hp.com",
            "google.at", "joomla.org", "icio.us", "hudong.com", "csdn.net", "getfirebug.com",
            "ups.com", "cj.com", "google.ch", "camzap.com", "wordreference.com", "tagged.com",
            "wp.pl", "mozilla.com", "google.ru", "usps.com", "china.com", "themeforest.net",
            "search-results.com", "tribalfusion.com", "thefreedictionary.com", "isohunt.com",
            "linkwithin.com", "cam4.com", "plentyoffish.com", "wellsfargo.com", "metacafe.com",
            "depositfiles.com", "freelancer.com", "opendns.com", "homeway.com", "engadget.com",
            "10086.cn", "360.cn", "marca.com", "dropbox.com", "ign.com", "match.com", "google.pt",
            "facemoods.com", "hardsextube.com", "google.com.ph", "lockerz.com", "istockphoto.com",
            "partypoker.com", "netlog.com", "outbrain.com", "elpais.com", "fiverr.com",
            "biglobe.ne.jp", "corriere.it", "love21cn.com", "yesky.com", "spankwire.com",
            "ig.com.br", "imagevenue.com", "hubpages.com", "google.co.ve"};

// TODO - this works, but is slow and cts doesn't do anything with the result.
// Maybe require a min performance, a min cache size (detectable) and/or move
// to perf testing
    private static final int LOOKUP_COUNT_GOAL = URLS.length;
    public void skiptestDnsPerf() {
        ArrayList<String> results = new ArrayList<String>();
        int failures = 0;
        try {
            for (int numberOfUrls = URLS.length; numberOfUrls > 0; numberOfUrls--) {
                failures = 0;
                int iterationLimit = LOOKUP_COUNT_GOAL / numberOfUrls;
                long startTime = SystemClock.elapsedRealtimeNanos();
                for (int iteration = 0; iteration < iterationLimit; iteration++) {
                    for (int urlIndex = 0; urlIndex < numberOfUrls; urlIndex++) {
                        try {
                            InetAddress addr = InetAddress.getByName(URLS[urlIndex]);
                        } catch (UnknownHostException e) {
                            Log.e(TAG, "failed first lookup of " + URLS[urlIndex]);
                            failures++;
                            try {
                                InetAddress addr = InetAddress.getByName(URLS[urlIndex]);
                            } catch (UnknownHostException ee) {
                                failures++;
                                Log.e(TAG, "failed SECOND lookup of " + URLS[urlIndex]);
                            }
                        }
                    }
                }
                long endTime = SystemClock.elapsedRealtimeNanos();
                float nsPer = ((float)(endTime-startTime) / iterationLimit) / numberOfUrls/ 1000;
                String thisResult = new String("getByName for " + numberOfUrls + " took " +
                        (endTime - startTime)/1000 + "(" + nsPer + ") with " +
                        failures + " failures\n");
                Log.d(TAG, thisResult);
                results.add(thisResult);
            }
            // build up a list of addresses
            ArrayList<byte[]> addressList = new ArrayList<byte[]>();
            for (String url : URLS) {
                try {
                    InetAddress addr = InetAddress.getByName(url);
                    addressList.add(addr.getAddress());
                } catch (UnknownHostException e) {
                    Log.e(TAG, "Exception making reverseDNS list: " + e.toString());
                }
            }
            for (int numberOfAddrs = addressList.size(); numberOfAddrs > 0; numberOfAddrs--) {
                int iterationLimit = LOOKUP_COUNT_GOAL / numberOfAddrs;
                failures = 0;
                long startTime = SystemClock.elapsedRealtimeNanos();
                for (int iteration = 0; iteration < iterationLimit; iteration++) {
                    for (int addrIndex = 0; addrIndex < numberOfAddrs; addrIndex++) {
                        try {
                            InetAddress addr = InetAddress.getByAddress(addressList.get(addrIndex));
                            String hostname = addr.getHostName();
                        } catch (UnknownHostException e) {
                            failures++;
                            Log.e(TAG, "Failure doing reverse DNS lookup: " + e.toString());
                            try {
                                InetAddress addr =
                                        InetAddress.getByAddress(addressList.get(addrIndex));
                                String hostname = addr.getHostName();

                            } catch (UnknownHostException ee) {
                                failures++;
                                Log.e(TAG, "Failure doing SECOND reverse DNS lookup: " +
                                        ee.toString());
                            }
                        }
                    }
                }
                long endTime = SystemClock.elapsedRealtimeNanos();
                float nsPer = ((endTime-startTime) / iterationLimit) / numberOfAddrs / 1000;
                String thisResult = new String("getHostName for " + numberOfAddrs + " took " +
                        (endTime - startTime)/1000 + "(" + nsPer + ") with " +
                        failures + " failures\n");
                Log.d(TAG, thisResult);
                results.add(thisResult);
            }
            for (String result : results) Log.d(TAG, result);

            InetAddress exit = InetAddress.getByName("exitrightnow.com");
            Log.e(TAG, " exit address= "+exit.toString());

        } catch (Exception e) {
            Log.e(TAG, "bad URL in testDnsPerf: " + e.toString());
        }
    }

    private boolean activeNetworkInfoIsProxy() {
        NetworkInfo info = mCm.getActiveNetworkInfo();
        if (PROXY_NETWORK_TYPE.equals(info.getTypeName())) {
            return true;
        }

        return false;
    }

    private void ensureIpv6Connectivity() throws InterruptedException {
        CountDownLatch latch = new CountDownLatch(1);
        final int TIMEOUT_MS = 5_000;

        final NetworkCallback callback = new NetworkCallback() {
            @Override
            public void onLinkPropertiesChanged(Network network, LinkProperties lp) {
                if (lp.hasGlobalIPv6Address()) {
                    latch.countDown();
                }
            }
        };
        mCm.registerDefaultNetworkCallback(callback);

        String msg = "Default network did not provide IPv6 connectivity after " + TIMEOUT_MS
                + "ms. Please connect to an IPv6-capable network. lp="
                + mCm.getActiveLinkProperties();
        try {
            assertTrue(msg, latch.await(TIMEOUT_MS, TimeUnit.MILLISECONDS));
        } finally {
            mCm.unregisterNetworkCallback(callback);
        }
    }
}
