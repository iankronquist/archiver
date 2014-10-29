#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <assert.h>
#include <utime.h>
#include <time.h>
#include <stdbool.h>

#include "oscar.h"
FILE* create_new_archive(char* filename);
int get_file_meta_data(struct oscar_hdr* md, FILE *get_file, char* file_name);
int write_file_meta_data(struct oscar_hdr* md, FILE *file_out);
int read_file_meta_data(struct oscar_hdr *md, FILE *file_in);
int twiddle_meta_data(struct oscar_hdr *md, FILE *file_in);
int append_file(FILE *in_file, char* in_file_name, FILE *archive);
int read_and_write_file(FILE* file_in, FILE *file_out);
int n_read_and_write_file(FILE* file_in, FILE *file_out, size_t num_bytes);
void extract_archive(char* archive_name, int num_files, char** file_names);
void print_md(struct oscar_hdr *md);
void trim_trailing_spaces(char *fn, int length);
void mark_for_deletion(char *archive_name, size_t number_of_files,
                       char** file_names);
bool list_contains(char** file_names, int num_files, char* file_name);

int main() {
    char* files_to_delete[] = {"2-s.txt", "4-s.txt"};
    extract_archive("arch24.oscar", 2, files_to_delete);
    //extract_archive("test.ar", 1, files_to_delete);
    FILE *new_archive = create_new_archive("test.ar");
    FILE *read_file_1 = fopen("2-s.txt", "r");
    FILE *read_file_2 = fopen("4-s.txt", "r");
    append_file(read_file_1, "2-s.txt", new_archive);
    append_file(read_file_2, "4-s.txt", new_archive);
    fclose(new_archive);
    fclose(read_file_1);
    fclose(read_file_2);
    mark_for_deletion("test.ar", 1, files_to_delete);
    return 0;
}

// To extract all the files give num_files the value of 0
void extract_archive(char* archive_name, int num_files, char** file_names) {
    struct oscar_hdr md;
    FILE *current_file = NULL;
    FILE *archive = fopen(archive_name, "r");
    struct oscar_hdr arch_md;
    get_file_meta_data(&arch_md, archive, "archive!!");

    fseek(archive, OSCAR_ID_LEN+1, SEEK_SET);
    long arch_size = strtol(arch_md.oscar_size, 0, 10);
    while (ftell(archive) < arch_size) {
        printf("%lu, %lu asdf\n", ftell(archive), arch_size);
        print_md(&arch_md);
        fseek(archive, -1, SEEK_CUR);
        read_file_meta_data(&md, archive);
        char file_name[OSCAR_MAX_FILE_NAME_LEN];
        memcpy(&file_name, md.oscar_name, OSCAR_MAX_FILE_NAME_LEN);
        trim_trailing_spaces(&file_name, OSCAR_MAX_FILE_NAME_LEN);
        if (num_files == 0 || list_contains(file_names, num_files, file_name)) {
            //TODO: check if file exists
            current_file = fopen(&file_name, "w");
            assert(current_file != NULL);
            n_read_and_write_file(archive, current_file,
                strtol(md.oscar_size, NULL, 10)
            );
            twiddle_meta_data(&md, current_file);
            fclose(current_file);
        } else {
            printf("\t%lu\n", strtol(md.oscar_size, NULL, 10));
            fseek(archive, strtol(md.oscar_size, NULL, 10), SEEK_CUR);
        }
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
    fchmod(fileno(out_file), S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
    return out_file;
}

int get_file_meta_data(struct oscar_hdr* md, FILE *get_file, char* file_name) {
    struct stat statdat;
    // Open the file and stat it
    assert(get_file != NULL);
    if (fstat(fileno(get_file), &statdat) != 0) {
        printf("Could not get metadata from %s!", file_name);
        assert(0);
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
    memset(&md->oscar_sha1, ' ', OSCAR_SHA_DIGEST_LEN);
    memset(&md->oscar_hdr_end, ' ', OSCAR_HDR_END_LEN);

    int num_bytes = 0;
    num_bytes = snprintf(&md->oscar_name, OSCAR_MAX_FILE_NAME_LEN, "%s", file_name);
    if (num_bytes != OSCAR_MAX_FILE_NAME_LEN)
        md->oscar_name[num_bytes] = ' ';
    int file_name_len = strlen(file_name);
    // UGLY HACK!
    if (file_name_len >= 10) {
        num_bytes = snprintf(&md->oscar_name_len, 2, "%lu", file_name_len);
        if (num_bytes != 2)
            md->oscar_name_len[num_bytes] = ' ';
    } else {
        puts("left");
        num_bytes = snprintf(&md->oscar_name_len + 1, 2, "%lu", file_name_len);
    }
    num_bytes = snprintf(&md->oscar_cdate, OSCAR_DATE_SIZE, "%ld", statdat.st_ctime);
    if (num_bytes != OSCAR_DATE_SIZE)
        md->oscar_cdate[num_bytes] = ' ';
    num_bytes = snprintf(&md->oscar_adate, OSCAR_DATE_SIZE, "%ld", statdat.st_atime);
    if (num_bytes != OSCAR_MAX_FILE_NAME_LEN)
        md->oscar_adate[num_bytes] = ' ';
    num_bytes = snprintf(&md->oscar_mdate, OSCAR_DATE_SIZE, "%ld", statdat.st_mtime);
    if (num_bytes != OSCAR_MAX_FILE_NAME_LEN)
        md->oscar_mdate[num_bytes] = ' ';
    //puts("these two are broken");
    num_bytes = snprintf(&md->oscar_uid, OSCAR_UGID_SIZE, "%u", statdat.st_uid);
    if (num_bytes != OSCAR_MAX_FILE_NAME_LEN)
        md->oscar_uid[num_bytes] = ' ';
    num_bytes = snprintf(&md->oscar_gid, OSCAR_UGID_SIZE, "%u", statdat.st_gid);
    if (num_bytes != OSCAR_MAX_FILE_NAME_LEN)
        md->oscar_gid[num_bytes] = ' ';
    num_bytes = snprintf(&md->oscar_mode, OSCAR_MODE_SIZE, "%o", statdat.st_mode);
    if (num_bytes != OSCAR_MAX_FILE_NAME_LEN)
        md->oscar_mode[num_bytes] = ' ';
    num_bytes = snprintf(&md->oscar_size, OSCAR_FILE_SIZE, "%lld", statdat.st_size);
    if (num_bytes != OSCAR_FILE_SIZE)
        md->oscar_size[num_bytes] = ' ';
    md->oscar_deleted = "y";
    num_bytes = snprintf(&md->oscar_hdr_end, OSCAR_HDR_END_LEN, "%s", OSCAR_HDR_END);
    if (num_bytes != OSCAR_HDR_END_LEN)
        md->oscar_hdr_end[num_bytes] = ' ';
    return 0;
}

void mark_for_deletion(char *archive_name, size_t number_of_files,
                      char** file_names) {
    if (access(archive_name, F_OK) == -1) {
        printf("Archive %s doesn't exist!\n", archive_name);
        exit(-1);
    }
    FILE *archive = fopen(archive_name, "r+b");
    struct oscar_hdr md;
    fseek(archive, OSCAR_ID_LEN+1, SEEK_SET);
    while (!feof(archive)) {
        fseek(archive, -1, SEEK_CUR);
        int total_bytes = fread(&md.oscar_name, OSCAR_MAX_FILE_NAME_LEN, 1,
                             archive);
        if (list_contains(file_names, number_of_files, md.oscar_name)) {
            fseek(archive, 2 +
                      OSCAR_DATE_SIZE * 3 + OSCAR_UGID_SIZE * 2 + 
                      OSCAR_MODE_SIZE + OSCAR_FILE_SIZE + 1, SEEK_CUR);
            fwrite("y", 1, 1, archive);
            fseek(archive, OSCAR_SHA_DIGEST_LEN + OSCAR_HDR_END_LEN, SEEK_CUR);
        }
    }
}

int write_file_meta_data(struct oscar_hdr* md, FILE *file_out) {
    int total_bytes = 0;
    total_bytes += write(fileno(file_out), &md->oscar_name, OSCAR_MAX_FILE_NAME_LEN);
    //total_bytes += write(fileno(file_out), "  ", 2);
    total_bytes += write(fileno(file_out), &md->oscar_name_len, 2);
    total_bytes += write(fileno(file_out), &md->oscar_cdate, OSCAR_DATE_SIZE);
    // total_bytes += write(fileno(file_out), " ", 1);
    total_bytes += write(fileno(file_out), &md->oscar_adate, OSCAR_DATE_SIZE);
    //total_bytes += write(fileno(file_out), " ", 1);
    total_bytes += write(fileno(file_out), &md->oscar_mdate, OSCAR_DATE_SIZE);
    //total_bytes += write(fileno(file_out), " ", 1);
    total_bytes += write(fileno(file_out), &md->oscar_uid, OSCAR_UGID_SIZE);
    //total_bytes += write(fileno(file_out), " ", 1);
    total_bytes += write(fileno(file_out), &md->oscar_gid, OSCAR_UGID_SIZE);
    //total_bytes += write(fileno(file_out), " ", 1);
    total_bytes += write(fileno(file_out), &md->oscar_mode, OSCAR_MODE_SIZE);
    //total_bytes += write(fileno(file_out), " ", 1);
    total_bytes += write(fileno(file_out), &md->oscar_size, OSCAR_FILE_SIZE);
    //total_bytes += write(fileno(file_out), " ", 1);
    total_bytes += write(fileno(file_out), &md->oscar_deleted, 1);
    total_bytes += write(fileno(file_out), &md->oscar_sha1, OSCAR_SHA_DIGEST_LEN);
    total_bytes += write(fileno(file_out), OSCAR_HDR_END, OSCAR_HDR_END_LEN);
    return 0;
}

int read_file_meta_data(struct oscar_hdr *md, FILE *file_in) {
    assert(!feof(file_in));
    printf("%lu\n", ftell(file_in));
    int total_bytes = 0;
    total_bytes += fread(&md->oscar_name, OSCAR_MAX_FILE_NAME_LEN, 1, file_in);
    total_bytes += fread(&md->oscar_name_len, 2, 1, file_in);
    total_bytes += fread(&md->oscar_cdate, OSCAR_DATE_SIZE, 1, file_in);
    total_bytes += fread(&md->oscar_adate, OSCAR_DATE_SIZE, 1, file_in);
    total_bytes += fread(&md->oscar_mdate, OSCAR_DATE_SIZE, 1, file_in);
    total_bytes += fread(&md->oscar_uid, OSCAR_UGID_SIZE, 1, file_in);
    total_bytes += fread(&md->oscar_gid, OSCAR_UGID_SIZE, 1, file_in);
    total_bytes += fread(&md->oscar_mode, OSCAR_MODE_SIZE, 1, file_in);
    total_bytes += fread(&md->oscar_size, OSCAR_FILE_SIZE, 1, file_in);
    total_bytes += fread(&md->oscar_deleted, 1, 1, file_in);
    total_bytes += fread(&md->oscar_sha1, OSCAR_SHA_DIGEST_LEN, 1, file_in);
    total_bytes += fread(&md->oscar_hdr_end, OSCAR_HDR_END_LEN, 1, file_in);
    printf("tb%i\n", total_bytes);
    printf("%lu\n", ftell(file_in));
    assert(!feof(file_in));
    return 0;
}

void print_md(struct oscar_hdr *md) {
    /*
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
    */
    puts("commented out");
}

int twiddle_meta_data(struct oscar_hdr *md, FILE *file_in) {
    struct utimbuf buf;
    time_t atime, mtime = 0;
    atime = strtoul(md->oscar_adate, NULL, 0);
    buf.actime = atime;
    buf.modtime = mtime;
    puts("TODO: change birthtime!");
    futimes(fileno(file_in), &buf);
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

bool list_contains(char** file_names, int num_files, char* file_name) {
    for (int i = 0; i < num_files; i++) {
        if (strncmp(file_names[i], file_name, OSCAR_MAX_FILE_NAME_LEN) == 0) {
            return true;
        }
    }
    return false;
}
