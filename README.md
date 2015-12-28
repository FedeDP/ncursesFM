# NcursesFM
Ncurses File Manager for linux.  

It aims to be as user friendly and lightweight as possible, while being good looking (my own tastes) and simple.  
Being simple doesn't imply being useless; indeed it is a full featured fm.  
It can be built with a very small set of dependencies, as i tried to make as many deps as possible optional (compile time or runtime).

## Features:

**Press 'l' while in program to view a detailed helper message**

* Every feature you would expect by a basic FM.
* Terminal window resize support.
* 2 tabs support. Their content is kept in sync. Jump between tabs with arrow keys (left/right).
* Fast browse mode: enable it with ','. It lets you jump between files by just typing their names.
* '.' to change files/dirs sorting: alphabetically (default), by size or by last modified.
* If executed on a X screen, and xdg-open is found, ncursesFM will open files with xdg-open.
Otherwise, it will use $editor (config file defined) var. It fallbacks to environment $EDITOR if none is set.
* Stats support (permissions and sizes).
* Search support: it will search your string in current directory tree. It can search your string inside archives too.
* Basic print support through libcups.
* shasum support (both printing file shasum/checking a given one), if built with openssl support.
* Extract/compress files/folders through libarchive.
* File operations are performed in a different thread. You'll get a notification when the job is done.
* If you try to quit while a job is still running, you'll be asked if ncursesFM must wait for the thread to finish its work.
* You can queue as many file operations as you wish, they'll be taken into care one by one.
* It has an internal udev monitor, to poll udev for new devices. It can automount new connected devices too.

**Optional (compile time) features that require sd-bus API (systemd)**
* Powermanagement inhibition while processing a job (eg: while pasting a file) to avoid data loss.
* It can (un)mount drives/usb sticks/ISO files through udisks2. For drives/usb sticks mount, you also need libudev to list all mountable drives.  
ISO files cannot be unmounted from within ncursesFM: they will be automagically unmounted and removed by udev when no more in use (eg: after a reboot).
* If built with libudev support, you can enable devices automount too.
* It can install your distro package files: pressing enter on a package file will ask user if he wants to install the package. It relies upon packagekit, so it should be distro agnostic.

---

If built with libconfig support, it reads following variables from /etc/default/ncursesFM.conf...remember to set them!
* editor -> editor used to open files, in non X environment (or when xdg-open is not available)
* show_hidden -> whether to show hidden files by default or not.
* starting_directory -> default starting directory.
* use_default_starting_dir_second_tab -> whether to use "starting_directory" when opening second tab, or to open it in current directory.
* inhibit -> whether to inhibit powermanagement functions. Defaults to 0.
* starting_helper -> whether to show helper win after program started. Defaults to 1.
* monitor -> wheter to enable udev monitor. Defaults to 1.
* automount -> whether to enable devices automount. Defaults to 0.
* loglevel -> to change program loglevel. Defaults to 0.
* persistent_log -> to enable log persistency across program restarts. Defaults to 0.

NcursesFM ships an autocompletion script for cmdline options. It supports following options:
* "--editor" /path/to/editor
* "--starting_dir" /path/to/dir
* "--helper" {0,1} to disable(enable) helper window show at program start.
* "--inhibit" {0,1} to disable(enable) powermanagement inhibition.
* "--monitor" {0,1} to disable(enable) udev monitor.
* "--automount" {0,1} to disable(enable) automount of connected devices.
* "--loglevel" {0,1,2,3} to change program loglevel.
* "--persistent_log" {0,1} -> to disable(enable) log persistency across program restarts.

Log file is located at "$USERHOME/.ncursesfm.log". It is overwritten each time ncursesFM starts. Log levels:
* 0 : log errors only.
* 1 : log warnings and errors.
* 2 : log info messages, warnings and errors.
* 3 : log disabled.

## Build requirements

* ncurses    -> UI
* libarchive -> archiving/extracting support
* pkg-config -> to manage libraries link in makefile
* glibc      -> to set locale
* git        -> to clone repo

## Optional compile time dependencies

* libcups   -> print support.
* libconfig -> config file parsing.
* libx11    -> check whether ncursesFM is started in a X environment or not.
* sd-bus    -> to switch off powermanagement functions, devices/iso mount and packages installation.
* libudev   -> needed to list mountable drives and to enable udev monitor.
* openssl   -> for shasum functions support.

## Runtime dependencies

**required:**
* ncurses, libarchive, glibc plus every optional build dep if compiled with its support.

**optional:**
* xdg-utils (if compiled with libx11 support)
* a message bus (dbus/kdbus) plus: logind (for inhibition support), udisks2 (for mount support), packagekit (for packages installation support).

## Install instructions:

*Archlinux users can install ncursesFM from aur*: https://aur.archlinux.org/packages/ncursesfm-git/

Clone the repo and move inside new dir, then:

    $ make
    $ make install

To remove, just move inside the folder and run:

    $ make uninstall

make {install/uninstall} require root privileges unless you specify a $DESTDIR variable to install/uninstall targets.  
Be aware that it will disable config file support and bash autocompletion script (you can still source it manually anyway) though.

![Alt text](ncursesfm.png?raw=true)