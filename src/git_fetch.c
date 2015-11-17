#ifdef LIBGIT2_PRESENT

#include "../inc/git_fetch.h"

static int cred_acquire_cb(git_cred **out,
                           const char *url,
                           const char *username_from_url,
                           unsigned int allowed_types,
                           void *payload);
static void print_error(void);
static int mynotify(git_checkout_notify_t why,
                    const char *path,
                    const git_diff_file *baseline,
                    const git_diff_file *target,
                    const git_diff_file *workdir,
                    void *payload);

void fetch(const char *path) {
    git_libgit2_init();
    
    git_repository *repo = NULL;
    git_remote *remote = NULL;
    git_buf buf = GIT_BUF_INIT_CONST(NULL, 0);
    const git_transfer_progress *stats;
    git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;
    char success[200], remote_name[80];
    int error;
    
    error = git_repository_discover(&buf, path, 0, NULL);
    if (!error) {
        error = git_repository_open(&repo, buf.ptr);
        git_buf_free(&buf);
    }
    if (!error) {
        ask_user("Remote name (default: origin): ", remote_name, 80, 0);
        if (!strlen(remote_name)) {
            strcpy(remote_name, "origin");
        }
        error = git_remote_lookup(&remote, repo, remote_name);
    }
    if (!error) {
        fetch_opts.callbacks.credentials = cred_acquire_cb;
        /*
         * here valgrind complains about LOTS of errors.
         * They're related to openssl (https://github.com/libgit2/libgit2/issues/3509)
         */
        error = git_remote_fetch(remote, NULL, &fetch_opts, NULL);
    }
    if (!error) {
        stats = git_remote_stats(remote);
        if (stats->received_bytes) {
            sprintf(success, "Received %d/%d objects in %zu bytes",
            stats->indexed_objects, stats->total_objects, stats->received_bytes);
        } else {
            sprintf(success, "Already up to date.");
        }
        print_info(success, INFO_LINE);
    } else {
        print_error();
    }
    if (remote) {
        if(git_remote_connected(remote)) {
            git_remote_disconnect(remote);
        }
        git_remote_free(remote);
    }
    if (repo) {
        git_repository_free(repo);
    }
    git_libgit2_shutdown();
}

void checkout(const char *path) {
    git_libgit2_init();
    
    git_repository *repo = NULL;
    git_buf buf = GIT_BUF_INIT_CONST(NULL, 0);
    git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
    int error, num_of_changed_files = 0;
    char success[30];
    
    error = git_repository_discover(&buf, path, 0, NULL);
    if (!error) {
        error = git_repository_open(&repo, buf.ptr);
        git_buf_free(&buf);
    }
    if (!error) {
        opts.notify_flags = GIT_CHECKOUT_NOTIFY_DIRTY;
        opts.notify_cb = mynotify;
        opts.notify_payload = &num_of_changed_files;
        error = git_checkout_head(repo, &opts);
    }
    if (!error) {
        sprintf(success, "%d files changed on disk.", num_of_changed_files);
        print_info(success,INFO_LINE);
    } else {
        print_error();
    }
    if (repo) {
        git_repository_free(repo);
    }
    git_libgit2_shutdown();
}

static int mynotify(git_checkout_notify_t why,
                    const char *path,
                    const git_diff_file *baseline,
                    const git_diff_file *target,
                    const git_diff_file *workdir,
                    void *payload) {
    print_info(path, INFO_LINE);
    sleep(1);
    (*(int *)payload)++;
    return 0;
}

static int cred_acquire_cb(git_cred **out,
                           const char *url,
                           const char *username_from_url,
                           unsigned int allowed_types,
                           void *payload) {
    char username[128] = {0};
    char password[128] = {0};
    
    ask_user("Username: ", username, 128, 0);
    ask_user("Password: ", password, 128, 0);
    return git_cred_userpass_plaintext_new(out, username, password);
}

static void print_error(void) {
    const git_error *err = giterr_last();
    if (err) {
        print_info(err->message, ERR_LINE);
    }
}

#endif