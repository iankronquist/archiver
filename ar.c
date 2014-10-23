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
int get_file_meta_data(struct oscar_hdr* md, FILE *get_file, char* file_name);
int write_file_meta_data(struct oscar_hdr* md, FILE *file_out);
int read_file_meta_data(struct oscar_hdr *md, FILE *file_in);
int twiddle_meta_data(struct oscar_hdr *md, FILE *file_in);
int append_file(FILE *in_file, char* in_file_name, FILE *archive);
int read_and_write_file(FILE* file_in, FILE *file_out);

int main() {
    puts("0");
    FILE *new_archive = create_new_archive("test.ar");
    puts("1");
    FILE *read_file_1 = fopen("1-s.txt", "r");
    puts("2");
    FILE *read_file_2 = fopen("2-s.txt", "r");
    puts("3");
    append_file(read_file_1, "1-s.txt", new_archive);
    puts("4");
    append_file(read_file_2, "2-s.txt", new_archive);
    puts("5");
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

int get_file_meta_data(struct oscar_hdr* md, FILE *get_file, char* file_name) {
    struct stat statdat;
    // Open the file and stat it
    if (fstat((int)get_file, &statdat) != 0) {
        //assert(0);
        puts("err!");
    }
    memset(&md->oscar_name, ' ', OSCAR_MAX_FILE_NAME_LEN);
    memset(&md->oscar_name_len, ' ', 2);
    memset(&md->oscar_cdate, ' ', OSCAR_DATE_SIZE);
    memset(&md->oscar_adate, ' ', OSCAR_DATE_SIZE);
    memset(&md->oscar_mdate, ' ', OSCAR_DATE_SIZE);
    memset(&md->oscar_uid, ' ', OSCAR_UGID_SIZE);
    memset(&md->oscar_gid, ' ', OSCAR_UGID_SIZE);
    memset(&md->oscar_mode, ' ', OSCAR_MODE_SIZE);
    memset(&md->oscar_size, ' ', OSCAR_FILE_SIZE);
    memset(&md->oscar_deleted, ' ', 1);
    memset(&md->oscar_hdr_end, ' ', OSCAR_HDR_END_LEN);

    snprintf(&md->oscar_name, OSCAR_MAX_FILE_NAME_LEN, "%s", file_name);
    snprintf(&md->oscar_name_len, 2, "%lu", strlen(file_name));
    snprintf(&md->oscar_cdate, OSCAR_DATE_SIZE, "%ld", statdat.st_birthtime);
    snprintf(&md->oscar_adate, OSCAR_DATE_SIZE, "%ld", statdat.st_atime);
    snprintf(&md->oscar_mdate, OSCAR_DATE_SIZE, "%ld", statdat.st_mtime);
    puts("these two are broken");
    snprintf(&md->oscar_uid, OSCAR_UGID_SIZE, "%u", statdat.st_uid);
    snprintf(&md->oscar_gid, OSCAR_UGID_SIZE, "%u", statdat.st_gid);
    snprintf(&md->oscar_mode, OSCAR_MODE_SIZE, "%hu", statdat.st_mode);
    snprintf(&md->oscar_size, OSCAR_FILE_SIZE, "%lld", statdat.st_size);
    snprintf(&md->oscar_deleted, 1, "%s", " ");
    snprintf(&md->oscar_hdr_end, OSCAR_HDR_END_LEN, "%s", OSCAR_HDR_END);
    printf("ee %s, %i", md->oscar_hdr_end, strlen(md->oscar_hdr_end));
    return 0;
}

int write_file_meta_data(struct oscar_hdr* md, FILE *file_out) {
    fwrite(&md->oscar_name, OSCAR_MAX_FILE_NAME_LEN, 1, file_out);
    fwrite("  ", 2, 1, file_out);
    fwrite(&md->oscar_name_len, 2, 1, file_out);
    fwrite(&md->oscar_cdate, OSCAR_DATE_SIZE, 1, file_out);
    fwrite("  ", 2, 1, file_out);
    fwrite(&md->oscar_adate, OSCAR_DATE_SIZE, 1, file_out);
    fwrite("  ", 2, 1, file_out);
    fwrite(&md->oscar_mdate, OSCAR_DATE_SIZE, 1, file_out);
    fwrite("  ", 2, 1, file_out);
    fwrite(&md->oscar_uid, OSCAR_UGID_SIZE, 1, file_out);
    fwrite("  ", 2, 1, file_out);
    fwrite(&md->oscar_gid, OSCAR_UGID_SIZE, 1, file_out);
    fwrite("  ", 2, 1, file_out);
    fwrite(&md->oscar_mode, OSCAR_MODE_SIZE, 1, file_out);
    fwrite("  ", 2, 1, file_out);
    fwrite(&md->oscar_size, OSCAR_FILE_SIZE, 1, file_out);
    fwrite("  ", 2, 1, file_out);
    fwrite(&md->oscar_deleted, 1, 1, file_out);
    fwrite("  ", 2, 1, file_out);
    fwrite(&md->oscar_sha1, OSCAR_SHA_DIGEST_LEN, 1, file_out);
    fwrite("  ", 2, 1, file_out);
    fwrite(OSCAR_HDR_END, OSCAR_HDR_END_LEN, 1, file_out);
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

int append_file(FILE *in_file, char* in_file_name, FILE *archive) {
    struct oscar_hdr md;
    fseek(archive, 0, SEEK_END);
    get_file_meta_data(&md, in_file, in_file_name);
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
