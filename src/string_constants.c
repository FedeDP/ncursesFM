const char *config_file_missing = "Config file (/etc/default/ncursesFM.conf) not found. Using default values.\n";
const char *editor_missing = "You have to specify a valid editor in config file.";
const char *fuseiso_missing = "You need fuseiso for iso mounting support.";

const char *generic_mem_error = "Memory allocation failed.";
const char *fatal_mem_error = "No more memory available. Program will exit.";

const char *already_searching = "There's already a search in progress. Wait for it.";
const char *file_used_by_thread = "This file is being pasted/archived.";
const char *no_w_perm = "You do not have write permissions here.";
const char *unmounted = "Succesfully unmounted.";
const char *mounted = "Succesfully mounted.";
const char *file_created = "File created.";
const char *file_not_created = "Could not create the file.";
const char *file_selected = "This file is already selected. Cancel its selection before.";
const char *rm_fail = "Could not cut/remove. Check user permissions.";
const char *removed = "File/dir removed.";
const char *file_sel1 = "File selected.";
const char *file_sel2 = "File deleted from selected list.";
const char *file_sel3 = "File deleted from selected list. Selected list empty.";
const char *pasted_mesg = "Every files has been copied/moved.";
const char *renamed = "File renamed.";
const char *dir_being_used_by_thread = "This dir is being pasted/archived. Cannot create new dir here.";
const char *dir_created = "Folder created.";

const char *search_mem_fail = "Stopping search as no more memory can be allocated.";
const char *search_insert_name = "Insert filename to be found, at least 5 chars:> ";
const char *search_archives = "Do you want to search in archives too? Search can result slower and has higher memory usage. y/N:> ";
const char *searched_string_minimum = "At least 5 chars...";
const char *too_many_found = "Too many files found; try with a larger string.";
const char *no_found = "No files found.";
const char *search_loop_str1 = "Open file? Y to open, n to switch to the folder:> ";
const char *search_loop_str2 = "This file is inside an archive. Switch to its directory? Y/n:> ";
const char *search_tab_title = "q to leave search win";

const char *print_question = "Do you really want to print this file? Y/n:> ";
const char *print_ok = "Print job done.";
const char *print_fail = "No printers available.";

const char *archive_ready = "The archive is ready.";
const char *already_extracting = "There's already an extractig operation. Please wait.";
const char *extr_question = "Do you really want to extract this archive? Y/n:> ";
const char *extracted = "Succesfully extracted.";

const char *md5sum_warn = "This file is quite large. Md5 sum can take very long time (up to some minutes). Continue? Y/n";

const char *quit_with_running_thread = "A thread is still running. Do you want to wait for it?(You should!) Y/n:> ";

const char *extracting_mess = "Extracting...";
const char *searching_mess = "Searching...";
const char *pasting_mess = "Pasting...";
const char *archiving_mess = "Archiving...";
const char *found_searched_mess = "Search finished. Press f anytime to view the results.";
const char *selected_mess = "There are selected files.";

const char *thread_running = "There's already a thread working on this file list. This thread will be queued.";
const char *archiving_mesg = "Insert new file name (defaults to first entry name):> ";

const char *helper_string[] = { "n and r to create/remove a file.",
                                "Enter to surf between folders or to open files with either xdg-open (if in a X session) or (text only) $editor var.",
                                "Enter will eventually ask to extract archives, or mount your ISO files.",
                                "To mount ISO you must have isomount installed. To unmount, simply press again enter on the same iso file.",
                                "Press h to trigger the showing of hide files. s to see stat about files in current folder.",
                                "c or x to select files. v to paste: files will be copied if selected with c, or cut if selected with x.",
                                "p to print a file. b to compress selected files. a to view md5/shasum of highlighted file.",
                                "You can copy as many files/dirs as you want. c/x again on a file/dir to remove it from file list.",
                                "o to rename current file/dir; d to create new dir. f to search (case sensitive) for a file.",
                                "t to create new tab (at most one more). w to close tab. u to view current file's mimetype.",
                                "You can't close first tab. Use q to quit.",
                                "Take a look to /etc/default/ncursesFM.conf file to change some settings."};