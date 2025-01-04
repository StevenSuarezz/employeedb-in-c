#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "common.h"
#include "file.h"
#include "parse.h"

/**
* Prints program usage information to stdout with all available command line flags
* 
* @param argv: Pointer to array of command line arguments, used to print program name
*
* Usage flags:
* -n: Create new database file
* -f: (Required) Specify database file path 
* -a: Add employee in format "name,address,hours"
* -l: List all employees
*/
void print_usage(char *argv[]) {
  printf("Usage: %s -n -f <database file>\n", argv[0]);
  printf("\t -n: create new database file\n");
  printf("\t -f: (required) path to database file\n");
  printf("\t -a: append new employee to the database in the format \"<name>,<address>,<hours_worked>\"\n");
  printf("\t -l: list employees in the database\n");
}

int main(int argc, char *argv[]) {
  char *filepath = NULL;
  char *addstring = NULL;
  bool newfile = false;
  bool list = false;
  int dbfd = -1;
  struct dbheader_t *dbheader = NULL;
  struct employee_t *employees = NULL;

  // Use getopt for parsing cmd line flags and arguments
  int c;
  while ((c = getopt(argc, argv, "nf:a:l")) != -1) {
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
    case 'l':
      list = true;
      break;
    case '?':
      printf("Unknown option -%c\n", c);
      break;
    default:
      return -1;
    }
  }

  // Process args parsed with getopt, starting with -f, -n, then -a and -l 
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

  if (list) {
    list_employees(dbheader, employees); 
  }

  output_file(dbfd, dbheader, employees);
  free(dbheader);
  free(employees);
}
