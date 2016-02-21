# SecureLoader

The idea is to create an USB HID bootloader that can easily upgrade an AVR firmware
with even increasing the security of the device and its firmware.
The bootloader protects against potential backdoors and malicious firmware upgrades.
This paper describes use cases, features, potential attacks and solutions to prevent them.

This readme describes a **temporary concept** with some ideas.
It is **not final**. Contributions appreciated.

## Bootloader overview

### Bootloader use case
* Security updates
* Feature updates
* Uploading a self recompiled firmware
* Developing new firmware features
* Verify bootloader and firmwares authenticity and integrity

### Bootloader Properties
* Protects firmwares confidentiality, authenticity, integrity
* Uses USB HID Protocol
* No special drivers required
* Coded for AVR USB Microcontrollers
* Optimized for 32u4
* Fits into 4kb bootloader section TODO
* Based on LUFA
* Reusable AES implementation
* Open Source

### Attack Scenario
Conditions for device security:
* The AVR is programmed with the correct fuses.
* The initial password is kept secure by the vendor until the user requests it.
* The security also relies on the firmware, not only the bootloader (firmware authenticity)!
* The bootloader key is changed after every firmware upgrade.

The following assumptions describe a worst case scenario and might differ to the real world.
* The firmware handles secure information that can be leaked via a firmware backdoor.
* The attacker has full physical access of the device.
* The device can be opened and an ISP can be used without a visible change.
* The attacker is able to steal the device and put it back at any time.
* The uploading PC is compromised when doing firmware upgrades.

## Technical Details

### Boot Process
0. Device startup, always run the bootloader (BOOTRST=0)
1. POST: Check the bootloader and firmware integrity (checksum) at startup
2. Special case if no bootloader key was set (after ISP the bootloader)
  1. Force to set a bootloader key via USB HID
  2. Clear RAM and reboot via watchdog reset
3. Recovery mode entered (press special hardware keys at startup)
  * Verify the firmwares checksum if the PC requests it TODO link
  * Do a [firmware upgrade](#firmware-upgrade)
  * Change the bootloader key with a signed and encrypted new password
  * [Authenticate the bootloader to the PC (via bootloader key)](#authenticate-the-bootloader-to-the-pc) TODO???
  * Clear RAM and reboot via watchdog reset
4. Recovery mode not entered (just plug in the device)
  1. Write the firmware identifier into a special ram position
  2. Start the firmware
  3. The user needs to authenticate the firmware TODO link

### Power on self test (POST)
The bootloader checks the bootloader and firmware checksum at every boot to **prevent flash corruption**.
Some parts of the SBS are excluded from this check like the booloader checksum and the FWVC.
Fuse and Lock bits will also be checked.

### Recovery mode
After [device startup](TODO) the bootloader normally executes the firmware after a POST.
To enter the recovery mode you need to manually press a physical button.
This ensures that you cannot enter the recovery mode automatic from the PC side.
Firmware upgrades are done less often so the user have to explicitly call the recovery mode.
A note how to start the recovery mode should be placed inside the firmware (manual).

After entering the recovery mode the user can TODO

### Secure bootloader section (SBS)
The bootloader has to store a few settings in the protected bootloader flash section:
* Bootloader Checksum
* Bootloader Key
* Firmware Checksum
* Firmware Counter (32bit)
* Firmware Identifier
* Firmware Violation Counter (32bit)

They are stored in **special flash page** to avoid flash corruption of the bootloader code.
The bootloaders flash is protected by fuses and though it is safe from being read via ISP.
**TODO why is it secure from malicious firmware?**
This secure bootloader section is also excluded from the bootloaders checksum.
Typically the last bootloader flash page is used to store the bootloader settings.
The firmware has no direct (read/write) access to the data except the BJT.

### Bootloader Jump Table (BJT)
The Bootloader Jump Table is used to call functions of the bootloader from the firmware.
This can be used to implement further security functions inside the firmware.

Available Jump Table functions are:
* Get FID
* Get FWC
* Get FWVC
* AES

TODO

### Bootloader key (BK)
The Bootloader Key is used to sign the firmware and every bootloader related crypto.
Initially the vendor set a BK to verify the device authenticity and integrity.
The BK can be changed at any time and is stored inside the SBS.
The BK needs to be kept **highly secure** and should be changed be changed after exchanging.

TODO length
TODO encryption algorithm
TODO used for: Authenticate the bootloader to the PC

#### Change the bootloader key
You need to change the bootloader key from the bootloader if no one was set yet.
Also you might want to change it for security purposes or add an initial bootloader key.
This is useful when setting an initial vendor bootloader key.

#### No bootloader key was set
Only after burning the bootloader via ISP no bootloader key was set.
If no bootkey was set at startup you need to set a bootloader key from the bootloader via USB HID.
It is intended that it is **not possbile to upload a firmware until a bootloader key was set**.
It is not possible to remove a bootloader key afterwards.
**Never ship a device without an inital vendor bootloader key.** TODO link

#### Initial bootloader key
A **unique BK is initially set by the vendor** and can be exchanged **after receiving the device**.
This ensures the **integrity of the initial bootloader** that is stored inside the SBS.
The initial bootloader key can be used to provide firmware upgrades without leaking the BK.

#### Authenticate the bootloader to the PC

**Discussion: is this feature essential? This section contains some thought trash.**

The Bootloader key is used to **authenticate the bootloader to the PC**.
The PC sends a random challenge to the bootloader which it has to hash with the BK.
The bootloaders integrity is then ensured, the bootloader can be trusted.
TODO a long timeout/password is required here to not brute force all combinations

TODO is this essential? discuss with other people
* pro: This can be used to verify the bootloaders integrity after receiving the device.
* contra: optionally this could be done (once) via the flashed firmware
* pro: you can verify the bootloader before each uploading(send the expected response with the challenge)
* contra: a compromised pc could always say the bootloader is okay
* pro: you can use it to let the BK owner verify the bootloader (send the response back to the vendor)
* contra: it is not simple and a compromised pc could fake the email back.

you (vendor) send the user a firmware, a firmware hash, a new encrypted password, 2 authentification challenges and 1 expected challenge answer.

  the flashing tool sends the 2 challenges and the bootloader has to answer both. with the 1st you can trust the bootloader if you trust your pc. with the 2nd you can send the answer to the vendor and he can verify this challenge as well.

The (symmetric key) should **not always** be used to authenticate the bootloader at **every startup**, since the PC then always have the key stored, use the firmware instead for this purpose

You could (and can) do this with the bootloader.
For usability it is very bad though, because you always need a PC with an app
that checks the bootloader integrity first and then firmware.
Also it might be considered to be a security risk to always use a PC
with a symmetric key to authenticate the device.

The concept is, that the bootloader can be considered trusted. dont we need this then?

**A firmware bug could leak the FID hash or UID. The only way to verify the device is the bootloader key.**

Other option via UID (one time only)
1) enter uid request code in the app, press enter
2) check the returned code is the good one
3) add mooltipass user
4) remember hash

#### Authenticate the PC to the bootloader
~~It also prevents attackers from uploading new firmwares to the device.
Before the PC can upload a firmware the **PC has to authenticate itself to the bootloader**.
With the same symmetric BK the PC answers a challenge from the bootloader.

TODO on wrong authentication set some flag, that the firmware can read next time
TODO a good random challenge is required here~~


### Firmware upgrade
It is very important to **only upload trusted firmware** from an untrusted PC
while still keeping the **ability to create custom firmwares** and dont lock out firmware developers.

The firmware upgrade only accepts signed firmware by the bootloader key.
Signing the firmware authenticates the firmware (rather than the PC) to the bootloader.
This means you can create new firmwares from a trusted PC and do **firmware upgrades from an untrusted PC**.
The vendor can provide firmware upgrades without leaking the secret bootloader key.

The vendor still can give away the initial bootloader key to the user if he requests it.
Then the user can **compile and sign his own firmwares** and play with the device.
Then the user is responsible for further firmware upgrades.
After exchanging the initial bootloader key from the vendor the user should change it.

To prevent replay attacks (firmware downgrades) the BK should be changed before each upload.
This ensures that a new bootloader key was changed and an old firmware can not be used again.
The new firmware has to be signed with the new bootloader key.
The vendor can force a BK change to upload a new firmwares.
Developers (who own the BK) do not have to change the BK for each upload.

##### Firmware upgrade sequence
1. Receive signed firmware checksums from the PC
2. Verify the authenticity of the firmware checksums
3. Receive next firmware page from the PC and verify the page checksum
4. Abort if the checksum is invalid
5. Flash the valid page
6. Verify the whole firmware checksum
7. Abort and delete the whole firmware if the checksum is invalid
8. Write new firmware identifier and new firmware counter

##### Signed firmware checksums content
* A checksum for each firmware page
* A checksum of the whole firmware
* A signature of the checksums, signed with the bootloader key

If one checksum is valid all other checksums should be valid too.
Otherwise there was an error while creating the checksums or the signing algorithm is insecure.
If the uploading failed a flag will be set to let the firmware notice the upload violation.

TODO how to remove this flag, only authorized people should be able to?

### Firmware Checksum (FW Checksum)
The Firmware Checksum is generated after uploading a new firmware by the bootloader and stored in the SBS.
The PC can **verify the Firmware Integrity** with the Firmware Checksum and the Firmware Counter.
The Firmware checksum is used as part of the POST and the FID.

TODO hash algorithm: crc32?

### Firmware Counter (FWC)
The bootloader keeps track of the number of firmware uploads.
This information is important to check if someone even tried to hack your device.
The firmware counter is part of the FID, stored inside the SBS and 32 bit large.

### Firmware Violation Counter (FWVC)
The Firmware Violation Counter keeps track of the number of firmware uploads that fail.
This can happen if the signature or the checksums of the firmware are wrong.
The firmware is able to read this value at any time and can use if for any user warnings.
The FWVC is stored inside the SBS.

TODO Required? Checksum needs to be excluded from this.

### Firmware Identifier (FID)
The firmware identifier lets the firmware authenticate itself to the user.
It is a **unique ID** generated by the bootloader for **each firmware flash**,
stored within the secure bootloader section and passed to the firmware via RAM.
The FID consists of the firmware checksum, firmware counter and a nonce.

Uploading a new firmware or bootloader will destroy the FID and
a **violation of the firmware authenticity can be noticed** by the user.
Even uploading the same firmware twice will result in a different FID.
The FID is used in the Firmware ID Hash and securely stored inside the SBS.

TODO FID byte size (16 or 32 bit. 16 bit should be enough for 10k write cycles)

### Firmware ID Hash (FID Hash)
The **firmware is responsible** to authenticate itself with the use of the FID.
It should use a Firmware ID Hash to authenticate itself to the user.
**Only authorized people** should be able to verify (see) the Firmware ID Hash.

The Firmware ID Hash consists of the FID, a nonce and (optional) the user nonce.
This way the nonce can be changed at any time if an unauthorized people sees the FID Hash.
The firmware will display the FID Hash and the **user needs to verify it**.

The advantage is that the firmware has access to the special hardware
such as display and smartcard and makes it easier for the user to control.
The authentication mechanism is not described here and part of the firmware.

If the firmware is not trusted, you can use the bootloader again
to verify the bootloader and the firmware integrity.
This needs to be done explicit when the user runs the bootloader in its recovery mode.

**Conclusion: The security also relies on the firmware, not only the bootloader!**

### Firmware Authentification
Before the user uses the firmware functions it should verify the firmware authenticity.
Therefor the FID Hash is used and should be accepted by the user.
If the FID Hash is different the firmware was upgraded or otherwise modified.

1. Start the firmware
2. Loade the desired user
3. Authenticate the user
4. Get the FID
5. Generate FID Hash from FID, User nonce and another nonce
6. User verifies the FID Hash

### Fuse Settings

ATmega32u4 fuse settings:
* Low: 0xFF
* High: 0xD8
* Extended: 0xF8
* Lock: 0xCC

#### Fuse explanation
- Ext. Crystal Osc.; Frequency 8.0- MHz; Start-up time: 16K CK + 65 ms; [CKSEL=1111 SUT=11]
- [ ] Clock output on PORTC7; [CKOUT=0]
- [ ] Divide clock by 8 internally; [CKDIV8=0]
- [x] Boot Reset vector Enabled (default address=$0000); [BOOTRST=0]
- Boot Flash size=2048 words start address=$3800; [BOOTSZ=00]
- [ ] Preserve EEPROM memory through the Chip Erase cycle; [EESAVE=0]
- [ ] Watchdog timer always on; [WDTON=0]
- [x] Serial program downloading (SPI) enabled; [SPIEN=0]
- [ ] JTAG Interface Enabled; [JTAGEN=0]
- [ ] On-Chip Debug Enabled; [OCDEN=0]
- Brown-out detection level at VCC=4.3V; [BODLEVEL=000]
- [ ] Hardware Boot Enable; [HWBE=0]

See [AVR Fuse Calculator](http://www.engbedded.com/fusecalc/) for more information.

#### Lock bit explanation
- [x] Lock Bit Protection Modes (Memory Lock); [BLB0=00]
- [ ] Boot Lock Bit0 Protection Modes (Application Section); [BLB0=11]
- [x] Boot Lock Bit1 Protection Modes (Boot Loader Section); [BLB1=00]

```
32u4 Datasheet Page 346: Table 28-2. Lock Bit Protection Modes

LP Mode 3:
Further programming and verification of the Flash and EEPROM is
disabled in Parallel and Serial Programming mode. The Boot Lock
bits and Fuse bits are locked in both Serial and Parallel
Programming mode.

BLB0 Mode 1:
"No restrictions for SPM or (E)LPM accessing the Application
section."

BLB1 Mode 3:
"SPM is not allowed to write to the Boot Loader section,
and (E)LPM executing from the Application section is not
allowed to read from the Boot Loader section. If Interrupt
Vectors are placed in the Application section, interrupts
are disabled while executing from the Boot Loader section."
```

## Provided guarantees TODO security featurs

### Flash corruption protection
* Brown out detection
* Secure bootloader section
* Power on self test

TODO links

### Unauthorized firmware upgrade/downgrade protection
Only signed firmwares can be flashed with the bootloader.
You can even flash the device from a not trusted PC.

The bootloader key to sign the firmware needs to be kept secure.
This can be handled by the vendor or the user.

Flashing a new firmware will also change the firmware ID Hash (TODO link).
The user is able notice a firmware change, even with an ISP.

Firmware downgrades (replay attacks) are prevented via bootloader ket changes.

A compromised PC cannot initiate a firmware upgrade.
The bootloader can only be started via a physical key press.

### Hacking the bootloader from the firmware protection
If someone is able to upload malicious firmware to the device he needs access to the BK.
If he has got the BK, he could simply fake and burn a new (fake) bootloader instead.
Therefor it is mostly useless to hack the bootloader from the firmware.
except if opening the device to ISP can be visually noticed.

This attack scenario concentrates more on bootloader hacking via firmware vulnerabilities.
Even if a firmware vulnerability was found you can hardly hack the bootloader.
AVR use [harvard architecture](https://en.wikipedia.org/wiki/Harvard_architecture),
not [Von Neumann architecture](https://en.wikipedia.org/wiki/Von_Neumann_architecture).
Also you can do a [firmware upgrade](TODO) to get rid of the security vulnerability.

But the bootloader prevents from [uploading unauthorized firmware](TODO) anyways.
You first have to leak the bootloader key.
And then also the [Firmware authenticity protection](TODO) will take account of this.
Apart from this you should check and apply security firmware upgrades regularly.

### Firmware authenticity protection
You will notice a firmware change because the FID Hash has changed.
This check has to be done by the user at every boot and needs to be coded in the firmware.
You will also notice this if a new bootloader was burned.
To check the firmwares authenticity you can always read the checksum from bootloader.
This way you can ensure that the bootloader did not manipulate the firmware.

### Bootloader authenticity protection
Overwriting the bootloader via ISP will also overwrite the FID Hash and BK.
ISP also requires access to the PCB which can be visually noticed on some devices.
Bootloader authenticity can be checked from the bootloader. TODO link, TODO do we implement this?
This can be used to verify the device after receiving it from the vendor.
Even a 1:1 copy of the device could be noticed through the bootloader key.
Even though the bootloader can be authenticated the firmware should also do it at every boot.

### Device authenticity protection
Bootloader authenticity can be used with the bootloader.
This way you can securely ship the device and verify its authenticity.
TODO link, TODO do we implement this?
Or rather use UID?

### Bootloader key protection
Each device comes with a unique bootloader key that was set by the vendor.
The vendor is responsible for keeping the BK secret and also maintains firmware updates.
The responsibility can be transferred to the user (and also back to the vendor).
You still have [firmware authentication protection](TODO) if the bootloader key was leaked.

### Passive Attack Protection
The firmware is responsible for passive attacks such as securing sensible data.

### Active Attack Protection
SecureLoader protects against several active attacks as listed above and below.
See [Attack Scenario](TODO) for a worst case attack scenario.

### Compromised PC protection
TODO
Firmware upgrade/downgrade protection.
Firmware checksum.
Firmware ID Hash.
Bootloader initiation protection via physical button press.

### Firmware Brick Protection
If you upload a bricked firmware it is always possible to enter the bootloader again.
Then you can upload another firmware instead and continue testing.
You should not lose you BK to be able to upload another firmware again.

### Open Source Guarantee
The bootloader design is open source. This means it can be reviewed by many people.
Preventing flashing unauthorized firmware does not essentially restrict custom firmwares. TODO link

You are still able to burn again the bootloader on your own.
Keep in mind that the FID Hash will change and all bootloader and firmware data will be lost.

### Links

#### Security
* [Atmel AVR231: AES Bootloader](http://www.atmel.com/images/doc2589.pdf)
* [AVR230: DES Bootloader](http://www.atmel.com/images/doc2541.pdf)
* [Overwriting a Protected AVR Bootloader](http://hackaday.com/2014/07/05/overwriting-a-protected-avr-bootloader/)
* [BootJacker: The Amazing AVR Bootloader Hack!](http://oneweekwonder.blogspot.de/2014/07/bootjacker-amazing-avr-bootloader-hack.html)
* [NaCl](https://nacl.cr.yp.to/box.html)
* [AVRNaCl](http://munacl.cryptojedi.org/atmega.shtml)
* ["Key-logger, Video, Mouse" Talk at 32c3](https://media.ccc.de/v/32c3-7189-key-logger_video_mouse)



#### AVR Bootloaders
* [avr-libc Bootloader Support Utilities](http://www.nongnu.org/avr-libc/user-manual/group__avr__boot.html)
* [Bootloader FAQ](http://www.avrfreaks.net/forum/faq-c-writing-bootloader-faq?page=all)
* [AVR Bootloader in C - eine einfache Anleitung](https://www.mikrocontroller.net/articles/AVR_Bootloader_in_C_-_eine_einfache_Anleitung)
* [All you need to know about AVR fuses](http://www.embedds.com/all-you-need-to-know-about-avr-fuses/)
* [Engbedded Atmel AVR Fuse Calculator](http://www.engbedded.com/fusecalc/)


### License
TODO
GPL incompatible with CDDL


### FAQ

#### Why not PublicKey/PrivateKey?
Asymmetric encryption/signing is not required as the BK is considered to be kept secure.
An exchange via an insecure channel is not required.
If the vendor gives the user the initial BK you can change the BK afterwards.
Symmetric AES implementation is smaller and can be reused in the firmware.
An asymmetric signing could make the bootloader key authentication simpler though.

#### Why not only allow signed firmwares?
TODO this is wrong
As a developer I like to play with open source devices. The user should also be able to make use of the bootloader. It does not lower the security, if the user carefully checks the firmware checksum on the PC. This needs to be integrated (forced) into the flashing tool.

#### I lost my Bootloader key. What can I do?
The BK is normally **maintained by the vendor**. Ask him first for the initial BK.
There is **no way to recover a bootloader key** if you have changed the initial vendor BK.
You can continue to use the current firmware but wont be able to upload any new firmware.
You might want to check the firmware authenticity first to exclude a faked the bootloader.
You may use an ISP to burn a new bootloader, but this will **destroy any data** on the AVR.

### Other ideas:
Just some ideas, or maybe things that needs to find their way into the readme.
Unstructured.

  * The bootloader could share its AES implementation to the firmware code.
  * the bootloader recovery mode can only be entered via physical button press for more security
  * Do not leave any trace in EEPROM or RAM
  * Add an user interface to download the firmware again
  * bootloader authentification needs to be discussed
  * bootloader violation flag pased to firmware?
  -> pass the FID via ram and also anothe byte to identify wrong bootloader attacks, BK forced changed and future use things.
  * A symmetric key for each device requires a lot of different signed firmwares
  * Initial symmetric key **needs to be changed**, after it was exchanged via insecure email
  -> maybe create a secure encrypted "reset password" package. so the key will be displayed on the bootloader rather than visible in an email?
  * Symmetric key should be random and not generate via a serial number algorithm
  * Optional (closed source) encrypted(!) firmwares are possible. reading the firmware back has to be forbidden then. checksum might be allowed though.
  * You need to trust the seller/signing person (or youself if you request the BK)
  * we have a problem if the BK was lost (only reburning via ISP is possible, but a reasonable option. the firmware should be able to do self backups)
  * If bootloader crypto functions are insecure, we have a problem (ISP required for new bootloader. means the secureloader has to be good/veryfied befor using it.)
  * If we use symmetric signing the firmware should not be able to see the secret.
  * opening the avr for xxx$ will leak the BK. this should be noted, maybe as assumption?
  * It is important that a brute forcing the unlock is impossible
  * verifying the firmware after receiving the package should be required or at least noticed somehow.
  * The concept is, that the bootloader can be considered trusted
  * Therefor the **firmware** needs to **authenticate itself** at **every start**. This can be done before any USB communication (fo the firmware) was started.
  * we need to ensure that a fake firmware has no access to the bootloader.
  * A modified bootloader could not check the firmware integrity (checksum) reliable. There should be no way to modify the bootloader by anyone.
  * maybe use something bigger than the 32u4?
  * The user is responsible for not uploading malicious firmware if he owns the BK.
  * Random number generation needs to be secure.
  * assuming a compromised pc does not secure the firmware though obviously
  * Access the FID via bootloader jump table function instead of ram, to make it simpler.
  * Forbit EEPROM reading from the bootloader?
  * Should every bootloader get a unique ID too? this way you can identify the device appart from the sticker on the backside.



### TODO
 * TODO write Firmware Identifier etc capital/idential
 * TODO write headlines capital
 * TODO check integrity and authenticity words if they match
 * TODO numbe the section, to make it more clear what sub headline is for what topic.
 * TODO add a lot of links
 * Add minimum RAM requirement if the bootloader is finished.
