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

int main() {
    puts("0");
    FILE *new_archive = create_new_archive("test.ar");
    puts("1");
    FILE *read_file_1 = fopen("2-s.txt", "r");
    puts("2");
    FILE *read_file_2 = fopen("4-s.txt", "r");
    puts("3");
    append_file(read_file_1, "2-s.txt", new_archive);
    puts("4");
    append_file(read_file_2, "4-s.txt", new_archive);
    puts("5");
    fclose(new_archive);
    fclose(read_file_1);
    fclose(read_file_2);
    puts("6");
    //extract_archive("Project2/arch24.oscar");
    puts("7");
    return 0;
}

void extract_archive(char* archive_name) {
    struct oscar_hdr md;
    char c = 0;
    FILE *current_file;
    FILE *archive = fopen(archive_name, "r");
    fseek(archive, OSCAR_ID_LEN, SEEK_SET);
    puts("6.5");
    while ((c = fgetc(archive)) != EOF) {
        ungetc(c, archive);
        read_file_meta_data(&md, archive);
        // TODO: add existence checks
        current_file = fopen(md.oscar_name, "w");
        n_read_and_write_file(archive, current_file,
            strtol(md.oscar_size, md.oscar_size + OSCAR_FILE_SIZE, 10)
        );
        twiddle_meta_data(&md, current_file);
        fclose(current_file);
    }
    fclose(archive);
    puts("6.75");
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

    snprintf(&md->oscar_name, OSCAR_MAX_FILE_NAME_LEN, "%s", file_name);
    snprintf(&md->oscar_name_len, 2, "%lu", strlen(file_name));
    snprintf(&md->oscar_cdate, OSCAR_DATE_SIZE, "%ld", statdat.st_birthtime);
    snprintf(&md->oscar_adate, OSCAR_DATE_SIZE, "%ld", statdat.st_atime);
    snprintf(&md->oscar_mdate, OSCAR_DATE_SIZE, "%ld", statdat.st_mtime);
    puts("these two are broken");
    snprintf(&md->oscar_uid, OSCAR_UGID_SIZE, "%u", statdat.st_uid);
    snprintf(&md->oscar_gid, OSCAR_UGID_SIZE, "%u", statdat.st_gid);
    snprintf(&md->oscar_mode, OSCAR_MODE_SIZE, "%u", statdat.st_mode);
    snprintf(&md->oscar_size, OSCAR_FILE_SIZE, "%lld", statdat.st_size);
    snprintf(&md->oscar_deleted, 1, "%s", " ");
    snprintf(&md->oscar_hdr_end, OSCAR_HDR_END_LEN, "%s", OSCAR_HDR_END);
    printf("ee %s, %zu", md->oscar_hdr_end, strlen(md->oscar_hdr_end));
    return 0;
}

int write_file_meta_data(struct oscar_hdr* md, FILE *file_out) {
    fwrite(&md->oscar_name, OSCAR_MAX_FILE_NAME_LEN, 1, file_out);
    fwrite("  ", 2, 1, file_out);
    fwrite(&md->oscar_name_len, 2, 1, file_out);
    fwrite(&md->oscar_cdate, OSCAR_DATE_SIZE, 1, file_out);
    fwrite(" ", 2, 1, file_out);
    fwrite(&md->oscar_adate, OSCAR_DATE_SIZE, 1, file_out);
    fwrite(" ", 2, 1, file_out);
    fwrite(&md->oscar_mdate, OSCAR_DATE_SIZE, 1, file_out);
    fwrite(" ", 2, 1, file_out);
    fwrite(&md->oscar_uid, OSCAR_UGID_SIZE, 1, file_out);
    fwrite(" ", 2, 1, file_out);
    fwrite(&md->oscar_gid, OSCAR_UGID_SIZE, 1, file_out);
    fwrite(" ", 2, 1, file_out);
    fwrite(&md->oscar_mode, OSCAR_MODE_SIZE, 1, file_out);
    fwrite(" ", 2, 1, file_out);
    fwrite(&md->oscar_size, OSCAR_FILE_SIZE, 1, file_out);
    fwrite(" ", 2, 1, file_out);
    fwrite(&md->oscar_deleted, 1, 1, file_out);
    fwrite(" ", 2, 1, file_out);
    fwrite(&md->oscar_sha1, OSCAR_SHA_DIGEST_LEN, 1, file_out);
    fwrite(" ", 2, 1, file_out);
    fwrite(OSCAR_HDR_END, OSCAR_HDR_END_LEN, 1, file_out);
    return 0;
}

int read_file_meta_data(struct oscar_hdr *md, FILE *file_in) {
    if (feof(file_in)) return 1;
    fread(&md->oscar_name, OSCAR_MAX_FILE_NAME_LEN, 1, file_in);
    fseek(file_in, 2, SEEK_CUR);
    fread(&md->oscar_name_len, 2, 1, file_in);
    fread(&md->oscar_cdate, OSCAR_DATE_SIZE, 1, file_in);
    fseek(file_in, 2, SEEK_CUR);
    fread(&md->oscar_adate, OSCAR_DATE_SIZE, 1, file_in);
    fseek(file_in, 2, SEEK_CUR);
    fread(&md->oscar_mdate, OSCAR_DATE_SIZE, 1, file_in);
    fseek(file_in, 2, SEEK_CUR);
    fread(&md->oscar_uid, OSCAR_UGID_SIZE, 1, file_in);
    fseek(file_in, 2, SEEK_CUR);
    fread(&md->oscar_gid, OSCAR_UGID_SIZE, 1, file_in);
    fseek(file_in, 2, SEEK_CUR);
    fread(&md->oscar_mode, OSCAR_MODE_SIZE, 1, file_in);
    fseek(file_in, 2, SEEK_CUR);
    fread(&md->oscar_size, OSCAR_MAX_MEMBER_FILE_SIZE, 1, file_in);
    fseek(file_in, 2, SEEK_CUR);
    fread(&md->oscar_deleted, 1, 1, file_in);
    fseek(file_in, 2, SEEK_CUR);
    fread(&md->oscar_sha1, OSCAR_SHA_DIGEST_LEN, 1, file_in);
    fseek(file_in, 2, SEEK_CUR);
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
    //futimes(fileno(file_in), &buf);
    puts("TODO: verify that md->oscar_mode is in octal!");
    unsigned int uid, gid = 0;
    unsigned short mode = 0;
    mode = (unsigned short)strtol(&md->oscar_mode,
                                  &md->oscar_mode + OSCAR_MODE_SIZE, 8);
    uid = (unsigned int)strtol(&md->oscar_uid,
                               &md->oscar_uid + OSCAR_UGID_SIZE, 0);
    gid = (unsigned int)strtol(&md->oscar_gid,
                               &md->oscar_gid + OSCAR_UGID_SIZE, 0);
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
    while((ch = fgetc(file_in)) != EOF && bytes_read < num_bytes) {
        bytes_read++;
        fputc(ch, file_out);
    }
    return 0;
}
