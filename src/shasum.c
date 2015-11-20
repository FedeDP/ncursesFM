#ifdef OPENSSL_PRESENT

#include "../inc/shasum.h"

void shasum_func(const char *filename) {
    unsigned char *hash = NULL, *buffer = NULL;
    FILE *fp;
    long size;
    int i = 1, length = SHA_DIGEST_LENGTH;
    char input[4];
    const char *question = "Which shasum do you want? Choose between 1, 224, 256, 384, 512. Defaults to 1.> ";
    const char *matching_quest = "Insert matching shasum (enter to just print desired shasum):> ";

    if(!(fp = fopen(filename, "rb"))) {
        print_info(strerror(errno), ERR_LINE);
        return;
    }
    fseek(fp, 0L, SEEK_END);
    size = ftell(fp);
    rewind(fp);
    if (!(buffer = malloc(size))) {
        print_info(generic_mem_error, ERR_LINE);
        fclose(fp);
        return;
    }
    fread(buffer, size, 1, fp);
    fclose(fp);
    ask_user(question, input, 4, 1);
    i = atoi(input);
    if ((i == 224) || (i == 256) || (i == 384) || (i == 512)) {
        length = i / 8;
    }
    if (!(hash = malloc(sizeof(unsigned char) * length))) {
        print_info(generic_mem_error, ERR_LINE);
        return;
    }
    switch(i) {
    case 224:
        SHA224(buffer, size, hash);
        break;
    case 256:
        SHA256(buffer, size, hash);
        break;
    case 384:
        SHA384(buffer, size, hash);
        break;
    case 512:
        SHA512(buffer, size, hash);
        break;
    default:
        SHA1(buffer, size, hash);
        break;
    }
    free(buffer);
    char matching_hash[2 * length];
    ask_user(matching_quest, matching_hash, 2 * length, 0);
    char s[2 * length];
    s[0] = 0;
    for(i = 0; i < length; i++) {
        sprintf(s + strlen(s), "%02x", hash[i]);
    }
    if (strlen(matching_hash)) {
        if (strcmp(s, matching_hash) == 0) {
            print_info("OK", INFO_LINE);
        } else {
            print_info("Shasum not matching.", INFO_LINE);
        }
    } else {
        print_info(s, INFO_LINE);
    }
    free(hash);
}

#endif