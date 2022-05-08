#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>

#define BLOCK_SIZE      512

#define EXE_NAME        "mytar"
#define MISSING_OPTIONS "need at least one option"
#define UNKNOWN_OPTION  "invalid option -- '%s'"
#define NO_FILE_NAME    "you must specify one of the options"
#define REQUIRES_ARG    "option requires an argument -- '%s'"
#define FILE_NOT_FOUND  "%s: Cannot open: No such file or directory\n%s: Error is not recoverable: exiting now"
#define UNEXPECTED_EOF  "Unexpected EOF in achive\n%s: Error is not recoverable: exiting now"
#define WRONG_FORMAT    "This does not look like a tar archive\n%s: Error is not recoverable: exiting now"

struct posix_header{            /* byte offset */
  char name[100];               /*   0 */
  char mode[8];                 /* 100 */
  char uid[8];                  /* 108 */
  char gid[8];                  /* 116 */
  char size[12];                /* 124 */
  char mtime[12];               /* 136 */
  char chksum[8];               /* 148 */
  char typeflag;                /* 156 */
  char linkname[100];           /* 157 */
  char magic[6];                /* 257 */
  char version[2];              /* 263 */
  char uname[32];               /* 265 */
  char gname[32];               /* 297 */
  char devmajor[8];             /* 329 */
  char devminor[8];             /* 337 */
  char prefix[155];             /* 345 */
                                /* 500 */
};

enum archive_error { 
  NO_ERROR,
  INCOMAPTIBLE_ARCHIVE,
  EOF_ERROR
};

void handle_error(int error_code) { 
  switch (error_code) {
    case 1: // INCOMPATIBLE_ARCHIVE
      errx(2, WRONG_FORMAT, EXE_NAME);
      break;
    case 2: // EOF_ERROR
      errx(2, UNEXPECTED_EOF, EXE_NAME);
      break;
  }
}

void process_tar_file() { 

}

int get_file_size(FILE *fp) { 
  int sz;
  fseek(fp, 0L, SEEK_END);
  sz = ftell(fp);
  fseek(fp, 0L, SEEK_SET);
  return sz;
}

int tar_file_status(FILE *fp, char **file_name, char **holder, int file_size) {
  if (fp == NULL) { 
    errx(2, FILE_NOT_FOUND, *file_name, EXE_NAME);
  }

  if (fread(*holder, sizeof(char), file_size, fp) != (size_t)file_size) {
    return EOF_ERROR;
  }

  return 0;
}

void list_archive_members(char *holder) { 

}

int main(int argc, char *argv[]) { 
  if (argc < 2) 
    errx(2, MISSING_OPTIONS);

  int i;
  int t_option = 0;
  int f_option = 0;
  int f_file   = 0;
  char *file_name;
  struct posix_header header;

  for (i = 1; i < argc; i++) {
    if (f_option == 1 && f_file == 0) { 
      if (argv[i][0] == '-') {
        errx(2, NO_FILE_NAME);
      }

      file_name = argv[i];
      f_file = 1;
      continue;
    } 

    if (argv[i][0] == '-') {
      char curr_char = argv[i][1];
      switch (curr_char) {
        case 'f':
          f_option = (curr_char == 'f');
          break;
        case 't':
          t_option = (curr_char == 't');
          break;
        default:
          errx(2, UNKNOWN_OPTION, ++argv[i]);
      }
    }
  }























  if (i == argc && f_option == 1 && f_file == 0) {
    errx(2, REQUIRES_ARG, ++argv[--i]);
  }

  FILE *fp = fopen(file_name, "rb");

  int file_size = get_file_size(fp);
  char *holder = malloc(file_size);

  int archive_status = tar_file_status(fp, &file_name, &holder, file_size);

  if (!archive_status) {
    if (t_option) {
      list_archive_members(&holder);
    }
  }

  handle_error(archive_status);
}