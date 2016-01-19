[![Build Status](https://travis-ci.org/FedeDP/ncursesFM.svg?branch=master)](https://travis-ci.org/FedeDP/ncursesFM)

# NcursesFM

Ncurses File Manager for linux.

It aims to be as user friendly and lightweight as possible, while being good looking (my own tastes) and simple.  
Being simple doesn't imply being useless; indeed it is a full featured fm.  
It can be built with a very small set of dependencies, as i tried to make as many deps as possible optional (compile time or runtime).  
Despite its features, it still manages to be very light on resources: full build uses only c3.2Mb of (private + shared)ram; disabling libcups cuts down ram usage to c2.5Mb; of these, c50% are used by ncurses screen.  
Ncurses screen ram usage depends upon window size; my tests were made on a fullscreen session on my 15" laptop, using ps_mem.

![](https://github.com/FedeDP/ncursesFM/raw/master/ncursesFM.png)

## Features:

**Press 'l' while in program to view a detailed helper message**

* Every feature you would expect by a basic FM.
* Terminal window resize support.
* 2 tabs support. Their content is kept in sync. Jump between tabs with arrow keys (left/right).
* Simple clock and battery monitor (it will check every 30s for changes). It will list every battery. Battmon needs libudev.
If you've got Upower installed, AC (dis)connection will refresh battery status instantly, instead of waiting up to 30s until next refresh.
* Fast browse mode: enable it with ','. It lets you jump between files by just typing their names.
* '.' to change files/dirs sorting mode: alphabetically (default), by size, by last modified or by type.
* If executed on a X screen, and xdg-open is found, ncursesFM will open files with xdg-open.
Otherwise, it will use $editor (config file defined) var. It fallbacks to environment $EDITOR if none is set.
* Stats support (permissions and sizes).
* Bookmarks support: it will load bookmarks from $HOME/.config/user-dirs.dirs and $HOME/.config/ncursesFM-bookmarks.
You can add whatever type of file you wish as bookmark from within ncursesFM. You can remove bookmarks too.
* Search support: it will search your string in current directory tree. It can search your string inside archives too.
* Basic print support through libcups.
* shasum support (both printing file shasum/checking a given one), if built with openssl support.
* Extract/compress files/folders through libarchive.
* Long file operations are performed in a different thread. You'll get a notification when the job is done.
* If you try to quit while a job is still running, you'll be asked if ncursesFM must wait for the thread to finish its work.
* You can queue as many file operations as you wish, they'll be taken into care one by one.

**Optional (compile time) features that require sd-bus API (systemd)**
* Powermanagement inhibition while processing a job (eg: while pasting a file) to avoid data loss.
* Internal udisks2 monitor, to poll for new devices. It can automount new connected devices too.
Device monitor will list only mountable devices, eg: dvd reader will not be listed until a cd/dvd is inserted.
* Drives/usb sticks/ISO files (un)mount through udisks2. For drives/usb sticks mount, you also need libudev.
* Distro package files installation.
* It can reveice AC (dis)connection events from upower, to instantly update battery monitor.

---

If built with libconfig support, it reads following variables from /etc/default/ncursesFM.conf...remember to set them!
* editor -> editor used to open files, in non X environment (or when xdg-open is not available)
* show_hidden -> whether to show hidden files by default or not. Defaults to 0.
* starting_directory -> default starting directory.
* use_default_starting_dir_second_tab -> whether to use "starting_directory" when opening second tab, or to open it in current directory. Defaults to 0.
* inhibit -> whether to inhibit powermanagement functions. Defaults to 0.
* starting_helper -> whether to show helper win after program started. Defaults to 1.
* automount -> whether to enable devices automount. Defaults to 0.
* loglevel -> to change program loglevel. Defaults to 0.
* persistent_log -> to enable log persistency across program restarts. Defaults to 0.
* bat_low_level -> to set threshold to signal user about low battery. Defaults to 15%.

NcursesFM ships an autocompletion script for cmdline options. It supports following options:
* "--editor" /path/to/editor
* "--starting_dir" /path/to/dir
* "--helper_win" {0,1} to disable(enable) helper window show at program start.
* "--inhibit" {0,1} to disable(enable) powermanagement inhibition.
* "--automount" {0,1} to disable(enable) automount of connected devices.
* "--loglevel" {0,1,2,3} to change program loglevel.
* "--persistent_log" {0,1} -> to disable(enable) log persistency across program restarts.
* "--low_level" {$level} -> threshold to signal user about low battery.

Log file is located at "$HOME/.ncursesfm.log". It is overwritten each time ncursesFM starts (unless persistent_log option is != 0). Log levels:
* 0 : log errors only.
* 1 : log warnings and errors.
* 2 : log info messages, warnings and errors.
* 3 : log disabled.

## Build requirements

* ncurses    -> UI
* libarchive -> archiving/extracting support
* pkg-config -> to manage libraries link in makefile
* glibc      -> to set locale and for mntent functions (useful only if compiled with libudev too)
* git        -> to clone repo

## Optional compile time dependencies

* libcups   -> print support.
* libconfig -> config file parsing.
* libx11    -> check whether ncursesFM is started in a X environment or not, and xdg-open support.
* sd-bus    -> needed for powermanagement inhibition functions, devices/iso mount and packages installation.
* libudev   -> needed for devices/iso mount, and batteries polling.
* openssl   -> for shasum function support.

## Runtime dependencies

**required:**
* ncurses, libarchive, glibc plus every optional build dep if compiled with its support.

**optional:**
* if compiled with libx11 support: xdg-utils.
* if compiled with sd-bus support: a message bus (dbus/kdbus) plus logind (for inhibition support), udisks2 (for mount support), packagekit (for packages installation support) and upower (to get AC (dis)connection events).

## Known bugs
* installing packages segfaults if package is for the wrong arch, and packagekit daemon segfaults too: https://github.com/hughsie/PackageKit/issues/87.

## Install instructions:

*Archlinux users can install ncursesFM from aur*: https://aur.archlinux.org/packages/ncursesfm-git/

On Ubuntu install required packages:

    # apt-get install libncursesw5-dev libarchive-dev pkg-config git build-essential

Optional:

    # apt-get install libcups2-dev libconfig-dev libx11-dev libsystemd-dev libudev-dev libssl-dev

Clone the repo and move inside new dir, then:

    $ make
    # make install

To remove, just move inside the folder and run:

    # make uninstall

make {install/uninstall} require root privileges unless you specify a $DESTDIR variable to install/uninstall targets.
Be aware that it will disable config file support and bash autocompletion script (you can still source it manually anyway) though.