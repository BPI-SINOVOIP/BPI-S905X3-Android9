set(PULSEAUDIO_FOUND TRUE)

set(PULSEAUDIO_VERSION_MAJOR 1)
set(PULSEAUDIO_VERSION_MINOR 1)
set(PULSEAUDIO_VERSION 1.1)
set(PULSEAUDIO_VERSION_STRING "1.1")

find_path(PULSEAUDIO_INCLUDE_DIR pulse/pulseaudio.h HINTS "/usr/include")
find_library(PULSEAUDIO_LIBRARY NAMES pulse libpulse HINTS "/usr/lib/x86_64-linux-gnu")
find_library(PULSEAUDIO_MAINLOOP_LIBRARY NAMES pulse-mainloop-glib libpulse-mainloop-glib HINTS "/usr/lib/x86_64-linux-gnu")
