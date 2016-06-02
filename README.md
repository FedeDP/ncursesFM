[![Build Status](https://travis-ci.org/FedeDP/ncursesFM.svg?branch=master)](https://travis-ci.org/FedeDP/ncursesFM)

# NcursesFM

Ncurses File Manager for linux.

It aims to be as user friendly and lightweight as possible, while being good looking (my own tastes) and simple.  
Being simple doesn't imply being useless; indeed it is a full featured fm.  
It can be built with a very small set of dependencies, as i tried to make as many deps as possible optional (compile time or runtime).  

![](https://github.com/FedeDP/ncursesFM/raw/master/ncursesFM.png)

## Features:

**Remember that every shortcut in ncursesFM is case insensitive!**

*Press 'L' while in program to view a detailed helper message*

* Every feature you would expect by a basic FM.
* Terminal window resize support.
* i18n support: for now en, it.
* 2 tabs support. Their content is kept in sync. Jump between tabs with arrow keys (left/right).
* image previewing support through w3mimgdisplay.
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

*To clarify: to paste/move a file you'll have to select it with space, then move to another dir or change tab, and press v/x.*

For a full list of feature, have a look at the wiki page: https://github.com/FedeDP/ncursesFM/wiki/Getting-Started.  
Wiki has lots of information. You'll find info about how to build, deps, config file...have a look: https://github.com/FedeDP/ncursesFM/wiki.
