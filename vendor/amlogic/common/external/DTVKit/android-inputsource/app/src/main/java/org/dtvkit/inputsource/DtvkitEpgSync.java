package org.dtvkit.inputsource;

import android.media.tv.TvContract;
import android.media.tv.TvContentRating;
import android.net.Uri;
import android.util.Log;

import org.dtvkit.companionlibrary.EpgSyncJobService;
import org.dtvkit.companionlibrary.model.Channel;
import org.dtvkit.companionlibrary.model.EventPeriod;
import org.dtvkit.companionlibrary.model.InternalProviderData;
import org.dtvkit.companionlibrary.model.Program;
import org.droidlogic.dtvkit.DtvkitGlueClient;

import org.json.JSONArray;
import org.json.JSONObject;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

public class DtvkitEpgSync extends EpgSyncJobService {
    private static final String TAG = "EpgSyncJobService";

    @Override
    public List<Channel> getChannels() {
        List<Channel> channels = new ArrayList<>();

        Log.i(TAG, "Get channels for epg sync");

        try {
            JSONObject obj = DtvkitGlueClient.getInstance().request("Dvb.getListOfServices", new JSONArray());

            Log.i(TAG, "getChannels=" + obj.toString());

            JSONArray services = obj.getJSONArray("data");

            for (int i = 0; i < services.length(); i++)
            {
                JSONObject service = services.getJSONObject(i);
                String uri = service.getString("uri");

                InternalProviderData data = new InternalProviderData();
                data.put("dvbUri", uri);
                data.put("hidden", service.getBoolean("hidden"));
                data.put("network_id", service.getInt("network_id"));
                data.put("frequency", service.getInt("freq"));
                JSONObject satellite = new JSONObject();
                satellite.put("satellite_info_name", service.getString("sate_name"));
                data.put("satellite_info", satellite.toString());
                JSONObject transponder = new JSONObject();
                String tranponderDisplay = service.getString("transponder");
                transponder.put("transponder_info_display_name", tranponderDisplay);
                if (tranponderDisplay != null) {
                    String[] splitTransponder = tranponderDisplay.split("/");
                    if (splitTransponder != null && splitTransponder.length == 3) {
                        transponder.put("transponder_info_satellite_name", service.getString("sate_name"));
                        transponder.put("transponder_info_frequency", splitTransponder[0]);
                        transponder.put("transponder_info_polarity", splitTransponder[1]);
                        transponder.put("transponder_info_symbol", splitTransponder[2]);
                    }
                }
                data.put("transponder_info", transponder.toString());
                data.put("video_pid", service.getInt("video_pid"));
                data.put("video_codec", service.getString("video_codec"));
                data.put("is_data", service.getBoolean("is_data"));
                data.put("channel_signal_type", service.getString("sig_name"));
                channels.add(new Channel.Builder()
                        .setDisplayName(service.getString("name"))
                        .setDisplayNumber(String.format(Locale.ENGLISH, "%d", service.getInt("lcn")))
                        .setServiceType(service.getBoolean("radio") ? TvContract.Channels.SERVICE_TYPE_AUDIO :
                                TvContract.Channels.SERVICE_TYPE_AUDIO_VIDEO)
                        .setOriginalNetworkId(Integer.parseInt(uri.substring(6, 10), 16))
                        .setTransportStreamId(Integer.parseInt(uri.substring(11, 15), 16))
                        .setServiceId(Integer.parseInt(uri.substring(16, 20), 16))
                        .setInternalProviderData(data)
                        .build());
            }

        } catch (Exception e) {
            Log.e(TAG, "getChannels Exception = " + e.getMessage());
        }

        return channels;
    }

    @Override
    public List<Program> getProgramsForChannel(Uri channelUri, Channel channel, long startMs, long endMs) {
        List<Program> programs = new ArrayList<>();
        long starttime, endtime;
        int parental_rating;
        try {
        String dvbUri = String.format("dvb://%04x.%04x.%04x", channel.getOriginalNetworkId(), channel.getTransportStreamId(), channel.getServiceId());

            Log.i(TAG, String.format("Get channel programs for epg sync. Uri %s, startMs %d, endMs %d",
                dvbUri, startMs, endMs));

            JSONArray args = new JSONArray();
            args.put(dvbUri); // uri
            args.put(startMs/1000);
            args.put(endMs/1000);
            JSONArray events = DtvkitGlueClient.getInstance().request("Dvb.getListOfEvents", args).getJSONArray("data");

            for (int i = 0; i < events.length(); i++)
            {
                JSONObject event = events.getJSONObject(i);

                InternalProviderData data = new InternalProviderData();
                data.put("dvbUri", dvbUri);
                starttime = event.getLong("startutc") * 1000;
                endtime = event.getLong("endutc") * 1000;
                parental_rating = event.getInt("rating");
                if (starttime >= endMs || endtime <= startMs) {
                    Log.i(TAG, "Skip##  startMs:endMs=["+startMs+":"+endMs+"]  event:startT:endT=["+starttime+":"+endtime+"]");
                    continue;
                }else{
                    Program pro = new Program.Builder()
                            .setChannelId(channel.getId())
                            .setTitle(event.getString("name"))
                            .setStartTimeUtcMillis(starttime)
                            .setEndTimeUtcMillis(endtime)
                            .setDescription(event.getString("description"))
                            .setCanonicalGenres(getGenres(event.getString("genre")))
                            .setInternalProviderData(data)
                            .setContentRatings(parental_rating == 0 ? null : parseParentalRatings(parental_rating, event.getString("name")))
                            .build();
                    programs.add(pro);
                }
            }
        } catch (Exception e) {
            Log.e(TAG, e.getMessage());
        }

        return programs;
    }

    @Override
    public List<Program> getAllProgramsForChannel(Uri channelUri, Channel channel) {
        List<Program> programs = new ArrayList<>();
        int parental_rating;

        try {
        String dvbUri = String.format("dvb://%04x.%04x.%04x", channel.getOriginalNetworkId(), channel.getTransportStreamId(), channel.getServiceId());

            Log.i(TAG, String.format("Get channel programs for epg sync. Uri %s", dvbUri));

            JSONArray args = new JSONArray();
            args.put(dvbUri); // uri
            //starttime\endtime use min and max.
            JSONArray events = DtvkitGlueClient.getInstance().request("Dvb.getListOfEvents", args).getJSONArray("data");

            for (int i = 0; i < events.length(); i++)
            {
                JSONObject event = events.getJSONObject(i);

                InternalProviderData data = new InternalProviderData();
                data.put("dvbUri", dvbUri);
                parental_rating = event.getInt("rating");
                Program pro = new Program.Builder()
                        .setChannelId(channel.getId())
                        .setTitle(event.getString("name"))
                        .setStartTimeUtcMillis(event.getLong("startutc") * 1000)
                        .setEndTimeUtcMillis(event.getLong("endutc") * 1000)
                        .setDescription(event.getString("description"))
                        .setLongDescription(event.getString("description_extern"))
                        .setCanonicalGenres(getGenres(event.getString("genre")))
                        .setInternalProviderData(data)
                        .setContentRatings(parental_rating == 0 ? null : parseParentalRatings(parental_rating, event.getString("name")))
                        .build();
                programs.add(pro);
                Log.i("cz_debug", "## get pro by event:"+ pro.toString()+ " ##");
            }
        } catch (Exception e) {
            Log.e(TAG, e.getMessage());
        }
        Log.i(TAG, "## programs["+ programs.size() +"] ##");

        return programs;
    }

    public List<Program> getNowNextProgramsForChannel(Uri channelUri, Channel channel) {
        List<Program> programs = new ArrayList<>();

        try {
            String dvbUri = String.format("dvb://%04x.%04x.%04x", channel.getOriginalNetworkId(), channel.getTransportStreamId(), channel.getServiceId());

            Log.i(TAG, "Get channel now next programs for epg sync. Uri " + dvbUri);

            JSONArray args = new JSONArray();
            args.put(dvbUri); // uri
            JSONObject events = DtvkitGlueClient.getInstance().request("Dvb.getNowNextEvents", args).getJSONObject("data");

            JSONObject now = events.optJSONObject("now");
            if (now != null) {
                InternalProviderData data = new InternalProviderData();
                data.put("dvbUri", dvbUri);
                programs.add(new Program.Builder()
                        .setChannelId(channel.getId())
                        .setTitle(now.getString("name"))
                        .setStartTimeUtcMillis(now.getLong("startutc") * 1000)
                        .setEndTimeUtcMillis(now.getLong("endutc") * 1000)
                        .setDescription(now.getString("description"))
                        .setCanonicalGenres(getGenres(now.getString("genre")))
                        .setInternalProviderData(data)
                        .build());
            }

            JSONObject next = events.optJSONObject("next");
            if (next != null) {
                InternalProviderData data = new InternalProviderData();
                data.put("dvbUri", dvbUri);
                programs.add(new Program.Builder()
                        .setChannelId(channel.getId())
                        .setTitle(next.getString("name"))
                        .setStartTimeUtcMillis(next.getLong("startutc") * 1000)
                        .setEndTimeUtcMillis(next.getLong("endutc") * 1000)
                        .setDescription(next.getString("description"))
                        .setCanonicalGenres(getGenres(next.getString("genre")))
                        .setInternalProviderData(data)
                        .build());
            }
        } catch (Exception e) {
            Log.e(TAG, e.getMessage());
        }

        return programs;
    }

    @Override
    public List<EventPeriod> getListOfUpdatedEventPeriods()
    {
        List<EventPeriod> eventPeriods = new ArrayList<>();

        Log.i(TAG, "getListOfUpdatedEventPeriods");

        try {
            JSONArray periods = DtvkitGlueClient.getInstance().request("Dvb.getListOfUpdatedEventPeriods", new JSONArray()).getJSONArray("data");

            Log.i(TAG, periods.toString());

            for (int i = 0; i < periods.length(); i++)
            {
                JSONObject period = periods.getJSONObject(i);

                eventPeriods.add(new EventPeriod(period.getString("uri"),period.getLong("startutc"),period.getLong("endutc")));
            }
        } catch (Exception e) {
            Log.e(TAG, e.getMessage());
        }

        return eventPeriods;
    }

    private String[] getGenres(String genre)
    {
        switch (genre)
        {
            case "movies":
                return new String[]{TvContract.Programs.Genres.MOVIES};
            case "news":
                return new String[]{TvContract.Programs.Genres.NEWS};
            case "entertainment":
                return new String[]{TvContract.Programs.Genres.ENTERTAINMENT};
            case "sport":
                return new String[]{TvContract.Programs.Genres.SPORTS};
            case "childrens":
                return new String[]{TvContract.Programs.Genres.FAMILY_KIDS};
            case "music":
                return new String[]{TvContract.Programs.Genres.MUSIC};
            case "arts":
                return new String[]{TvContract.Programs.Genres.ARTS};
            case "social":
                return new String[]{TvContract.Programs.Genres.LIFE_STYLE};
            case "education":
                return new String[]{TvContract.Programs.Genres.EDUCATION};
            case "leisure":
                return new String[]{TvContract.Programs.Genres.LIFE_STYLE};
            default:
                return new String[]{};
        }
    }

    private TvContentRating[] parseParentalRatings(int parentalRating, String title)
    {
        TvContentRating ratings_arry[];
        String ratingSystemDefinition = "DVB";
        String ratingDomain = "com.android.tv";
        String DVB_ContentRating[] = {"DVB_4", "DVB_5", "DVB_6", "DVB_7", "DVB_8", "DVB_9", "DVB_10", "DVB_11", "DVB_12", "DVB_13", "DVB_14", "DVB_15", "DVB_16", "DVB_17", "DVB_18"};

        ratings_arry = new TvContentRating[1];
        parentalRating += 3; //minimum age = rating + 3 years
        Log.d(TAG, "parseParentalRatings parentalRating:"+ parentalRating + ", title = " + title);
        if (parentalRating >= 4 && parentalRating <= 18) {
            TvContentRating r = TvContentRating.createRating(ratingDomain, ratingSystemDefinition, DVB_ContentRating[parentalRating-4], null);
            if (r != null) {
                ratings_arry[0] = r;
                Log.d(TAG, "parse ratings add rating:"+r.flattenToString()  + ", title = " + title);
            }
        }else {
            ratings_arry = null;
        }

        return ratings_arry;
    }
}
