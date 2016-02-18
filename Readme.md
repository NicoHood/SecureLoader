# SecureLoader

## Goal
Our firmware deals with some information that is secured.
A manipulated firmware or bootloader could introduce backdoors to leak this secure information.
The most important thing we want to achieve in our project
is to ensure that our firmware and bootloader is running unmodified without any backdoors.

Therefore we need to ensure a few things:
* The firmware is the one we uploaded and has not been reuploaded (integrity?? TODO)
* The firmware was not manipulated by a bootloader that runs before executing the firmware

This leads us to a few conclusions:
* The **firmware itself** needs to authenticate itself at **every start** to ensure it is the correct firmware
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

### Bootloader Mode/Device Start TODO name
0. Device startup, always run the bootloader (BOOTRST=0)
1. POST: Check the bootloader and firmware integrity (checksum) at startup
2. Special case if no boot key was set/removed (after ISP the bootloader or removing the BK)
  1. Force to set a bootloader key via USB HID
  2. Reboot via watchdog reset
3. Recovery mode entered (press special hardware keys at startup)
  1. Authenticate the bootloader to the PC (via bootloader key) TODO LINK
  2. Provide an option to verify the firmwares integrity (checksum) if the PC requests it TODO link
  3. Authenticate the PC to the bootloader (via bootloader key)
    1. Provide an option to flash new firmware TODO link
    2. Provide an option to change the bootloader key from the firmware TODO link
  4. Reboot via watchdog reset
4. Recovery mode not entered (just plug in the device)
  1. Write the firmware identifier into a special ram position
  2. Start the firmware
  3. The user needs to authenticate the firmware TODO link

### Secure bootloader section (SBS)
The bootloader has to store a few settings in the protected bootloader flash section:
* Bootloader Checksum
* Bootloader Key
* Firmware Checksum
* Firmware counter
* Firmware Identifier

They are stored in **special flash page** to avoid flash corruption of the bootloader code.
The bootloaders flash is protected by fuses and though it is safe from being read via ISP.
**TODO why is it secure from malicious firmware?**
This secure bootloader section is also excluded from the bootloaders checksum.
Typically the last bootloader flash page is used to store the bootloader settings.

### Bootloader key (BK)

#### Authenticate the bootloader to the PC
The Bootloader key is used to **authenticate the bootloader to the PC**.
The PC sends a random challenge to the bootloader which it has to hash with the BK.
The bootloaders integrity is then ensured, the bootloader can be trusted.
TODO a long timeout/password is required here to not brute force all combinations

#### Authenticate the PC to the bootloader
It also prevents attackers from uploading new firmwares to the device.
Before the PC can upload a firmware the **PC has to authenticate itself to the bootloader***.
With the same symmetric BK the PC answers a challenge from the bootloader.

TODO on wrong authentication set some flag, that the firmware can read next time
TODO a good random challenge is required here

#### Flash new firmware
The new firmware then can be uploaded to the MCU after you have authenticated from the PC.

* Delete firmware identifier
* Increase firmware counter
* Write firmware
* Verify the firmware checksum
* Firmware is written successful
* Write new random firmware identifier with new firmware counter
* Firmware is corrupted/checksum incorrect
* Write only the new firmware counter
* Send an error to the PC

A random challenge is used instead of a signed firmware
to prevent firmware downgrades with a known firmware hash.
It also makes the code faster and more simple.

#### Change the bootloader key
You need to change the bootloader key from the bootloader if no one was set yet.
Also you might want to change it for security purposes or as initial bootloader key.
You can also force to change the bootloader key the next time you want to flash a firmware.
This is useful when setting an initial vendor bootloader key.

#### Remove bootloader key / Change bootloader key from the firmware
If the PC is not trusted, you want to change the bootloader key from the firmware.
Therefor you can remove the key after you have authenticated from the PC.
The firmware will be directly booted and you need to change the bootloader key.

The firmware gets a special flag via RAM to notice a not set bootloader key.
The firmware is only allowed to change the bootloader key if it was not set.
If you do not set the bootloader key, the bootloader requires you to set it next time via PC.
Therefor it is **highly recommended to only use this feature if its essentially required**.

#### Initial bootkey
A **unique BK is initially set by the vendor** and exchanged after receiving the device.
This ensures the **integrity of the initial bootloader** and is stored inside the SBS.
The BK needs to be kept **highly secure** and changed after the first authentication.

#### No bootloader key was set
Situation where it can happen that no bootloader key was set:
* The bootloader was just burned via ISP
* The bootloader key from the firmware was not successful TODO link

If no bootkey was set at startup you need to do it from the bootloader via USB HID.
It is intended that the firmware will not be loaded until a bootloader key is set.
This feature is essential to ensure security and give a clear warning.
**Never ship a device without an inital vendor ID.** TODO link

### Firmware Checksum (FW Checksum)
The Firmware Checksum is generated after uploading a new firmware by the bootloader.
The bootloader checks the FW checksum at every boot to **prevent flash corruption**.
It stores the FW checksum in the secure bootloader section.
The PC can **verify the Firmware Integrity** with the Firmware Checksum and the Firmware Counter.
The FW Hash is also used as part of the FID.

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
The FID is used in the Firmware ID Hash.

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
