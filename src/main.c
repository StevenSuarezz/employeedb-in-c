#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "common.h"
#include "file.h"
#include "parse.h"

void print_usage(char *argv[]) {
  printf("Usage: %s -n -f <database file>\n", argv[0]);
  printf("\t -n: create new database file\n");
  printf("\t -f: (required) path to database file\n");
}

int main(int argc, char *argv[]) {
  char *filepath = NULL;
  bool newfile = false;
  int dbfd = -1;
  struct dbheader_t *dbheader = NULL;
  struct employee_t *employees = NULL;
  char *addstring = NULL;
  int c;

  while ((c = getopt(argc, argv, "nf:a:")) != -1) {
    switch (c) {
    case 'n':
      newfile = true;
      break;
    case 'f':
      filepath = optarg;
      break;
    case 'a':
      addstring = optarg;
      break;
    case '?':
      printf("Unknown option -%c\n", c);
      break;
    default:
      return -1;
    }
  }

  if (filepath == NULL) {
    printf("Filepath is a required argument\n");
    print_usage(argv);

    return 0;
  }

  if (newfile) {
    dbfd = create_db_file(filepath);
    if (dbfd == STATUS_ERROR) {
      printf("Unable to create database file\n");
      return -1;
    }

    if (create_db_header(dbfd, &dbheader) == STATUS_ERROR) {
      printf("Failed to create databade header \n");
      return -1;
    }
  } else {
    dbfd = open_db_file(filepath);
    if (dbfd == STATUS_ERROR) {
      printf("Unable to open database file\n");
      return -1;
    }

    if (validate_db_header(dbfd, &dbheader) == STATUS_ERROR) {
      printf("Failed to validate database header\n");
      return -1;
    }
  }

  if (read_employees(dbfd, dbheader, &employees) != STATUS_OK) {
    printf("Failed to read employees\n");
    return -1;
  }

  if (addstring) {
    dbheader->count++;
    employees = realloc(employees, dbheader->count * sizeof(struct employee_t));
    memset(&employees[dbheader->count - 1], 0, sizeof(struct employee_t));
    add_employee(dbheader, employees, addstring);
  }

  output_file(dbfd, dbheader, employees);
}
