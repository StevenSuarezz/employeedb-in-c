#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "parse.h"

/**
 * Lists all employees stored in the database by printing their details to stdout
 * Displays name, address, and hours for each employee
 * 
 * @param dbheader: Pointer to the database header containing metadata
 * @param employees: Array of employee records to display
 */
void list_employees(struct dbheader_t *dbheader, struct employee_t *employees) {
  int employeeCount = dbheader->count;
  
  for(int i = 0; i < employeeCount; i++) {
    printf("Employee %d\n", i);
    printf("\t Name: %s\n", employees[i].name);
    printf("\t Address: %s\n", employees[i].address);
    printf("\t Hours: %d\n", employees[i].hours);
  }
}

/**
 * Adds a new employee to the database from a comma separated string
 * Expected format: "name,address,hours"
 * Assumes the employees array has already been reallocated to accommodate the new employee
 * 
 * @param dbheader: Pointer to the database header containing metadata
 * @param employees: Array of employee records to add to
 * @param addstring: Comma-separated string containing employee data
 * 
 * @return STATUS_OK on success, STATUS_ERROR on failure
 */int add_employee(struct dbheader_t *dbheader, struct employee_t *employees, char *addstring) {
  char *name = strtok(addstring, ",");
  char *address = strtok(NULL, ",");
  char *hours = strtok(NULL, ",");

  int employeeIndex = dbheader->count - 1;
  strncpy(employees[employeeIndex].name, name, sizeof(employees[employeeIndex].name));
  strncpy(employees[employeeIndex].address, address, sizeof(employees[employeeIndex].address));
  employees[employeeIndex].hours = atoi(hours);

  return STATUS_OK;
}

/**
 * Reads employee records from the database file and allocates memory for them
 * The caller is responsible for freeing the allocated memory pointed to by employeesOut
 * 
 * @param fd: File descriptor for the database file
 * @param dbheader: Pointer to the database header containing metadata
 * @param employeesOut: Output parameter - will contain pointer to newly allocated employee array
 * 
 * @return STATUS_OK on success, STATUS_ERROR on failure
 *         Failures can occur on:
 *         - Invalid file descriptors
 *         - Memory allocation failures
 *         - Read operation failures
 */int read_employees(int fd, struct dbheader_t *dbheader, struct employee_t **employeesOut) {
  if (fd < 0) {
    printf("Got a bad file descriptor from user\n");
    return STATUS_ERROR;
  }

  int employeeCount = dbheader->count;

  struct employee_t *employees = calloc(employeeCount, sizeof(struct employee_t));
  if (employees == NULL) {
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

/**
 * Writes the complete database to disk, including header and employee records
 * Converts all multi-byte values to network byte order before writing
 * 
 * @param fd: File descriptor for the database file
 * @param dbheader: Pointer to database header to write
 * @param employees: Array of employee records to write
 * 
 * @return STATUS_OK on success, STATUS_ERROR on failure
 *         Failures can occur on:
 *         - An invalid file descriptor
 *         - Write operation failures
 */
int output_file(int fd, struct dbheader_t *dbheader, struct employee_t *employees) {
  if (fd < 0) {
    printf("Got a bad file descriptor from user\n");
    return STATUS_ERROR;
  }

  int employeeCount = dbheader->count;

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

/**
 * Validates an existing database header from disk
 * Checks magic number, version, and file size for consistency
 * Converts multi-byte values from network to host byte order
 * The caller is responsible for freeing the allocated header memory
 * 
 * @param fd: File descriptor for the database file
 * @param headerOut: Output parameter - will contain pointer to newly allocated and validated header
 * 
 * @return STATUS_OK on success, STATUS_ERROR on failure
 *         Failures can occur on:
 *         - Invalid file descriptors
 *         - Memory allocation failures
 *         - Read operation failures
 *         - An invalid magic number
 *         - A wrong version number
 *         - A corrupted file size
 */
int validate_db_header(int fd, struct dbheader_t **headerOut) {
  if (fd < 0) {
    printf("Got a bad file descriptor from user\n");
    return STATUS_ERROR;
  }

  struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
  if (header == NULL) {
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

/**
 * Creates a new database header with default values
 * The caller is responsible for freeing the allocated header memory
 * 
 * @param fd: File descriptor for the database file (unused, could be removed)
 * @param headerOut: Output parameter - will contain pointer to newly allocated header
 * 
 * @return STATUS_OK on success, STATUS_ERROR on failure
 *         Failures can occur on:
 *         - Memory allocation failures
 */
int create_db_header(int fd, struct dbheader_t **headerOut) {
  struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));

  if (header == NULL) {
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
