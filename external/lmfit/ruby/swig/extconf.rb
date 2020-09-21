require 'mkmf'
$CFLAGS += " -I/usr/lib/ruby/1.8/i386-linux/"
$LDFLAGS += " /usr/local/lib/liblmmin.so"
create_makefile('lmfit')
