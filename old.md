# SecureLoader

Our firmware deals with some information that is secured.
A manipulated firmware or bootloader could introduce backdoors to leak this secure information.
The most important thing we want to achieve in our project
is to ensure that our firmware and bootloader is running unmodified without any backdoors.

## Bootloader overview

### Bootloader use case
 * Uploading a more trusted/self recompiled firmware
 * Security updates
 * Feature updates
 * Developing new firmware features

### Bootloader aims
 * Protect against malicious firmware upgrades
 * Security checksums of firmware and bootloader
 * Small size (TODO?)
 * Flexibility and compatibility through USB HID
 * Open Source
 * Only available for AVR, developed for 32u4
 * Always be able to flash a new bootloader and firmware without closing the source.
 * Based on LUFA
 * Reusable AES implementation

### Security assumptions
 * ~~Without opening the device you cannot modify the bootloader~~
 * Without [access to the firmware](http://oneweekwonder.blogspot.co.uk/2014/07/bootjacker-amazing-avr-bootloader-hack.html) you are not able to modify the bootloader
 * We consider the bootloader integrity secure then

### User interface requirements
 * Accessible from a browser Addin (USB HID)
 * A command line tool would also be very important for debugging
 * Bootloader cannot be updated later, so it needs to be of high quality
 * The bootloader execution should not rely on the (possibly broken) firmware
 * [Simple bootloader execution mechanism](https://github.com/NicoHood/HoodLoader2/wiki/How-to-use-reset)
 * ~~Or connect HWB to a hardware button (pulled HIGH) and start bootloader
   if this button is pressed when plugged in at startup~~
 * Verifying a firmware should be possible at any time

### Implementation requirements
 * Small size in flash (2kb would be nice)
 * Locking the bootloader after each firmware upgrade increases security
 * The bootloader could keep track of the number of upgrades (check buffer overflow!)
 * Verify the bootloader checksum before starting
 * Verify the firmware checksum before running it
 * Verify the fuse settings as well
 * Use proper lock bits
 * Add an user interface to download the firmware again
 * Do not leave any trace in EEPROM or RAM
 * Do a watchdog reset for application starting to keep all registers cleared
 * Ensure the firmware cannot modify the bootloader. -> but if we have a malicious firmware, didnt we already fail in out task?

### Links

#### Security aspects
 * http://wiki.hacdc.org/index.php/Secure_bootloader
 * http://hackaday.com/2014/07/05/overwriting-a-protected-avr-bootloader/
 * https://media.ccc.de/v/32c3-7189-key-logger_video_mouse
 * http://jtxp.org/tech/tinysafeboot_en.htm
 * https://en.wikipedia.org/wiki/CBC-MAC
 * https://nacl.cr.yp.to/box.html
 * [AVRNaCl](http://munacl.cryptojedi.org/atmega.shtml)
 * http://www.atmel.com/images/doc2589.pdf
 * http://www.atmel.com/images/doc2541.pdf

#### AVR Bootloaders
 * http://www.nongnu.org/avr-libc/user-manual/group__avr__boot.html
 * http://www.avrfreaks.net/forum/faq-c-writing-bootloader-faq?page=all
 * https://www.mikrocontroller.net/articles/AVR_Bootloader_in_C_-_eine_einfache_Anleitung
 * http://www.embedds.com/all-you-need-to-know-about-avr-fuses/
 * http://www.engbedded.com/fusecalc/


Introduction

Making a firmware secure and trusted while keeping the option to update it is a challange.
There are a lot of potential risks you have to think of that should all be considered.
If a single piece of the software is insecure, the whole concept is insecure.
The SecureLoader aims to consider all this cases. This paper described potential attacks and how to prevent them.

TODO add section "where you would want to use secure loader"


### booting the firmware
* Recovery mode not entered (just plug in the device)
 * Write the firmware identifier into a special ram position
 * The firmware should hash this identifier with a nonce to authenticate itself
 * The user needs to check the firmware integrity by accepting the displayed hash
 * Correct firmware hash
  * The user can trust the firmware
 * Incorrect firmware hash
  * The firmware and/or bootloader was modified
  * The bootloader can be used to check the bootloader and firmware integrity
  * Uploading a different firmware and then the original again will be noticed





  Anforderungen
  * The bootloader is able to flash new authenticated firmwares
  * The bootloader is able to provide a recovery mode to authenticate itself and check its and the firmwares integrity
  * The bootloader needs to check its and the firmwares integrity (checksum)
  * The bootloader prevents hacks to leak the secret key or modify the bootloader from the firmware





  ### The attack scenario
   The attacker could be someone getting access to the device (that holds your passwords), modify it, add a backdoor and give it back to you. You then use the firmware to save your passwords, his backdoor writes them in EEPROM and downloads them later again. An evil colleague could be that guy. Or someone who gets the device when you are away/while shipping it to you.

   This is an **active attack** where the attacker also somehow needs to get back the data later.
   The encrypted data on the device is still secure unless the malicious firmware backdoors it.





  TODO atgtacker gets pc virus

  ### Attacker is able to manipulate the bootloader
   As part of the bootloader tasks, it should check the firmware for any unauthorized changes.
   A modified bootloader could not check the firmware integrity (checksum) reliable.
   It could also introduce a backdoor to upload malicious firmware while keeping the previous checksum.

   An attacker could abuse this to make the firmware itself not trusted anymore.
   This could be done by getting physical access to the device when carrying it with you or even while shipping it to you.





  #### Manipulating the bootloader from firmware
   With some methods it could be possible to [modify a bootloader from the firmware](http://oneweekwonder.blogspot.de/2014/07/bootjacker-amazing-avr-bootloader-hack.html).

  ##### Solution
   First the attacker needs to get access to write a new firmware to the device.
   The protection against that (and the firmware integrity check) is described later in this document.
   TODO secure this with initial (individual) password by manufacturer?

   **TODO AES encrypted password hash?**

   If the attacker is able to flash an unauthorized firmware (or the user uploads one)
   he will not be able to modify or execute the bootloader in any way.
   By setting the correct fuse bits you are not able to access the bootloader at all.
   TODO is this true??? this is critical!

   Securing/checking the bootloader from the firmware is impossible, because it is considered untrusted and also has no access to read the data.

   **Uploading a malicious firmware from an unknown source that does other bad stuff is not considered/unimportant here.**

  #### Manipulating the bootloader via ISP
    With an external ISP you could manipulate (especially) the bootloader code.

  ##### Solution
    An ISP can only be used if the device was opened up.
    We assume that opening the device will break the device in a way the user will notice.

    Even if the device was opened without breaking it, the user password is considered to be true.
    The bootloader data cannot be read from the firmware nor from the ISP due to lock bits.
    Thatswhy a new bootloader would function different or wont accept your password or accept all passwords. The bootloader is still open source, but the password will be kept in the secret flash section of the bootloader.

    Knowing the secret key **and** being able to use an ISP can make the attacker to modify the bootloader. Therefor the initial key needs to be exchanged after the device has shipped and should be changed afterwards.

    Reading the password from the AVR hardware requires it to open. This costs around 500$ some people say. You could then replace the avr with a new bootloader and the same password.
    You should rather leave the country if its that critical or assume that the shipped device is already faked completely when receiving it.

      TODO short explanation

  ### The hardware was fakes from someone else
     The hardware could be fakes from someone else at a whole. This requires and 1:1 copy of the device and is very unlikely.

  ##### Solution
     Each device has an initial password from the vendor or your changed password. You can verify this yourself. The device would not let you unlock it or every password would unlock the bootloader.

  TODO Securing/checking the bootloader from the firmware is impossible, because it is considered untrusted and also has no access to read the data.








    To check the firmware integrity the bootloader has options to do so.
    Since the bootloader itself is considered to be original, you can always verify the firmware checksum.
    The checksums are placed in the flash of the bootloader, so the firmware cannot fake a wrong checksum.








  ### An attacker gets access to the PC side communication
   An attacker could modify the uploaded firmware and fake the whole uploading process.
   Furthermore it could compromise the firmwares USB actions.

  ##### Solution
   The bootloader will only start if you manually start it via hardware button presses.
   Not even a software crash (watchdog reset) will be able to run the bootloader flashing option. Also the password is needed to start the firmware flashing.

   The bootloader is considered to be secure. Therefor you can always check the firmware integrity from another more trusted PC. A check from the firmware makes no sense, because it has no access to the checksum and it could be malicious already.

   Securing the firmware USB transactions is not part of the bootloader.

  ### Attacker is able to upload different firmware
   If an attacker is able to upload a malicious firmware he can backup the passwords and leak them somehow.

  ##### Solution
   We need to check the integrity of the firmware. The bootloader is considered to be trusted and not modified. With a trusted bootloader you can always check the checksum of a firmware.
   The bootloader could also lock you out from flashing new firmwares with a password.
   The bootloader could keep track of the number of firmware flashes.
   So even if the firmware is not trusted, the bootloader can always check the firmware state and upload a more trusted firmware.
   The bootloader should check a special password before uploading.
   If its wrong, it will not flash the firmware. It will need to unlock the flash process every time.

  ### Someone changes your bootloader password
   If someone changes your bootloader password before you did, you will not be able to reflash new firmware. This might also happen if you lose the password.

  ##### Solution
    Newer lose you bootloader password or give it away. If someone else changed the password you can still verify the firmware checksum. Then you are locked out, but the firmware can be checked again for integrity. **However there is a risk that the bad guy will flash an untrusted firmware soon.** If you still have the password, change it again.

  ### The firmware/bootloader flash bits get corrupted
   If for some reason the flash get corrupted weird errors might occur and the device could be insecure.

  ##### Solution
   The bootloader first does some kind of POST and checks its own flash for integrity.
   Then it will check the firmware for integrity. If the bootloader of firmware checksums are wrong the bootloader will not start the sketch. This will be done before any usb stuff is initialized to get maximum fail security.

  ### The uploaded firmware is not working, you locked out of your device
   If you upload a firmware that crashes your device in some way you need to recover another firmware, without relying on the firmware to start the bootloader process.

  ##### Solution
   The bootloader is always accessible at any time, even if the user sketch is not working.
   With pressing special keys at startup you can start the bootloader.
   You can do recovery stuff like checking the checksum of the firmware and upload a new firmware.
   The bootloader will work without any external devices, except the buttons to start the bootloader.
