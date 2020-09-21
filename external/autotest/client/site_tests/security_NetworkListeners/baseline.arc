# These first entries are due to autotest noise. As you maintain this
# list, consider offsetting it with a blacklist in
# ensure_no_nonrelease_files.config.
#
# On ARC++ sshl-fork listens on port 22 and selects between adb and
# sshd
sshd *:2222
adb 127.0.0.1:5037
sslh-fork *:ssh
