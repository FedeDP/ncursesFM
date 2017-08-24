## TODO

### Full rewriting
Every modality will be "loaded" and will set a char (eg: 'm') and a function (eg: 'change_modality(device_mode)').
When user presses a char, strchr(char, modalities_string) will be checked and if index exist, proper function will be called.

- [ ] let user switch modality from within each other (eg: from device_mode to bookmarks)
- [ ] let 'w' close current tab no matter if it is the first
- [ ] rename bookmarks to "Places" and add mounted fs + trash there.
- [ ] new mode: job's mode -> to check and review all queued jobs
- [ ] add a config to start with 2 tabs and to select for each tab the modality
- [ ] Add man page and drop helper window/helper print?
- [ ] Drop resize support?
- [ ] switch to libpopt instead of getopt
- [ ] add icon and desktop file

### Trash support: (MID)
- [ ] add trash support using Trashd daemon (optional)
- [ ] by default, "r" will move to trash instead of delete. User can customize this behaviour through a conf file setting. (use_trash = 0)
