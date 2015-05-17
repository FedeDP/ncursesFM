# ncursesFM
Ncurses File Manager for linux

It has following features:
* file/dir functions support: copy/paste/cut, new/remove, rename. Copy/cut: to remove a file previously selected for copy just press c/x again on it.
* 2 tabs support.
* iso mount support -> you must have fuseiso installed.
* open files with $editor (settings defined) var.
* show hidden files.
* stats support (permissions and sizes).
* in-program helper message -> press 'l'.
* sync between tabs.
* rename file/folders support.
* move file/folders support -> use cut for this. If a file is "cut" on the same filesystem, it will be moved, otherwise it will be copied.
* Paste is executed in a different thread. If you try to quit while paste thread is still running, you'll be asked if ncursesFM must wait for the thread to finish its work.
* Search support: it will search your string in current directory tree, and, if anything was found, you'll be able to open it, if it was a file, or to move in your searched location.
* It can search your string inside archives too. Then, if found, you can go to the folder of the archive, to extract it.
* Basic print support: you need "libcups" for this to work.
* Extract (compressed) archive through libarchive.
* Compress files/folders through libarchive -> to select files/folders, use c/x (same as copy/cut)

It reads following variables from ncursesFM.conf (using libconfig)...remember to set them!
* editor -> editor used to open files
* show_hidden -> whether to show hidden files by default or not.
* starting_directory -> default starting directory.
* use_default_starting_dir_second_tab -> whether to use "starting_directory" when opening second tab. Otherwise current tab will be used.

IT DOES NOT SUPPORT TERMINAL RESIZE. It is meant to be used maximized, or from a tty.

Archlinux users can now install ncursesFM from aur: https://aur.archlinux.org/packages/ncursesfm-git/

Build requires ncurses, libconfig, libcups and libarchive.
At runtime it requires ncurses, libconfig and (optionally) {fuseiso, libcups, libarchive}

![Alt text](ncursesfm.png?raw=true)