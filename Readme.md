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
* Verify Bootloader and Firmwares authenticity and integrity

### 1.2 Bootloader Properties
* Protects Firmwares confidentiality, authenticity, integrity
* Uses USB HID Protocol
* No special drivers required
* Coded for AVR USB Microcontrollers
* Optimized for [ATmega32u4](http://www.atmel.com/devices/atmega32u4.aspx)
* Fits into 4kb Bootloader section TODO
* Based on [LUFA](www.lufa-lib.org)
* [Reusable](#313-bootloader-jump-table-bjt) [AES implementation](#316-crypto-algorithms)
* [Open Source](#210-open-source-guarantee)

### 1.3 Attack Scenario

##### Required Conditions for Device Security:
* The AVR is programmed with the correct fuses.
* The initial password is kept secure by the vendor until the user requests it.
* The security also relies on the Firmware (Firmware authenticity!).
* The Bootloader Key is changed before every Firmware upgrade.

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

### 2.1 Bootloader/Device Authenticity Protection
Each device comes with a unique [Bootloader Key](#32-bootloader-key-bk) that was
[set by the vendor](#323-initial-bootloader-key) or
[the user](#324-change-the-bootloader-key). **The owner of the
[Bootloader Key](#32-bootloader-key-bk) is responsible for keeping it secret**.

The [Bootloader and device authenticity](#325-authenticate-the-bootloader-to-the-pc)
is ensured through the [Bootloader Key](#32-bootloader-key-bk).
Techniques how the [Bootloader Key](#32-bootloader-key-bk) is kept secure are
described below.

You also want to check Bootloader authenticity if your
[FID Hash](#343-firmware-id-hash-fid-hash) changed. If the Bootloader gets
overwritten, the [FID Hash](#343-firmware-id-hash-fid-hash) will change. Even
though the Bootloader can be authenticated manually the **Firmware should also
authenticate itself at every boot though the
[FID Hash](#343-firmware-id-hash-fid-hash)**.

### 2.2 ISP Protection
An ISP could be used to read the Bootloader and application flash content. This
way one could burn a new faked Bootloader with a backdoor. ISP requires physical
access to the PCB which can be visually noticed on some devices.

Moreover on AVR the [Lock Bits](#363-lock-bits-explanation) prevent an
[attacker](#13-attack-scenario) from reading the flash content. Overwriting the
Bootloader via ISP will also overwrite the [BK](#32-bootloader-key-bk) and the
[FID Hash](#343-firmware-id-hash-fid-hash). This way the
[Bootloaders authenticity](#21-bootloaderdevice-authenticity-protection) can be
ensured.

### 2.3 Brute Force Protection
The [BK](#32-bootloader-key-bk) has an
[AES-256 symmetric key](#316-crypto-algorithms). To brute force a signed
[Firmware upgrade](#26-unauthorized-firmware-upgradedowngrade-protection) you'd
(currently) need too much time to crack it. Also there is a timeout after each
failed upload and the
[Firmware Violation Counter](#334-firmware-violation-counter-fwvc) will keep
track of those. After 3 failed attempts the Bootloader has to be
([manually](Compromised PC protection))
restarted.

### 2.4 Compromised PC protection
A compromised PC (via virus) could not hack the Bootloader in any way. The
[firmware upgrade](#26-unauthorized-firmware-upgradedowngrade-protection) is
protected (unless the PC does not get the BK). Even then a new
[FID Hash](#343-firmware-id-hash-fid-hash) will let the user notice a
[Bootloaders authenticity](#21-bootloaderdevice-authenticity-protection)
violation. Also the Bootloader [Recovery Mode](#315-recovery-mode) can only be
entered via a physical button press.

### 2.5 Hacking the Bootloader from the Firmware Protection
If someone is able to
[upload malicious Firmware](#26-unauthorized-firmware-upgradedowngrade-protection)
to the device he needs access to the BK. If he has got the BK, he could simply
fake and burn a new (fake) Bootloader instead. Therefore it is mostly useless to
hack the Bootloader from the Firmware except if opening the device to
[use ISP](#22-isp-protection) can be visually noticed.

This hack can be prevented with storing the secrets into the
[SBS](#312-secure-bootloader-section-sbs) and setting the correct
[Lock Bits](#363-lock-bits-explanation).

Therefore this attack scenario concentrates more on Bootloader hacking via
Firmware vulnerabilities. Even if a Firmware vulnerability was found you can
hardly hack the Bootloader. AVR use
[Harvard architecture](https://en.wikipedia.org/wiki/Harvard_architecture), not
[Von Neumann architecture](https://en.wikipedia.org/wiki/Von_Neumann_architecture).
Also you can do a Firmware upgrade to get rid of the security vulnerability.

### 2.6 Unauthorized Firmware Upgrade/Downgrade Protection
Only signed Firmwares can be flashed with the Bootloader.
You can even flash the device from a not trusted PC.

Every Firmware needs to be signed with the Bootloader Key.
The BK is kept secure by the vendor or by the user.

Flashing a new Firmware will also change the Firmware ID Hash (TODO link).
The user is able notice a Firmware change, even with an ISP.

Firmware downgrades (replay attacks) are prevented via Bootloader key changes.

A compromised PC cannot initiate a Firmware upgrade.
The Bootloader can only be started via a physical key press.

### 2.7 Firmware Authenticity Protection
You will notice a Firmware change because the
[FID Hash](#343-firmware-id-hash-fid-hash) has changed. This check has to be
done by the user at every boot and needs to be
[coded in the Firmware](#34-firmware-authentication). You will also notice this
if a new Bootloader was burned. To check the Firmwares authenticity you can
always read the [checksum](#332-firmware-checksum-fw-checksum) from the
Bootloader after you checked its
[authenticity](#21-bootloaderdevice-authenticity-protection). This way you can
ensure that the Firmware was not manipulated.

**Conclusion:
The security also relies on the Firmware, not only the Bootloader!**

### 2.8 Flash Corruption Protection
* [Brown-out detection (Fuse)](#36-fuse-settings)
* [Secure Bootloader section (SBS)](#312-secure-bootloader-section-sbs)
* [Power on self test (POST)](#314-power-on-self-test-post)

### 2.9 Firmware Brick Protection
If you upload a bricked Firmware it is always possible to enter the Bootloader
[Recovery Mode](#315-recovery-mode) again. The Bootloader does **not rely on any
part of the Firmware**. Then you can upload another Firmware instead and
continue testing. You should not lose your BK to be able to upload another
Firmware again. Also see
[Flash Corruption Protection](#28-flash-corruption-protection)

### 2.10 Open Source Guarantee
The Bootloader software is [open source](#7-license-and-copyright). This means
it can be reviewed and improved by many people. You always able to burn the
Bootloader on your own at any time. Keep in mind that the
[FID Hash](#343-firmware-id-hash-fid-hash) will change and all Bootloader and
Firmware data will be lost.

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
1. [POST](#314-power-on-self-test-post):
   Check the Bootloader and Firmware integrity (checksum) at startup
2. Special case if [no Bootloader Key was set](#322-no-bootloader-key-set)
   (after ISP the Bootloader)
  1. Force to set a Bootloader Key via USB HID
  2. Clear RAM and reboot via watchdog reset
3. [Recovery mode](#315-recovery-mode) entered
   (press special hardware keys at startup)
  * Verify the [Firmwares checksum](#332-firmware-checksum-fw-checksum)
    if the PC requests it
  * Do a [Firmware upgrade](#Firmware-upgrade)
  * [Change the Bootloader Key](#324-change-the-bootloader-key)
    with a signed and encrypted new password
  * [Authenticate the Bootloader to the PC (via Bootloader Key)](#authenticate-the-bootloader-to-the-pc) TODO???
  * Clear RAM and reboot via watchdog reset
4. Recovery mode not entered (just plug in the device)
  1. Start the Firmware
  2. The user needs to [authenticate the Firmware](#34-firmware-authentication)

TODO shorter?

#### 3.1.2 Secure Bootloader Section (SBS)
Secrets are stored inside the protected Bootloader flash section:
* Bootloader Checksum
* Bootloader Key
* Firmware Checksum
* Firmware Upgrade Counter (32bit)
* Firmware Identifier
* Firmware Violation Counter (32bit)

They are stored in **special flash page** to avoid
[flash corruption](#28-flash-corruption-protection) of the Bootloader code. The
Bootloaders flash is protected by fuses and though it is safe from
[being read via ISP](#22-isp-protection). The
[Lock Bits](#363-lock-bits-explanation) also prevent the
[Firmware from hacking the Bootloader code](#25-hacking-the-bootloader-from-the-firmware-protection).

This secure Bootloader section is also excluded from the Bootloaders checksum.
Typically the last Bootloader flash page is used to store the Bootloader
settings. The **Firmware has
[no direct (read/write) access](#363-lock-bits-explanation)** to the data except
via the [BJT](#313-bootloader-jump-table-bjt).

#### 3.1.3 Bootloader Jump Table (BJT)
The Bootloader Jump Table is used to call functions of the Bootloader from the
Firmware. This can be used to implement further security functions inside the
Firmware.

Available Jump Table functions are:
* Get [FID](#342-firmware-identifier-fid)
* Get FWUC
* Get FWVC
* AES

TODO

#### 3.1.4 Power on Self Test (POST)
The Bootloader checks the Bootloader and Firmware checksum at every boot to
**prevent flash corruption**. Some parts of the
[SBS](#312-secure-bootloader-section-sbs) are excluded from this check like the
booloader checksum and the FWVC. Fuse and Lock bits will also be checked.

TODO we need to use CRC16/32 for this. Is it essential to do this at every boot?

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
AES-256 MAC for signing
[AES-256 CBC-MAC for hashing](https://en.wikipedia.org/wiki/CBC-MAC)
[CRC](http://www.nongnu.org/avr-libc/user-manual/group__util__crc.html) for POST TODO do we need this?

TODO links

### 3.2 Bootloader Key (BK)

#### 3.2.1 Overview
Each device [comes with](TODO) a **secret unique symmetric AES-256 Bootloader Key**.

The BK can be used to **verify the devices authenticity** at any time.
The Bootloader Key is also used to **sign new Firmware upgrades**. TODO link.

The **initial BK was set by the vendor** who is also responsible for keeping the
BK secret. The BK can be **changed at any time** and is stored inside the [SBS](#312-secure-bootloader-section-sbs).

The BK needs to be kept **highly secure** and should be changed after exchanging
it over an untrusted connection (Internet!).

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
The initial Bootloader Key can be used to provide
[Firmware upgrades](#33-firmware-upgrade) without
exposing the BK to the user and enable Firmware upgrades on
[untrusted Computers](#24-compromised-pc-protection).
The responsibility of the BK can be transferred to any other user but should be
changed after exchanging it over an untrusted connection (Internet!).

#### 3.2.4 Change the Bootloader Key
You need to change the Bootloader Key from the Bootloader if
[no one was set yet](#323-initial-bootloader-key). Also you might want to change
it for security purposes or add an initial Bootloader Key. This is useful when
setting an [initial vendor Bootloader Key](#323-initial-bootloader-key) or
changing it to give the
[user control over the Bootloader](#210-open-source-guarantee).

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
