[![Build Status](https://travis-ci.org/FedeDP/ncursesFM.svg?branch=master)](https://travis-ci.org/FedeDP/ncursesFM)

# NcursesFM

*Ncurses File Manager for linux*

Written in C, with ncurses library, it aims to be as user friendly and lightweight as possible, while being good looking and simple.  
Being simple doesn't imply being useless; indeed it is a full featured fm.  
For a **full list of features**, **deps** and **how to build**, please refer to [wiki pages](https://github.com/FedeDP/ncursesFM/wiki/).

![](https://github.com/FedeDP/ncursesFM/raw/master/ncursesFM.png)

## Main features:

**Remember that every shortcut in ncursesFM is case insensitive!**

*Press 'L' while in program to view a detailed helper message*

* Every feature you would expect by a basic FM.
* Terminal window resize support.
* i18n support: for now en, it, de, es_AR (list of translators: https://github.com/FedeDP/ncursesFM/wiki/I18n#translators).
* 2 tabs support. Their content is kept in sync. Jump between tabs with arrow keys (left/right).
* config support through libconfig.
* Basic mouse support.
* Simple sysinfo monitor that will refresh every 30s: clock, battery and some system info.
* Fast browse mode: enable it with ','. It lets you jump between files by just typing their names.
* 4 sorting modes.
* Stats support (permissions and sizes).
* Inotify monitor to check for fs events in current opened directories.
* Bookmarks support.
* Search support: it will search your string in current directory tree. It can search your string inside archives too.
* Basic print support through libcups.
* Extract/compress files/folders through libarchive.
* Powermanagement inhibition while processing a job (eg: while pasting a file) to avoid data loss.
* Internal udisks2 monitor, to poll for new devices. It can automount new connected devices too. Device monitor will list only mountable devices, eg: dvd reader will not be listed until a cd/dvd is inserted.
* Drives/usb sticks/ISO files (un)mount through udisks2.
