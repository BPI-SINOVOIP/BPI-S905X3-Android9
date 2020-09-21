# 3rd party instructions for using Cloud EPG feature of Live Channels

Partners can ask Live Channels to retrieve EPG data for their TV Input Service
using live channels

## Prerequisites

*   Updated agreement with Google
*   Oreo or patched Nougat

## Nougat

To use cloud epg with Nougat you will need the following changes.

### Patch TVProvider

To run in Nougat you must cherry pick [change
455319](https://android-review.googlesource.com/c/platform/packages/providers/TvProvider/+/455319)
to TV Provider.

### Customisation

Indicate TvProvider is patched by including the following in their TV
customization resource

```
<bool name="tvprovider_allows_system_inserts_to_program_table">true</bool>
```

See https://source.android.com/devices/tv/customize-tv-app

## **Input Setup**

During the input setup activity, the TIS will query the content provider for
lineups in a given postal code. The TIS then inserts a row to the inputs table
with input_id and lineup_id

On completion of the activity the TIS sets the extra data in the result to

*   `com.android.tv.extra.USE_CLOUD_EPG = true`
*   `TvInputInfo.EXTRA_INPUT_ID` with their input_id

This is used to tell Live Channels to immediately start the EPG fetch for that
input.

### Sample Input Setup code.

A complete sample is at
../third_party/samples/src/com/example/partnersupportsampletvinput

#### query lineup

```java
 private AsyncTask<Void, Void, List<Lineup>> createFetchLineupsTask() {
        return new AsyncTask<Void, Void, List<Lineup>>() {
            @Override
            protected List<Lineup> doInBackground(Void... params) {
                ContentResolver cr = getActivity().getContentResolver();

                List<Lineup> results = new ArrayList<>();
                Cursor cursor =
                        cr.query(
                                Uri.parse(
                                        "content://com.android.tv.data.epg/lineups/postal_code/"
                                                + ZIP),
                                null,
                                null,
                                null,
                                null);

                while (cursor.moveToNext()) {
                    String id = cursor.getString(0);
                    String name = cursor.getString(1);
                    String channels = cursor.getString(2);
                    results.add(new Lineup(id, name, channels));
                }

                return results;
            }

            @Override
            protected void onPostExecute(List<Lineup> lineups) {
                showLineups(lineups);
            }
        };
    }
```

#### Insert cloud_epg_input

```java
ContentValues values = new ContentValues();
values.put(EpgContract.EpgInputs.COLUMN_INPUT_ID, SampleTvInputService.INPUT_ID);
values.put(EpgContract.EpgInputs.COLUMN_LINEUP_ID, lineup.getId());
ContentResolver contentResolver = getActivity().getContentResolver();
EpgInput epgInput = EpgInputs.queryEpgInput(contentResolver, SampleTvInputService.INPUT_ID);
if (epgInput == null) {
    contentResolver.insert(EpgContract.EpgInputs.CONTENT_URI, values);
} else {
    values.put(EpgContract.EpgInputs.COLUMN_ID, epgInput.getId());
    EpgInputs.update(contentResolver, EpgInput.createEpgChannel(values));
}
```

#### Return use_cloud_epg

```java
Intent data = new Intent();
data.putExtra(TvInputInfo.EXTRA_INPUT_ID, inputId);
data.putExtra(com.android.tv.extra.USE_CLOUD_EPG, true);
setResult(Activity.RESULT_OK, data);
```
