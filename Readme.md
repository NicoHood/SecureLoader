# SecureLoader

The idea is to create an USB HID bootloader that can easily upgrade an AVR firmware
with even increasing the security of the device. This paper describes use cases,
features, potential attacks and solutions to prevent them.

This readme describes a **temporary concept** with some ideas.
It is **not final**. Contributions appreciated.

## Goal
Our firmware deals with some information that is secured.
A manipulated firmware or bootloader could introduce backdoors to leak this secure information.
The most important thing we want to achieve in our project
is to ensure that our firmware and bootloader is running unmodified without any backdoors.

Therefore we need to ensure a few things:
* The firmware is the one we uploaded and has not been reuploaded (integrity?? TODO)
* The firmware was not manipulated by a bootloader that runs before executing the firmware

This leads us to a few conclusions:
* The **firmware needs to authenticate itself at every start** to ensure it is the correct firmware
* TODO the nonce is cleared after every ISP chip erase
* This is one of the **most important goals**. The security also relies on the firmware, not only the bootloader!
* The bootloader is not responsible for uploading an unauthorized firmware
* We need to ensure that the bootloader is not able to manipulate the firmware before it runs

To solve this we need:
* At least a bootloader on the bootloader section, to ensure its not a backdoor installed
* The bootloader cannot be read from an ISP. An ISP can only do a full flash erase.
* The bootloader needs to always authenticate itself first via a (symmetric) secret key challenge
* If the bootloader is trusted, you can trustfully check the firmwares checksum
* The bootloader needs to prevent hacks to leak the secret key or modify the bootloader from the firmware or an ISP
* The initial secret key has to be unique from the vendor, is exchanged after shipping the device and should be changed afterwards
* The (symmetric key) should **not always** be used to authenticate the bootloader at **every startup**,
  since the PC then always have the key stored, use the firmware instead for this purpose

Optionally the bootloader could also be used to:
* The bootloader needs to check its and the firmwares integrity (checksum) before starting
* The bootloader is able to provide a recovery mode to authenticate itself
* The bootloader is also be able to verify the firmware integrity (checksum) at any time
* The bootloader is able to flash new firmwares to authorized people
* The bootloader needs to authenticate itself to the PC before flashing new firmwares
* A PK/SK pair would improve the authenticity
* The PC needs to authenticate itself to the bootloader for flashing new firmwares
* A PK/SK pair would not improve this TODO?

Advantages:
* You can ship the device with a secure initial bootloader
* You can(!) ship the device without an initial firmware (force latest version install)
* You can still recompile the bootloader with a changeable initial key
* You can recompile new firmwares and still develop new stuff


### Assumptions
* The attacker has full physical access of the device.
* The device can be opened an an ISP can be used without a visible change.
* The attacker is able to steal the device and put it back at any time.
* The initial vendor password is kept secure until the device has arrived.
* The AVR is programmed with the correct fuses.
* The bootloader password is kept secure. (Still got firmware integrity check)
* A PC virus could upload a malicious firmware (button press, checksum and firmware integrity check)
* **The firmware cannot hack the bootloader (Still got firmware integrity check). TODO**
* The user (or the flashing tool) is responsible for not uploading malicious firmware.

### Bootloader Mode/Device Start TODO name
0. Device startup, always run the bootloader (BOOTRST=0)
1. POST: Check the bootloader and firmware integrity (checksum) at startup
2. Special case if no boot key was set (after ISP the bootloader)
  1. Force to set a bootloader key via USB HID
  2. Reboot via watchdog reset
3. Recovery mode entered (press special hardware keys at startup)
  * Verify the firmwares checksum if the PC requests it TODO link
  * Flash a new signed firmware
  * Change the bootloader key with a signed and encrypted new password
  * [Authenticate the bootloader to the PC (via bootloader key)](#authenticate-the-bootloader-to-the-pc)
  * Reboot via watchdog reset
4. Recovery mode not entered (just plug in the device)
  1. Write the firmware identifier into a special ram position
  2. Start the firmware
  3. The user needs to authenticate the firmware TODO link

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

However the vendor still can (and should) give away the inital bootloader key to the user if it requests it.
Then the user can **compile and sign his own firmwares** and play with the device.
Then the user is responsible for further firmware upgrades.

To prevent replay attacks (firmware downgrades) the bootloader key should be changed after each upload.
After exchanging the initial bootloader key from the vendor to the user it should be changed too.

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

TODO how to remove this flag, only authorized peole should be able to?

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





### Other implementation details:
 * All secret keys needs to be saved in the bootloader section
 * Those keys needs to be on whole flash page to avoid flash corruption
 * Use brown out detection to avoid flash corruption
 * The bootloader could share its AES implementation to the firmware code.
 * Do not leave any trace in EEPROM or RAM
 * Add an user interface to download the firmware again
 * TODO MAYBE let the bootloader key be changed (set a flag ONCE for a single boot) from the user firmware for more security.
 * pass the FID via ram and also anothe byte to identify wrong bootloader attacks, BK forced changed and future use things.


```
you need to sign the firmware with the hash. the vendor (you) know the initial hash. so you can always sign any firmware. you can transfere it via an insecure email, since its is just a hash. the bootloader will only accept signed firmwares. you dont even need a password to flash the firmware. so people who dont want to remember any bootloader key can keep the responsiblity to you.
for those who want to develop their own firmwares they need to request the key from you. they should change it afterwards for security reasons though. with the key they can singn the firmware themselves. but need to keep the key secure themselves

and how do we prevent firmware downgrades? that would mean ANYONE with the firmware can flash an old firmware again.
simply encrypt the new bootloader password (shiped with the firmware) with the current bootloader password
so you pass a new bootloader password with the new firmware and the firmware cannot be flashed again?
you (as vendor) can provide password changes with thefirmware
```

### FAQ

Why not PublicKey/PrivateKey?
Uses a lot of flash and is not required.
Does not secure anything more. Pc is not more trusted than before.
Symmetric AES is used anyways

Why not only allow signed firmwares?
As a developer I like to play with open source devices. The user should also be able to make use of the bootloader. It does not lower the security, if the user carefully checks the firmware checksum on the PC. This needs to be integrated (forced) into the flashing tool.

### TODO
TODO write Firmware Identifier etc capital/idential
TODO check integrity and authenticity words if they match
TODO numbe the section, to make it more clear what sub headline is for what topic.
 * Add minimum RAM requirement if the bootloader is finished.
