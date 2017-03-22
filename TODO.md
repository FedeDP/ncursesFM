## TODO

### Generic:

- [ ] symlink() support? (easy)
- [ ] add icon and desktop file 
- [ ] update doc

### Modalities rework:  (HIGH)
Every modality will be "loaded" and will set a char (eg: 'm') and a function (eg: 'change_modality(device_mode)').
When user presses a char, strchr(char, modalities_string) will be checked and if index exist, proper function will be called.

Moreover, let user switch modality from within each other (eg: from device_mode to bookmarks)

- [ ] add a "helper mode" instead of helper_win (way simpler and probably better) (HIGH PRIORITY)
- [ ] drop all helper_win related opts/config and fix (eg during resize) (HIGH)
- [ ] rename bookmarks to "Places" and add mounted fs + trash there. (MID)
- [ ] new mode: job's mode -> to check and review all queued jobs (LOW)

### Trash support: (MID)
- [ ] add trash support (https://specifications.freedesktop.org/trash-spec/trashspec-latest.html) (move file to trash, on click see trashed files. From there you can remove them all or only one of them. You can restore them too.) (MID)
- [ ] by default, "r" will move to trash instead of delete. User can customize this behaviour through a conf file setting. (move_to_trash = 0;) (MID)

### ssh support ideas: (LOW)
Config file with {
	ip address;
	mountpoint (defaults to "/run/media/$username/");
	name (defaults to "remote$counter");
	port (defaults to 22);
	eventually pem file (?);
}

We will show that folder as eg:
"ssh:remote0"
in device_mode.
When user clicks it, it will be mounted in its mountpoint.

- [ ] sshfs/nfs support (LOW)
