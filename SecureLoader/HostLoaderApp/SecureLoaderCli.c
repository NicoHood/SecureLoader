
#define SPM_PAGESIZE 128
#define VENDOR_ID 0x7777
#define PRODUCT_ID 0x7777
#define CODE_SIZE (32 * 1024)
#define BOOTLOADER_SIZE (4 * 1024)

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include "../AES/aes256.h"
#include "../Protocol.h"

// Bootloader API
void authenticate(uint8_t* signkey);
void writeData(uint8_t* signkey);
void changeKey(uint8_t* oldkey, uint8_t* newkey);
void verifyData(void);

// USB Access Functions
void SecureLoader_init(void);
void SecureLoader_exit(void);
int SecureLoader_open(void);
int SecureLoader_write(void *buf, int len, double timeout);
int SecureLoader_read(void *buf, int len, double timeout);
void SecureLoader_close(void);

// Intel Hex File Functions
int read_intel_hex(const char *filename);
int ihex_bytes_within_range(int begin, int end);
void ihex_get_data(int addr, int len, unsigned char *bytes);

// Misc stuff
int printf_verbose(const char *format, ...);
int printf_high_verbose(const char *format, ...);
void hexdump(uint8_t * data, size_t len);
void delay(double seconds);
void die(const char *str, ...);
void parse_options(int argc, char **argv);

// options (from user via command line args)
int wait_for_device_to_appear = 0;
int reboot_after_programming = 1;
int verbose = 0;
const char *filename=NULL;


// AES
aes256_ctx_t ctx;

static uint8_t key[32] = {
    0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
    0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
    0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7,
    0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4
};

static uint8_t key2[32] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
};

void usage(void)
{
    fprintf(stderr, "Usage: hid_bootloader_cli [-w] [-h] [-n] [-v] <file.hex>\n");
    fprintf(stderr, "\t-w  : Wait for device to appear\n");
    fprintf(stderr, "\t-n  : No reboot after programming\n");
    fprintf(stderr, "\t-v  : Verbose output\n");
    fprintf(stderr, "\t-vv : High verbose output\n");
    SecureLoader_exit();
    exit(1);
}


/****************************************************************/
/*                                                              */
/*                       Main Program                           */
/*                                                              */
/****************************************************************/

int main(int argc, char **argv)
{
    int num, waited=0;

    // Init USB context
    SecureLoader_init();

    // parse command line arguments
    parse_options(argc, argv);
    if (!filename) {
        fprintf(stderr, "Filename must be specified\n\n");
        usage();
    }
    printf_verbose("SecureLoader Loader, Command Line, Version 1.0\n");

    // Read the intel hex file
    // This is done first so any error is reported before using USB
    num = read_intel_hex(filename);
    if (num < 0) die("Error reading Intel hex file \"%s\"", filename);
    printf_verbose("Read \"%s\": %d bytes, %.1f%% usage\n",
        filename, num, (double)num / (double)CODE_SIZE * 100.0);

    // Open the USB device
    while (!SecureLoader_open()) {
        if (!wait_for_device_to_appear) die("Unable to open device\n");
        if (!waited) {
            printf_verbose("Waiting for device...\n");
            waited = true;
        }
        delay(0.25);
    }
    printf_verbose("Found Bootloader\n");

    // if we waited for the device, read the hex file again
    // perhaps it changed while we were waiting?
    if (waited) {
        num = read_intel_hex(filename);
        if (num < 0) die("Error reading intel hex file \"%s\"", filename);
        printf_verbose("Read \"%s\": %d bytes, %.1f%% usage\n",
             filename, num, (double)num / (double)CODE_SIZE * 100.0);
    }

    fflush(stdout);

    // TODO verify via authentification package?
    authenticate(key);
    changeKey(key, key2);
    authenticate(key2);
    writeData(key2);
    verifyData();

    authenticate(key2);
    writeData(key2);
    verifyData();
    changeKey(key2, key);
    authenticate(key);

    // reboot to the user's new code
    if (reboot_after_programming) {
        printf_verbose("Booting\n");
        SetFlashPage_t SetFlashPage = { .PageAddress = COMMAND_STARTAPPLICATION };
        SecureLoader_write(SetFlashPage.raw, sizeof(SetFlashPage), 1);
    }
    SecureLoader_close();
    SecureLoader_exit();
    return 0;
}

void authenticate(uint8_t* signkey)
{
    printf_verbose("Authenticating Secureloader\n");

    // Get the data ready
    authenticateBootloader_t authenticateBootloader;
    uint8_t challenge[AES256_CBC_LENGTH];
    for(int i = 0; i < sizeof(challenge); i++){
        challenge[i] = rand();
    }
    if(verbose > 1)
    {
        printf_high_verbose("Seed:\n");
        hexdump(challenge, sizeof(challenge));
    }
    memcpy(authenticateBootloader.data.challenge, challenge, sizeof(authenticateBootloader.data.challenge));

    // Initialize key schedule inside CTX
    aes256_init(signkey, &ctx);

    // Encrypt the data
    aes256CbcEncrypt(&ctx, authenticateBootloader.IV, sizeof(authenticateBootloader.data.challenge));

    // Calculate and save CBC-MAC
    aes256CbcMacCalculate(&ctx, authenticateBootloader.data.raw, sizeof(authenticateBootloader.data.challenge));

    // Write data to the AVR
    int r = SecureLoader_write(authenticateBootloader.data.raw, sizeof(authenticateBootloader.data), 1);
    if (!r) die("Error writing to SecureLoader\n");

    // Get data from AVR
    r = SecureLoader_read(authenticateBootloader.data.challenge, sizeof(authenticateBootloader.data.challenge), 1);
    if (!r) die("Error reading SecureLoader\n");

    // Compare the data
    if(memcmp(authenticateBootloader.data.challenge, challenge, sizeof(challenge))){
        printf_verbose("Expected:\n");
        hexdump(challenge, sizeof(challenge));
        printf_verbose("Received:\n");
        hexdump(authenticateBootloader.data.challenge, sizeof(authenticateBootloader.data.challenge));
        die("Error authentification mismatch\n");
    }
}

void writeData(uint8_t* signkey)
{
    printf_verbose("Programming\n");
    for (int addr = 0; addr < CODE_SIZE; addr += SPM_PAGESIZE) {
        printf_high_verbose("\n%d", addr);
        if (addr > 0 && !ihex_bytes_within_range(addr, addr + SPM_PAGESIZE - 1)) {
            // don't waste time on blocks that are unused,
            // but always do the first one to erase the chip
            printf_high_verbose(" Empty Block!");
            continue;
        }
        if(addr >= (CODE_SIZE - BOOTLOADER_SIZE)){
            printf_verbose("Warning: Skipping BootLoader Section!");
            continue;
        }

        if(verbose == 1){
            printf_verbose(".");
        }

        // Special case for large flash MCUs
        if (CODE_SIZE > 0xFFFF)
        {
            addr >>= 8;
        }

        // Create a new flash page data structure
        ProgrammFlashPage_t ProgrammFlashPage;

        // Load the actual flash page address and data
        ProgrammFlashPage.PageAddress = addr;
        ihex_get_data(addr, sizeof(ProgrammFlashPage.PageDataBytes), ProgrammFlashPage.PageDataBytes);

        // Save key and initialization vector inside context
        aes256_init(signkey, &ctx);

        // Calculate and save CBC-MAC
        aes256CbcMacCalculate(&ctx, ProgrammFlashPage.raw, sizeof(ProgrammFlashPage.PageDataBytes) + sizeof(ProgrammFlashPage.padding));

        // Write data to the AVR
        int r = SecureLoader_write(ProgrammFlashPage.raw, sizeof(ProgrammFlashPage), 1);
        if (!r) die("Error writing to SecureLoader\n");
    }
    printf_verbose("\n");
}

void changeKey(uint8_t* oldkey, uint8_t* newkey)
{
    printf_verbose("Changing key\n");

    // Get the data ready
    newBootloaderKey_t newBootloaderKey;
    memcpy(newBootloaderKey.data.BootloaderKey, newkey, sizeof(newBootloaderKey.data.BootloaderKey));

    // Initialize key schedule inside CTX
    aes256_init(oldkey, &ctx);

    // Encrypt the data
    aes256CbcEncrypt(&ctx, newBootloaderKey.IV, sizeof(newBootloaderKey.data.BootloaderKey));

    // Calculate and save CBC-MAC
    aes256CbcMacCalculate(&ctx, newBootloaderKey.data.raw, sizeof(newBootloaderKey.data.BootloaderKey));

    // Write data to the AVR
    int r = SecureLoader_write(newBootloaderKey.data.raw, sizeof(newBootloaderKey.data), 1);
    if (!r) die("Error writing to SecureLoader\n");
}

void verifyData(void)
{
    printf_verbose("Verifing\n");
    for (int addr = 0; addr < CODE_SIZE; addr += SPM_PAGESIZE) {
        printf_high_verbose("\n%d", addr);
        if (addr > 0 && !ihex_bytes_within_range(addr, addr + SPM_PAGESIZE - 1)) {
            // don't waste time on blocks that are unused,
            // but always do the first one to erase the chip
            printf_high_verbose(" Empty Block!");
            continue;
        }
        if(addr >= (CODE_SIZE - BOOTLOADER_SIZE)){
            printf_verbose("Warning: Skipping BootLoader Section!");
            continue;
        }

        if(verbose == 1){
            printf_verbose(".");
        }

        // Special case for large flash MCUs
        if (CODE_SIZE > 0xFFFF)
        {
            addr >>= 8;
        }

        // Request page
        SetFlashPage_t SetFlashPage = { .PageAddress = addr};
        int r = SecureLoader_write(SetFlashPage.raw, sizeof(SetFlashPage), 1);
        if (!r) die("Error writing to SecureLoader\n");

        // Get data from AVR
        ReadFlashPage_t verifybuf;
        r = SecureLoader_read(verifybuf.raw, sizeof(verifybuf), 1);
        if (!r) die("Error reading SecureLoader\n");

        // Get hex file data
        ReadFlashPage_t originalbuf = { .PageAddress = addr};
        ihex_get_data(addr, sizeof(originalbuf.PageDataBytes), originalbuf.PageDataBytes);

        // Compare the data
        if(memcmp(verifybuf.raw, originalbuf.raw, sizeof(originalbuf))){
            printf_verbose("Expected:\n");
            hexdump(originalbuf.raw, sizeof(originalbuf));
            printf_verbose("Received:\n");
            hexdump(verifybuf.raw, sizeof(verifybuf));
            die("Error verification mismatch\n");
        }
    }
    printf_verbose("\n");
}


/****************************************************************/
/*                                                              */
/*             USB Access - libusb (Linux & FreeBSD)            */
/*                                                              */
/****************************************************************/

#if defined(USE_LIBUSB)

// http://libusb.sourceforge.net/doc/index.html
#include <usb.h>

usb_dev_handle * open_usb_device(int vid, int pid)
{
    struct usb_bus *bus;
    struct usb_device *dev;
    usb_dev_handle *h;
    #ifdef LIBUSB_HAS_GET_DRIVER_NP
    char buf[128];
    #endif
    int r;

    usb_init();
    usb_find_busses();
    usb_find_devices();
    //printf_verbose("\nSearching for USB device:\n");
    for (bus = usb_get_busses(); bus; bus = bus->next) {
        for (dev = bus->devices; dev; dev = dev->next) {
            //printf_verbose("bus \"%s\", device \"%s\" vid=%04X, pid=%04X\n",
            //    bus->dirname, dev->filename,
            //    dev->descriptor.idVendor,
            //    dev->descriptor.idProduct
            //);
            if (dev->descriptor.idVendor != vid) continue;
            if (dev->descriptor.idProduct != pid) continue;
            h = usb_open(dev);
            if (!h) {
                printf_verbose("Found device but unable to open");
                continue;
            }
            #ifdef LIBUSB_HAS_GET_DRIVER_NP
            r = usb_get_driver_np(h, 0, buf, sizeof(buf));
            if (r >= 0) {
                r = usb_detach_kernel_driver_np(h, 0);
                if (r < 0) {
                    usb_close(h);
                    printf_verbose("Device is in use by \"%s\" driver", buf);
                    continue;
                }
            }
            #endif
            // Mac OS-X - removing this call to usb_claim_interface() might allow
            // this to work, even though it is a clear misuse of the libusb API.
            // normally Apple's IOKit should be used on Mac OS-X
            r = usb_claim_interface(h, 0);
            if (r < 0) {
                usb_close(h);
                printf_verbose("Unable to claim interface, check USB permissions");
                continue;
            }
            return h;
        }
    }
    return NULL;
}

static usb_dev_handle *libusb_SecureLoader_handle = NULL;

int SecureLoader_open(void)
{
    SecureLoader_close();
    libusb_SecureLoader_handle = open_usb_device(VENDOR_ID, PRODUCT_ID);

    if (!libusb_SecureLoader_handle)
        libusb_SecureLoader_handle = open_usb_device(0x03eb, 0x2067);

    if (!libusb_SecureLoader_handle) return 0;
    return 1;
}

int SecureLoader_write(void *buf, int len, double timeout)
{
    int r;

    if (!libusb_SecureLoader_handle) return 0;
    // 0x0100 out, 0x0200 feature
    r = usb_control_msg(libusb_SecureLoader_handle, 0x21, 9, 0x0200, 0, (char *)buf,
        len, (int)(timeout * 1000.0));
    if (r < 0) return 0;
    return 1;
}

void SecureLoader_close(void)
{
    if (!libusb_SecureLoader_handle) return;
    usb_release_interface(libusb_SecureLoader_handle, 0);
    usb_close(libusb_SecureLoader_handle);
    libusb_SecureLoader_handle = NULL;
}

#endif


#if defined(USE_HIDAPI)

// http://www.signal11.us/oss/hidapi/
#include <hidapi.h>

static hid_device* hidapi_device = NULL;

void SecureLoader_init(void)
{
    hid_init();
}

void SecureLoader_exit(void)
{
    hid_exit();
}

int SecureLoader_open(void)
{
    hidapi_device = hid_open(VENDOR_ID, PRODUCT_ID, NULL);

    if(!hidapi_device){
        hidapi_device = hid_open(0x03eb, 0x2067, NULL);
    }

    if (!hidapi_device) return 0;
    return 1;
}

int SecureLoader_write(void *buf, int len, double timeout)
{
    if (!hidapi_device) return 0;

    // Add report ID (0)
    uint8_t newbuf[len + 1];
    newbuf[0] = 0x00;
    memcpy(newbuf + 1, buf, len);

    int r = hid_write(hidapi_device, newbuf, len + 1);
    //int r = hid_send_feature_report(hidapi_device, newbuf, len + 1);

    if (r < 0) return 0;
    return 1;
}

int SecureLoader_read(void *buf, int len, double timeout)
{
    if (!hidapi_device) return 0;

    // Add report ID (0)
    uint8_t newbuf[len + 1];
    newbuf[0] = 0x00;

    int r = hid_get_feature_report(hidapi_device, newbuf, len + 1);
    memcpy(buf, newbuf + 1, len);

    if (r < 0) return 0;
    return 1;
}

void SecureLoader_close(void)
{
    if (!hidapi_device) return;
    hid_close (hidapi_device);
    hidapi_device = NULL;
}

#endif


/****************************************************************/
/*                                                              */
/*               USB Access - Microsoft WIN32                   */
/*                                                              */
/****************************************************************/

#if defined(USE_WIN32)

// http://msdn.microsoft.com/en-us/library/ms790932.aspx
#include <windows.h>
#include <setupapi.h>
#include <ddk/hidsdi.h>
#include <ddk/hidclass.h>

HANDLE open_usb_device(int vid, int pid)
{
    GUID guid;
    HDEVINFO info;
    DWORD index, required_size;
    SP_DEVICE_INTERFACE_DATA iface;
    SP_DEVICE_INTERFACE_DETAIL_DATA *details;
    HIDD_ATTRIBUTES attrib;
    HANDLE h;
    BOOL ret;

    HidD_GetHidGuid(&guid);
    info = SetupDiGetClassDevs(&guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (info == INVALID_HANDLE_VALUE) return NULL;
    for (index=0; 1 ;index++) {
        iface.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        ret = SetupDiEnumDeviceInterfaces(info, NULL, &guid, index, &iface);
        if (!ret) {
            SetupDiDestroyDeviceInfoList(info);
            break;
        }
        SetupDiGetInterfaceDeviceDetail(info, &iface, NULL, 0, &required_size, NULL);
        details = (SP_DEVICE_INTERFACE_DETAIL_DATA *)malloc(required_size);
        if (details == NULL) continue;
        memset(details, 0, required_size);
        details->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
        ret = SetupDiGetDeviceInterfaceDetail(info, &iface, details,
            required_size, NULL, NULL);
        if (!ret) {
            free(details);
            continue;
        }
        h = CreateFile(details->DevicePath, GENERIC_READ|GENERIC_WRITE,
            FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED, NULL);
        free(details);
        if (h == INVALID_HANDLE_VALUE) continue;
        attrib.Size = sizeof(HIDD_ATTRIBUTES);
        ret = HidD_GetAttributes(h, &attrib);
        if (!ret) {
            CloseHandle(h);
            continue;
        }
        if (attrib.VendorID != vid || attrib.ProductID != pid) {
            CloseHandle(h);
            continue;
        }
        SetupDiDestroyDeviceInfoList(info);
        return h;
    }
    return NULL;
}

int write_usb_device(HANDLE h, void *buf, int len, int timeout)
{
    static HANDLE event = NULL;
    unsigned char tmpbuf[1040];
    OVERLAPPED ov;
    DWORD n, r;

    if (len > sizeof(tmpbuf) - 1) return 0;
    if (event == NULL) {
        event = CreateEvent(NULL, TRUE, TRUE, NULL);
        if (!event) return 0;
    }
    ResetEvent(&event);
    memset(&ov, 0, sizeof(ov));
    ov.hEvent = event;
    tmpbuf[0] = 0;
    memcpy(tmpbuf + 1, buf, len);
    if (!WriteFile(h, tmpbuf, len + 1, NULL, &ov)) {
        if (GetLastError() != ERROR_IO_PENDING) return 0;
        r = WaitForSingleObject(event, timeout);
        if (r == WAIT_TIMEOUT) {
            CancelIo(h);
            return 0;
        }
        if (r != WAIT_OBJECT_0) return 0;
    }
    if (!GetOverlappedResult(h, &ov, &n, FALSE)) return 0;
    return 1;
}

static HANDLE win32_SecureLoader_handle = NULL;

int SecureLoader_open(void)
{
    SecureLoader_close();
    win32_SecureLoader_handle = open_usb_device(VENDOR_ID, PRODUCT_ID);

    if (!win32_SecureLoader_handle)
        win32_SecureLoader_handle = open_usb_device(0x03eb, 0x2067);

    if (!win32_SecureLoader_handle) return 0;
    return 1;
}

int SecureLoader_write(void *buf, int len, double timeout)
{
    int r;
    if (!win32_SecureLoader_handle) return 0;
    r = write_usb_device(win32_SecureLoader_handle, buf, len, (int)(timeout * 1000.0));
    return r;
}

void SecureLoader_close(void)
{
    if (!win32_SecureLoader_handle) return;
    CloseHandle(win32_SecureLoader_handle);
    win32_SecureLoader_handle = NULL;
}

#endif



/****************************************************************/
/*                                                              */
/*             USB Access - Apple's IOKit, Mac OS-X             */
/*                                                              */
/****************************************************************/

#if defined(USE_APPLE_IOKIT)

// http://developer.apple.com/technotes/tn2007/tn2187.html
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDLib.h>
#include <IOKit/hid/IOHIDDevice.h>

struct usb_list_struct {
    IOHIDDeviceRef ref;
    int pid;
    int vid;
    struct usb_list_struct *next;
};

static struct usb_list_struct *usb_list=NULL;
static IOHIDManagerRef hid_manager=NULL;

void attach_callback(void *context, IOReturn r, void *hid_mgr, IOHIDDeviceRef dev)
{
    CFTypeRef type;
    struct usb_list_struct *n, *p;
    int32_t pid, vid;

    if (!dev) return;
    type = IOHIDDeviceGetProperty(dev, CFSTR(kIOHIDVendorIDKey));
    if (!type || CFGetTypeID(type) != CFNumberGetTypeID()) return;
    if (!CFNumberGetValue((CFNumberRef)type, kCFNumberSInt32Type, &vid)) return;
    type = IOHIDDeviceGetProperty(dev, CFSTR(kIOHIDProductIDKey));
    if (!type || CFGetTypeID(type) != CFNumberGetTypeID()) return;
    if (!CFNumberGetValue((CFNumberRef)type, kCFNumberSInt32Type, &pid)) return;
    n = (struct usb_list_struct *)malloc(sizeof(struct usb_list_struct));
    if (!n) return;
    //printf("attach callback: vid=%04X, pid=%04X\n", vid, pid);
    n->ref = dev;
    n->vid = vid;
    n->pid = pid;
    n->next = NULL;
    if (usb_list == NULL) {
        usb_list = n;
    } else {
        for (p = usb_list; p->next; p = p->next) ;
        p->next = n;
    }
}

void detach_callback(void *context, IOReturn r, void *hid_mgr, IOHIDDeviceRef dev)
{
    struct usb_list_struct *p, *tmp, *prev=NULL;

    p = usb_list;
    while (p) {
        if (p->ref == dev) {
            if (prev) {
                prev->next = p->next;
            } else {
                usb_list = p->next;
            }
            tmp = p;
            p = p->next;
            free(tmp);
        } else {
            prev = p;
            p = p->next;
        }
    }
}

void init_hid_manager(void)
{
    CFMutableDictionaryRef dict;
    IOReturn ret;

    if (hid_manager) return;
    hid_manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (hid_manager == NULL || CFGetTypeID(hid_manager) != IOHIDManagerGetTypeID()) {
        if (hid_manager) CFRelease(hid_manager);
        printf_verbose("no HID Manager - maybe this is a pre-Leopard (10.5) system?\n");
        return;
    }
    dict = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    if (!dict) return;
    IOHIDManagerSetDeviceMatching(hid_manager, dict);
    CFRelease(dict);
    IOHIDManagerScheduleWithRunLoop(hid_manager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    IOHIDManagerRegisterDeviceMatchingCallback(hid_manager, attach_callback, NULL);
    IOHIDManagerRegisterDeviceRemovalCallback(hid_manager, detach_callback, NULL);
    ret = IOHIDManagerOpen(hid_manager, kIOHIDOptionsTypeNone);
    if (ret != kIOReturnSuccess) {
        IOHIDManagerUnscheduleFromRunLoop(hid_manager,
            CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        CFRelease(hid_manager);
        printf_verbose("Error opening HID Manager");
    }
}

static void do_run_loop(void)
{
    while (CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, true) == kCFRunLoopRunHandledSource) ;
}

IOHIDDeviceRef open_usb_device(int vid, int pid)
{
    struct usb_list_struct *p;
    IOReturn ret;

    init_hid_manager();
    do_run_loop();
    for (p = usb_list; p; p = p->next) {
        if (p->vid == vid && p->pid == pid) {
            ret = IOHIDDeviceOpen(p->ref, kIOHIDOptionsTypeNone);
            if (ret == kIOReturnSuccess) return p->ref;
        }
    }
    return NULL;
}

void close_usb_device(IOHIDDeviceRef dev)
{
    struct usb_list_struct *p;

    do_run_loop();
    for (p = usb_list; p; p = p->next) {
        if (p->ref == dev) {
            IOHIDDeviceClose(dev, kIOHIDOptionsTypeNone);
            return;
        }
    }
}

static IOHIDDeviceRef iokit_SecureLoader_reference = NULL;

int SecureLoader_open(void)
{
    SecureLoader_close();
    iokit_SecureLoader_reference = open_usb_device(VENDOR_ID, PRODUCT_ID);

    if (!iokit_SecureLoader_reference)
        iokit_SecureLoader_reference = open_usb_device(0x03eb, 0x2067);

    if (!iokit_SecureLoader_reference) return 0;
    return 1;
}

int SecureLoader_write(void *buf, int len, double timeout)
{
    IOReturn ret;

    // timeouts do not work on OS-X
    // IOHIDDeviceSetReportWithCallback is not implemented
    // even though Apple documents it with a code example!
    // submitted to Apple on 22-sep-2009, problem ID 7245050
    if (!iokit_SecureLoader_reference) return 0;
    ret = IOHIDDeviceSetReport(iokit_SecureLoader_reference,
        kIOHIDReportTypeOutput, 0, buf, len);
    if (ret == kIOReturnSuccess) return 1;
    return 0;
}

void SecureLoader_close(void)
{
    if (!iokit_SecureLoader_reference) return;
    close_usb_device(iokit_SecureLoader_reference);
    iokit_SecureLoader_reference = NULL;
}

#endif



/****************************************************************/
/*                                                              */
/*              USB Access - BSD's UHID driver                  */
/*                                                              */
/****************************************************************/

#if defined(USE_UHID)

// Thanks to Todd T Fries for help getting this working on OpenBSD
// and to Chris Kuethe for the initial patch to use UHID.

#include <sys/ioctl.h>
#include <fcntl.h>
#include <dirent.h>
#include <dev/usb/usb.h>
#ifndef USB_GET_DEVICEINFO
#include <dev/usb/usb_ioctl.h>
#endif

#ifndef USB_GET_DEVICEINFO
# define USB_GET_DEVICEINFO 0
# error The USB_GET_DEVICEINFO ioctl() value is not defined for your system.
#endif

int open_usb_device(int vid, int pid)
{
    int r, fd;
    DIR *dir;
    struct dirent *d;
    struct usb_device_info info;
    char buf[256];

    dir = opendir("/dev");
    if (!dir) return -1;
    while ((d = readdir(dir)) != NULL) {
        if (strncmp(d->d_name, "uhid", 4) != 0) continue;
        snprintf(buf, sizeof(buf), "/dev/%s", d->d_name);
        fd = open(buf, O_RDWR);
        if (fd < 0) continue;
        r = ioctl(fd, USB_GET_DEVICEINFO, &info);
        if (r < 0) {
            // NetBSD: added in 2004
            // OpenBSD: added November 23, 2009
            // FreeBSD: missing (FreeBSD 8.0) - USE_LIBUSB works!
            die("Error: your uhid driver does not support"
              " USB_GET_DEVICEINFO, please upgrade!\n");
            close(fd);
            closedir(dir);
            SecureLoader_exit();
            exit(1);
        }
        //printf("%s: v=%d, p=%d\n", buf, info.udi_vendorNo, info.udi_productNo);
        if (info.udi_vendorNo == vid && info.udi_productNo == pid) {
            closedir(dir);
            return fd;
        }
        close(fd);
    }
    closedir(dir);
    return -1;
}

static int uhid_SecureLoader_fd = -1;

int SecureLoader_open(void)
{
    SecureLoader_close();
    uhid_SecureLoader_fd = open_usb_device(VENDOR_ID, PRODUCT_ID);

    if (uhid_SecureLoader_fd < 0)
        uhid_SecureLoader_fd = open_usb_device(0x03eb, 0x2067);

    if (uhid_SecureLoader_fd < 0) return 0;
    return 1;
}

int SecureLoader_write(void *buf, int len, double timeout)
{
    int r;

    // TODO: implement timeout... how??
    r = write(uhid_SecureLoader_fd, buf, len);
    if (r == len) return 1;
    return 0;
}

void SecureLoader_close(void)
{
    if (uhid_SecureLoader_fd >= 0) {
        close(uhid_SecureLoader_fd);
        uhid_SecureLoader_fd = -1;
    }
}

#endif



/****************************************************************/
/*                                                              */
/*                     Read Intel Hex File                      */
/*                                                              */
/****************************************************************/

// the maximum flash image size we can support
// chips with larger memory may be used, but only this
// much intel-hex data can be loaded into memory!
#define MAX_MEMORY_SIZE 0x10000

static unsigned char firmware_image[MAX_MEMORY_SIZE];
static unsigned char firmware_mask[MAX_MEMORY_SIZE];
static int end_record_seen=0;
static int byte_count;
static unsigned int extended_addr = 0;
static int parse_hex_line(char *line);

int read_intel_hex(const char *filename)
{
    FILE *fp;
    int i, lineno=0;
    char buf[1024];

    byte_count = 0;
    end_record_seen = 0;
    for (i=0; i<MAX_MEMORY_SIZE; i++) {
        firmware_image[i] = 0xFF;
        firmware_mask[i] = 0;
    }
    extended_addr = 0;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        //printf("Unable to read file %s\n", filename);
        return -1;
    }
    while (!feof(fp)) {
        *buf = '\0';
        if (!fgets(buf, sizeof(buf), fp)) break;
        lineno++;
        if (*buf) {
            if (parse_hex_line(buf) == 0) {
                //printf("Warning, parse error line %d\n", lineno);
                fclose(fp);
                return -2;
            }
        }
        if (end_record_seen) break;
        if (feof(stdin)) break;
    }
    fclose(fp);
    return byte_count;
}


/* from ihex.c, at http://www.pjrc.com/tech/8051/pm2_docs/intel-hex.html */

/* parses a line of intel hex code, stores the data in bytes[] */
/* and the beginning address in addr, and returns a 1 if the */
/* line was valid, or a 0 if an error occurred.  The variable */
/* num gets the number of bytes that were stored into bytes[] */


int
parse_hex_line(char *line)
{
    int addr, code, num;
        int sum, len, cksum, i;
        char *ptr;

        num = 0;
        if (line[0] != ':') return 0;
        if (strlen(line) < 11) return 0;
        ptr = line+1;
        if (!sscanf(ptr, "%02x", &len)) return 0;
        ptr += 2;
        if ((int)strlen(line) < (11 + (len * 2)) ) return 0;
        if (!sscanf(ptr, "%04x", &addr)) return 0;
        ptr += 4;
          /* printf("Line: length=%d Addr=%d\n", len, addr); */
        if (!sscanf(ptr, "%02x", &code)) return 0;
    if (addr + extended_addr + len >= MAX_MEMORY_SIZE) return 0;
        ptr += 2;
        sum = (len & 255) + ((addr >> 8) & 255) + (addr & 255) + (code & 255);
    if (code != 0) {
        if (code == 1) {
            end_record_seen = 1;
            return 1;
        }
        if (code == 2 && len == 2) {
            if (!sscanf(ptr, "%04x", &i)) return 1;
            ptr += 4;
            sum += ((i >> 8) & 255) + (i & 255);
                if (!sscanf(ptr, "%02x", &cksum)) return 1;
            if (((sum & 255) + (cksum & 255)) & 255) return 1;
            extended_addr = i << 4;
            //printf("ext addr = %05X\n", extended_addr);
        }
        if (code == 4 && len == 2) {
            if (!sscanf(ptr, "%04x", &i)) return 1;
            ptr += 4;
            sum += ((i >> 8) & 255) + (i & 255);
                if (!sscanf(ptr, "%02x", &cksum)) return 1;
            if (((sum & 255) + (cksum & 255)) & 255) return 1;
            extended_addr = i << 16;
            //printf("ext addr = %08X\n", extended_addr);
        }
        return 1;    // non-data line
    }
    byte_count += len;
        while (num != len) {
                if (sscanf(ptr, "%02x", &i) != 1) return 0;
        i &= 255;
        firmware_image[addr + extended_addr + num] = i;
        firmware_mask[addr + extended_addr + num] = 1;
                ptr += 2;
                sum += i;
                (num)++;
                if (num >= 256) return 0;
        }
        if (!sscanf(ptr, "%02x", &cksum)) return 0;
        if (((sum & 255) + (cksum & 255)) & 255) return 0; /* checksum error */
        return 1;
}

int ihex_bytes_within_range(int begin, int end)
{
    int i;

    if (begin < 0 || begin >= MAX_MEMORY_SIZE ||
       end < 0 || end >= MAX_MEMORY_SIZE) {
        return 0;
    }
    for (i=begin; i<=end; i++) {
        if (firmware_mask[i]) return 1;
    }
    return 0;
}

void ihex_get_data(int addr, int len, unsigned char *bytes)
{
    int i;

    if (addr < 0 || len < 0 || addr + len >= MAX_MEMORY_SIZE) {
        for (i=0; i<len; i++) {
            bytes[i] = 255;
        }
        return;
    }
    for (i=0; i<len; i++) {
        if (firmware_mask[addr]) {
            bytes[i] = firmware_image[addr];
        } else {
            bytes[i] = 255;
        }
        addr++;
    }
}

/****************************************************************/
/*                                                              */
/*                       Misc Functions                         */
/*                                                              */
/****************************************************************/

int printf_verbose(const char *format, ...)
{
    va_list ap;
    int r = 0;

    va_start(ap, format);
    if (verbose) {
        r = vprintf(format, ap);
        fflush(stdout);
    }
    va_end(ap);

    return r;
}

int printf_high_verbose(const char *format, ...)
{
    va_list ap;
    int r = 0;

    va_start(ap, format);
    if (verbose > 1) {
        r = vprintf(format, ap);
        fflush(stdout);
    }
    va_end(ap);

    return r;
}

void hexdump(uint8_t * data, size_t len)
{
    size_t i;
    for (i = 0; i < len; i++) {
        printf_verbose("0x%02X\t", data[i]);
        if(i%16==15){
            printf_verbose("\n");
        }
    }
    printf_verbose("\n");
}

void delay(double seconds)
{
    #ifdef USE_WIN32
    sleep(seconds * 1000.0);
    #else
    usleep(seconds * 1000000.0);
    #endif
}

void die(const char *str, ...)
{
    va_list  ap;

    va_start(ap, str);
    vfprintf(stderr, str, ap);
    fprintf(stderr, "\n");
    va_end(ap);

    SecureLoader_exit();

    exit(1);
}

#if defined USE_WIN32
#define strcasecmp stricmp
#endif

void parse_options(int argc, char **argv)
{
    int i;
    const char *arg;

    for (i=1; i<argc; i++) {
        arg = argv[i];

        if (*arg == '-') {
            if (strcmp(arg, "-w") == 0) {
                wait_for_device_to_appear = 1;
            } else if (strcmp(arg, "-n") == 0) {
                reboot_after_programming = 0;
            } else if (strcmp(arg, "-v") == 0) {
                verbose = 1;
            } else if (strcmp(arg, "-vv") == 0) {
                    verbose = 2;
            }
        } else {
            filename = argv[i];
        }
    }
}
