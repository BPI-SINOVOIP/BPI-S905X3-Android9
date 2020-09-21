This folder contains tools to be used in the testing lab.

NOTE: The config.json file in this folder contains random values - for the full
config file, look at the corresponding folder in google3

Python dependencies:
  - subprocess32
  - shellescape
  - psutil
  - IPy

Metrics that can be gathered, listed by name of file and then key to response
dict:

* adb_hash:
    env: whether $ADB_VENDOR_KEYS is set (bool)
    hash: hash of keys in $ADB_VENDOR_KEYS (string)
* cpu:
    cpu: list of CPU core percents (float)
* disk:
    total: total space in 1k blocks (int)
    used: total used in 1k blocks (int)
    avail: total available in 1k blocks (int)
    percent_used: percentage space used (0-100) (int)
* name:
    name: system's hostname(string)
* network:
    connected: whether the network is connected (list of bools)
* num_users:
    num_users: number of users (int)
* process_time:
    adb_processes, fastboot_processes: a list of (PID, serialnumber)
                tuples
    num_adb_processes, num_fastboot_processes: the number of tuples
                in the previous lists
* ram:
    total: total physical RAM available in KB (int)
    used: total RAM used by system in KB (int)
    free: total RAM free for new process in KB (int)
    buffers: total RAM buffered by different applications in KB
    cached: total RAM for caching of data in KB
* read:
    cached_read_rate: cached reads in MB/sec (float)
    buffered_read_rate: buffered disk reads in MB/sec (float)
* system_load:
    load_avg_1_min: system load average for past 1 min (float)
    load_avg_5_min: system load average for past 5 min (float)
    load_avg_15_min: system load average for past 15 min (float)
* time:
    date_time: system date and time (string)
* time_sync:
    is_synchronized: whether NTP synchronized (bool)
* uptime:
    time_seconds: uptime in seconds (float)
* usb:
    devices: a list of Device objects, each with name of device, number of bytes
    transferred, and the usb bus number/device id.
* verify:
    unauthorized: list of phone sn's that are unauthorized
    offline: list of phone sn's that are offline
    recovery: list of phone sn's that are in recovery mode
    question: list of phone sn's in ??? mode
    device: list of phone sn's that are in device mode
    total: total number of offline, recovery, question or unauthorized devices
* version:
    fastboot_version: which version of fastboot (string)
    adb_version: which version of adb (string)
    python_version: which version of python (string)
    kernel_version: which version of kernel (string)
* zombie:
    adb_zombies, fastboot_zombies, other_zombies: lists of
                (PID, serial number) tuples
    num_adb_zombies, num_fastboot_zombies, num_other_zombies: int
