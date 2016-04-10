# SecureLoader

The idea is to create an **USB HID Bootloader** that can easily upgrade an AVR
Firmware and even **increase the security** of the device and its Firmware.
The Bootloader **protects against potential backdoors and malicious Firmware
upgrades**. This paper describes use cases, features, potential attacks and
solutions to prevent them.

This Readme describes a **temporary concept** with some ideas.
It is **not final**. Contributions appreciated.

## Table of Contents
1. [Bootloader Overview](#1-bootloader-overview)
2. [Provided Guarantees TODO security features / Concept](TODO)
3. [Technical Details](#3-technical-details)
4. [FAQ](#4-faq)
5. [Links](#5-links)
6. [Version History](#6-version-history)
7. [License and Copyright](#7-license-and-copyright)

## 1. Bootloader Overview
1. [Bootloader Use Case](#11-bootloader-use-case)
2. [Bootloader Properties](#12-bootloader-properties)
3. [Attack Scenario](#13-attack-scenario)

### 1.1 Bootloader Use Case
* Security updates
* Feature updates
* Uploading a self recompiled Firmware
* Developing new Firmware features

### 1.2 Bootloader Properties
* Protects Firmwares confidentiality, authenticity, integrity
* Uses USB HID Protocol
* No special drivers required
* Coded for AVR USB Microcontrollers
* Optimized for [ATmega32u4](http://www.atmel.com/devices/atmega32u4.aspx)
* Fits into 4kb Bootloader section
* Based on a very reduced version of [LUFA](www.lufa-lib.org)
* [Reusable](#313-bootloader-jump-table-bjt) [AES implementation](#316-crypto-algorithms)
* [Open Source](#210-open-source-guarantee)

### 1.3 Attack Scenario

##### Required Conditions for Device Security:
* The AVR is programmed with the correct fuses.
* The initial [Bootloader Key](TODO) is kept secure [by the vendor until the user requests it.](TODO)
* The security also relies on the Firmware ([Firmware Authenticity!](TODO)).
* The Bootloader Key is [changed before every Firmware upgrade](TODO).

##### Worst Case Scenario (might differ to the Real World):
* The Firmware handles secure information that a Firmware backdoor could leak.
* The attacker has full physical access of the device.
* The device can be opened and an ISP can be used without a visible change.
* This excludes opening the AVR and reading the flash bytes with a Microscope.
* The attacker is able to steal the device and put it back at any time.
* The uploading PC is compromised when doing Firmware upgrades.

## 2. Provided Guarantees TODO security features / Concept
1. [Bootloader/Device Authenticity Protection](#21-bootloaderdevice-authenticity-protection)
2. [ISP Protection](#22-isp-protection)
3. [Brute Force Protection](#23-brute-force-protection)
4. [Compromised PC protection](#24-compromised-pc-protection)
5. [Hacking the Bootloader from the Firmware Protection](#25-hacking-the-bootloader-from-the-firmware-protection)
6. [Unauthorized Firmware Upgrade/Downgrade Protection](#26-unauthorized-firmware-upgradedowngrade-protection)
7. [Firmware Authenticity Protection](#27-firmware-authenticity-protection)
8. [Flash Corruption Protection](#28-flash-corruption-protection)
9. [Firmware Brick Protection](#29-firmware-brick-protection)
10. [Open Source Guarantee](#210-open-source-guarantee)


### 2.1 Unauthorized Firmware Upgrade/Downgrade Protection
Only signed Firmwares from a trusted source can be flashed with the Bootloader.

Each device comes with a unique shared secret
[Bootloader Key](#32-bootloader-key-bk) that was
[set by the vendor](#323-initial-bootloader-key) or
[the user](#324-change-the-bootloader-key).

The owner of the [Bootloader Key](#32-bootloader-key-bk) is responsible for
keeping it secret. Techniques how the [Bootloader Key](#32-bootloader-key-bk) is
kept secure are described below.

Every Firmware needs to be signed with a the Bootloader Key. You can even flash
the device from a not trusted PC, as it will only allow signed Firmware to be
uploaded. Firmware downgrades (replay attacks) are prevented via Bootloader Key
changes.

The device authenticity **is not** ensured through the Bootloader, it has to be
done by the firmware. Techniques how to ensure device authenticity are described below.

### 2.2 ISP Protection
An ISP could be used to read the Bootloader and application flash content. This
way one could burn a new faked Bootloader with a backdoor. ISP requires physical
access to the PCB which can be visually noticed on some devices.

Moreover on AVR the [Lock Bits](#363-lock-bits-explanation) prevent an
[attacker](#13-attack-scenario) from reading the flash content. Overwriting the
Bootloader via ISP will also overwrite the [BK](#32-bootloader-key-bk) and the
[UID](TODO). This way the
[Firmware Authenticity](TODO) can be
ensured.

### 2.3 Brute Force Protection
The [BK](#32-bootloader-key-bk) has an
[AES-256 symmetric key](#316-crypto-algorithms). To brute force a signed
[Firmware upgrade](#26-unauthorized-firmware-upgradedowngrade-protection) you'd
(currently) need too much time to crack it. Also there is a timeout after each
failed upload.

### 2.4 Compromised PC protection
A compromised PC (via virus) could not hack the Bootloader in any way. The
[firmware upgrade](#26-unauthorized-firmware-upgradedowngrade-protection) is
protected (unless the PC does not get the BK).

### Firmware Vulnerability Risk
If the Firmware has a security vulnerability that exposes the BK there is no
chance to ensure device security. Make sure you are always using the latest
firmware.

[If someone is able to
[upload malicious Firmware](#26-unauthorized-firmware-upgradedowngrade-protection)
to the device he needs access to the BK or the upgrade mechanism is insecure.
In this case the whole concept is useless. This risk can be reduced with
[Open Source](TODO)
If you upload a malicious Firmware that you signed with your own key, its an
user fault.

### 2.9 Firmware Brick Protection
If you upload a bricked Firmware it is always possible to enter the
[Bootloader Mode](#315-recovery-mode) again. The Bootloader does **not rely on
any part of the Firmware**. Then you can upload another Firmware instead and
continue testing. You should not lose your BK to be able to upload another
Firmware again. Also see
[Flash Corruption Protection](#28-flash-corruption-protection)

### 2.8 Flash Corruption Protection
* [Brown-out detection (Fuse)](#36-fuse-settings)

### 2.2 Firmware Authenticity Protection
It is **highly suggested** that the Firmware also provides a way to authenticate
itself to the user. This can be done with a unique ID (UID) that is stored
inside EEPROM. First the user has to authenticate itself to the Firmware which
then will display the secret UID. The user has to verify the UID message. If the
UID changes the device got wiped with an ISP and the user will notice.

A good practice is to create an UID from a special user nonce and another (per
user) random nonce. This way you can create a special UID for each user and
change it if it was compromised (seen by someone else).

The UID will not protect against Bootloader Key/Firmware attacks. TODO 2 links
The UID also does not prevent the user from ignoring its importance though.

The process could look like this:
1. Merge user nonce with the random user nonce
2. Display a warning that the UID has to be kept secret
3. Display UID
4. The user has to accept it.

**Conclusion:
The security also relies on the Firmware, not only the Bootloader!**

### 2.10 Open Source Guarantee
The Bootloader software is [open source](#7-license-and-copyright). This means
it can be reviewed and improved by many people. You always able to burn the
Bootloader on your own at any time. Keep in mind that the
[UID](TODO) will change and all Bootloader and Firmware data will be lost.

Preventing
[unauthorized Firmware upgrades](#26-unauthorized-firmware-upgradedowngrade-protection)
does not essentially restrict
[custom Firmwares](#324-change-the-bootloader-key). Only signed firmware is
accepted, but it is not encrypted (open source).

## 3. Technical Details
1. [Bootloader Components](#31-bootloader-components)
 1. [Boot Process](#311-boot-process)
 2. [Secure Bootloader Section (SBS)](#312-secure-bootloader-section-sbs)
 3. [Bootloader Jump Table (BJT)](#313-bootloader-jump-table-bjt)
 4. [Power on Self Test (POST)](#314-power-on-self-test-post)
 5. [Recovery Mode](#315-recovery-mode)
 6. [Crypto Algorithms](#316-crypto-algorithms)
2. [Bootloader Key (BK)](#32-bootloader-key-bk)
 1. [Overview](#321-overview)
 2. [No Bootloader Key set](#322-no-bootloader-key-set)
 3. [Initial Bootloader Key](#323-initial-bootloader-key)
 4. [Change the Bootloader Key](#324-change-the-bootloader-key)
 5. [Authenticate the Bootloader to the PC](#325-authenticate-the-bootloader-to-the-pc)
3. [Firmware Upgrade](r#33-firmware-upgrade)
 1. [Overview](#331-overview)
 2. [Firmware Checksum (FW Checksum)](#332-firmware-checksum-fw-checksum)
 3. [Firmware Upgrade Counter (FWUC)](#333-firmware-upgrade-counter-fwuc)
 4. [Firmware Violation Counter (FWVC)](#334-firmware-violation-counter-fwvc)
 5. [Firmware Upgrade Sequence](#335-firmware-upgrade-sequence)
4. [Firmware Authentication](#34-firmware-authentication)
 1. [Overview](#341-overview)
 2. [Firmware Identifier (FID)](#342-firmware-identifier-fid)
 3. [Firmware ID Hash (FID Hash)](#343-firmware-id-hash-fid-hash)
6. [Fuse Settings](#36-fuse-settings)
 1. [Fuse Overview](#361-fuse-overview)
 2. [Fuse Explanation](#362-fuse-explanation)
 3. [Lock Bits Explanation](#363-lock-bits-explanation)

### 3.1 Bootloader Components

#### 3.1.1 Boot Process
0. Device startup, always run the Bootloader (BOOTRST=0)
1. Check if the Bootloader should be loaded (Hardware Button)
  * Do a [Firmware upgrade](#Firmware-upgrade)
  * [Change the Bootloader Key](#324-change-the-bootloader-key)
    with a signed and encrypted new password
  * Verify the Firmware
  * Start the Firmware

#### Enter Bootloader Mode
After device startup the Bootloader normally executes the Firmware. The Firmware
can request to enter the Bootloader Mode. It has to put a special `uint8_t
MagicBootloaderKey = 0x77` inside `RAMEND` which is normally reserved by gcc as
main() return value. After a watchdog reset the Bootloader Mode will be entered.

As 2nd recovery option you can manually enter the Bootloader Mode. You need to
manually press and hold a physical button while plugging in the device. The
Bootloader will wait for any command from the PC. If no command was received yet
and you release the button for more than 3 seconds it will load the Firmware.
This is meant as a recovery mode if you Firmware does not start at all. A note
how to start the Bootloader Mode should be placed inside the Firmware manual.

#### Bootloader Section Overview

The page size of the Atmega23u4 is 128 bytes (64 words), the datasheet is wrong.

```
    +----------------------------+ 0x0000
    |                            |
    |                            |
    |                            |
    |                            |
    |                            |
    |                            |
    |                            |
    |                            |
    |      User Application      |
    |                            |
    |                            |
    |                            |
    |                            |
    |                            |
    |                            |
    |                            |
    +----------------------------+ FLASHEND - BOOT_SECTION_SIZE (4KiB)
    |                            |
    |   Bootloader Application   |
    | (Not User App. Accessible) |
    |                            |
    +----------------------------+ FLASHEND - 256
    | Secure Bootloader Section  |
    |  Bootloader Key (32 byte)  |
    | (Not User App. Accessible) |
    +----------------------------+ FLASHEND - 128
    |  Bootloader API Functions  |
    |    EraseFillWritePage()    |
    |         ReadByte()         |
    |   (User App. Accessible)   |
    +----------------------------+ FLASHEND - 4
    | Bootloader API Jump Table: |
    |  rjmp EraseFillWritePage   |
    |        rjmp ReadByte       |
    |   (User App. Accessible)   |
    +----------------------------+ FLASHEND
```

#### Secure Bootloader Section (SBS)
The [penultimate Bootloader flash page](TODO) is used to store the secret BK.
The Bootloaders flash is protected by fuses to safe it from being read via ISP.
The Firmware has no direct (read/write) access to the data except via the [Bootloader Read/Write API](TODO) to avoid a [Firmware Vulnerability Risk](TODO).

#### Bootloader Read/Write API
On AVR it is only possible to write new flash pages from the Bootloader Section.
Also the Bootloader Section is protected from direct read access to avoid a
[Firmware Vulnerability Risk](TODO).

Therefore the Bootloader contains a special interface with functions to allow
read/write access of the Bootloader Section. Those functions are placed inside
a single flash page at the very end of the flash. This flash page also contains
a jump table in the last 4 flash bytes. It is used to call those two functions:
* `bool BootloaderAPI_EraseFillWritePage(const address_size_t address,
                                         const uint16_t* words)`
* `uint8_t BootloaderAPI_ReadByte(const address_size_t address)`

**The Bootloader Read/Write API should be used with care inside the Firmware.**
It should only be used to access the Bootloader in a very critical situation.
It does not protect against accessing the [SBS](TODO) but avoids overwriting itself.


### Additional Features

#### 3.1.6 Crypto Algorithms
AES-256 for enc/decrypting
[AES-256 CBC-MAC for signing](https://en.wikipedia.org/wiki/CBC-MAC)
CBC

TODO links

### 3.2 Bootloader Key (BK)

#### 3.2.1 Overview
Each device [comes with](TODO) a **secret unique symmetric AES-256 Bootloader Key**.

The Bootloader Key is used to **sign new Firmware upgrades**. TODO link.

The **initial BK was set by the vendor** who is also responsible for keeping the
BK secret. The BK can be **changed at any time**.

The BK needs to be kept **highly secure** and should be changed after exchanging
it over an untrusted connection (Internet!).

It is stored inside EEPROM and the Firmware has full access to it and can also
change it. Since only signed Firmware can be uploaded, the Firmware is trusted.

TODO links

#### 3.2.3 Initial Bootloader Key
A **unique BK is initially set by the vendor** who is also responsible for
keeping the BK secret. This ensures the **integrity of the initial Bootloader**.
The initial Bootloader Key can be used to provide
[Firmware upgrades](#33-firmware-upgrade) without
exposing the BK to the user and enable Firmware upgrades on
[untrusted Computers](#24-compromised-pc-protection).
The responsibility of the BK can be transferred to any other user but
[should be changed](TODO) after exchanging it over an untrusted connection (Internet!).

#### 3.2.4 Change the Bootloader Key
You might want to change the BK for security purposes. This is useful when
setting an [initial vendor Bootloader Key](#323-initial-bootloader-key) or
changing it to give the
[user control over the Bootloader](#210-open-source-guarantee).

The main reason why the BK should be changed is to **forbid replay attacks** of
old Firmware that might have a security vulnerability.

To change the BK you need to **encrypt the new BK with the old BK**. Enter the
[Recovery Mode](#315-recovery-mode) and instruct a BK change command with the
new (encrypted) BK.






#### 3.2.5 Authenticate the Bootloader to the PC

**[Discussion: Is this feature essential?](#discussion)**
TODO remove?

The BK can be used to
**[verify the devices authenticity](#21-bootloaderdevice-authenticity-protection)**
at any time. This can be used for example after receiving the device from the
vendor or after a [FID Hash](#343-firmware-id-hash-fid-hash)
[violation](#27-firmware-authenticity-protection). Even a 1:1 copy of the device
could be noticed through the secret Bootloader Key.

The Bootloader Key is used to **authenticate the Bootloader to the PC**. The PC
sends a random challenge to the Bootloader which it has to hash with the BK. If
it is correct, the Bootloaders authenticity is then ensured, the Bootloader can
be trusted. A timeout is used to not [brute force](#23-brute-force-protection)
all combinations.

The vendor could send the user 2 challenges with 1 expected result. With the 1st
answer you can trust the Bootloader if you trust your PC. With the 2nd you can
send the answer to the vendor and he can verify this challenge as well. This way
the vendor and the user can verify the challenges.

The (symmetric key) should **not always** be used to authenticate the Bootloader
at **every startup**, since the PC then always have the key stored, use the
[Firmware instead](#34-firmware-authentication) for this purpose. Also then it
does not rely on the PC and is simpler to use.

### 3.3 Firmware Upgrade

#### 3.3.1 Overview
It is very important to **only upload trusted Firmware** from an untrusted PC
while still keeping the **ability to create custom Firmwares**.

The Firmware upgrade only accepts signed Firmwares by the
[Bootloader Key](#32-bootloader-key-bk). Signing the Firmware authenticates the
BK owner to the Bootloader, rather than the PC. This means you can create new
Firmwares from a trusted PC and do **Firmware upgrades from an untrusted PC**.
The vendor can provide Firmware upgrades without leaking the secret
[Bootloader Key](#32-bootloader-key-bk).

The vendor still can give away the
[initial Bootloader Key](#323-initial-bootloader-key) to the user if he requests
it. Then the user can **compile and sign his own Firmwares** and play with the
device. The user is responsible for further Firmware upgrades. The initial
[Bootloader Key should be changed](#324-change-the-bootloader-key) after
exchanging it over an untrusted connection (Internet!).

To prevent [replay attacks (Firmware downgrades)](TODO) the BK should be [changed](#324-change-the-bootloader-key) before
each upload. This ensures that a new Bootloader Key was set and an old
Firmware can not be used again. The new Firmware has to be signed with the new
Bootloader Key. The vendor can force a BK change to upload a new Firmwares.
Developers (who own the BK) do not have to change the BK for each upload.

#### 3.3.2 Firmware Checksum (FW Checksum)
The Firmware Checksum is generated after uploading a new Firmware by the
Bootloader and stored in the [SBS](#312-secure-bootloader-section-sbs). The PC
can **verify the Firmware Integrity**
with the Firmware Checksum and the [Firmware Upgrade Counter](#333-firmware-upgrade-counter-fwuc). The Firmware checksum is
used as part of ~~the POST and~~ the [FID](#342-firmware-identifier-fid).

TODO differentiate between checksum and hash?

**Signed Firmware Checksums Content**
* A checksum for each Firmware page
* A checksum of the whole Firmware
* A signature of the checksums, signed with the Bootloader Key

If one checksum is valid all other checksums should be valid too. Otherwise
there was an error while creating the checksums or the signing algorithm is
insecure. If the uploading failed the FWVC get increased to let the Firmware notice
the upload violation. After a successfull upload the FWUC get increased instead and the FWVC will be cleared.

TODO hash algorithm: CBC-MAC

TODO POST
TODO recovery mode, verify firmware: crc??

#### 3.3.3 Firmware Upgrade Counter (FWUC)
The Bootloader keeps track of the number of Firmware upgrades. This information
is important to check if someone even tried to hack your device. The Firmware
counter is part of the [FID](#342-firmware-identifier-fid), stored inside the
[SBS](#312-secure-bootloader-section-sbs) and 32 bit large. It can be
read from the PC and the firmware (via [BJT](#313-bootloader-jump-table-bjt)) at any time.

#### 3.3.4 Firmware Violation Counter (FWVC)
The Firmware Violation Counter keeps track of the number of Firmware uploads
that fail. This can happen if the signature or the checksums of the Firmware are
wrong. It can be
read from the PC and the firmware (via [BJT](#313-bootloader-jump-table-bjt)) at any time.
It can be used by the Firmware for user warnings.
The FWVC is stored inside the [SBS](#312-secure-bootloader-section-sbs) and 8 bit large.

TODO Required? Checksum needs to be excluded from this.

#### 3.3.5 Firmware Upgrade Sequence
1. Receive signed Firmware checksums from the PC
2. Verify the authenticity of the Firmware checksums
3. Receive next Firmware page from the PC and verify the page checksum
4. Abort if the checksum is invalid
5. Flash the valid page
6. Verify the whole Firmware checksum
7. Abort and delete the whole Firmware if the checksum is invalid
8. Write new Firmware identifier and new Firmware Upgrade Counter

### 3.4 Firmware Authentication

#### 3.4.1 Overview
Before the user uses the Firmware functions it should verify the Firmware
authenticity. Therefore the [FID Hash](#343-firmware-id-hash-fid-hash) Hash is used and should be accepted by the user.
If the [FID Hash](#343-firmware-id-hash-fid-hash) is different the Firmware was upgraded or otherwise modified.

1. Start the Firmware
2. Load the desired user
3. Authenticate the user
4. Get the [FID](#342-firmware-identifier-fid)
5. Generate [FID Hash](#343-firmware-id-hash-fid-hash) from [FID](#342-firmware-identifier-fid), User nonce and another nonce
6. User verifies the [FID Hash](#343-firmware-id-hash-fid-hash)

#### 3.4.2 Firmware Identifier (FID)
The Firmware identifier lets the Firmware authenticate itself to the user. It is
a **unique ID** generated by the Bootloader for **each Firmware flash**, stored
within the secure Bootloader section and passed to the Firmware via RAM. The [FID](#342-firmware-identifier-fid)
consists of the Firmware checksum, Firmware Upgrade Counter and a nonce.

Uploading a new Firmware or Bootloader will destroy the [FID](#342-firmware-identifier-fid) and a **violation of
the Firmware authenticity can be noticed** by the user. Even uploading the same
Firmware twice will result in a different [FID](#342-firmware-identifier-fid). The [FID](#342-firmware-identifier-fid) is used in the Firmware
ID Hash and securely stored inside the [SBS](#312-secure-bootloader-section-sbs).

TODO FID byte size (16 or 32 bit. 16 bit should be enough for 10k write cycles)

#### 3.4.3 Firmware ID Hash (FID Hash)
The **Firmware is responsible** to authenticate itself with the use of the [FID](#342-firmware-identifier-fid).
It should use a Firmware ID Hash to authenticate itself to the user. **Only
authorized people** should be able to verify (see) the Firmware ID Hash.

The Firmware ID Hash consists of the [FID](#342-firmware-identifier-fid), a nonce and (optional) the user nonce.
This way the nonce can be changed at any time if an unauthorized people sees the
[FID Hash](#343-firmware-id-hash-fid-hash). The Firmware will display the [FID Hash](#343-firmware-id-hash-fid-hash) and the **user needs to verify
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

#### 3.6.1 Fuse Overview
ATmega32u4 fuse settings:
* Low: 0xFF
* High: 0xD8
* Extended: 0xF8
* Lock: 0x0C

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

#### 3.6.3 Lock Bits Explanation
- [x] Lock Bit Protection Modes (Memory Lock); [LB=00]
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

### Other
* http://jtxp.org/tech/tinysafeboot_en.htm
* http://wiki.hacdc.org/index.php/Secure_bootloader
* https://www.blackhat.com/html/bh-dc-10/bh-dc-10-archives.html
* https://www.blackhat.com/presentations/bh-dc-10/Grand_Joe/BlackHat-DC-2010-Grand-HW-is-the-new-SW-slides.pdf
* http://blog.ioactive.com/2007/11/atmega169p-quick-peek.html
* http://blog.ioactive.com/2010/08/atmel-atmega2560-analysis-blackhat.html

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
4. Not all people might care (remember) about [FID Hash](#343-firmware-id-hash-fid-hash). BK authenticity can always be used.
5. Device authentication should not rely on Firmware
6. You can use it to let the BK owner verify the Bootloader (send the response back to him). Assumes emails cannot be faked.
7. Adds another layer of security BESIDE [FID Hash](#343-firmware-id-hash-fid-hash)
8. Note: The [FID](#342-firmware-identifier-fid) should still be used for all day use. Bootloader is ment for FW upgrades and recovery mode.
9. The concept is, that the Bootloader can be considered trusted. This is then essential if you not want to rely on Firmware.
10. **A Firmware bug could leak (or display wrong) the [FID Hash](#343-firmware-id-hash-fid-hash) or UID. The only way to verify the device is the Bootloader Key.**

#### Cons
1. Can be done with Firmware
2. [FID Hash](#343-firmware-id-hash-fid-hash) can also be used to ensure authenticity
3. A compromised PC could always say the Bootloader is okay
4. Alternative is a Firmware side authentication via UID

#### Compromis
It does not hurt to include if enough flash is available

### Do a POST

**Discussion: Is this feature essential?**

#### Pro
1. It might not use much flash (CRC, shasum does use a lot)
2. Increases Security (flash corruption)
3. Is fast with CRC
4. The user can ALWAYS verify the firmware.
5. Could also be used inside the firmware via BJT

#### Cons
1. CRC or something else is required. AES Hash cannot be used, due to changing BK.
2. Very unlikely that flash corrupts
3. If the user wants to verify the firmware (integrity), the bootloader authenticity also needs to be checked. Otherwise the feature could have been faked. So AES MAC could be used. This means this feature is only important for POST or very simple untrusted checks.

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
  * Therefore the **Firmware** needs to **authenticate itself** at **every start**. This can be done before any USB communication (fo the Firmware) was started.
  * we need to ensure that a fake Firmware has no access to the Bootloader.
  * A modified Bootloader could not check the Firmware integrity (checksum) reliable. There should be no way to modify the Bootloader by anyone.
  * maybe use something bigger than the 32u4?
  * The user is responsible for not uploading malicious Firmware if he owns the BK.
  * Random number generation needs to be secure.
  * assuming a compromised pc does not secure the Firmware though obviously
  * Access the FID via Bootloader jump table function instead of ram, to make it simpler.
  * Forbit EEPROM reading from the Bootloader?
  * Should every Bootloader get a unique ID too? this way you can identify the device appart from the sticker on the backside.

### Implementation requirements
  - [ ] Bootloader should be accessible from a browser addon (USB HID)
  - [ ] A command line tool for uploading is also essential for debugging
  - [ ] Bootloader cannot be updated later, so it needs to be of high quality
  - [ ] The bootloader should not touch EEPROM at all.
  - [ ] Simple bootloader execution mechanism to enter bootloader mode.
  - [ ] Firmware verification should be possible at any time
  - [ ] The bootloader could keep track of the number of upgrades (check buffer overflow!)
  - [ ] Verify the bootloader checksum before starting
  - [ ] Verify the firmware checksum before running it
  - [ ] Verify the fuse settings as well
  - [ ] Use proper lock bits
  - [ ] Add an user interface to download the firmware again
  - [ ] Do not leave any trace in EEPROM or RAM
  - [ ] Do a watchdog reset for application starting to keep all registers cleared


### TODO
 * TODO write Firmware Identifier etc capital/idential
 * TODO write headlines capital
 * TODO check integrity and authenticity words if they match
 * TODO numbe the section, to make it more clear what sub headline is for what topic.
 * TODO add a lot of links
 * Add minimum RAM requirement if the Bootloader is finished.
 * Add implementation requirements checklist of all features
 * better line feeds (80chars)
