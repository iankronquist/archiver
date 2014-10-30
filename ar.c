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
void delete(char *archive_name, size_t number_of_files,
                       char** file_names);

long ltell(int file_des);

bool list_contains(char** file_names, int num_files, char* file_name);

int main() {
    char* files_to_delete[] = {"2-s.txt", "4-s.txt"};
    extract_archive("arch24.oscar", 2, files_to_delete);
    FILE *new_archive = create_new_archive("test.ar");
    FILE *read_file_1 = fopen("2-s.txt", "r");
    FILE *read_file_2 = fopen("4-s.txt", "r");
    append_file(read_file_1, "2-s.txt", new_archive);
    append_file(read_file_2, "4-s.txt", new_archive);
    fclose(new_archive);
    fclose(read_file_1);
    fclose(read_file_2);
    delete("test.ar", 1, files_to_delete);
    extract_archive("test.ar", 1, files_to_delete);
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
        fseek(archive, -1, SEEK_CUR);
        read_file_meta_data(&md, archive);
        char file_name[OSCAR_MAX_FILE_NAME_LEN];
        memcpy(&file_name, md.oscar_name, OSCAR_MAX_FILE_NAME_LEN);
        trim_trailing_spaces(&file_name, OSCAR_MAX_FILE_NAME_LEN);
        if ((num_files == 0 || list_contains(file_names, num_files, file_name))
            && md.oscar_deleted != 'y') {
            //TODO: check if file exists
            current_file = fopen(&file_name, "w");
            assert(current_file != NULL);
            n_read_and_write_file(archive, current_file,
                strtol(md.oscar_size, NULL, 10)
            );
            twiddle_meta_data(&md, current_file);
            fclose(current_file);
        } else {
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
    assert(out_file != NULL);
    write(fileno(out_file), OSCAR_ID, OSCAR_ID_LEN);
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
    char buf[3];

    int num_bytes = 0;
    num_bytes = snprintf(&md->oscar_name, OSCAR_MAX_FILE_NAME_LEN, "%s", file_name);
    if (num_bytes != OSCAR_MAX_FILE_NAME_LEN)
        md->oscar_name[num_bytes] = ' ';
    size_t file_name_len = strlen(file_name);
    num_bytes = snprintf(&buf, 2, "%zu", file_name_len);
    memcpy(md->oscar_name_len, &buf, 1);
    // HACK
    if (num_bytes != 2) {
        md->oscar_name_len[1] = md->oscar_name_len[0];
        md->oscar_name_len[0] = ' ';
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
    md->oscar_deleted = ' ';
    num_bytes = snprintf(&md->oscar_hdr_end, OSCAR_HDR_END_LEN, "%s", OSCAR_HDR_END);
    //if (num_bytes != OSCAR_HDR_END_LEN)
    //    md->oscar_hdr_end[num_bytes] = ' ';
    return 0;
}

void mark_for_deletion(char *archive_name, size_t number_of_files,
                      char** file_names) {
    if (access(archive_name, F_OK) == -1) {
        printf("Archive %s doesn't exist!\n", archive_name);
        exit(-1);
    }
    FILE *archive = fopen(archive_name, "r+");
    struct oscar_hdr md;
    fseek(archive, OSCAR_ID_LEN+1, SEEK_SET);
    struct oscar_hdr arch_md;
    get_file_meta_data(&arch_md, archive, "archive!!");
    long archive_name_length = strtol(arch_md.oscar_size, NULL, 10);
    assert(archive_name_length > 0);
    long file_name_len = 0;
    long file_size = 0;
    char file_name[OSCAR_MAX_FILE_NAME_LEN+1];
    while (ftell(archive) < archive_name_length) {
        fseek(archive, -1, SEEK_CUR);
        fread(&md.oscar_name, OSCAR_MAX_FILE_NAME_LEN, 1, archive);
        fread(&md.oscar_name_len, 2, 1, archive);
        file_name_len = strtol(&md.oscar_name_len, NULL, 10);
        assert(file_name_len > 0);
        fseek(archive, 3 * OSCAR_DATE_SIZE + 2 * OSCAR_UGID_SIZE +
            OSCAR_MODE_SIZE, SEEK_CUR);
        fread(&md.oscar_size, OSCAR_FILE_SIZE, 1, archive);
        file_size = strtol(&md.oscar_name_len, NULL, 10);
        assert(file_size >= 0);
        strncpy(file_name, &md.oscar_name, file_size);
        if (list_contains(file_names, number_of_files, file_name)) {
            fwrite("y", 1, 1, archive);
        } else {
            fseek(archive, 1, SEEK_CUR);
        }
        fseek(archive, OSCAR_SHA_DIGEST_LEN + OSCAR_HDR_END_LEN, SEEK_CUR);
        fseek(archive, file_size, SEEK_CUR);
    }
}

void delete(char *archive_name, size_t number_of_files,
                      char** file_names) {
    if (access(archive_name, F_OK) == -1) {
        printf("Archive %s doesn't exist!\n", archive_name);
        exit(-1);
    }
    char temp_archive_name[] = ".tmp_archive";
    FILE *archive = fopen(archive_name, "r");
    FILE *temp_archive = create_new_archive(&temp_archive_name);
    struct oscar_hdr md;
    fseek(archive, OSCAR_ID_LEN, SEEK_SET);
    struct oscar_hdr arch_md;
    get_file_meta_data(&arch_md, archive, "archive!!");
    long archive_name_length = strtol(arch_md.oscar_size, NULL, 10);
    assert(archive_name_length > 0);
    long file_name_len = 0;
    long file_size = 0;
    char file_name[OSCAR_MAX_FILE_NAME_LEN+1];
    char file_name_len_buffer[3];
    while (ftell(archive) < archive_name_length) {
        //fseek(archive, -2, SEEK_CUR);

        read_file_meta_data(&md, archive);

        strncpy(&file_name_len_buffer, &md.oscar_name_len, 2);
        file_name_len = strtol(&file_name_len_buffer, NULL, 10);
        assert(file_name_len > 0);

        file_size = strtol(&md.oscar_size, NULL, 10);

        assert(file_size >= 0);
        assert(file_name_len < OSCAR_MAX_FILE_NAME_LEN);
        strncpy(&file_name, &md.oscar_name, file_name_len);
        if (list_contains(file_names, number_of_files, file_name)) {
            fseek(archive, file_size, SEEK_CUR);
        } else {
            write_file_meta_data(&md, temp_archive);
            n_read_and_write_file(archive, temp_archive, file_size);
        }
    }
    fclose(archive);
    fclose(temp_archive);
    rename(temp_archive_name, archive_name);
}

int write_file_meta_data(struct oscar_hdr* md, FILE *file_out) {
    int total_bytes = 0;
    total_bytes += write(fileno(file_out), md, sizeof(struct oscar_hdr)-OSCAR_HDR_END_LEN);
    total_bytes += write(fileno(file_out), OSCAR_HDR_END, OSCAR_HDR_END_LEN);
    return 0;
}

long ltell(int file_des) {
    return lseek(file_des, 0, SEEK_CUR);
}

int read_file_meta_data(struct oscar_hdr *md, FILE *file_in) {
    assert(!feof(file_in));
    int total_bytes = 0;
    /*
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
    */
    total_bytes += fread(md, sizeof(struct oscar_hdr), 1, file_in);
    //assert(total_bytes == sizeof(struct oscar_hdr));
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
    time_t atime, mtime = 0;
    struct timeval amtimeval[2];
    atime = strtoul(md->oscar_adate, NULL, 0);
    mtime = strtoul(md->oscar_mdate, NULL, 0);
    amtimeval[0].tv_sec = atime;
    amtimeval[0].tv_usec = 0;
    amtimeval[1].tv_sec = mtime;
    amtimeval[1].tv_usec = 0;
    futimes(fileno(file_in), amtimeval);
    unsigned int uid, gid = 0;
    unsigned short mode = 0;
    mode = (unsigned short)strtol(&md->oscar_mode, NULL, 8);
    uid = (unsigned int)strtol(&md->oscar_uid, NULL, 0);
    gid = (unsigned int)strtol(&md->oscar_gid, NULL, 0);
    fchmod(fileno(file_in), mode);
    fchown(fileno(file_in), uid, gid);
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
