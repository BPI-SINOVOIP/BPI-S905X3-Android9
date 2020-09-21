// These are some test snippets
//
// [START snip1]
import android.support.v4.app.NotificationCompat;
import android.support.v4.app.NotificationManagerCompat;
import android.support.v4.app.NotificationCompat.WearableExtender;
// [END snip1]


// [START snip2]
int notificationId = 001;
The channel ID of the notification.
String id = "my_channel_01";
// Build intent for notification content
Intent viewIntent = new Intent(this, ViewEventActivity.class);
viewIntent.putExtra(EXTRA_EVENT_ID, eventId);
PendingIntent viewPendingIntent =
        PendingIntent.getActivity(this, 0, viewIntent, 0);

        // Notification channel ID is ignored for Android 7.1.1
        // (API level 25) and lower.
        NotificationCompat.Builder notificationBuilder =
            new NotificationCompat.Builder(this, id)
                    .setSmallIcon(R.drawable.ic_event)
                            .setContentTitle(eventTitle)
                                    .setContentText(eventLocation)
                                           .setContentIntent(viewPendingIntent);
// [END snip2]
