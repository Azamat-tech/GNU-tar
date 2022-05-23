#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>

#define BLOCK_SIZE        512
#define OLDGNU_MAGIC      "ustar  "  /* 7 chars and a null */

#define EXE_NAME          "mytar"
#define MISSING_OPTIONS   "need at least one option"
#define REQUIRES_ARG      "option requires an argument -- '%s'"
#define FILE_NOT_FOUND    "%s: Cannot open: No such file or directory\n%s: Error is not recoverable: exiting now"
#define UNEXPECTED_EOF    "Unexpected EOF in archive\n%s: Error is not recoverable: exiting now"
#define WRONG_FORMAT      "This does not look like a tar archive\n%s: Error is not recoverable: exiting now"
#define TAR_FILE_MISSING  "Refusing to read archive contents from terminal (missing -f option?)"
#define UNKNOWN_TYPE      "Unsupported header type: %d\n"
#define FILE_ERROR        "Cannot read: Input/output error"
#define FAILURE_STATUS    "Exiting with failure status due to previous errors"
#define ZERO_BLOCK        "%s: A lone zero block at %d\n"
#define NOT_FOUND_IN_TAR  "%s: %s: Not found in archive\n"
#define UNKNOWN_OPTION    "invalid option -- '%c'"

typedef struct posix_header {            /* byte offset */
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
  char dummy[12];               /* 500 */
                                /* 512 */
} header;

typedef struct option { 
  int file;
  int list;
} option;

// stores the start and end index of t files if specified
typedef struct t_files_position { 
  int start;
  int end;
} t_pos;

// print info of the file header
void print_information(header* header) { 
  char *ptr;
  long size = strtoul(header->size, &ptr, 8);
  printf("name: %s\n", header->name);
  printf("mode: %s\n", header->mode);
  printf("uid: %s\n", header->uid);
  printf("gid: %s\n", header->gid);
  printf("size: %lu\n", size);
  printf("mtime: %s\n", header->mtime);
  printf("chksum: %s\n", header->chksum);
  printf("typeflag: %c\n", header->typeflag);
  printf("linkname: %s\n", header->linkname);
  printf("magic: %s\n", header->magic);
  printf("version: %s\n", header->version);
  printf("uname: %s\n", header->uname);
  printf("gname: %s\n", header->gname);
  printf("devmajor: %s\n", header->devmajor);
  printf("devminor: %s\n", header->devminor);
  printf("prefix: %s\n", header->prefix);
}

// checks if -f contains the argument
void check_foption(int argc, char *argv[], int i, int t_option) { 
  // case: tar -f
  if (argc == ++i) 
    errx(2, REQUIRES_ARG, ++argv[--i]);

  // case: tar -f -t (missing argument for -f)
  if (argv[i][0] == '-') 
    errx(2, MISSING_OPTIONS);

  // case: tar -f test.tar (missing options)
  if (argc == ++i && !t_option) 
    errx(2, MISSING_OPTIONS);
}

// processing the options and their arguments
void process_arguments(int argc, char *argv[], int *file_selected, char **file, option* op, t_pos *pos) {
  int i;
  
  for (i = 1; i < argc; i++) { 
    char char_zero = argv[i][0];
    char char_one  = argv[i][1];
    // options 
    if (char_zero == '-') 
      switch (char_one) {
        case 'f':
          check_foption(argc, argv, i, op->list);
          op->file = (char_one == 'f');
          file_selected++;
          *file = argv[++i];
          break;
        case 't':
          op->list = (char_one == 't');
          break;
        default:
          errx(2, UNKNOWN_OPTION, char_one);
      }
    // arguments
    else if (op->list) {
      if (file_selected && pos->start == 0) 
        pos->start = i;
    }
    else if (!op->file && !op->list) {
      errx(2, UNKNOWN_OPTION, char_one);
    }
  }

  if (op->list && !op->file) 
    errx(2, TAR_FILE_MISSING);
}

// returns the total size of the file 
int get_file_size(FILE *fp) { 
  int sz;
  fseek(fp, 0L, SEEK_END);
  sz = ftell(fp);
  fseek(fp, 0L, SEEK_SET);
  return sz;
}

// checks TAR magic
void check_magic(header *header) { 
  if (strcmp(header->magic, OLDGNU_MAGIC)) 
    errx(2, WRONG_FORMAT, EXE_NAME);
}

// checks the first 512 bytes to confirm it is tar format
void check_tar_format(FILE *fp) { 
  header header; 
  
  if (!fread(&header, BLOCK_SIZE, 1, fp))
    errx(2, WRONG_FORMAT, EXE_NAME);

  check_magic(&header);

  // reset the pointer of the file back 512 bytes
  fseek(fp, -BLOCK_SIZE, SEEK_CUR);
}

int check_EOF(FILE *fp, int file_size, long content_size, int cur_pos) { 
 char buffer1[BLOCK_SIZE];

  // check for partial
  if (file_size - BLOCK_SIZE < content_size) 
    errx(2, UNEXPECTED_EOF, EXE_NAME);

  // check two_zero block
  if (!fread(buffer1, BLOCK_SIZE, 1, fp)) 
    return 1;

  char buffer[BLOCK_SIZE];

  if (!fread(buffer, 512, 1, fp)) { 
    printf(ZERO_BLOCK, EXE_NAME, cur_pos / BLOCK_SIZE);
    return 1;
  }

  for (int i = 0; i < BLOCK_SIZE; i++) 
    if (buffer[i] || buffer1[i]) 
      return 0;
  return 1;
}

// -t option listing the items with/without arguments
void list_items(header *header, char *argv[], t_pos *pos, int arr[]) { 
  // -t without arguments
  if (pos->start == 0) { 
    printf("%s\n", header->name);
    fflush(stdout);
  }
  // -t with arguments
  else {
    int old_start = pos->start;
    while (old_start != pos->end) { 
      int curr_offset = old_start - pos->start;
      if (strcmp(header->name, argv[old_start]) == 0) { 
        if (arr[curr_offset] == 0) {
          printf("%s\n", header->name);
          arr[curr_offset] = 1;
        }
      }
      old_start++;
    }
  }
}

void print_not_found_files(char *argv[], t_pos *pos, int arr[], int *status) {
  if (pos->start != 0) 
    for (int i = 0; i < pos->end - pos->start; i++)
      if (arr[i] == 0) { 
        printf(NOT_FOUND_IN_TAR, EXE_NAME, argv[pos->start + i]);
        fflush(stdout);
        *status = 2;
      }
}

// this method processes the file based on the options selected
int list_archive_members(FILE *fp, t_pos *pos, char *argv[]) { 
  header header;

  int cur_pos = 0;
  int status = 0;
  int offset = pos->end - pos->start;
  int archive_size = get_file_size(fp);
  
  char *ptr;
  long size = 0;

  int index_for_t[offset];
  for (int i = 0; i < offset; i++) {
    index_for_t[i] = 0;
  }

  while (1) { 
    if (check_EOF(fp, archive_size, size, cur_pos)) 
      break;
    else 
      fseek(fp, -2 * BLOCK_SIZE, SEEK_CUR);

    check_tar_format(fp);

    fread(&header, BLOCK_SIZE, 1, fp);
    cur_pos += BLOCK_SIZE;

    if (archive_size <= cur_pos) 
      break;

    if (header.typeflag && header.typeflag != '0') 
			errx(2, UNKNOWN_TYPE, header.typeflag);

    // -t option
    list_items(&header, argv, &*pos, index_for_t);

    // get content size
    size = strtoul(header.size, &ptr, 8);
    // set the file pointer after the content
    fseek(fp, size, SEEK_CUR);
    cur_pos += BLOCK_SIZE + size;
  }

  print_not_found_files(argv, &*pos, index_for_t, &status);

  return status;
}

int main(int argc, char *argv[]) { 
  int file_selected = 0;

  char *file;

  option op = { 
    .file = 0,
    .list = 0
  };
  
  t_pos pos = { 
    .start = 0,
    .end = argc
  };
  
  if (argc < 2) 
    errx(2, MISSING_OPTIONS);

  // process the arguments given and stores to main variables
  process_arguments(argc, argv, &file_selected, &file, &op, &pos);

  // open tar file
  FILE *fp = fopen(file, "rb");

  // case: no such file given options provided
  if (fp == NULL) 
    errx(2, FILE_NOT_FOUND, file, EXE_NAME);

  if (op.list) 
    if (list_archive_members(fp, &pos, argv) == 2)
      errx(2, FAILURE_STATUS);
}