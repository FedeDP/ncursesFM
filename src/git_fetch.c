#ifdef LIBGIT2_PRESENT

#include "../inc/git_fetch.h"

static int cred_acquire_cb(git_cred **out,
                           const char *url,
                           const char *username_from_url,
                           unsigned int allowed_types,
                           void *payload);
static void print_error(void);

static int cred_acquire_cb(git_cred **out,
                    const char *url,
                    const char *username_from_url,
                    unsigned int allowed_types,
                    void *payload)
{
    char username[128] = {0};
    char password[128] = {0};
    
    ask_user("Username: ", username, 128, 0);
    ask_user("Password: ", password, 128, 0);
    return git_cred_userpass_plaintext_new(out, username, password);
}

static void print_error(void)
{
    const git_error *err = giterr_last();
    if (err) {
        print_info(err->message, ERR_LINE);
    }
}

void fetch(const char *path)
{
    git_repository *repo = NULL;
    git_remote *remote = NULL;
    git_buf buf = GIT_BUF_INIT_CONST(NULL, 0);
    const git_transfer_progress *stats;
    git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;
    char success[200];
    int error;
    
    git_libgit2_init();
    error = git_repository_discover(&buf, path, 0, NULL);
    if (!error) {
        error = git_repository_open(&repo, buf.ptr);
    }
    if ((!error) && (git_remote_lookup(&remote, repo, buf.ptr) < 0)) { // shouldn't here be "origin"?
        error = git_remote_create_anonymous(&remote, repo, buf.ptr);    // not sure about this one...
    }
    if (!error) {
        fetch_opts.callbacks.credentials = cred_acquire_cb;
        error = git_remote_fetch(remote, NULL, &fetch_opts, "fetch");
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
    git_buf_free(&buf);
    git_libgit2_shutdown();
}

#endif