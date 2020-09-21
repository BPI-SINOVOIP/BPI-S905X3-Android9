## 5.8\. Secure Media

If device implementations support secure video output and are capable of
supporting secure surfaces, they:

*    [C-1-1] MUST declare support for `Display.FLAG_SECURE`.

If device implementations declare support for `Display.FLAG_SECURE` and support
wireless display protocol, they:

*    [C-2-1] MUST secure the link with a cryptographically strong mechanism such
as HDCP 2.x or higher for the displays connected through wireless protocols
such as Miracast.

If device implementations declare support for `Display.FLAG_SECURE` and
support wired external display, they:

*    [C-3-1] MUST support HDCP 1.2 or higher for all wired external displays.

