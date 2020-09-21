CC=gcc
CFLAGS=-Wall -Werror -fPIC -I/usr/include/openssl
LIBS=-lcrypto -lssl

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $< $(LIBS)

OBJS=ec_helper_native.o

ec_helper_native: $(OBJS)
	$(CC) -shared -o ec_helper_native.so $(OBJS) $(LIBS)

clean:
	rm *.o ec_helper_native.so
