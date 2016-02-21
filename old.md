# SecureLoader


## Bootloader overview

### Implementation requirements
- [ ] Bootloader should be accessible from a browser addon (USB HID)
- [ ] A command line tool for uploading is also essential for debugging
- [ ] Bootloader cannot be updated later, so it needs to be of high quality
- [ ] The bootloader execution should not rely on the (possibly bricked) firmware
- [ ] The bootloader should not touch EEPROM at all.
- [ ] Simple bootloader execution mechanism to enter bootloader mode.
- [ ] Firmware verification should be possible at any time

### Implementation requirements
- [ ] The bootloader could keep track of the number of upgrades (check buffer overflow!)
- [ ] Verify the bootloader checksum before starting
- [ ] Verify the firmware checksum before running it
- [ ] Verify the fuse settings as well
- [ ] Use proper lock bits
- [ ] Add an user interface to download the firmware again
- [ ] Do not leave any trace in EEPROM or RAM
- [ ] Do a watchdog reset for application starting to keep all registers cleared
- [ ] Ensure the firmware cannot modify the bootloader. -> but if we have a malicious firmware, didnt we already fail in out task?


### Links
 - [ ] http://jtxp.org/tech/tinysafeboot_en.htm
 - [ ] https://en.wikipedia.org/wiki/CBC-MAC
 - [ ] http://wiki.hacdc.org/index.php/Secure_bootloader
