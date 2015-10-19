# ncursesFM
Ncurses File Manager for linux

## Features:

* File/dir functions support: copy/paste/cut (cut = move, if on the same FS), new/remove, rename.
* 2 tabs support. Their content is kept in sync.
* Fast browse mode: enable it with ','. It lets you jump between files by just typing their names.
* If executed on a X screen, and xdg-open is found, ncursesFM will open files with xdg-open.
* Otherwise, it will open (text and blank)files with $editor (ncursesFM.conf defined) var. Editor var fallbacks to environment $EDITOR if none is set.
* Stats support (permissions and sizes).
* Detailed in-program helper message -> press 'l'.
* It supports terminal resize.
* Search support: it will search your string in current directory tree, and, if anything was found, you'll be able to move to its folder.
* It can search your string inside archives too. Then, if found, you can go to the folder of the archive, to extract it.
* Basic print support: you need libcups for this to work.
* Extract/compress files/folders through libarchive.
* File operations are performed in a different thread. You'll get a notification when the job is done.
* If you try to quit while a job is still running, you'll be asked if ncursesFM must wait for the thread to finish its work.
* You can queue as many file operations as you wish, they'll be taken into care one by one.
* It supports following command line switches: "--editor=", "--starting-dir=" and "--inhibit={0/1}".
* Powermanagement inhibition while processing a job(eg: while pasting a file) to avoid data loss. It relies upon systemd/logind (sd-bud API), so it requires a systemd booted system.
It is switched off by default. You can enable this feature from the config file or with "--inhibit=1" cmdline switch.
* It can mount your external usb drives/ISO files through sd-bus and udisks2. For usb drives mount, you also need libudev to list all mountable drives.
* ISO files cannot be unmounted from within ncursesFM: they will be automagically unmounted and removed by udev when no more in use (eg after a reboot).
* It can install your distro package files: pressing enter on a package file will ask user if he wants to install the package. It relies upon packagekit, so it should be distro agnostic.

If built with libconfig support, it reads following variables from /etc/default/ncursesFM.conf...remember to set them!
* editor -> editor used to open files, in non X environment (or when xdg-open is not available)
* show_hidden -> whether to show hidden files by default or not.
* starting_directory -> default starting directory.
* use_default_starting_dir_second_tab -> whether to use "starting_directory" when opening second tab. Otherwise current tab will be used.
* inhibit -> whether to inhibit powermanagement functions. Defaults to 0.


## Build requirements

* ncurses    -> UI
* libarchive -> archiving/extracting support
* libmagic   -> internal mimetype support
* pkg-config -> to manage libraries link in makefile
* glibc      -> for locale config

## Optional compile time dependencies

* libmagic  -> to avoid (in a not X environment) opening every kind of file with $EDITOR (eg: images/videos), and only open text files.
* libcups   -> print support
* libconfig -> config file parsing
* libx11    -> check whether ncursesFM is started in a X environment or not.
* sd-bus    -> to switch off powermanagement functions, devices/iso mount and packages installation.
* libudev  -> needed to list mountable drives.

## Runtime dependencies

**required:**
* ncurses, libmagic, libarchive, glibc plus every optional build dep if compiled with its support.

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