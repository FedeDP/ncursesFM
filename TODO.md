## TODO

**DEVICE MODE**:  

- [x] Add available space for mounted fs?
- [x] Move available space (and other info, may be, such as mount opts) to show_stat?
- [x] FIX: if unmount a dev while stats are active, stats get not updated
- [x] FIX: move_cursor_to_file fucked up with stats when delta > 0

**PWD-PROTECTED ARCHIVE**:  

- [x] add support for pwd protected archives
- [x] Add pwd for every archive that needs to be extracted... For now it is same pwd for every archive!
- [x] fix: ask_user in archiver_callback must be done in main thread (write a byte in a eventfd)
- [x] what happens if ask_user is called from archive_callback while user is being asked another question? AVOID
- [x] add new ask strings to msg.po

**VARIOUS**:  

- [x] selected_mode: 'k' to remove from list. del to delete every selected file.
- [x] del to delete all user bookmarks
- [x] question before removing in selected mode/bookmarks mode
- [x] add a "safe" config /cmd line option.
- [x] space on ".." to select all visible files in current folder
- [x] space twice on ".." to remove all selected files from current folder
- [x] move generic funcs to "helper.c/h" file
- [x] drop image previewer support (really useless)

**WIKI**:  

- [x] remove image previewer section
- [x] document safe config/cmd line option 
- [ ] add "MODES" section
- [ ] add SEARCH section (explain serch_inside_archive and search LAZY)

**GENERIC**:  

- [ ] vi keybindings/keybinding profiles (probably WON'T IMPLEMENT)
- [ ] update doc
