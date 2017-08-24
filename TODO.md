## TODO

### Generic:

- [ ] symlink() support
- [ ] add icon and desktop file 
- [ ] update doc

### Modalities rework:  (HIGH)
Every modality will be "loaded" and will set a char (eg: 'm') and a function (eg: 'change_modality(device_mode)').
When user presses a char, strchr(char, modalities_string) will be checked and if index exist, proper function will be called.

- [ ] let user switch modality from within each other (eg: from device_mode to bookmarks)
- [ ] let 'w' close current tab no matter if it is the first
- [ ] rename bookmarks to "Places" and add mounted fs + trash there.
- [ ] new mode: job's mode -> to check and review all queued jobs

### Trash support: (MID)
- [ ] add trash support using Trashd daemon (optional)
- [ ] by default, "r" will move to trash instead of delete. User can customize this behaviour through a conf file setting. (move_to_trash = 0;)

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
