# MupenGUI
### Summary
Frontend Application written in Vala and C.
This frontend features:

* Basic graphics configuration (Fullscreen, resolution ...)
* Mupen64plus plugins configuration
* Complete input configuration (including keyboard, mouse and joysticks)
* Rom directory listing and remembering the last opened Rom directory

Although primarily targeted for the Elementary OS Desktop and AppStore, it may work under any Desktop Environment
given that libgranite is available - including KDE.

Differently from most frontends available for Mupen64plus, this frontend interfaces
directly with the emulator by loading its dynamic library instead of sending
commands to the Mupen64plus CLI frontend. This allows for a greater control over
the emulation and its configurations.

### Building and Installing

Your system must be at least compatible with a Gnome Environment and libgranite must be installed.

**Configure and create a build directory**

`meson build/`

**If you want to install it on your system, use**

`meson build/ --prefix=/usr`
*or anyother prefix you'd like*

**Go into the build directory and run**

`ninja`

**and for installing...**

`ninja install`
*must be sudo if you're installing on your system*

###Credits
Base N64 Controller used to design the icon by David Swanson from the Noun Project.
