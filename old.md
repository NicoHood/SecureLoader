# SecureLoader

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

### Other ideas:
 * Lock the firmware for the first use until the user requests an initial boot key.
 * Read up on DFU lock bits to preserve firmware reading?
 * Add a (symmetric AES) secret per device that can sign a "challange" to proof the bootloader is not modified. this check can be provided via special email to the seller. The fuse bits should prevent the secret from being read. however opening the device should prevent this. this would help on top of that and secure until opening the mcu package which is impossible to operate afterwards. this requires aes though.

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

#### Implementation details
 * [AVRNaCl](http://munacl.cryptojedi.org/atmega.shtml) could be used
 * If we use symmetric signing (we wont, see above), the firmware should not be able to see the secret.

#### Links
 * http://www.atmel.com/images/doc2589.pdf
 * http://www.atmel.com/images/doc2541.pdf
 * [Symmetric key is insecure](http://www.avrfreaks.net/comment/1134251#comment-1134251)

### Un/Locking the firmware flashing

#### Concept
 * The bootloader should be lock- and unlockable.
 * Only authorized people should be able to unlock the bootloader.
 * The initial unlock code ~~should not~~ must rely on the seller and **must** be changed after first unlocking
 * Code should be different to smartcard pin
 * It is important that a brute forcing the unlock is impossible
 * Bootloader can be unlocked from the PC via HID (and does not rely on the firmware to unlock)
 * The flashing of new firmware should always be locked and needs to be unlocked first.
   So people dont forget to lock the device again after a firmware upgrade

#### Unsolved attack scenarios
 * Initial firmware of the device should not be considered trusted, if NSA opens the post package
 -> Sending the initial password AFTER receiving? -> you rely on the seller and production again
 -> paranoid people should upload new firmware
  * Ensure the firmware cannot modify the bootloader. Bootloader should be considered trusted was the concept. (because of the reason above)


Introduction

Making a firmware secure and trusted while keeping the option to update it is a challange.
There are a lot of potential risks you have to think of that should all be considered.
If a single piece of the software is insecure, the whole concept is insecure.
The SecureLoader aims to consider all this cases. This paper described potential attacks and how to prevent them.

TODO add section "where you would want to use secure loader"


### Feature overview
* Check the bootloader and firmware integrity (checksum) at startup
* Recovery mode entered (press special hardware keys)
 * Authenticate the bootloader to the PC (via symmetric bootloader key)
  * PC sends a random challenge, bootloader sends back a hash of challenge + bootloader key
  * The user can trust the bootloader
 * Provide an option to verify the firmwares integrity (checksum)
 * Provide an option to flash new firmware
  * No bootloader key set
   * Bootloader key needs to be entered
   * Initial unique bootloader key from vendor is highly recommended
  * Bootloader key was set
   * Authenticate the PC to the bootloader (via symmetric bootloader key)
    * Bootloader sends a random challenge, PC sends back a hash of challenge + bootloader key
    * The random challenge forbids firmware downgrades with the same firmware + bootloader key hash
    * TODO on wrong authentication set some flag, that the firmware can read next time
   * Delete firmware identifier
   * Increase firmware counter
   * Write firmware
   * Verify the firmware checksum
   * Firmware is written successful
    * Write new random firmware identifier with new firmware counter
   * Firmware is corrupted/checksum incorrect
    * Write only the new firmware counter
    * Send an error to the PC
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







  Attacks:

  Someone tried to downgrade the firmware

  Solution:
   1. Each firmware update results in a different firmware identifier. The firmware cannot authenticate itself anymore.
   2. The PC needs to authenticate itself to the bootloader via a random challenge, not a hashed firmware checksum that is always the same




  Anforderungen
  * The bootloader is able to flash new firmwares to authorized people
  * The bootloader is able to provide a recovery mode to authenticate itself and check its and the firmwares integrity

  * The bootloader needs to authenticate itself to the PC
  * The PC needs to authenticate itself to the bootloader for flashing new firmwares
  * The bootloader needs to check its and the firmwares integrity (checksum)
  * The bootloader prevents hacks to leak the secret key or modify the bootloader from the firmware


  * The bootloader is not responsible for uploading an unauthorized firmware
  * The firmware need to authenticate itself at **every start** to ensure it is original, not the bootloader. The bootloader could still do so.



  Anforderungen oder Zielsetzung oder overview
  * The bootloader does not prevent from using an ISP to overwrite it
  -> It rather authenticate itself to the PC to check its integrity
  * The bootloader prevents from uploading firmwares from unauthorized people
  -> The PC (user) needs to authenticate itself to the bootloader
  * The bootloader prevents not from uploading an unauthorized firmware from authorized people
  -> But the bootloader prevents hacks to leak the secret key or modify the bootloader from the firmware
  * The bootloader can be used to check the firmwares (and itselfs) integrity (checksum) at any time
  -> The firmware should still authenticate itself at every start for security risks
  * The bootloader password needs to be kept secret
  -> This does not lower the security of the firmware itself, but the risk or an attacker flashing new firmware
  * The bootloader integrity
  -> To verify the


  To ensure the firmware you are running is the one you uploaded
  you need to verify at **every start** that the running firmware is original.

  You could (and can) do this with the bootloader.
  For usability it is very bad though, because you always need a PC with an app
  that checks the bootloader integrity first and then firmware.
  Also it might be considered to be a security risk to always use a PC
  with a symmetric key to authenticate the device.

  Therefor the **firmware** needs to **authenticate itself** at **every start**.
  This can be done before any USB communication was started.
  The advantage is that the firmware has access to the special hardware
  such as display and smartcard and makes it easier for the user to control.
  The authenticate mechanism is not described here and part of the firmware.
  If the firmware is not trusted, you can use the bootloader again
  to verify the bootloader and the firmware integrity.
  This needs to be done explicit when the user runs the bootloader in its recovery mode.
  Adding a bootloader also ensures, that there is no other fake bootloader
  which manipulates the firmware before running it.


  Therfore we need to ensure that a fake firmware has no access to the bootloader.






  ## Possible attacks and solutions

   The general risk of a bootloader is, that someone who is not allowed to can modify the firmware very simple. This can be done via PC side attack (virus) or via physical attack.

   To ensure that the firmwares integrity is intact we need to ensure that the bootloader is not hackable from various attacks. Furthermore it should secure the flashing of new firmwares and check their integrity.

   The following attack scenarios should give you an overview of possible attacks and the solutions/explanation what prevents the attack.

  ### The attack scenario
   The attacker could be someone getting access to the device (that holds your passwords), modify it, add a backdoor and give it back to you. You then use the firmware to save your passwords, his backdoor writes them in EEPROM and downloads them later again. An evil colleague could be that guy. Or someone who gets the device when you are away/while shipping it to you.

   This is an active attack where the attacker also somehow needs to get back the data later.
   The encrypted data on the device is still secure unless the malicious firmware backdoors it.

  ###Bootloader tasks:
   * Check firmware integrity (checksum) at any time from PC
   * Check bootloader and firmware integrity at boot
   * Upload a new firmware from authorized people

  TODO atgtacker gets pc virus

  ### Attacker is able to manipulate the bootloader
   As part of the bootloader tasks, it should check the firmware for any unauthorized changes.
   A modified bootloader could not check the firmware integrity (checksum) reliable.
   It could also introduce a backdoor to upload malicious firmware while keeping the previous checksum.

   An attacker could abuse this to make the firmware itself not trusted anymore.
   This could be done by getting physical access to the device when carrying it with you or even while shipping it to you.

   Therefor it is important that the bootloader integrity itself needs to be considered secure.
   There should be no way to modify the bootloader by anyone.

   **The initial bootloader integrity after manufacturing and the device is not faked is assumed here.**

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
