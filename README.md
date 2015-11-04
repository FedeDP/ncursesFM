# ncursesFM
Ncurses File Manager for linux

## Features:

**Press 'l' while in program to view a detailed helper message**

* Every feature you would expect by a FM.
* 2 tabs support. Their content is kept in sync. Jump between tabs with arrow keys (left/right).
* Fast browse mode: enable it with ','. It lets you jump between files by just typing their names.
* '.' to change files/dirs sorting: alphabetically (default), by size or by last modified.
* If executed on a X screen, and xdg-open is found, ncursesFM will open files with xdg-open.  
Otherwise, it will use $editor (config file defined) var. Editor var fallbacks to environment $EDITOR if none is set.
* Stats support (permissions and sizes).
* Terminal resize support.
* Search support: it will search your string in current directory tree. It can search your string inside archives too.
* Basic print support through libcups.
* Extract/compress files/folders through libarchive.
* File operations are performed in a different thread. You'll get a notification when the job is done.
* If you try to quit while a job is still running, you'll be asked if ncursesFM must wait for the thread to finish its work.
* You can queue as many file operations as you wish, they'll be taken into care one by one.
* It supports following command line switches: "--editor=", "--starting-dir=" and "--inhibit={0/1}".

**Optional (compile time) features that require sd-bus API (systemd)**
* Powermanagement inhibition while processing a job(eg: while pasting a file) to avoid data loss. It is switched off by default. Enable this feature from the config file or with "--inhibit=1" cmdline switch.
* It can mount your external usb drives/ISO files through udisks2. For usb drives mount, you also need libudev to list all mountable drives.
* ISO files cannot be unmounted from within ncursesFM: they will be automagically unmounted and removed by udev when no more in use (eg after a reboot).
* It can install your distro package files: pressing enter on a package file will ask user if he wants to install the package. It relies upon packagekit, so it should be distro agnostic.

---

If built with libconfig support, it reads following variables from /etc/default/ncursesFM.conf...remember to set them!
* editor -> editor used to open files, in non X environment (or when xdg-open is not available)
* show_hidden -> whether to show hidden files by default or not.
* starting_directory -> default starting directory.
* use_default_starting_dir_second_tab -> whether to use "starting_directory" when opening second tab. Otherwise current tab will be used.
* inhibit -> whether to inhibit powermanagement functions. Defaults to 0.

## Build requirements

* ncurses    -> UI
* libarchive -> archiving/extracting support
* pkg-config -> to manage libraries link in makefile
* glibc      -> to set locale
* git        -> to clone repo

## Optional compile time dependencies

* libcups   -> print support
* libconfig -> config file parsing
* libx11    -> check whether ncursesFM is started in a X environment or not.
* sd-bus    -> to switch off powermanagement functions, devices/iso mount and packages installation.
* libudev   -> needed to list mountable drives. Useful only if built with sd-bus.

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

make {install/uninstall) require root privileges.

![Alt text](ncursesfm.png?raw=true)