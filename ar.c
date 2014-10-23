#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <assert.h>
#include <utime.h>
#include <time.h>

#include "oscar.h"
FILE* create_new_archive(char* filename);
int get_file_meta_data(char* filename, struct oscar_hdr* md);
int write_file_meta_data(struct oscar_hdr* md, FILE *file_out);
int read_file_meta_data(struct oscar_hdr *md, FILE *file_in);
int twiddle_meta_data(struct oscar_hdr *md, FILE *file_in);
int append_file(FILE *in_file, FILE *archive);
int read_and_write_file(FILE* file_in, FILE *file_out);

int main() {
    FILE *new_archive = create_new_archive("test.ar");
    FILE *read_file_1 = fopen("1-s.txt", "r");
    FILE *read_file_2 = fopen("2-s.txt", "r");
    append_file(read_file_1, new_archive);
    append_file(read_file_2, new_archive);
    fclose(new_archive);
    fclose(read_file_1);
    fclose(read_file_2);
    return 0;
}

FILE* create_new_archive(char* filename) {
    /*if (access(filename, W_OK) != 0) {
        assert(0);
    }*/
    FILE *out_file = fopen(filename, "w");
    fprintf(out_file, "%s", OSCAR_ID);
    return out_file;
}

int get_file_meta_data(char* filename, struct oscar_hdr* md) {
    struct stat statdat;
    FILE *get_file;
    // Check that we can read the file and it exists
    /*if (access(filename, F_OK | R_OK) == -1) {
        assert(0);
    }*/
    // Open the file and stat it
    get_file = fopen(filename, "w");
    /*if (fstat((int)get_file, &statdat) != 0) {
        assert(0);
    }*/
    fclose(get_file);
    snprintf(&md->oscar_name, OSCAR_MAX_FILE_NAME_LEN, "%s", filename);
    snprintf(&md->oscar_name_len, 2, "%lu", strlen(filename));
    snprintf(&md->oscar_cdate, OSCAR_DATE_SIZE, "%ld", statdat.st_birthtime);
    snprintf(&md->oscar_adate, OSCAR_DATE_SIZE, "%ld", statdat.st_atime);
    snprintf(&md->oscar_mdate, OSCAR_DATE_SIZE, "%ld", statdat.st_mtime);
    snprintf(&md->oscar_uid, OSCAR_UGID_SIZE, "%u", statdat.st_uid);
    snprintf(&md->oscar_gid, OSCAR_UGID_SIZE, "%u", statdat.st_gid);
    snprintf(&md->oscar_mode, OSCAR_MODE_SIZE, "%hu", statdat.st_mode);
    snprintf(&md->oscar_size, OSCAR_FILE_SIZE, "%lld", statdat.st_size);
    snprintf(&md->oscar_deleted, 1, "%s", " ");
    snprintf(&md->oscar_hdr_end, OSCAR_HDR_END_LEN, "%s", OSCAR_HDR_END);
    memset(&md->oscar_sha1, 0, OSCAR_SHA_DIGEST_LEN);
    return 0;
}

int write_file_meta_data(struct oscar_hdr* md, FILE *file_out) {
    fprintf(file_out, "%s", md->oscar_name);
    fprintf(file_out, "%s", md->oscar_name_len);
    fprintf(file_out, "%s", md->oscar_cdate);
    fprintf(file_out, "%s", md->oscar_adate);
    fprintf(file_out, "%s", md->oscar_mdate);
    fprintf(file_out, "%s", md->oscar_uid);
    fprintf(file_out, "%s", md->oscar_gid);
    fprintf(file_out, "%s", md->oscar_mode);
    fprintf(file_out, "%s", md->oscar_size);
    fprintf(file_out, "%c", md->oscar_deleted);
    fprintf(file_out, "%s", md->oscar_sha1);
    fprintf(file_out, "%s", md->oscar_hdr_end);

    memset(&md->oscar_sha1, 0, OSCAR_SHA_DIGEST_LEN);
    return 0;
}

int read_file_meta_data(struct oscar_hdr *md, FILE *file_in) {
    fread(&md->oscar_name, OSCAR_MAX_FILE_NAME_LEN, 1, file_in);
    fread(&md->oscar_name_len, 2, 1, file_in);
    fread(&md->oscar_cdate, OSCAR_DATE_SIZE, 1, file_in);
    fread(&md->oscar_adate, OSCAR_DATE_SIZE, 1, file_in);
    fread(&md->oscar_mdate, OSCAR_DATE_SIZE, 1, file_in);
    fread(&md->oscar_uid, OSCAR_UGID_SIZE, 1, file_in);
    fread(&md->oscar_gid, OSCAR_UGID_SIZE, 1, file_in);
    fread(&md->oscar_mode, OSCAR_MODE_SIZE, 1, file_in);
    fread(&md->oscar_size, OSCAR_MAX_MEMBER_FILE_SIZE, 1, file_in);
    fread(&md->oscar_deleted, 1, 1, file_in);
    fread(&md->oscar_sha1, OSCAR_SHA_DIGEST_LEN, 1, file_in);
    fread(&md->oscar_hdr_end, OSCAR_HDR_END_LEN, 1, file_in);
    return 0;
}

int twiddle_meta_data(struct oscar_hdr *md, FILE *file_in) {
    struct utimbuf buf;
    time_t atime, mtime = 0;
    atime = strtoul(md->oscar_adate, NULL, 0);
    buf.actime = atime;
    buf.modtime = mtime;
    puts("TODO: change birthtime!");
    futimes(file_in, &buf);
    puts("TODO: verify that md->oscar_mode is in octal!");
    unsigned int uid, gid = 0;
    unsigned short mode = 0;
    mode = (unsigned short)strtol(&md->oscar_mode,
                                  &md->oscar_mode + OSCAR_MODE_SIZE, 8);
    uid = (unsigned int)strtol(&md->oscar_uid,
                               &md->oscar_uid + OSCAR_UGID_SIZE, 0);
    gid = (unsigned int)strtol(&md->oscar_gid,
                               &md->oscar_gid + OSCAR_UGID_SIZE, 0);
    fchmod((int)file_in, mode);
    fchown((int)file_in, uid, gid);
    return 0;
}

int append_file(FILE *in_file, FILE *archive) {
    struct oscar_hdr md;
    fseek(archive, 0, SEEK_END);
    get_file_meta_data(&md, in_file);
    write_file_meta_data(&md, archive);
    read_and_write_file(in_file, archive);
    return 0;    
}

int read_and_write_file(FILE* file_in, FILE *file_out) {
    char ch;
    while((ch = fgetc(file_in)) != EOF) {
        fputc(ch, file_out);
    }
    return 0;
}
