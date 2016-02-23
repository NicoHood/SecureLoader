# SecureLoader

The idea is to create an **USB HID Bootloader** that can easily upgrade an AVR
Firmware with even **increasing the security** of the device and its Firmware.
The Bootloader **protects against potential backdoors and malicious Firmware
upgrades**. This paper describes use cases, features, potential attacks and
solutions to prevent them.

This Readme describes a **temporary concept** with some ideas.
It is **not final**. Contributions appreciated.

## 0. Table of Contents
1. Bootloader Overview
2. Provided Guarantees TODO security features / Concept
3. Technical Details
4. FAQ
5. Links
6. Version History
7. License and Copyright

## 1. Bootloader Overview

### Bootloader Use Case
* Security updates
* Feature updates
* Uploading a self recompiled Firmware
* Developing new Firmware features
* Verify Bootloader and Firmwares authenticity and integrity

### Bootloader Properties
* Protects Firmwares confidentiality, authenticity, integrity
* Uses USB HID Protocol
* No special drivers required
* Coded for AVR USB Microcontrollers
* Optimized for 32u4
* Fits into 4kb Bootloader section TODO
* Based on LUFA
* Reusable AES implementation
* Open Source

### Attack Scenario

#### Conditions for device security:
* The AVR is programmed with the correct fuses.
* The initial password is kept secure by the vendor until the user requests it.
* The security also relies on the Firmware (Firmware authenticity!).
* The Bootloader Key is changed after every Firmware upgrade.

#### Worst case scenario (might differ to the real world):
* The Firmware handles secure information that a Firmware backdoor could leak.
* The attacker has full physical access of the device.
* The device can be opened and an ISP can be used without a visible change.
* The attacker is able to steal the device and put it back at any time.
* The uploading PC is compromised when doing Firmware upgrades.

## 2. Provided Guarantees TODO security features / Concept

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
The Bootloader and device authenticity is ensured through the Bootloader Key.
TODO link

#### Bootloader Key Protection
Each device comes with a unique Bootloader Key that was set by the vendor. The
vendor is responsible for keeping the BK secret and also maintains Firmware
updates. The responsibility can be transferred to the user (and also back to the
vendor). You still have Firmware authentication protection if the Bootloader Key
was leaked.

In order to keep the BK secure
ISP Protection
Firmware protection


TODO see BK protection?

Overwriting the Bootloader
via ISP will also overwrite the FID Hash and BK. **The BK needs to be kept
highly secure.**

You also want to check Bootloader authenticity if your FID Hash changed. The
Bootloader can authenticate itself to the PC via an authentication package. Even
though the Bootloader can be authenticated manually the **Firmware should also
authenticate itself it at every boot**.

TODO link, TODO do we implement this?
Or rather use UID?




### ISP Protection
TODO FID, Bootloader authenticity, fuses
ISP also requires physical access to the PCB which can be visually noticed on some devices.


### Firmware Authenticity Protection
You will notice a Firmware change because the FID Hash has changed. This check
has to be done by the user at every boot and needs to be coded in the Firmware.
You will also notice this if a new Bootloader was burned. To check the Firmwares
authenticity you can always read the checksum from Bootloader. This way you can
ensure that the Bootloader did not manipulate the Firmware.




### Compromised PC protection
TODO
Firmware upgrade/downgrade protection.
Firmware checksum.
Firmware ID Hash.
Bootloader initiation protection via physical button press.




### Unauthorized Firmware Upgrade/Downgrade Protection
Only signed Firmwares can be flashed with the Bootloader.
You can even flash the device from a not trusted PC.

Every Firmware needs to be signed with the Bootloade Key.
The BK is kept secure by the vendor or by the user.

Flashing a new Firmware will also change the Firmware ID Hash (TODO link).
The user is able notice a Firmware change, even with an ISP.

Firmware downgrades (replay attacks) are prevented via Bootloader key changes.

A compromised PC cannot initiate a Firmware upgrade.
The Bootloader can only be started via a physical key press.





### Hacking the Bootloader from the Firmware Protection
If someone is able to upload malicious Firmware to the device he needs access to
the BK. If he has got the BK, he could simply fake and burn a new (fake)
Bootloader instead. Therefor it is mostly useless to hack the Bootloader from
the Firmware except if opening the device to ISP can be visually noticed.

This attack scenario concentrates more on Bootloader hacking via Firmware
vulnerabilities. Even if a Firmware vulnerability was found you can hardly hack
the Bootloader. AVR use
[Harvard architecture](https://en.wikipedia.org/wiki/Harvard_architecture), not
[Von Neumann architecture](https://en.wikipedia.org/wiki/Von_Neumann_architecture).
Also you can do a Firmware upgrade to get rid of the security vulnerability.

But the Bootloader prevents from uploading unauthorized Firmware anyways. You
first have to leak the Bootloader Key. And then also the Firmware authenticity
protection will take account of this. Apart from this you should check and apply
security Firmware upgrades regularly.







### Passive Attack Protection
The Firmware is responsible for passive attacks such as securing sensible data.

### Active Attack Protection
SecureLoader protects against several active attacks as listed above and below.
See [Attack Scenario](TODO) for a worst case attack scenario.



### Flash Corruption Protection
* Brown out detection (Fuse)
* Secure Bootloader section (SBS)
* Power on self test (POST)

### Firmware Brick Protection
If you upload a bricked Firmware it is always possible to enter the Bootloader
again. The Bootloader does **not rely on any part of the Firmware**. Then you
can upload another Firmware instead and continue testing. You should not lose
you BK to be able to upload another Firmware again.

### Open Source Guarantee
The Bootloader design is open source. This means it can be reviewed by many people.
Preventing flashing unauthorized Firmware does not essentially restrict custom Firmwares. TODO link

You are still able to burn again the Bootloader on your own.
Keep in mind that the FID Hash will change and all Bootloader and Firmware data will be lost.


## 3. Technical Details

1. [Bootloader Components](#31-Bootloader-components)
 1. [Boot Process](#311-boot-process)
 2. [Secure Bootloader Section (SBS)](#312-secure-Bootloader-section-sbs)
 3. [Bootloader Jump Table (BJT)](#313-Bootloader-jump-table-bjt)
 4. [Power on Self Test (POST)](#314-power-on-self-test-post)
 5. [Recovery Mode](#315-recovery-mode)
 6. [Crypto Algorithms](#316-crypto-algorithms)
2. [Bootloader Key (BK)](#32-Bootloader-key-bk)
 1. [Overview](#321-overview)
 2. [No Bootloader Key set](#322-no-Bootloader-key-set)
 3. [Initial Bootloader Key](#323-initial-Bootloader-key)
 4. [Change the Bootloader Key](#324-change-the-Bootloader-key)
 5. [Authenticate the Bootloader to the PC](#325-authenticate-the-Bootloader-to-the-pc)
3. [Firmware Upgrade](r#33-Firmware-upgrade)
 1. [Overview](#331-overview)
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

### 3.1 Bootloader Components

#### 3.1.1 Boot Process
0. Device startup, always run the Bootloader (BOOTRST=0)
1. POST: Check the Bootloader and Firmware integrity (checksum) at startup
2. Special case if no Bootloader Key was set (after ISP the Bootloader)
  1. Force to set a Bootloader Key via USB HID
  2. Clear RAM and reboot via watchdog reset
3. Recovery mode entered (press special hardware keys at startup)
  * Verify the Firmwares checksum if the PC requests it TODO link
  * Do a [Firmware upgrade](#Firmware-upgrade)
  * Change the Bootloader Key with a signed and encrypted new password
  * [Authenticate the Bootloader to the PC (via Bootloader Key)](#authenticate-the-Bootloader-to-the-pc) TODO???
  * Clear RAM and reboot via watchdog reset
4. Recovery mode not entered (just plug in the device)
  1. Start the Firmware
  2. The user needs to authenticate the Firmware TODO link

#### 3.1.2 Secure Bootloader Section (SBS)
Secrets are stored inside the protected Bootloader flash section:
* Bootloader Checksum
* Bootloader Key
* Firmware Checksum
* Firmware Counter (32bit)
* Firmware Identifier
* Firmware Violation Counter (32bit)

They are stored in **special flash page** to avoid flash corruption of the
Bootloader code. The Bootloaders flash is protected by fuses and though it is
safe from being read via ISP. **TODO why is it secure from malicious Firmware?**
This secure Bootloader section is also excluded from the Bootloaders checksum.
Typically the last Bootloader flash page is used to store the Bootloader
settings. The Firmware has no direct (read/write) access to the data except the
BJT.

#### 3.1.3 Bootloader Jump Table (BJT)
The Bootloader Jump Table is used to call functions of the Bootloader from the
Firmware. This can be used to implement further security functions inside the
Firmware.

Available Jump Table functions are:
* Get FID
* Get FWC
* Get FWVC
* AES

TODO

#### 3.1.4 Power on Self Test (POST)
The Bootloader checks the Bootloader and Firmware checksum at every boot to
**prevent flash corruption**. Some parts of the SBS are excluded from this check
like the booloader checksum and the FWVC. Fuse and Lock bits will also be
checked.

#### 3.1.5 Recovery Mode
After device startup the Bootloader normally executes the Firmware after a POST.
To enter the recovery mode you need to manually press a physical button. This
ensures that you cannot enter the recovery mode automatic from the PC side.
Firmware upgrades are done less often so the user have to explicitly call the
recovery mode. A note how to start the recovery mode should be placed inside the
Firmware (manual).

After entering the recovery mode the user can TODO list

#### 3.1.6 Crypto Algorithms
AES-256 for encrypting
AES-256 -MAC for signing
[AES-256 -CBC-MAC for hashing](https://en.wikipedia.org/wiki/CBC-MAC)

TODO links

### 3.2 Bootloader Key (BK)

#### 3.2.1 Overview
Each device [comes with](TODO) a **secret unique symmetric AES-256 Bootloader Key**.

The BK can be used to **verify the devices authenticity** at any time.
The Bootloader Key is also used to **sign new Firmware upgrades**. TODO link.

The **initial BK was set by the vendor** who is also responsible for keeping the
BK secret. The BK can be **changed at any time** and is stored inside the SBS.

The BK needs to be kept **highly secure** and should be changed after exchanging
it over an untrusted connection.

TODO links

#### 3.2.2 No Bootloader Key set
Only after burning the Bootloader via ISP no Bootloader Key was set.
If no bootkey was set at startup you need to set a Bootloader Key from the
Bootloader via USB HID. It is intended that it is **not possbile to upload a
Firmware until a Bootloader Key was set**. It is not possible to remove a
Bootloader Key afterwards. **Never ship a device without an inital vendor
Bootloader Key.** TODO link

TODO will boot straight into recovery mode and require a password change first.

#### 3.2.3 Initial Bootloader Key
A **unique BK is initially set by the vendor** who is also responsible for
keeping the BK secret. This ensures the **integrity of the initial Bootloader**.
The initial Bootloader Key can be used to provide Firmware upgrades without
exposing the BK to the user and enable Firmware upgrades on untrusted Computers.
The responsibility of the BK can be transferred to any other user but should be
changed after exchanging it over a not trusted connection (Internet!).

#### 3.2.4 Change the Bootloader Key
You need to change the Bootloader Key from the Bootloader if no one was set yet.
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

The Bootloader Key is used to **authenticate the Bootloader to the PC**.
The PC sends a random challenge to the Bootloader which it has to hash with the BK.
The Bootloaders integrity is then ensured, the Bootloader can be trusted.
TODO a long timeout/password is required here to not brute force all combinations

you (vendor) send the user a Firmware, a Firmware hash, a new encrypted password, 2 authentication challenges and 1 expected challenge answer.

  the flashing tool sends the 2 challenges and the Bootloader has to answer both. with the 1st you can trust the Bootloader if you trust your pc. with the 2nd you can send the answer to the vendor and he can verify this challenge as well.

The (symmetric key) should **not always** be used to authenticate the Bootloader at **every startup**, since the PC then always have the key stored, use the Firmware instead for this purpose. Also then it does not rely on the pc and is simpler to use.

You could (and can) do this with the Bootloader.
For usability it is very bad though, because you always need a PC with an app
that checks the Bootloader integrity first and then Firmware.
Also it might be considered to be a security risk to always use a PC
with a symmetric key to authenticate the device.

The concept is, that the Bootloader can be considered trusted. dont we need this then?

**A Firmware bug could leak the FID hash or UID. The only way to verify the device is the Bootloader Key.**

Other option via UID (one time only)
1) enter uid request code in the app, press enter
2) check the returned code is the good one
3) add mooltipass user
4) remember hash

### 3.3 Firmware Upgrade

#### 3.3.1 Overview
It is very important to **only upload trusted Firmware** from an untrusted PC
while still keeping the **ability to create custom Firmwares** and dont lock
out Firmware developers.

The Firmware upgrade only accepts signed Firmware by the Bootloader Key.
Signing the Firmware authenticates the Firmware (rather than the PC) to the
Bootloader. This means you can create new Firmwares from a trusted PC and do
**Firmware upgrades from an untrusted PC**. The vendor can provide Firmware
upgrades without leaking the secret Bootloader Key.

The vendor still can give away the initial Bootloader Key to the user if he
requests it. Then the user can **compile and sign his own Firmwares** and play
with the device. Then the user is responsible for further Firmware upgrades.
After exchanging the initial Bootloader Key from the vendor the user should
change it.

To prevent replay attacks (Firmware downgrades) the BK should be changed before
each upload. This ensures that a new Bootloader Key was changed and an old
Firmware can not be used again. The new Firmware has to be signed with the new
Bootloader Key. The vendor can force a BK change to upload a new Firmwares.
Developers (who own the BK) do not have to change the BK for each upload.

#### 3.3.2 Firmware Upgrade Sequence
1. Receive signed Firmware checksums from the PC
2. Verify the authenticity of the Firmware checksums
3. Receive next Firmware page from the PC and verify the page checksum
4. Abort if the checksum is invalid
5. Flash the valid page
6. Verify the whole Firmware checksum
7. Abort and delete the whole Firmware if the checksum is invalid
8. Write new Firmware identifier and new Firmware counter

#### 3.3.3 Firmware Checksum (FW Checksum)
The Firmware Checksum is generated after uploading a new Firmware by the
Bootloader and stored in the SBS. The PC can **verify the Firmware Integrity**
with the Firmware Checksum and the Firmware Counter. The Firmware checksum is
used as part of the POST and the FID.

**Signed Firmware Checksums Content**
* A checksum for each Firmware page
* A checksum of the whole Firmware
* A signature of the checksums, signed with the Bootloader Key

If one checksum is valid all other checksums should be valid too. Otherwise
there was an error while creating the checksums or the signing algorithm is
insecure. If the uploading failed a flag will be set to let the Firmware notice
the upload violation.

TODO how to remove this flag, only authorized people should be able to?

TODO hash algorithm: crc32?

#### 3.3.4 Firmware Counter (FWC)
The Bootloader keeps track of the number of Firmware uploads. This information
is important to check if someone even tried to hack your device. The Firmware
counter is part of the FID, stored inside the SBS and 32 bit large.

#### 3.3.5 Firmware Violation Counter (FWVC)
The Firmware Violation Counter keeps track of the number of Firmware uploads
that fail. This can happen if the signature or the checksums of the Firmware are
wrong. The Firmware is able to read this value at any time and can use if for
any user warnings. The FWVC is stored inside the SBS.

TODO Required? Checksum needs to be excluded from this.

### 3.4 Firmware Authentication

#### 3.4.1 Overview
Before the user uses the Firmware functions it should verify the Firmware
authenticity. Therefor the FID Hash is used and should be accepted by the user.
If the FID Hash is different the Firmware was upgraded or otherwise modified.

1. Start the Firmware
2. Load the desired user
3. Authenticate the user
4. Get the FID
5. Generate FID Hash from FID, User nonce and another nonce
6. User verifies the FID Hash

#### 3.4.2 Firmware Identifier (FID)
The Firmware identifier lets the Firmware authenticate itself to the user. It is
a **unique ID** generated by the Bootloader for **each Firmware flash**, stored
within the secure Bootloader section and passed to the Firmware via RAM. The FID
consists of the Firmware checksum, Firmware counter and a nonce.

Uploading a new Firmware or Bootloader will destroy the FID and a **violation of
the Firmware authenticity can be noticed** by the user. Even uploading the same
Firmware twice will result in a different FID. The FID is used in the Firmware
ID Hash and securely stored inside the SBS.

TODO FID byte size (16 or 32 bit. 16 bit should be enough for 10k write cycles)

#### 3.4.3 Firmware ID Hash (FID Hash)
The **Firmware is responsible** to authenticate itself with the use of the FID.
It should use a Firmware ID Hash to authenticate itself to the user. **Only
authorized people** should be able to verify (see) the Firmware ID Hash.

The Firmware ID Hash consists of the FID, a nonce and (optional) the user nonce.
This way the nonce can be changed at any time if an unauthorized people sees the
FID Hash. The Firmware will display the FID Hash and the **user needs to verify
it**.

The advantage is that the Firmware has access to the special hardware such as
display and smartcard and makes it easier for the user to control. The
authentication mechanism is not described here and part of the Firmware.

If the Firmware is not trusted, you can use the Bootloader again to verify the
Bootloader and the Firmware integrity. This needs to be done explicit when the
user runs the Bootloader in its recovery mode.

**Conclusion:
The security also relies on the Firmware, not only the Bootloader!**

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

## 4. FAQ

### Why not Public Key/Private Key?
Asymmetric encryption/signing is not required as the BK is considered to be kept secure.
An exchange via an insecure channel is not required, asymmetric will not improve much.
If the vendor gives the user the initial BK you can change the BK afterwards.
Symmetric AES implementation is smaller and can be reused in the Firmware.
An asymmetric signing could make the Bootloader Key authentication simpler though.

TODO a privat key is required on both sides and dont need to be exchanged (thats one step after hybrid actually)

### Why not only allow signed Firmwares?
TODO this is wrong
As a developer I like to play with open source devices. The user should also be able to make use of the Bootloader. It does not lower the security, if the user carefully checks the Firmware checksum on the PC. This needs to be integrated (forced) into the flashing tool.

### I lost my Bootloader Key. What can I do?
The BK is normally **maintained by the vendor**. Ask him first for the initial BK.
There is **no way to recover a Bootloader Key** if you have changed the initial vendor BK.
You can continue to use the current Firmware but wont be able to upload any new Firmware.
You might want to check the Firmware authenticity first to exclude a faked the Bootloader.
You may use an ISP to burn a new Bootloader, but this will **destroy any data** on the AVR.

## 5. Links

### Security
* [Atmel AVR231: AES Bootloader](http://www.atmel.com/images/doc2589.pdf)
* [AVR230: DES Bootloader](http://www.atmel.com/images/doc2541.pdf)
* [Overwriting a Protected AVR Bootloader](http://hackaday.com/2014/07/05/overwriting-a-protected-avr-Bootloader/)
* [BootJacker: The Amazing AVR Bootloader Hack!](http://oneweekwonder.blogspot.de/2014/07/bootjacker-amazing-avr-Bootloader-hack.html)
* [NaCl](https://nacl.cr.yp.to/box.html)
* [AVRNaCl](http://munacl.cryptojedi.org/atmega.shtml)
* ["Key-logger, Video, Mouse" Talk at 32c3](https://media.ccc.de/v/32c3-7189-key-logger_video_mouse)

### AVR Bootloaders
* [avr-libc Bootloader Support Utilities](http://www.nongnu.org/avr-libc/user-manual/group__avr__boot.html)
* [Bootloader FAQ](http://www.avrfreaks.net/forum/faq-c-writing-Bootloader-faq?page=all)
* [AVR Bootloader in C - eine einfache Anleitung](https://www.mikrocontroller.net/articles/AVR_Bootloader_in_C_-_eine_einfache_Anleitung)
* [All you need to know about AVR fuses](http://www.embedds.com/all-you-need-to-know-about-avr-fuses/)
* [Engbedded Atmel AVR Fuse Calculator](http://www.engbedded.com/fusecalc/)


## 6. Version History

```
0.1 Concept Paper Release (xx.xx.2016)
* Added private Github repository with Readme
```

## 7. License and Copyright
```
Copyright (c) 2016 NicoHood

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
```

## Discussion

### Authenticate the Bootloader to the PC

**Discussion: Is this feature essential?**

#### Pro
1. It might not use much flash
2. Increases Security (authenticity)
3. Bootloader can be verified before flashing new Firmware
4. Not all people might care (remember) about FID Hash. BK authenticity can always be used.
5. Device authentication should not rely on Firmware
6. You can use it to let the BK owner verify the Bootloader (send the response back to him). Assumes emails cannot be faked.
7. Adds another layer of security BESIDE FID Hash
8. Note: The FID should still be used for all day use. Bootloader is ment for FW upgrades and recovery mode.
9. The concept is, that the Bootloader can be considered trusted. This is then essential if you not want to rely on Firmware.
10. **A Firmware bug could leak (or display wrong) the FID hash or UID. The only way to verify the device is the Bootloader Key.**

#### Cons
1. Can be done with Firmware
2. FID Hash can also be used to ensure authenticity
3. A compromised PC could always say the Bootloader is okay
4. Alternative is a Firmware side authentication via UID

#### Compromis
It does not hurt to include if enough flash is available

### AES placed into Bootloader or Firmware section

#### Arguments for Bootloader section
1. its better to NEVER rely on the Firmware. otherwise you can brick the whole device. I listed Firmware bricking Guarantee as feature so far
This is the most important point for me. See the Bootloader FAQ on avr freaks at section 13.
2. The Bootloader should just not rely on anything and also should be used flexible in other projects too
3. the Bootloader requires at least 2kb~3kb. AES fits anyways (assuming it takes ~1kb)

#### Arguments against Bootloader section
1. how can you brick the device if you are guaranteed that the correct Firmware has been flashed and
therefore has the correct AES routine. developers will use ISP anyways
2. nothing prevents you to also include it in your Bootloader for other uses (if it fits into 2 and 4 kb)
3. but if your Bootloader is actually less than 2k we are wasting 1k of flash

#### Compromis
if it fits into 2kb (without aes, what i aim for), we can make mooltipass include it from the Firmware.
the Firmware can place the AES into the same space where normally the Bootloader would be located.
(2more kb for Firmware). so even the Firmware function calls are at the same location.
we can also do a version that includes the Bootloader for people like me ;) (or for other projects)
if it does not fit into 2kb, there is no reason to exclude it (if it does fit into 4kb)

## Other Ideas:
Just some ideas, or maybe things that needs to find their way into the readme.
Unstructured.

  * The Bootloader could share its AES implementation to the Firmware code.
  * the Bootloader recovery mode can only be entered via physical button press for more security
  * Do not leave any trace in EEPROM or RAM
  * Add an user interface to download the Firmware again
  * Bootloader Authentication needs to be discussed
  * Bootloader violation flag pased to Firmware?
  -> pass the FID via ram and also anothe byte to identify wrong Bootloader attacks, BK forced changed and future use things.
  * A symmetric key for each device requires a lot of different signed Firmwares
  * Initial symmetric key **needs to be changed**, after it was exchanged via insecure email
  -> maybe create a secure encrypted "reset password" package. so the key will be displayed on the Bootloader rather than visible in an email?
  * Symmetric key should be random and not generate via a serial number algorithm
  * Optional (closed source) encrypted(!) Firmwares are possible. reading the Firmware back has to be forbidden then. checksum might be allowed though.
  * You need to trust the seller/signing person (or youself if you request the BK)
  * we have a problem if the BK was lost (only reburning via ISP is possible, but a reasonable option. the Firmware should be able to do self backups)
  * If Bootloader crypto functions are insecure, we have a problem (ISP required for new Bootloader. means the secureloader has to be good/veryfied befor using it.)
  * If we use symmetric signing the Firmware should not be able to see the secret.
  * opening the avr for xxx$ will leak the BK. this should be noted, maybe as assumption?
  * It is important that a brute forcing the unlock is impossible
  * verifying the Firmware after receiving the package should be required or at least noticed somehow.
  * The concept is, that the Bootloader can be considered trusted
  * Therefor the **Firmware** needs to **authenticate itself** at **every start**. This can be done before any USB communication (fo the Firmware) was started.
  * we need to ensure that a fake Firmware has no access to the Bootloader.
  * A modified Bootloader could not check the Firmware integrity (checksum) reliable. There should be no way to modify the Bootloader by anyone.
  * maybe use something bigger than the 32u4?
  * The user is responsible for not uploading malicious Firmware if he owns the BK.
  * Random number generation needs to be secure.
  * assuming a compromised pc does not secure the Firmware though obviously
  * Access the FID via Bootloader jump table function instead of ram, to make it simpler.
  * Forbit EEPROM reading from the Bootloader?
  * Should every Bootloader get a unique ID too? this way you can identify the device appart from the sticker on the backside.



### TODO
 * TODO write Firmware Identifier etc capital/idential
 * TODO write headlines capital
 * TODO check integrity and authenticity words if they match
 * TODO numbe the section, to make it more clear what sub headline is for what topic.
 * TODO add a lot of links
 * Add minimum RAM requirement if the Bootloader is finished.
 * Add implementation requirements checklist of all features
 * better line feeds (80chars)
