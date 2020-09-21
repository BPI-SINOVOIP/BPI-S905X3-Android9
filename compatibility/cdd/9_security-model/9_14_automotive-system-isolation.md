## 9.14\. Automotive Vehicle System Isolation

Android Automotive devices are expected to exchange data with critical vehicle
subsystems by using the [vehicle HAL](http://source.android.com/devices/automotive.html)
to send and receive messages over vehicle networks such as CAN bus.

The data exchange can be secured by implementing security features below the
Android framework layers to prevent malicious or unintentional interaction with
these subsystems.

