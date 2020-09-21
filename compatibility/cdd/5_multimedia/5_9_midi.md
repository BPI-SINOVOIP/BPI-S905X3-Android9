## 5.9\. Musical Instrument Digital Interface (MIDI)

If device implementations report support for feature `android.software.midi`
via the [`android.content.pm.PackageManager`](
http://developer.android.com/reference/android/content/pm/PackageManager.html)
class, they:

*    [C-1-1] MUST support MIDI over _all_ MIDI-capable hardware transports for
which they provide generic non-MIDI connectivity, where such transports are:

    *   USB host mode, [section 7.7](#7_7_USB)
    *   USB peripheral mode, [section 7.7](#7_7_USB)
    *   MIDI over Bluetooth LE acting in central role, [section 7.4.3](#7_4_3_bluetooth)

*    [C-1-2] MUST support the inter-app MIDI software transport
(virtual MIDI devices)
