# SecureLoader

The idea is to create an USB HID bootloader that can easily upgrade an AVR firmware
with even increasing the security of the device. This paper describes use cases,
features, potential attacks and solutions to prevent them.

This readme describes a **temporary concept** with some ideas.
It is **not final**. Contributions appreciated.

### Assumptions
The following assumptions describe a worst case scenario and might differ to the real world.
* The attacker has full physical access of the device.
* The device can be opened and an ISP can be used without a visible change.
* The attacker is able to steal the device and put it back at any time.
* The initial password is kept secure by the vendor until the user requests it.
* The uploading PC is compromised when doing firmware upgrades.
* The AVR is programmed with the correct fuses.

### Boot Process
0. Device startup, always run the bootloader (BOOTRST=0)
1. POST: Check the bootloader and firmware integrity (checksum) at startup
2. Special case if no boot key was set (after ISP the bootloader)
  1. Force to set a bootloader key via USB HID
  2. Reboot via watchdog reset
3. Recovery mode entered (press special hardware keys at startup)
  * Verify the firmwares checksum if the PC requests it TODO link
  * Do a [firmware upgrade](#firmware-upgrade)
  * Change the bootloader key with a signed and encrypted new password
  * [Authenticate the bootloader to the PC (via bootloader key)](#authenticate-the-bootloader-to-the-pc)
  * Reboot via watchdog reset
4. Recovery mode not entered (just plug in the device)
  1. Write the firmware identifier into a special ram position
  2. Start the firmware
  3. The user needs to authenticate the firmware TODO link


### Reboot Mechanism
TODO magic bootkey in RAMEND (16 bit)
Use a single bit instead?
TODO dont allow to start the bootloader via firmware for security?
~~use hwb instead of bootrst~~ -> POST
Use the other bits for FID and firmware upgrade violation? but how to reset the flag and let the (all!) user know?

### Secure bootloader section (SBS)
The bootloader has to store a few settings in the protected bootloader flash section:
* Bootloader Checksum
* Bootloader Key
* Firmware Checksum
* Firmware Counter
* Firmware Identifier

They are stored in **special flash page** to avoid flash corruption of the bootloader code.
The bootloaders flash is protected by fuses and though it is safe from being read via ISP.
**TODO why is it secure from malicious firmware?**
This secure bootloader section is also excluded from the bootloaders checksum.
Typically the last bootloader flash page is used to store the bootloader settings.

### Bootloader key (BK)

TODO length
TODO encryption algorithm
TODO stored in sbs

used for:
firmware authentification when doing fw upgrade
Authenticate the bootloader to the PC

The BK needs to be kept **highly secure** and should be changed be changed after exchanging.

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

**Discussion: is this feature essential?**

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

### Power on self test (POST)
The bootloader checks the bootloader and firmware checksum at every boot to **prevent flash corruption**.

### Firmware Checksum (FW Checksum)
The Firmware Checksum is generated after uploading a new firmware by the bootloader and stored in the SBS.
The PC can **verify the Firmware Integrity** with the Firmware Checksum and the Firmware Counter.
The Firmware checksum is used as part of the POST and the FID.

TODO hash algorithm: crc32?

### Firmware Counter (FCNT)
The bootloader keeps track of the number of firmware uploads.
This information is important to check if someone even tried to hack your device.
The firmware counter is part of the FID.

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

The Firmware ID Hash consists of the FID and a (per firmware user) nonce.
This way the nonce can be changed at any time if an unauthorized people sees the FID Hash.
The firmware will display the FID Hash and the **user needs to verify it**.

**The security also relies on the firmware, not only the bootloader!**

### Fuse Settings

ATmega32u4 fuse settings:
* Low: 0xFF
* High: 0xD8
* Extended: 0xF8
* Lock: 0x??

**TODO check lock bits**

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

### Other implementation details:
 * All secret keys needs to be saved in the bootloader section
 * Those keys needs to be on whole flash page to avoid flash corruption
 * Use brown out detection to avoid flash corruption
 * The bootloader could share its AES implementation to the firmware code.
 * Do not leave any trace in EEPROM or RAM
 * Add an user interface to download the firmware again
 * TODO MAYBE let the bootloader key be changed (set a flag ONCE for a single boot) from the user firmware for more security.
 * pass the FID via ram and also anothe byte to identify wrong bootloader attacks, BK forced changed and future use things.

### Attacks TODO rename to "protection" caption
 * The bootloader password is kept secure. (Still got firmware integrity check)
 * A PC virus could upload a malicious firmware (button press, checksum and firmware integrity check)
 * **The firmware cannot hack the bootloader (Still got firmware integrity check). TODO**
 * The user (or the flashing tool) is responsible for not uploading malicious firmware.




## Provided guarantees

### Flash corruption protection
* Brown out detection
* Secure bootloader section
* Power on self test

TODO links

### Flashing unauthorized firmware protection
Only signed firmwares can be flashed with the bootloader.
You can even flash the device from a not trusted PC.

The bootloader key to sign the firmware needs to be kept secure.
This can be handled by the vendor or the user.

Flashing a new firmware will also change the firmware ID Hash (TODO link).
The user is able notice a firmware change, even with an ISP.

Firmware downgrades (replay attacks) are prevented via bootloader ket changes.

### Hacking the bootloader from the firmware protection
**TODO this needs to be checked and carefully coded. Maybe we dont even need to ensure this**

Even if a firmware vulnerability was found you can hardly hack the bootloader.
AVR use [harvard architecture](https://en.wikipedia.org/wiki/Harvard_architecture),
not [Von Neumann architecture](https://en.wikipedia.org/wiki/Von_Neumann_architecture).
Also you can do a [firmware upgrade](TODO) to get rid of the security vulnerability.

But the bootloader prevents from [uploading unauthorized firmware](TODO) anyways.
You first have to leak the bootloader key.
And then also the [Firmware authenticity protection](TODO) will take account of this.

### Firmware authenticity protection
You will notice a firmware change because the FID Hash has changed.
You will also notice this if a new bootloader was burned.
To check the firmwars authenticity you can always read the checksum from bootloader.
This way you can ensure that the bootloader did not manipulate the firmware.

### Bootloader authenticity protection
Overwriting the bootloader via ISP will also overwrite the FID Hash and BK.
Bootloader authenticity can be checked from the bootloader. TODO link, TODO do we implement this?
This can be used to verify the device after receiving it from the vendor.

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

### Open source guarantee
The bootloader design is open source. This means it can be reviewed by many people.
Preventing flashing unauthorized firmware does not essentially restrict custom firmwares. TODO link

You are still able to burn again the bootloader on your own.
Keep in mind that the FID Hash will change and all bootloader and firmware data will be lost.

### License
TODO


### FAQ

TODO improve

##### Why not PublicKey/PrivateKey?
Uses a lot of flash and is not required.
Does not secure anything more. Pc is not more trusted than before.
Symmetric AES is used anyways

##### Why not only allow signed firmwares? TODO this is wrong
As a developer I like to play with open source devices. The user should also be able to make use of the bootloader. It does not lower the security, if the user carefully checks the firmware checksum on the PC. This needs to be integrated (forced) into the flashing tool.

### TODO
TODO write Firmware Identifier etc capital/idential
TODO check integrity and authenticity words if they match
TODO numbe the section, to make it more clear what sub headline is for what topic.
 * Add minimum RAM requirement if the bootloader is finished.
