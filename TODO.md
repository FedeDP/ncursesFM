## TODO

**DEVICE MODE**:  

- [ ] Add available space for mounted fs?
- [ ] Move available space (and other info, may be, such as mount opts) to show_stat?

**SYSINFO MONITOR**:  

- [x] make layout configurable, eg: CPB -> clock, processes, batteries, or BPC -> batteries, processes, clock, etc etc

**HELPER SCREEN**:  

- [x] fix when fullname_win is shown
- [x] fix: bookmarks mode will leave with change_dir if enter is pressed
- [x] fix: switch helper screen when changing active tab mode

**UNICODE**:  

- [x] fix string printing after resize
- [x] fix creating file/folders after ask_user
- [x] fix 2 valgrind errors...
- [x] question too can have unicode chars
- [x] unicode support in fast_browse_mode too
- [x] implements insert mode too (KEY_IC) when pressing insert
- [x] implement http://man7.org/linux/man-pages/man3/wcwidth.3.html (in some lang, some chars may fill more than one column...)
- [x] fix issue with other threads moving away input cursor
- [x] add support for key_left/right/del buttons to ask_user

**PWD-PROTECTED ARCHIVE**:  

- [ ] add support for pwd protected archives -> https://groups.google.com/forum/#!topic/libarchive-discuss/8KO_Hrt4OE4
As soon as arch updates libarchive to 3.2.

**IMAGE PREVIEW**:  

- [x] fix first line while scrolling down (xterm only)
- [ ] understand why konsole is perfect only when scrolling...

**TEXT/PDF PREVIEW**:  

- [ ] understand how that can be implemented

**I18N**:  

- [x] fix "y/n" translation for default answer if enter is pressed.
- [x] fix info_print _(str) 
- [x] fix ncursesFM.pot (update with all new strings)
- [x] add more strings!

**VARIOUS**:  

- [ ] fix crash when scrolling image previews too fast
- [ ] move to KEY_TAB to change files/folders sorting?
- [ ] fix install target for i18n mo files
- [x] in make local pass current folder instead of "./"
- [x] fix move_cursor_to_file: it can be called from within other mod too (pg_up / pg_down) -> scroll down/up to take a "lines" arguments to scroll down/up?
It would simplify move_cursor_to_file too!
- [x] move to const char[] instead of const char*
- [x] extractor_th too with selected files
- [x] make user conf file override global conf file (now if user conf file is found, global conf is not even read)
- [x] use another button to extract archives, and enter to xdg-open them
- [x] switch to KEY_ESC to leave ncursesFM/current mode/leave input mode in ask_user
- [ ] vi keybindings/keybinding profiles
- [ ] switch back to libmagic mimetype support? (it is buggy...as of now it freezes on folders. Plus, i cannot use it for distro packages detection, neither for search_inside_archive because it is so slooooooooow)
- [x] per-user config
- [x] add some wiki to lighten readme
- [ ] update doc
