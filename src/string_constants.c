const char yes[] = "y";
const char no[] = "n";

const char editor_missing[] = "You have to specify a valid editor in config file.";

const char generic_error[] = "A generic error occurred. Check log.";

const char *info_win_str[] = {"?: ", "I: ", "E: "};

const char *arch_ext[] = {".tgz", ".tar.gz", ".zip", ".rar", ".xz", ".ar"};

const char *sorting_str[] = {"Files will be sorted alphabetically now.",
                             "Files will be sorted by size now.",
                             "Files will be sorted by last access now.",
                             "Files will be sorted by type now."};

const char bookmarks_add_quest[] = "Adding current file to bookmarks. Proceed? Y/n:> ";
const char bookmarks_rm_quest[] = "Removing current file from bookmarks. Proceed? Y/n:> ";
const char bookmarks_xdg_err[] = "You cannot remove xdg defined user dirs.";
const char bookmarks_file_err[] = "Could not open bookmarks file.";
const char bookmark_added[] = "Added to bookmarks!";
const char bookmarks_rm[] = "Removed from bookmarks!";
const char no_bookmarks[] = "No bookmarks found.";
const char inexistent_bookmark_quest[] = "It seems current bookmark no longer exists. Remove it? Y/n:> ";
const char inexistent_bookmark[] = "Bookmark was deleted as it was no more existent.";
const char bookmark_already_present[] = "Bookmark already present. Would you like to remove it? Y/n:> ";
const char bookmarks_cleared[] = "User bookmarks have been cleared.";

const char no_selected_files[] = "There are no selected files.";
const char *file_sel[] = {"File selected.", 
                        "File unselected.", 
                        "File unselected. Selected list empty.", 
                        "Every file in this directory has been selected.",
                        "Every file in this directory has been unselected.",
                        "Every file in this directory has been unselected. Selected list empty."};
const char selected_cleared[] = "List of selected files has been cleared.";

const char sure[] = "Are you serious? y/N:> ";

const char already_searching[] = "There's already a search in progress. Wait for it.";
const char search_insert_name[] = "Insert filename to be found, at least 5 chars, max 20 chars.:> ";
const char search_archives[] = "Do you want to search in archives too? y/N:> ";
const char lazy_search[] = "Do you want a lazy search (less precise but faster)? y/N:>";
const char searched_string_minimum[] = "At least 5 chars...";
const char too_many_found[] = "Too many files found; try with a larger string.";
const char no_found[] = "No files found.";
const char *searching_mess[] = {"Searching...", "Search finished. Press f anytime from normal mode to view the results."};

#ifdef LIBCUPS_PRESENT
const char print_question[] = "Do you really want to print this file? Y/n:> ";
const char print_ok[] = "Print job added.";
const char print_fail[] = "No printers available.";
#endif

const char archiving_mesg[] = "Insert new file name (defaults to first entry name):> ";

const char ask_name[] = "Insert new name:> ";

const char extr_question[] = "Do you really want to extract this archive? Y/n:> ";

const char pwd_archive[] = "Current archive is encrypted. Enter a pwd:> ";

const char *thread_job_mesg[] = {"Cutting...", "Pasting...", "Removing...", "Archiving...", "Extracting..."};
const char *thread_str[] = {"Every file has been cut.", "Every files has been pasted.", "File/dir removed.", "Archive is ready.", "Succesfully extracted."};
const char *thread_fail_str[] = {"Could not cut", "Could not paste.", "Could not remove every file.", "Could not archive.", "Could not extract every file."};
const char *short_msg[] = {"File created.", "Dir created.", "File renamed."};

const char selected_mess[] = "There are selected files.";

const char thread_running[] = "There's already an active job. This job will be queued.";
const char quit_with_running_thread[] = "Queued jobs still running. Waiting...";

#ifdef SYSTEMD_PRESENT
const char pkg_quest[] = "Do you really want to install this package? y/N:> ";
const char install_th_wait[] = "Waiting for package installation to finish...";
const char install_success[] = "Installed.";
const char install_failed[] = "Could not install.";
const char package_warn[] = "Currently there is no check against wrong package arch: it will crash packagekit and ncursesfm.";
const char device_mode_str[] =  "Devices:";
const char no_devices[] = "No devices found.";
const char dev_mounted[] = "%s mounted in: %s.";
const char dev_unmounted[] = "%s unmounted.";
const char ext_dev_mounted[] = "External tool has mounted %s.";
const char ext_dev_unmounted[] = "External tool has unmounted %s.";
const char device_added[] = "New device connected.";
const char device_removed[] = "Device removed.";
const char no_proc_mounts[] = "Could not find /proc/mounts.";
const char polling[] = "Still polling for devices.";
const char monitor_err[] = "Monitor is not active. An error occurred, check log file.";
#endif

const char bookmarks_mode_str[] = "Bookmarks:";

const char search_mode_str[] = "%d files found searching %s:";

const char selected_mode_str[] = "Selected files:";

const char ac_online[] = "On AC";
const char power_fail[] = "No power supply info available.";

const char *insert_string[] = {"Disabled overtype mode.", "Enabled overtype mode."};

const char win_too_small[] = "Window too small. Enlarge it.";

#ifdef SYSTEMD_PRESENT
const int HELPER_HEIGHT[] = {16, 9, 9, 9, 9, 9};
#else
const int HELPER_HEIGHT[] = {14, 8, 9, 9, 9, 9};
#endif

const char helper_title[] = "Press 'L' to trigger helper";

const char * helper_string_token = "%";

const char helper_string[][16][150] =
{
    {
        {"Remember: every shortcut in ncursesFM is case insensitive."},
        {"%Enter% -> surf between folders or to open files."},
#ifdef SYSTEMD_PRESENT
        {"It will eventually (un)mount your ISO files or install your distro downloaded packages."},
#endif
        {"%,% -> enable fast browse mode: it lets you jump between files by just typing their name."},
        {"%PG_UP/DOWN% -> jump straight to first/last file. %i% -> check files fullname."},
        {"%h% -> trigger the showing of hidden files; %s% -> see files stats."},
        {"%TAB% -> change sorting function: alphabetically (default), by size, by last modified or by type."},
        {"%Space% -> select files. Once more to remove the file from selected files."},
        {"%o% -> rename current file/dir; %n/d% -> create new file/dir. %f% -> search for a file."},
#ifdef LIBCUPS_PRESENT
        {"%v/x% -> paste/cut, %b% -> compress, %r% -> remove, %z% -> extract, %p% -> print."},
#else
        {"%v/x% -> paste/cut, %b% -> compress, %r% -> remove, %z% -> extract."},
#endif
        {"%t% -> create second tab. %w% -> close second tab. %Arrow keys% -> switch between tabs."},
        {"%g% -> switch to bookmarks mode. %e% -> add/remove current file to bookmarks."},
#ifdef SYSTEMD_PRESENT
        {"%m% -> switch to device mode. %k% -> switch to selected mode."},
#endif
        {"%ESC% -> quit."}
    }, {
        {"Remember: every shortcut in ncursesFM is case insensitive."},

        {"%Enter% -> surf between folders or to open files."},
#ifdef SYSTEMD_PRESENT
        {"It will eventually (un)mount your ISO files or install your distro downloaded packages."},
#endif
        {"Just start typing your desired filename, to move right to its position."},
        {"%PG_UP/DOWN% -> jump straight to first/last file."},
        {"%TAB% -> change sorting function: alphabetically (default), by size, by last modified or by type."},
        {"%ESC% -> leave fast browse mode."}
    }, {
        {"Remember: every shortcut in ncursesFM is case insensitive."},
        {"%s% -> see files stats. %i% -> check files fullname."},
        {"%PG_UP/DOWN% -> jump straight to first/last file."},
        {"%t% -> create second tab. %w% -> close second tab. %Arrow keys% -> switch between tabs."},
        {"%r% -> remove selected file from bookmarks. %del% -> remove all user bookmakrs."},
        {"enter on a file will move to the folder/file selected."},
        {"%ESC% -> leave bookmarks mode."}
    }, {
        {"Remember: every shortcut in ncursesFM is case insensitive."},
        {"%s% -> see files stats. %i% -> check files fullname."},
        {"%PG_UP/DOWN% -> jump straight to first/last file."},
        {"%t% -> create second tab. %w% -> close second tab. %Arrow keys% -> switch between tabs."},
        {"enter on a file will move to the folder/file selected."},
        {"%ESC% -> leave search mode."}
    }, {
        {"Remember: every shortcut in ncursesFM is case insensitive."},
        {"%s% -> see files stats. %i% -> check files fullname."},
        {"%PG_UP/DOWN% -> jump straight to first/last file."},
        {"%t% -> create second tab. %w% -> close second tab. %Arrow keys% -> switch between tabs."},
        {"%m% -> (un)mount current device."},
        {"%enter% -> move to current device mountpoint, mounting it if necessary."},
        {"%ESC% -> leave device mode."}
    },{
        {"Remember: every shortcut in ncursesFM is case insensitive."},
        {"%s% -> see files stats. %i% -> check files fullname."},
        {"%PG_UP/DOWN% -> jump straight to first/last file."},
        {"%t% -> create second tab. %w% -> close second tab. %Arrow keys% -> switch between tabs."},
        {"%r% -> remove current file selection. %del% -> remove all selected files."},
        {"enter on a file will move to the folder/file selected."},
        {"%ESC% -> leave selected mode."}
    }
};
