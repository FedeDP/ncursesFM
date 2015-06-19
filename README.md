# ncursesFM
Ncurses File Manager for linux

## Features:

* File/dir functions support: copy/paste/cut, new/remove, rename. Copy/cut: to remove a file previously selected for copy just press c/x again on it.
* 2 tabs support.
* ISO mount support -> you must have fuseiso installed.
* If executed on a X screen, and xdg-open is found, ncursesFM will open files with xdg-open.
* Otherwise, it will open (text and blank)files with $editor (ncursesFM.conf defined) var.
* Hidden files view support.
* Stats support (permissions and sizes).
* In-program helper message -> press 'l'.
* Sync between tabs.
* Rename file/folders support.
* Shasum/md5sum, through openssl.
* Move file/folders support -> use cut for this. If a file is "cut" on the same filesystem, it will be moved, otherwise it will be copied.
* Search support: it will search your string in current directory tree, and, if anything was found, you'll be able to open it, if it was a file, or to move in your searched location.
* It can search your string inside archives too. Then, if found, you can go to the folder of the archive, to extract it.
* Basic print support: you need "libcups" for this to work.
* Extract (compressed) archive through libarchive.
* Compress files/folders through libarchive -> to select files/folders, use c/x (same as copy/cut)
* View current file's mimetype through libmagic.
* Pasting, compressing, extracting, printing and searching are executed in other threads. You'll get a notification when the job is done.
* If you try to quit while a {pasting, compressing, extracting} thread is still running, you'll be asked if ncursesFM must wait for the thread to finish its work. (Printing thread and search thread are safer, no data corruption is possible)
* You can queue as many pasting/archiving threads as you wish, they'll be taken into care one by one.

It reads following variables from /etc/default/ncursesFM.conf (using libconfig)...remember to set them!
* editor -> editor used to open files, in non X environment (or when xdg-open is not available)
* show_hidden -> whether to show hidden files by default or not.
* starting_directory -> default starting directory.
* use_default_starting_dir_second_tab -> whether to use "starting_directory" when opening second tab. Otherwise current tab will be used.

**IT DOES NOT SUPPORT TERMINAL RESIZE**. It is meant to be used maximized, or from a tty.

## Build requirements

* ncurses -> UI
* libconfig -> config file parsing
* libcups -> print support
* libarchive -> archiving/extracting support
* openssl -> md5/shasum support
* libx11 -> check whether ncursesFM is started in a X environment or not.
* libmagic -> mimetype support

## Runtime dependencies

* required: ncurses, libconfig, libmagic, libarchive
* optional: fuseiso, libcups, openssl, libx11, xdg-utils
In Archlinux libmagic is provided by "file" package, that is part of base-devel group. So it is not listed inside PKGBUILD.


## Install instructions:

*Archlinux users can install ncursesFM from aur*: https://aur.archlinux.org/packages/ncursesfm-git/

* clone the repo
* cd inside the new dir
* "make"
* "make install" (as root)
* To remove, just run "make uninstall" (as root).

![Alt text](ncursesfm.png?raw=true)