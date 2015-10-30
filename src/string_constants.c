#include "../inc/string_constants.h"

#ifdef LIBCONFIG_PRESENT
const char *config_file_missing = "Config file (/etc/default/ncursesFM.conf) not found. Using default values.\n";
#endif
const char *editor_missing = "You have to specify a valid editor in config file.";

const char *generic_mem_error = "Memory allocation failed.";
const char *fatal_mem_error = "No more memory available. Program will exit.";

const char *no_w_perm = "You do not have write permissions here.";

const char *arch_ext[] = {".tgz", ".tar.gz", ".zip", ".rar", ".xz", ".ar"};

const char *file_selected = "This file is already selected. Cancel its selection before.";
const char *no_selected_files = "There are no selected files.";
const char *file_sel[] = {"File selected.", "File deleted from selected list.", "File deleted from selected list. Selected list empty."};

const char *sure = "Are you serious? y/N:> ";

const char *big_file = "This file is quite big. Do you really want to open it? Y/n:> ";

const char *already_searching = "There's already a search in progress. Wait for it.";
const char *search_mem_fail = "Stopping search as no more memory can be allocated.";
const char *search_insert_name = "Insert filename to be found, at least 5 chars, max 20 chars.:> ";
const char *search_archives = "Do you want to search in archives too? y/N:> ";
const char *searched_string_minimum = "At least 5 chars...";
const char *too_many_found = "Too many files found; try with a larger string.";
const char *no_found = "No files found.";
const char *searching_mess[] = {"Searching...", "Search finished. Press f anytime to view the results."};

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
const char *short_fail_msg[] = {"Could not create file.", "Could not create folder.", "Could not rename."};

const char *selected_mess = "There are selected files.";

const char *thread_running = "There's already a thread working. This thread will be queued.";
const char *quit_with_running_thread = "A job is still running. Do you want to wait for it?(You should!) Y/n:> ";

const char *helper_string[] = { "Enter to surf between folders or to open files.",
                                "Enter will eventually ask to extract archives, mount your ISO files or install your distro downloaded packages.",
                                "Press ',' to enable fast browse mode. Now move between files by just typing your desired file name.",
                                "Press h to trigger the showing of hidden files. s to see stat about files in current folder.",
                                "Press '.' to change files/dirs sorting function: alphabetically (default), by size or by last modified.",
                                "Space to select files. Space again to remove the file from selected files.",
                                "v/x to paste/cut(move), b to compress and r to remove selected files. p to print a file.",
                                "o to rename current file/dir; n/d to create new file/dir. f to search (case sensitive) for a file.",
                                "t to create new tab (at most one more). w to close tab. Arrow left or right to switch between tabs.",
                                "m to switch current tab to device tab. Press enter on your desired device to (un)mount it.",
                                "You can't close first tab. Use q to quit/leave current mode."};


#ifdef SYSTEMD_PRESENT
const char *bus_error = "Failed to open system bus";
const char *pkg_quest = "Do you really want to install this package? y/N:> ";
const char *install_th_wait = "Waiting for package installation to finish before exiting. It can really harm your OS otherwise.";
#ifdef LIBUDEV_PRESENT
const char *device_mode_str =  "Choose your desired device to (un)mount it.";
#endif
#endif