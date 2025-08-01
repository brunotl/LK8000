/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

package org.LK8000;

import static android.content.pm.ServiceInfo.FOREGROUND_SERVICE_TYPE_CONNECTED_DEVICE;
import static android.content.pm.ServiceInfo.FOREGROUND_SERVICE_TYPE_LOCATION;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Build;
import android.os.IBinder;

import androidx.core.app.NotificationCompat;
import androidx.core.app.ServiceCompat;

/**
 * All this Service implementation does is put itself in foreground.
 * It shall reduce the risk of getting killed by the Android Activity
 * Manager, because foreground services will only be killed under
 * extreme pressure.  Since this services runs in-process, Android
 * will also not terminate our main Activity.
 *
 * LK8000 needs high reliability, because it usually runs in
 * foreground as the only active application and should not be killed
 * by an incoming phone call.
 *
 * This is bad style for general-purpose Android apps, don't imitate
 * unless you have an excuse as good as ours ;-)
 */
public class MyService extends Service {
  private static final String TAG = "LK8000";

  static final int intent_flags = getIntentFlags();
  static final String CHANNEL_ID = BuildConfig.APPLICATION_ID + "_NotificationChannel";

  static int getIntentFlags() {
    int flags = PendingIntent.FLAG_UPDATE_CURRENT;
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
      flags |= PendingIntent.FLAG_IMMUTABLE;
    }
    return flags;
  }

  @Override
  public void onCreate() {
    super.onCreate();

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      NotificationManager manager =  (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
      // Support for Android Oreo: Notification Channels
      NotificationChannel channel = manager.getNotificationChannel(CHANNEL_ID);
      if (channel == null) {
        channel = new NotificationChannel(
                CHANNEL_ID,
                "LK8000",
                NotificationManager.IMPORTANCE_LOW);

        manager.createNotificationChannel(channel);
      }
    }
  }

  @Override public int onStartCommand(Intent intent, int flags, int startId) {

    /* notification is required to start foreground service */
    Intent notificationIntent = new Intent(this, LK8000.class);

    PendingIntent contentIntent =
            PendingIntent.getActivity(this, 0, notificationIntent, intent_flags);

    final Bitmap largeIcon = BitmapFactory.decodeResource(getResources(), getApplicationInfo().icon);

    Notification notification =
            new NotificationCompat.Builder(this, CHANNEL_ID)
                    .setContentIntent(contentIntent)
                    .setSmallIcon(R.drawable.notification_icon)
                    .setLargeIcon(largeIcon)
                    .setContentTitle("LK8000 is running")
                    .setContentText("Touch to open")
                    .setWhen(System.currentTimeMillis())
                    .setShowWhen(false)
                    .setOngoing(true)
                    .setOnlyAlertOnce(true)
                    .setPriority(NotificationCompat.PRIORITY_HIGH)
                    .build();

    try {
      ServiceCompat.startForeground(this, 1, notification,
              FOREGROUND_SERVICE_TYPE_LOCATION | FOREGROUND_SERVICE_TYPE_CONNECTED_DEVICE);
    }
    catch(Exception e) {
      e.printStackTrace();
    }
    /* We want this service to continue running until it is explicitly
       stopped, so return sticky */
    return START_STICKY;
  }

  @Override public void onDestroy() {

    stopForeground(true);

    super.onDestroy();
  }

  @Override public IBinder onBind(Intent intent) {
    return null;
  }
}
