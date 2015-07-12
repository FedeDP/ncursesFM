const char *config_file_missing = "Config file (/etc/default/ncursesFM.conf) not found. Using default values.\n";
const char *editor_missing = "You have to specify a valid editor in config file.";

const char *generic_mem_error = "Memory allocation failed.";
const char *fatal_mem_error = "No more memory available. Program will exit.";

const char *no_w_perm = "You do not have write permissions here.";

const char *file_selected = "This file is already selected. Cancel its selection before.";
const char *file_sel1 = "File selected.";
const char *file_sel2 = "File deleted from selected list.";
const char *file_sel3 = "File deleted from selected list. Selected list empty.";

const char *sure = "Are you serious? y/N:> ";

const char *already_searching = "There's already a search in progress. Wait for it.";
const char *search_mem_fail = "Stopping search as no more memory can be allocated.";
const char *search_insert_name = "Insert filename to be found, at least 5 chars, max 20 chars.:> ";
const char *search_archives = "Do you want to search in archives too? Search can result slower and has higher memory usage. y/N:> ";
const char *searched_string_minimum = "At least 5 chars...";
const char *too_many_found = "Too many files found; try with a larger string.";
const char *no_found = "No files found.";
const char *search_tab_title = "q to leave search win";
const char *searching_mess[] = {"Searching...", "Search finished. Press f anytime to view the results."};

const char *print_question = "Do you really want to print this file? Y/n:> ";
const char *print_ok = "Print job done.";
const char *print_fail = "No printers available.";

const char *archiving_mesg = "Insert new file name (defaults to first entry name):> ";

const char *ask_name = "Insert new file name:> ";

const char *extr_question = "Do you really want to extract this archive? Y/n:> ";

const char *thread_job_mesg[] = {"Pasting...", "Archiving...", "Extracting...", "Removing...", "Renaming...", "Creating file...", "Creating dir...", "(Un)Mounting iso..."};
const char *thread_str[] = {"Every files has been copied/moved.", "The archive is ready.", "Succesfully extracted.", "File/dir removed.",
                            "File renamed.", "File created.", "Dir created.", "ISO (un)mounted."};
const char *thread_fail_str[] = {"Could not paste.", "Could not archive.", "Could not extract.", "Could not remove.", "Could not rename.",
                                "Could not create file.", "Could not create folder.", "Fuseiso missing."};
const char *selected_mess = "There are selected files.";

const char *file_used_by_thread = "There's a thread working on this file/dir. Please wait.";
const char *thread_running = "There's already a thread working. This thread will be queued.";
const char *quit_with_running_thread = "A job is still running. Do you want to wait for it?(You should!) :> ";
const char *quit_waiting_only_current = "Wait for all the jobs (Y) or only current job (n)?";

const char *helper_string[] = { "n and r to create/remove a file.",
                                "Enter to surf between folders or to open files with either xdg-open (if in a X session) or (text only) $editor var.",
                                "Enter will eventually ask to extract archives, or mount your ISO files.",
                                "To mount ISO you must have isomount installed. To unmount, simply press again enter on the same iso file.",
                                "Press h to trigger the showing of hide files. s to see stat about files in current folder.",
                                "c or x to select files. v to paste: files will be copied if selected with c, or cut if selected with x.",
                                "p to print a file. b to compress selected files. u to view current file's mimetype.",
                                "You can copy as many files/dirs as you want. c/x again on a file/dir to remove it from file list.",
                                "o to rename current file/dir; d to create new dir. f to search (case sensitive) for a file.",
                                "t to create new tab (at most one more). w to close tab. ",
                                "You can't close first tab. Use q to quit.",
                                "Take a look to /etc/default/ncursesFM.conf file to change some settings."};