package com.google.android.car.kitchensink.notification;

import static android.security.KeyStore.getApplicationContext;

import android.annotation.Nullable;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Person;
import android.app.RemoteInput;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;

import com.google.android.car.kitchensink.KitchenSinkActivity;
import com.google.android.car.kitchensink.R;

/**
 * Test fragment that can send all sorts of notifications.
 */
public class NotificationFragment extends Fragment {
    private static final String CHANNEL_ID_1 = "kitchensink.channel1";
    private static final String CHANNEL_ID_2 = "kitchensink.channel2";
    private static final String CHANNEL_ID_3 = "kitchensink.channel3";
    private static final String CHANNEL_ID_4 = "kitchensink.channel4";
    private static final String CHANNEL_ID_5 = "kitchensink.channel5";
    private static final String CHANNEL_ID_6 = "kitchensink.channel6";

    @Override
    public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.notification_fragment, container, false);
        Button cancelAllButton = view.findViewById(R.id.cancel_all_button);
        Button importanceHighButton = view.findViewById(R.id.importance_high_button);
        Button importanceHighButton2 = view.findViewById(R.id.importance_high_button_2);
        Button importanceLowButton = view.findViewById(R.id.importance_low_button);
        Button importanceMinButton = view.findViewById(R.id.importance_min_button);
        Button importanceDefaultButton = view.findViewById(R.id.importance_default_button);
        Button ongoingButton = view.findViewById(R.id.ongoing_button);
        Button messageButton = view.findViewById(R.id.category_message_button);

        NotificationManager manager =
                (NotificationManager) getActivity().getSystemService(Context.NOTIFICATION_SERVICE);

        // cancel all button
        cancelAllButton.setOnClickListener(v -> manager.cancelAll());

        // importance high notifications
        NotificationChannel highImportanceChannel =
                new NotificationChannel(
                        CHANNEL_ID_1, "Importance High", NotificationManager.IMPORTANCE_HIGH);
        manager.createNotificationChannel(highImportanceChannel);

        importanceHighButton.setOnClickListener(v -> {

            Notification notification = new Notification.Builder(getActivity(), CHANNEL_ID_1)
                    .setContentTitle("Importance High")
                    .setContentText("blah")
                    .setSmallIcon(R.drawable.car_ic_mode)
                    .build();
            manager.notify(1, notification);
        });

        importanceHighButton2.setOnClickListener(v -> {
            Notification notification = new Notification.Builder(getActivity(), CHANNEL_ID_1)
                    .setContentTitle("Importance High 2")
                    .setContentText("blah blah blah")
                    .setSmallIcon(R.drawable.car_ic_mode)
                    .build();
            manager.notify(2, notification);
        });

        // importance default
        importanceDefaultButton.setOnClickListener(v -> {
            NotificationChannel channel =
                    new NotificationChannel(
                            CHANNEL_ID_3,
                            "Importance Default",
                            NotificationManager.IMPORTANCE_DEFAULT);
            manager.createNotificationChannel(channel);

            Notification notification = new Notification.Builder(getActivity(), CHANNEL_ID_3)
                    .setContentTitle("Importance Default")
                    .setSmallIcon(R.drawable.car_ic_mode)
                    .build();
            manager.notify(4, notification);
        });

        // importance low
        importanceLowButton.setOnClickListener(v -> {
            NotificationChannel channel =
                    new NotificationChannel(
                            CHANNEL_ID_4, "Importance Low", NotificationManager.IMPORTANCE_LOW);
            manager.createNotificationChannel(channel);

            Notification notification = new Notification.Builder(getActivity(), CHANNEL_ID_4)
                    .setContentTitle("Importance Low")
                    .setContentText("low low low")
                    .setSmallIcon(R.drawable.car_ic_mode)
                    .build();
            manager.notify(5, notification);
        });

        // importance min
        importanceMinButton.setOnClickListener(v -> {
            NotificationChannel channel =
                    new NotificationChannel(
                            CHANNEL_ID_5, "Importance Min", NotificationManager.IMPORTANCE_MIN);
            manager.createNotificationChannel(channel);

            Notification notification = new Notification.Builder(getActivity(), CHANNEL_ID_5)
                    .setContentTitle("Importance Min")
                    .setContentText("min min min")
                    .setSmallIcon(R.drawable.car_ic_mode)
                    .build();
            manager.notify(6, notification);
        });

        // ongoing
        ongoingButton.setOnClickListener(v -> {
            NotificationChannel channel =
                    new NotificationChannel(
                            CHANNEL_ID_6, "Ongoing", NotificationManager.IMPORTANCE_DEFAULT);
            manager.createNotificationChannel(channel);

            Notification notification = new Notification.Builder(getActivity(), CHANNEL_ID_6)
                    .setContentTitle("Playing music or something")
                    .setSmallIcon(R.drawable.car_ic_mode)
                    .setOngoing(true)
                    .build();
            manager.notify(7, notification);
        });

        // category message
        messageButton.setOnClickListener(v -> {
            NotificationChannel channel =
                    new NotificationChannel(
                            CHANNEL_ID_2, "Message", NotificationManager.IMPORTANCE_HIGH);
            manager.createNotificationChannel(channel);

            Intent intent = new Intent(getActivity(), KitchenSinkActivity.class);
            PendingIntent readIntent = PendingIntent.getActivity(getActivity(), 0, intent, 0);

            RemoteInput remoteInput = new RemoteInput.Builder("voice reply").build();
            PendingIntent replyIntent = PendingIntent.getBroadcast(getApplicationContext(),
                    12345,
                    intent,
                    PendingIntent.FLAG_UPDATE_CURRENT);

            Person personJohn = new Person.Builder().setName("John Doe").build();
            Person personJane = new Person.Builder().setName("Jane Roe").build();
            Notification.MessagingStyle messagingStyle =
                    new Notification.MessagingStyle(personJohn)
                            .setConversationTitle("Whassup")
                            .addHistoricMessage(
                                    new Notification.MessagingStyle.Message(
                                            "historic message",
                                            System.currentTimeMillis() - 3600,
                                            personJohn))
                            .addMessage(new Notification.MessagingStyle.Message(
                                    "message", System.currentTimeMillis(), personJane));

            Notification notification = new Notification.Builder(getActivity(), CHANNEL_ID_2)
                    .setContentTitle("Message from someone")
                    .setContentText("hi")
                    .setCategory(Notification.CATEGORY_MESSAGE)
                    .setSmallIcon(R.drawable.car_ic_mode)
                    .setStyle(messagingStyle)
                    .setAutoCancel(true)
                    .addAction(
                            new Notification.Action.Builder(null, "read", readIntent).build())
                    .addAction(
                            new Notification.Action.Builder(null, "reply", replyIntent)
                                    .addRemoteInput(remoteInput).build())
                    .extend(new Notification.CarExtender().setColor(R.color.car_red_500))
                    .build();
            manager.notify(3, notification);
        });

        return view;
    }
}
