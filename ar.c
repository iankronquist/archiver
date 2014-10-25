#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
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
int n_read_and_write_file(FILE* file_in, FILE *file_out, size_t num_bytes);
void extract_archive(char* archive_name);
void print_md(struct oscar_hdr *md);
void trim_trailing_spaces(char *fn, int length);

int main() {
    extract_archive("arch24.oscar");
    FILE *new_archive = create_new_archive("test.ar");
    FILE *read_file_1 = fopen("2-s.txt", "r");
    FILE *read_file_2 = fopen("4-s.txt", "r");
    append_file(read_file_1, "2-s.txt", new_archive);
    append_file(read_file_2, "4-s.txt", new_archive);
    fclose(new_archive);
    fclose(read_file_1);
    fclose(read_file_2);
    return 0;
}

void extract_archive(char* archive_name) {
    struct oscar_hdr md;
    FILE *current_file = NULL;
    FILE *archive = fopen(archive_name, "r");
    fseek(archive, OSCAR_ID_LEN+1, SEEK_SET);
    char ch;
    while (!feof(archive)) {
        fseek(archive, -1, SEEK_CUR);
        read_file_meta_data(&md, archive);
        char fn[OSCAR_MAX_FILE_NAME_LEN];
        memcpy(&fn, md.oscar_name, OSCAR_MAX_FILE_NAME_LEN);
        trim_trailing_spaces(&fn, OSCAR_MAX_FILE_NAME_LEN);
        current_file = fopen(&fn, "w");
        assert(current_file != NULL);
        n_read_and_write_file(archive, current_file,
            strtol(md.oscar_size, NULL, 10)
        );
        twiddle_meta_data(&md, current_file);
        fclose(current_file);
    }
    fclose(archive);
}

void trim_trailing_spaces(char *fn, int length) {
    for (int i = length - 1; i > 0; i--) {
        if (fn[i] != ' ') {
            if (i < length-2) {
                fn[i+1] = '\0';
            }
            return;
        }
    }
}

FILE* create_new_archive(char* filename) {
    if (access(filename, F_OK) != -1) {
        printf("Archive %s already exists!\n", filename);
        exit(-1);
    }
    FILE *out_file = fopen(filename, "w");
    fprintf(out_file, "%s", OSCAR_ID);
    return out_file;
}

int get_file_meta_data(struct oscar_hdr* md, FILE *get_file, char* file_name) {
    struct stat statdat;
    // Open the file and stat it
    if (fstat(fileno(get_file), &statdat) != 0) {
        printf("Could not get metadata from %s!", file_name);
        assert(0);
    }
    memset(&md->oscar_name, 0, OSCAR_MAX_FILE_NAME_LEN);
    memset(&md->oscar_name_len, 0, 2);
    memset(&md->oscar_cdate, 0, OSCAR_DATE_SIZE);
    memset(&md->oscar_adate, 0, OSCAR_DATE_SIZE);
    memset(&md->oscar_mdate, 0, OSCAR_DATE_SIZE);
    memset(&md->oscar_uid, 0, OSCAR_UGID_SIZE);
    memset(&md->oscar_gid, 0, OSCAR_UGID_SIZE);
    memset(&md->oscar_mode, 0, OSCAR_MODE_SIZE);
    memset(&md->oscar_size, 0, OSCAR_FILE_SIZE);
    memset(&md->oscar_deleted, 0, 1);
    memset(&md->oscar_sha1, 0, OSCAR_SHA_DIGEST_LEN);
    memset(&md->oscar_hdr_end, 0, OSCAR_HDR_END_LEN);

    snprintf(&md->oscar_name, OSCAR_MAX_FILE_NAME_LEN, "%s", file_name);
    snprintf(&md->oscar_name_len, 2, "%lu", strlen(file_name));
    snprintf(&md->oscar_cdate, OSCAR_DATE_SIZE, "%ld", statdat.st_birthtime);
    snprintf(&md->oscar_adate, OSCAR_DATE_SIZE, "%ld", statdat.st_atime);
    snprintf(&md->oscar_mdate, OSCAR_DATE_SIZE, "%ld", statdat.st_mtime);
    //puts("these two are broken");
    snprintf(&md->oscar_uid, OSCAR_UGID_SIZE, "%u", statdat.st_uid);
    snprintf(&md->oscar_gid, OSCAR_UGID_SIZE, "%u", statdat.st_gid);
    snprintf(&md->oscar_mode, OSCAR_MODE_SIZE, "%u", statdat.st_mode);
    snprintf(&md->oscar_size, OSCAR_FILE_SIZE, "%lld", statdat.st_size);
    snprintf(&md->oscar_deleted, 1, "%s", " ");
    snprintf(&md->oscar_hdr_end, OSCAR_HDR_END_LEN, "%s", OSCAR_HDR_END);

    return 0;
}

int write_file_meta_data(struct oscar_hdr* md, FILE *file_out) {
    fwrite(&md, sizeof(struct oscar_hdr), 1, file_out);
    return 0;
}

int read_file_meta_data(struct oscar_hdr *md, FILE *file_in) {
    assert(!feof(file_in));
    fread(md, sizeof(struct oscar_hdr), 1, file_in);
    assert(!feof(file_in));
    return 0;
}

void print_md(struct oscar_hdr *md) {
    puts("md begin");
    printf("%.32s\n", &md->oscar_name);
    printf("%.2s\n", &md->oscar_name_len);
    printf("%.12s\n", &md->oscar_cdate);
    printf("%.12s\n", &md->oscar_adate);
    printf("%.12s\n", &md->oscar_mdate);
    printf("%.8s\n", &md->oscar_uid);
    printf("%.8s\n", &md->oscar_gid);
    printf("%.8s\n", &md->oscar_mode);
    printf("%.8s\n", &md->oscar_size);
    printf("%.1s\n", &md->oscar_deleted);
    printf("%.2s\n", &md->oscar_hdr_end);
    puts("md end");
}

int twiddle_meta_data(struct oscar_hdr *md, FILE *file_in) {
    struct utimbuf buf;
    time_t atime, mtime = 0;
    atime = strtoul(md->oscar_adate, NULL, 0);
    buf.actime = atime;
    buf.modtime = mtime;
    puts("TODO: change birthtime!");
    //futimes(fileno(file_in), &buf);
    puts("TODO: verify that md->oscar_mode is in octal!");
    unsigned int uid, gid = 0;
    unsigned short mode = 0;
    mode = (unsigned short)strtol(&md->oscar_mode, NULL, 8);
    uid = (unsigned int)strtol(&md->oscar_uid, NULL, 0);
    gid = (unsigned int)strtol(&md->oscar_gid, NULL, 0);
    puts("converted");
    fchmod(fileno(file_in), mode);
    fchown(fileno(file_in), uid, gid);
    puts("chmoded and cowned");

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

int n_read_and_write_file(FILE* file_in, FILE *file_out, size_t num_bytes) {
    char ch;
    size_t bytes_read = 0;
    assert(!feof(file_in));
    while((ch = fgetc(file_in)) != EOF && bytes_read < num_bytes) {
        bytes_read++;
        fputc(ch, file_out);
    }
    assert(bytes_read == num_bytes);
    return 0;
}
