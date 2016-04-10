OS ?= LINUX
#OS ?= WINDOWS
#OS ?= MACOSX
#OS ?= BSD

ifeq ($(OS), LINUX)  # also works on FreeBSD
CC ?= gcc
CFLAGS ?= -O2 -Wall
SecureLoaderCli: SecureLoaderCli.c
	#$(CC) $(CFLAGS) -s -DUSE_LIBUSB -o SecureLoaderCli SecureLoaderCli.c ../AES/aes.c -lusb
	$(CC) $(CFLAGS) -s -DUSE_HIDAPI -o SecureLoaderCli SecureLoaderCli.c ../AES/aes.c -I/usr/include/hidapi/ -lhidapi-libusb


else ifeq ($(OS), WINDOWS)
CC = i586-mingw32msvc-gcc
CFLAGS ?= -O2 -Wall
LDLIB = -lsetupapi -lhid
SecureLoaderCli.exe: SecureLoaderCli.c
	$(CC) $(CFLAGS) -s -DUSE_WIN32 -o SecureLoaderCli.exe SecureLoaderCli.c $(LDLIB)


else ifeq ($(OS), MACOSX)
CC ?= gcc
SDK ?= /Developer/SDKs/MacOSX10.5.sdk
CFLAGS ?= -O2 -Wall
SecureLoaderCli: SecureLoaderCli.c
	$(CC) $(CFLAGS) -DUSE_APPLE_IOKIT -isysroot $(SDK) -o SecureLoaderCli SecureLoaderCli.c -Wl,-syslibroot,$(SDK) -framework IOKit -framework CoreFoundation


else ifeq ($(OS), BSD)  # works on NetBSD and OpenBSD
CC ?= gcct
CFLAGS ?= -O2 -Wall
SecureLoaderCli: SecureLoaderCli.c
	$(CC) $(CFLAGS) -s -DUSE_UHID -o SecureLoaderCli SecureLoaderCli.c


endif


clean:
	rm -f SecureLoaderCli SecureLoaderCli.exe
