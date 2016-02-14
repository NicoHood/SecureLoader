# SecureLoader

The idea is to create an USB bootloader that can easily upgrade an AVR firmware
without lowering the security of the device. This paper describes use cases,
potential attacks and solutions to prevent them.

This readme describes a **concept** with some ideas. It is **not final**.
Contributions appreciated.

## Bootloader overview

### Bootloader use case
 * Uploading a more trusted/self recompiled firmware
 * Security updates
 * Feature updates
 * Developing new firmware features

### Bootloader aims
 * Protect against malicious firmware upgrades
 * Security checksums of firmware and bootloader
 * Small size
 * Flexibility and compatibility through HID
 * Open Source

### Assumptions
 * Without opening the device you cannot modify the bootloader
 * We consider the bootloader integrity secure then

### User interface requirements
 * Accessible from a browser Addin (USB HID)
 * A command line tool would also be very important for debugging
 * Bootloader cannot be updated later, so it needs to be of high quality
 * The bootloader execution should not rely on the (possibly broken) firmware
 * [Simple bootloader execution mechanism](https://github.com/NicoHood/HoodLoader2/wiki/How-to-use-reset)
 * Or connect HWB to a hardware button (pulled HIGH) and start bootloader
   if this button is pressed when plugged in at startup
 * Verifying a firmware should be possible at any time

### Implementation requirements
 * Only available for AVR, developed for 32u4
 * Based on LUFA
 * Small size in flash (2kb would be nice)
 * Locking the bootloader after each firmware upgrade increases security
 * The bootloader could keep track of the number of upgrades (check buffer overflow!)
 * Verify the bootloader checksum before starting
 * Verify the firmware checksum before running it
 * Verify the fuse settings as well
 * Use proper lock bits
 * Add an user interface to download the firmware again
 * The bootloader should not leave any trace in EEPROM or RAM
 * Ensure the firmware cannot modify the bootloader. -> but if we have a malicious firmware, didnt we already fail in out task?

### Other ideas:
 * Lock the firmware for the first use until the user requests an initial boot key.
 * Read up on DFU lock bits to preserve firmware reading?

### Links

#### Security aspects
 * http://wiki.hacdc.org/index.php/Secure_bootloader
 * http://hackaday.com/2014/07/05/overwriting-a-protected-avr-bootloader/
 * https://media.ccc.de/v/32c3-7189-key-logger_video_mouse
 * http://jtxp.org/tech/tinysafeboot_en.htm

#### AVR Bootloaders
 * http://www.nongnu.org/avr-libc/user-manual/group__avr__boot.html
 * http://www.avrfreaks.net/forum/faq-c-writing-bootloader-faq?page=all
 * https://www.mikrocontroller.net/articles/AVR_Bootloader_in_C_-_eine_einfache_Anleitung


## Attack scenarios
 * A compromised PC uploads a malicious firmware
 * A local attacker uploads a malicious firmware
 * The malicious firmware adds a backdoor where the attacker can get your keys


## Solutions

TODO add a separate section for not used ideas, to show the cons when we finished with the final idea

### Asymmetric public/private key firmware signing

#### Concept
 * Only upload trusted firmware that was signed with a private key from the vendor

#### Why asymmetric and not symmetric
 * Do not leak the secret from inside the hardware
 * Otherwise you are not able to flash the bootloader again without the secret
 * A symmetric key for each device requires a lot of different signed firmwares
 * Initial symmetric key needs to be changed, after it was exchanged via insecure email
 * Symmetric key should be random and not generate via a serial number algorithm

#### Pros
 * Only trusted firmware can be uploaded
 * Maybe signing implementation can be reused
 * Optional (closed source) encrypted firmwares are possible

#### Cons
 * No proprietary firmware can be uploaded
 * Recompiling firmware needs the exact same build environment to confirm the source
 * You need to trust the seller/signing person
 * The private key of this person needs to be secure
 * If the person looses the key or dies we have a problem
 * Developing is not flexible or might add a backdoor
 * **Uses a lot of/too much flash probably**
 * If bootloader signing is insecure, we have a problem
 * Accessible all the time
 * You can still downgrade to a vulnerable signed version

#### Implementatio details
 * [AVRNaCl](http://munacl.cryptojedi.org/atmega.shtml) could be used
 * If we use symmetric signing (we wont, see above), the firmware should not be able to see the secret.

#### Links
 * http://www.atmel.com/images/doc2589.pdf
 * http://www.atmel.com/images/doc2541.pdf
 * [Symmetric key is insecure](http://www.avrfreaks.net/comment/1134251#comment-1134251)

### Un/Locking the bootloader

#### Concept
 * The bootloader should be lock- and unlockable.
 * Only authorized people should be able to unlock the bootloader.
 * The initial unlock code should not rely on the seller and be changed after first boot
 * Code should be different to smartcard pin
 * It is important that a brute forcing the unlock is impossible
 * Bootloader can be unlocked from the PC via HID (and does not rely on the firmware to unlock)

#### Unsolved attack scenarios
 * Initial firmware of the device should not be considered trusted, if NSA opens the post package
 -> Sending the initial password AFTER receiving? -> you rely on the seller and production again
 -> paranoid people should upload new firmware
  * Ensure the firmware cannot modify the bootloader. Bootloader should be considered trusted was the concept. (because of the reason above)
