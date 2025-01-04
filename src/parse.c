#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "parse.h"

void list_employees(struct dbheader_t *dbheader, struct employee_t *employees) {
}

int add_employee(struct dbheader_t *dbheader, struct employee_t *employees, char *addstring) {
  printf("%s\n", addstring);

  char *name = strtok(addstring, ",");
  char *address = strtok(NULL, ",");
  char *hours = strtok(NULL, ",");

  int employeeIndex = dbheader->count - 1;
  strncpy(employees[employeeIndex].name, name, sizeof(employees[employeeIndex].name));
  strncpy(employees[employeeIndex].address, address, sizeof(employees[employeeIndex].address));
  employees[employeeIndex].hours = atoi(hours);
  printf("In add_employee: %s %s %d\n", employees[employeeIndex].name, employees[employeeIndex].address, employees[employeeIndex].hours);

  return STATUS_OK;
}

int read_employees(int fd, struct dbheader_t *dbheader, struct employee_t **employeesOut) {
  if (fd < 0) {
    printf("Got a bad file descriptor from user\n");
    return STATUS_ERROR;
  }

  int employeeCount = dbheader->count;

  struct employee_t *employees = calloc(employeeCount, sizeof(struct employee_t));
  if (employees == -1) {
    printf("Malloc failed to create employee array\n");
    return STATUS_ERROR;
  }

  read(fd, employees, employeeCount * sizeof(struct employee_t));

  // Unpack employee hours to host endianness
  for (int i = 0; i < employeeCount; i++) {
    employees[i].hours = ntohl(employees[i].hours);
  }

  *employeesOut = employees;
  return STATUS_OK;
}

int output_file(int fd, struct dbheader_t *dbheader, struct employee_t *employees) {
  if (fd < 0) {
    printf("Got a bad file descriptor from user\n");
    return STATUS_ERROR;
  }

  int employeeCount = dbheader->count;
  if (employeeCount > 0)
    printf("In output_file: %s %s %d\n", employees[0].name, employees[0].address, employees[0].hours);

  // Pack into network endianness
  dbheader->magic = htonl(dbheader->magic);
  dbheader->version = htons(dbheader->version);
  dbheader->count = htons(dbheader->count);
  dbheader->filesize = htonl(sizeof(struct dbheader_t) + employeeCount * sizeof(struct employee_t));

  // Call lseek to reposition the open file offset to the beginning before we write the header
  lseek(fd, 0, SEEK_SET);
  write(fd, dbheader, sizeof(struct dbheader_t));

  for(int i = 0; i < employeeCount; i++) {
    employees[i].hours = htonl(employees[i].hours);
    write(fd, &employees[i], sizeof(struct employee_t));
  }

  return STATUS_OK;
}

int validate_db_header(int fd, struct dbheader_t **headerOut) {
  if (fd < 0) {
    printf("Got a bad file descriptor from user\n");
    return STATUS_ERROR;
  }

  struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
  if (header == -1) {
    printf("Malloc failed to create a db header\n");
    return STATUS_ERROR;
  }

  if (read(fd, header, sizeof(struct dbheader_t)) != sizeof(struct dbheader_t)) {
    perror("read");
    free(header);
    return STATUS_ERROR;
  }

  // Unpack to host endianness
  header->version = ntohs(header->version);
  header->count = ntohs(header->count);
  header->magic = ntohl(header->magic);
  header->filesize = ntohl(header->filesize);
  
  if (header->magic != HEADER_MAGIC_NUM) {
    printf("Improper header magic\n");
    free(header);
    return -1;
  }
  
  if (header->version != 1) {
    printf("Improper header version\n");
    free(header);
    return -1;
  }

  struct stat dbstat = {0};
  fstat(fd, &dbstat);

  if (header->filesize != dbstat.st_size) {
    printf("Corrupted database\n");
    free(header);
    return -1;
  }

  *headerOut = header;
  return STATUS_OK;
}

int create_db_header(int fd, struct dbheader_t **headerOut) {
  struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));

  if (header == -1) {
    printf("Malloc failed to create db header\n");
    return STATUS_ERROR;
  }

  header->magic = HEADER_MAGIC_NUM;
  header->version = 0x1;
  header->count = 0;
  header->filesize = sizeof(struct dbheader_t);

  *headerOut = header;

  return STATUS_OK;
}
