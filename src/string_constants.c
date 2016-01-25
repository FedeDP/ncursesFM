const char *editor_missing = "You have to specify a valid editor in config file.";

const char *generic_error = "A generic error occurred. Check log.";

const char *info_win_str[] = {"?: ", "I: ", "E: "};

const char *arch_ext[] = {".tgz", ".tar.gz", ".zip", ".rar", ".xz", ".ar"};

const char *sorting_str[] = {"Files will be sorted alphabetically now.",
                             "Files will be sorted by size now.",
                             "Files will be sorted by last access now.",
                             "Files will be sorted by type now."};

const char *too_many_bookmarks = "Too many bookmarks. Max 30.";
const char *bookmarks_add_quest = "Adding current file to bookmarks. Proceed? Y/n:> ";
const char *bookmarks_rm_quest = "Removing current file from bookmarks. Proceed? Y/n:> ";
const char *bookmarks_xdg_err = "You cannot remove xdg defined user dirs.";
const char *bookmarks_file_err = "Could not open bookmarks file.";
const char *bookmark_added = "Added to bookmarks!";
const char *bookmarks_rm = "Removed fron bookmarks!";
const char *no_bookmarks = "No bookmarks found.";
const char *inexistent_bookmark = "It seems current bookmark no longer exists. Remove it? Y/n:>";

const char *file_selected = "This file is already selected. Cancel its selection before.";
const char *no_selected_files = "There are no selected files.";
const char *file_sel[] = {"File selected.", "File deleted from selected list.", "File deleted from selected list. Selected list empty."};

const char *sure = "Are you serious? y/N:> ";

const char *big_file = "This file is quite big. Do you really want to open it? Y/n:> ";

const char *already_searching = "There's already a search in progress. Wait for it.";
const char *search_mem_fail = "Stopping search as no more memory can be allocated.";
const char *search_insert_name = "Insert filename to be found, at least 5 chars, max 20 chars.:> ";
const char *search_archives = "Do you want to search in archives too? y/N:> ";
const char *lazy_search = "Do you want a lazy search (less precise)? y/N:>";
const char *searched_string_minimum = "At least 5 chars...";
const char *too_many_found = "Too many files found; try with a larger string.";
const char *no_found = "No files found.";
char searching_mess[4][80] = {"Searching...", "Search finished. Press f anytime to view the results."};

#ifdef LIBCUPS_PRESENT
const char *print_question = "Do you really want to print this file? Y/n:> ";
const char *print_ok = "Print job done.";
const char *print_fail = "No printers available.";
#endif

const char *archiving_mesg = "Insert new file name (defaults to first entry name):> ";

const char *ask_name = "Insert new name:> ";

const char *extr_question = "Do you really want to extract this archive? Y/n:> ";

const char *thread_job_mesg[] = {"Moving...", "Pasting...", "Removing...", "Archiving...", "Extracting..."};
const char *thread_str[] = {"Every file has been moved.", "Every files has been copied.", "File/dir removed.", "The archive is ready.", "Succesfully extracted."};
const char *thread_fail_str[] = {"Could not move", "Could not paste.", "Could not remove every file.", "Could not archive.", "Could not extract."};
const char *short_msg[] = {"File created.", "Dir created.", "File renamed."};

const char *selected_mess = "There are selected files.";

const char *thread_running = "There's already a thread working. This thread will be queued.";
const char *quit_with_running_thread = "A job is still running. Do you want to wait for it?(You should!) Y/n:> ";

const char *helper_string[] = { "Enter to surf between folders or to open files.",
                                "It will eventually extract your archives, (un)mount your ISO files or install your distro downloaded packages.",
                                "',' to enable fast browse mode: it lets you jump between files by just typing their name.",
                                "'h' to trigger the showing of hidden files; 's' to see stat about files in current folder.",
                                "'.' to change files/dirs sorting function: alphabetically (default), by size, by last modified or by type.",
                                "Space to select files. Twice to remove the file from selected files. 'u' to check shasum of current file.",
                                "'v'/'x' to paste/cut, 'b' to compress and 'r' to remove selected files. 'p' to print a file.",
                                "'g' to switch to bookmarks window. 'e' to add current file to bookmarks/remove it if in bookmarks_mode.",
                                "'o' to rename current file/dir; 'n'/'d' to create new file/dir. 'f' to search (case sensitive) for a file.",
                                "'t' to create new tab (at most one more). 'w' to close tab. Arrow left or right to switch between tabs.",
                                "'m' to switch to devices tab. 'm' on a device to (un)mount it, enter to move to its mountpoint, mounting it if necessary.",
                                "You can't close first tab. Use 'q' to quit/leave current mode."};
                                
#ifdef SYSTEMD_PRESENT
const char *pkg_quest = "Do you really want to install this package? y/N:> ";
const char *install_th_wait = "Waiting for package installation to finish before exiting. It can really harm your OS otherwise.";
const char *package_warn = "Currently there is no check against wrong package arch: it will crash packagekit and ncursesfm.";
#ifdef LIBUDEV_PRESENT
const char *device_mode_str =  "Choose your desired device to (un)mount it";
#endif
#endif

const char *bookmarks_mode_str = "Bookmarks:";
const char *special_mode_title = "q to return to normal mode";