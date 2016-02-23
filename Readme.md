# SecureLoader

The idea is to create an USB HID bootloader that can easily upgrade an AVR
firmware with even increasing the security of the device and its firmware. The
bootloader protects against potential backdoors and malicious firmware upgrades.
This paper describes use cases, features, potential attacks and solutions to
prevent them.

This Readme describes a **temporary concept** with some ideas.
It is **not final**. Contributions appreciated.

## 1. Bootloader Overview

### Bootloader Use Case
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
* The security also relies on the firmware (firmware authenticity!).
* The Bootloader Key is changed after every firmware upgrade.

Worst case scenario assumptions (might differ to the real world).
* The firmware handles secure information that a firmware backdoor could leak.
* The attacker has full physical access of the device.
* The device can be opened and an ISP can be used without a visible change.
* The attacker is able to steal the device and put it back at any time.
* The uploading PC is compromised when doing firmware upgrades.

## Provided Guarantees TODO security features / Concept

**TODO just link to technical details and dont duplicate information**

1. Unauthorized Firmware Upgrade/Downgrade Protection


Bootloader/Device Authenticity Protection
ISP Protection
Compromised PC protection

Firmware Authenticity Protection
Bootloader Key Protection

Hacking the Bootloader from the Firmware Protection

Flash Corruption Protection
Firmware Brick Protection
Open Source Guarantee

TODO remove those?
Passive Attack Protection
Active Attack Protection

### Bootloader/Device Authenticity Protection
The bootloader and device authenticity is ensured through the Bootloader Key.
TODO link

#### Bootloader Key Protection
Each device comes with a unique Bootloader Key that was set by the vendor. The
vendor is responsible for keeping the BK secret and also maintains firmware
updates. The responsibility can be transferred to the user (and also back to the
vendor). You still have firmware authentication protection if the Bootloader Key
was leaked.

In order to keep the BK secure
ISP Protection
Firmware protection


TODO see BK protection?

Overwriting the bootloader
via ISP will also overwrite the FID Hash and BK. **The BK needs to be kept
highly secure.**

You also want to check bootloader authenticity if your FID Hash changed. The
bootloader can authenticate itself to the PC via an authentication package. Even
though the bootloader can be authenticated manually the **firmware should also
authenticate itself it at every boot**.

TODO link, TODO do we implement this?
Or rather use UID?




### ISP Protection
TODO FID, Bootloader authenticity, fuses
ISP also requires physical access to the PCB which can be visually noticed on some devices.


### Firmware Authenticity Protection
You will notice a firmware change because the FID Hash has changed. This check
has to be done by the user at every boot and needs to be coded in the firmware.
You will also notice this if a new bootloader was burned. To check the firmwares
authenticity you can always read the checksum from bootloader. This way you can
ensure that the bootloader did not manipulate the firmware.




### Compromised PC protection
TODO
Firmware upgrade/downgrade protection.
Firmware checksum.
Firmware ID Hash.
Bootloader initiation protection via physical button press.




### Unauthorized Firmware Upgrade/Downgrade Protection
Only signed firmwares can be flashed with the bootloader.
You can even flash the device from a not trusted PC.

Every firmware needs to be signed with the Bootloade Key.
The BK is kept secure by the vendor or by the user.

Flashing a new firmware will also change the firmware ID Hash (TODO link).
The user is able notice a firmware change, even with an ISP.

Firmware downgrades (replay attacks) are prevented via bootloader key changes.

A compromised PC cannot initiate a firmware upgrade.
The bootloader can only be started via a physical key press.





### Hacking the Bootloader from the Firmware Protection
If someone is able to upload malicious firmware to the device he needs access to
the BK. If he has got the BK, he could simply fake and burn a new (fake)
bootloader instead. Therefor it is mostly useless to hack the bootloader from
the firmware except if opening the device to ISP can be visually noticed.

This attack scenario concentrates more on bootloader hacking via firmware
vulnerabilities. Even if a firmware vulnerability was found you can hardly hack
the bootloader. AVR use
[Harvard architecture](https://en.wikipedia.org/wiki/Harvard_architecture), not
[Von Neumann architecture](https://en.wikipedia.org/wiki/Von_Neumann_architecture).
Also you can do a firmware upgrade to get rid of the security vulnerability.

But the bootloader prevents from uploading unauthorized firmware anyways. You
first have to leak the Bootloader Key. And then also the Firmware authenticity
protection will take account of this. Apart from this you should check and apply
security firmware upgrades regularly.







### Passive Attack Protection
The firmware is responsible for passive attacks such as securing sensible data.

### Active Attack Protection
SecureLoader protects against several active attacks as listed above and below.
See [Attack Scenario](TODO) for a worst case attack scenario.



### Flash Corruption Protection
* Brown out detection (Fuse)
* Secure bootloader section (SBS)
* Power on self test (POST)

### Firmware Brick Protection
If you upload a bricked firmware it is always possible to enter the bootloader
again. The bootloader does **not rely on any part of the firmware**. Then you
can upload another firmware instead and continue testing. You should not lose
you BK to be able to upload another firmware again.

### Open Source Guarantee
The bootloader design is open source. This means it can be reviewed by many people.
Preventing flashing unauthorized firmware does not essentially restrict custom firmwares. TODO link

You are still able to burn again the bootloader on your own.
Keep in mind that the FID Hash will change and all bootloader and firmware data will be lost.


## 3. Technical Details

1. Bootloader Overview
 1. Boot Process
 2. Secure Bootloader Section (SBS)
 3. Bootloader Jump Table (BJT)
 4. Power on Self Test (POST)
 5. Recovery Mode
 6. Crypto Algorithms
2. Bootloader Key (BK)
 1. Overview
 2. No Bootloader Key set
 3. Initial Bootloader Key
 4. Change the Bootloader Key
 5. Authenticate the Bootloader to the PC
3. Firmware Upgrade
 1. Overview
 2. Firmware Upgrade Sequence
 3. Firmware Checksum (FW Checksum)
 4. Firmware Counter (FWC)
 5. Firmware Violation Counter (FWVC)
4. Firmware Authentication
 1. Overview
 2. Firmware Identifier (FID)
 3. Firmware ID Hash (FID Hash)
6. Fuse Settings
 1. Overview
 2. Fuse Explanation
 3. Lock Bit Explanation

### 3.1 Bootloader Overview

### 3.1.1 Boot Process
0. Device startup, always run the bootloader (BOOTRST=0)
1. POST: Check the bootloader and firmware integrity (checksum) at startup
2. Special case if no Bootloader Key was set (after ISP the bootloader)
  1. Force to set a Bootloader Key via USB HID
  2. Clear RAM and reboot via watchdog reset
3. Recovery mode entered (press special hardware keys at startup)
  * Verify the firmwares checksum if the PC requests it TODO link
  * Do a [firmware upgrade](#firmware-upgrade)
  * Change the Bootloader Key with a signed and encrypted new password
  * [Authenticate the bootloader to the PC (via Bootloader Key)](#authenticate-the-bootloader-to-the-pc) TODO???
  * Clear RAM and reboot via watchdog reset
4. Recovery mode not entered (just plug in the device)
  1. ~~Write the firmware identifier into a special ram position~~ TODO
  2. Start the firmware
  3. The user needs to authenticate the firmware TODO link

### 3.1.2 Secure Bootloader Section (SBS)
Secrets are stored inside the protected bootloader flash section:
* Bootloader Checksum
* Bootloader Key
* Firmware Checksum
* Firmware Counter (32bit)
* Firmware Identifier
* Firmware Violation Counter (32bit)

They are stored in **special flash page** to avoid flash corruption of the
bootloader code. The bootloaders flash is protected by fuses and though it is
safe from being read via ISP. **TODO why is it secure from malicious firmware?**
This secure bootloader section is also excluded from the bootloaders checksum.
Typically the last bootloader flash page is used to store the bootloader
settings. The firmware has no direct (read/write) access to the data except the
BJT.

### 3.1.3 Bootloader Jump Table (BJT)
The Bootloader Jump Table is used to call functions of the bootloader from the
firmware. This can be used to implement further security functions inside the
firmware.

Available Jump Table functions are:
* Get FID
* Get FWC
* Get FWVC
* AES

TODO

### 3.1.4 Power on Self Test (POST)
The bootloader checks the bootloader and firmware checksum at every boot to
**prevent flash corruption**. Some parts of the SBS are excluded from this check
like the booloader checksum and the FWVC. Fuse and Lock bits will also be
checked.

### 3.1.5 Recovery Mode
After device startup the bootloader normally executes the firmware after a POST.
To enter the recovery mode you need to manually press a physical button. This
ensures that you cannot enter the recovery mode automatic from the PC side.
Firmware upgrades are done less often so the user have to explicitly call the
recovery mode. A note how to start the recovery mode should be placed inside the
firmware (manual).

After entering the recovery mode the user can TODO list

### 3.1.6 Crypto Algorithms
AES-256 for encrypting
AES-256 -MAC for signing
[AES-256 -CBC-MAC for hashing](https://en.wikipedia.org/wiki/CBC-MAC)

TODO links

### 3.2 Bootloader Key (BK)

#### 3.2.1 Overview
Each device [comes with](TODO) a **secret unique symmetric AES-256 Bootloader Key**.

The BK can be used to **verify the devices authenticity** at any time.
The Bootloader Key is also used to **sign new firmware upgrades**. TODO link.

The **initial BK was set by the vendor** who is also responsible for keeping the
BK secret. The BK can be **changed at any time** and is stored inside the SBS.

The BK needs to be kept **highly secure** and should be changed after exchanging
it over an untrusted connection.

TODO links

#### 3.2.2 No Bootloader Key set
Only after burning the bootloader via ISP no Bootloader Key was set.
If no bootkey was set at startup you need to set a Bootloader Key from the
bootloader via USB HID. It is intended that it is **not possbile to upload a
firmware until a Bootloader Key was set**. It is not possible to remove a
Bootloader Key afterwards. **Never ship a device without an inital vendor
Bootloader Key.** TODO link

#### 3.2.3 Initial Bootloader Key
A **unique BK is initially set by the vendor** who is also responsible for
keeping the BK secret. This ensures the **integrity of the initial bootloader**.
The initial Bootloader Key can be used to provide firmware upgrades without
exposing the BK to the user and enable firmware upgrades on untrusted Computers.
The responsibility of the BK can be transferred to any other user but should be
changed after exchanging it over a not trusted connection (Internet!).

#### 3.2.4 Change the Bootloader Key
You need to change the Bootloader Key from the bootloader if no one was set yet.
Also you might want to change it for security purposes or add an initial
Bootloader Key. This is useful when setting an initial vendor Bootloader Key.

To change the BK you need to **encrypt the new BK with the old BK**. Enter the
Recovery Mode and instruct a BK change command with the new (encrypted) BK.

#### 3.2.5 Authenticate the Bootloader to the PC

**Discussion: Is this feature essential?** TODO LINK

The BK can be used to **verify the devices authenticity** at any time.
This can be used for example after receiving the device from the vendor or after a FID Hash violation. Even a 1:1 copy of the device
could be noticed through the secret Bootloader Key.
TODO just add a link and move information

The Bootloader Key is used to **authenticate the bootloader to the PC**.
The PC sends a random challenge to the bootloader which it has to hash with the BK.
The bootloaders integrity is then ensured, the bootloader can be trusted.
TODO a long timeout/password is required here to not brute force all combinations

you (vendor) send the user a firmware, a firmware hash, a new encrypted password, 2 authentication challenges and 1 expected challenge answer.

  the flashing tool sends the 2 challenges and the bootloader has to answer both. with the 1st you can trust the bootloader if you trust your pc. with the 2nd you can send the answer to the vendor and he can verify this challenge as well.

The (symmetric key) should **not always** be used to authenticate the bootloader at **every startup**, since the PC then always have the key stored, use the firmware instead for this purpose. Also then it does not rely on the pc and is simpler to use.

You could (and can) do this with the bootloader.
For usability it is very bad though, because you always need a PC with an app
that checks the bootloader integrity first and then firmware.
Also it might be considered to be a security risk to always use a PC
with a symmetric key to authenticate the device.

The concept is, that the bootloader can be considered trusted. dont we need this then?

**A firmware bug could leak the FID hash or UID. The only way to verify the device is the Bootloader Key.**

Other option via UID (one time only)
1) enter uid request code in the app, press enter
2) check the returned code is the good one
3) add mooltipass user
4) remember hash

### 3.3 Firmware Upgrade

#### 3.3.1 Overview
It is very important to **only upload trusted firmware** from an untrusted PC
while still keeping the **ability to create custom firmwares** and dont lock
out firmware developers.

The firmware upgrade only accepts signed firmware by the Bootloader Key.
Signing the firmware authenticates the firmware (rather than the PC) to the
bootloader. This means you can create new firmwares from a trusted PC and do
**firmware upgrades from an untrusted PC**. The vendor can provide firmware
upgrades without leaking the secret Bootloader Key.

The vendor still can give away the initial Bootloader Key to the user if he
requests it. Then the user can **compile and sign his own firmwares** and play
with the device. Then the user is responsible for further firmware upgrades.
After exchanging the initial Bootloader Key from the vendor the user should
change it.

To prevent replay attacks (firmware downgrades) the BK should be changed before
each upload. This ensures that a new Bootloader Key was changed and an old
firmware can not be used again. The new firmware has to be signed with the new
Bootloader Key. The vendor can force a BK change to upload a new firmwares.
Developers (who own the BK) do not have to change the BK for each upload.

#### 3.3.2 Firmware Upgrade Sequence
1. Receive signed firmware checksums from the PC
2. Verify the authenticity of the firmware checksums
3. Receive next firmware page from the PC and verify the page checksum
4. Abort if the checksum is invalid
5. Flash the valid page
6. Verify the whole firmware checksum
7. Abort and delete the whole firmware if the checksum is invalid
8. Write new firmware identifier and new firmware counter

#### 3.3.3 Firmware Checksum (FW Checksum)
The Firmware Checksum is generated after uploading a new firmware by the
bootloader and stored in the SBS. The PC can **verify the Firmware Integrity**
with the Firmware Checksum and the Firmware Counter. The Firmware checksum is
used as part of the POST and the FID.

**Signed Firmware Checksums Content**
* A checksum for each firmware page
* A checksum of the whole firmware
* A signature of the checksums, signed with the Bootloader Key

If one checksum is valid all other checksums should be valid too. Otherwise
there was an error while creating the checksums or the signing algorithm is
insecure. If the uploading failed a flag will be set to let the firmware notice
the upload violation.

TODO how to remove this flag, only authorized people should be able to?

TODO hash algorithm: crc32?

#### 3.3.4 Firmware Counter (FWC)
The bootloader keeps track of the number of firmware uploads. This information
is important to check if someone even tried to hack your device. The firmware
counter is part of the FID, stored inside the SBS and 32 bit large.

#### 3.3.5 Firmware Violation Counter (FWVC)
The Firmware Violation Counter keeps track of the number of firmware uploads
that fail. This can happen if the signature or the checksums of the firmware are
wrong. The firmware is able to read this value at any time and can use if for
any user warnings. The FWVC is stored inside the SBS.

TODO Required? Checksum needs to be excluded from this.

### 3.4 Firmware Authentication

#### 3.4.1 Overview
Before the user uses the firmware functions it should verify the firmware
authenticity. Therefor the FID Hash is used and should be accepted by the user.
If the FID Hash is different the firmware was upgraded or otherwise modified.

1. Start the firmware
2. Load the desired user
3. Authenticate the user
4. Get the FID
5. Generate FID Hash from FID, User nonce and another nonce
6. User verifies the FID Hash

### 3.4.2 Firmware Identifier (FID)
The firmware identifier lets the firmware authenticate itself to the user. It is
a **unique ID** generated by the bootloader for **each firmware flash**, stored
within the secure bootloader section and passed to the firmware via RAM. The FID
consists of the firmware checksum, firmware counter and a nonce.

Uploading a new firmware or bootloader will destroy the FID and a **violation of
the firmware authenticity can be noticed** by the user. Even uploading the same
firmware twice will result in a different FID. The FID is used in the Firmware
ID Hash and securely stored inside the SBS.

TODO FID byte size (16 or 32 bit. 16 bit should be enough for 10k write cycles)

#### 3.4.3 Firmware ID Hash (FID Hash)
The **firmware is responsible** to authenticate itself with the use of the FID.
It should use a Firmware ID Hash to authenticate itself to the user. **Only
authorized people** should be able to verify (see) the Firmware ID Hash.

The Firmware ID Hash consists of the FID, a nonce and (optional) the user nonce.
This way the nonce can be changed at any time if an unauthorized people sees the
FID Hash. The firmware will display the FID Hash and the **user needs to verify
it**.

The advantage is that the firmware has access to the special hardware such as
display and smartcard and makes it easier for the user to control. The
authentication mechanism is not described here and part of the firmware.

If the firmware is not trusted, you can use the bootloader again to verify the
bootloader and the firmware integrity. This needs to be done explicit when the
user runs the bootloader in its recovery mode.

**Conclusion:
The security also relies on the firmware, not only the bootloader!**

### 3.6 Fuse Settings

#### 3.6.1 Overview
ATmega32u4 fuse settings:
* Low: 0xFF
* High: 0xD8
* Extended: 0xF8
* Lock: 0xCC

#### 3.6.2 Fuse Explanation
- Ext. Crystal Osc.; Frequency 8.0- MHz; Start-up time: 16K CK + 65 ms;
  [CKSEL=1111 SUT=11]
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

See [AVR Fuse Calculator](http://www.engbedded.com/fusecalc/) for more
information.

#### 3.6.3 Lock Bit Explanation
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

## FAQ

### Why not Public Key/Private Key?
Asymmetric encryption/signing is not required as the BK is considered to be kept secure.
An exchange via an insecure channel is not required.
If the vendor gives the user the initial BK you can change the BK afterwards.
Symmetric AES implementation is smaller and can be reused in the firmware.
An asymmetric signing could make the Bootloader Key authentication simpler though.

### Why not only allow signed Firmwares?
TODO this is wrong
As a developer I like to play with open source devices. The user should also be able to make use of the bootloader. It does not lower the security, if the user carefully checks the firmware checksum on the PC. This needs to be integrated (forced) into the flashing tool.

### I lost my Bootloader Key. What can I do?
The BK is normally **maintained by the vendor**. Ask him first for the initial BK.
There is **no way to recover a Bootloader Key** if you have changed the initial vendor BK.
You can continue to use the current firmware but wont be able to upload any new firmware.
You might want to check the firmware authenticity first to exclude a faked the bootloader.
You may use an ISP to burn a new bootloader, but this will **destroy any data** on the AVR.

## Links

### Security
* [Atmel AVR231: AES Bootloader](http://www.atmel.com/images/doc2589.pdf)
* [AVR230: DES Bootloader](http://www.atmel.com/images/doc2541.pdf)
* [Overwriting a Protected AVR Bootloader](http://hackaday.com/2014/07/05/overwriting-a-protected-avr-bootloader/)
* [BootJacker: The Amazing AVR Bootloader Hack!](http://oneweekwonder.blogspot.de/2014/07/bootjacker-amazing-avr-bootloader-hack.html)
* [NaCl](https://nacl.cr.yp.to/box.html)
* [AVRNaCl](http://munacl.cryptojedi.org/atmega.shtml)
* ["Key-logger, Video, Mouse" Talk at 32c3](https://media.ccc.de/v/32c3-7189-key-logger_video_mouse)

### AVR Bootloaders
* [avr-libc Bootloader Support Utilities](http://www.nongnu.org/avr-libc/user-manual/group__avr__boot.html)
* [Bootloader FAQ](http://www.avrfreaks.net/forum/faq-c-writing-bootloader-faq?page=all)
* [AVR Bootloader in C - eine einfache Anleitung](https://www.mikrocontroller.net/articles/AVR_Bootloader_in_C_-_eine_einfache_Anleitung)
* [All you need to know about AVR fuses](http://www.embedds.com/all-you-need-to-know-about-avr-fuses/)
* [Engbedded Atmel AVR Fuse Calculator](http://www.engbedded.com/fusecalc/)


## Version History

TODO

## License and Copyright
TODO
GPL incompatible with CDDL

## Discussion

### Authenticate the Bootloader to the PC

**Discussion: Is this feature essential?**

#### Pro
1. It might not use much flash
2. Increases Security (authenticity)
3. Bootloader can be verified before flashing new firmware
4. Not all people might care (remember) about FID Hash. BK authenticity can always be used.
5. Device authentication should not rely on firmware
6. You can use it to let the BK owner verify the bootloader (send the response back to him). Assumes emails cannot be faked.
7. Adds another layer of security BESIDE FID Hash
8. Note: The FID should still be used for all day use. Bootloader is ment for FW upgrades and recovery mode.
9. The concept is, that the bootloader can be considered trusted. This is then essential if you not want to rely on firmware.
10. **A firmware bug could leak (or display wrong) the FID hash or UID. The only way to verify the device is the Bootloader Key.**

#### Cons
1. Can be done with Firmware
2. FID Hash can also be used to ensure authenticity
3. A compromised PC could always say the bootloader is okay
4. Alternative is a firmware side authentication via UID

#### Compromis
It does not hurt to include if enough flash is available

### AES placed into bootloader or firmware section

#### Arguments for bootloader section
1. its better to NEVER rely on the firmware. otherwise you can brick the whole device. I listed firmware bricking Guarantee as feature so far
This is the most important point for me. See the bootloader FAQ on avr freaks at section 13.
2. The bootloader should just not rely on anything and also should be used flexible in other projects too
3. the bootloader requires at least 2kb~3kb. AES fits anyways (assuming it takes ~1kb)

#### Arguments against bootloader section
1. how can you brick the device if you are guaranteed that the correct firmware has been flashed and
therefore has the correct AES routine. developers will use ISP anyways
2. nothing prevents you to also include it in your bootloader for other uses (if it fits into 2 and 4 kb)
3. but if your bootloader is actually less than 2k we are wasting 1k of flash

#### Compromis
if it fits into 2kb (without aes, what i aim for), we can make mooltipass include it from the firmware.
the firmware can place the AES into the same space where normally the bootloader would be located.
(2more kb for firmware). so even the firmware function calls are at the same location.
we can also do a version that includes the bootloader for people like me ;) (or for other projects)
if it does not fit into 2kb, there is no reason to exclude it (if it does fit into 4kb)

## Other Ideas:
Just some ideas, or maybe things that needs to find their way into the readme.
Unstructured.

  * The bootloader could share its AES implementation to the firmware code.
  * the bootloader recovery mode can only be entered via physical button press for more security
  * Do not leave any trace in EEPROM or RAM
  * Add an user interface to download the firmware again
  * bootloader Authentication needs to be discussed
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
 * Add implementation requirements checklist of all features
