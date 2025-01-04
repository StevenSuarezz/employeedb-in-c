#include <stdio.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "file.h"

/**
* Creates a new database file with read/write permissions
* Checks if file already exists to prevent overwriting
* 
* @param filename: Path where database file should be created
* 
* @return File descriptor on success, STATUS_ERROR on failure
*         Failures occur if:
*         - File already exists
*         - Unable to create file (permissions/disk full)
*
* Note: Caller is responsible for closing the returned file descriptor
*/
int create_db_file(char *filename) {
  int fd = open(filename, O_RDONLY);
  if (fd != -1) {
    close(fd);
    printf("File %s already exists...\n", filename);
    return STATUS_ERROR;
  }

  fd = open(filename, O_RDWR | O_CREAT, 0644);
  if (fd == -1) {
    perror("open");
    return STATUS_ERROR;
  }

  return fd;
}

/**
* Opens an existing database file in read/write mode.
* 
* @param filename: Path to existing database file
* 
* @return File descriptor on success, STATUS_ERROR on failure
*         Failures occur if:
*         - File doesn't exist
*         - Insufficient permissions
*         - Other system errors (see errno)
*
* Note: Caller is responsible for closing the returned file descriptor
*/
int open_db_file(char *filename) {
  int fd = open(filename, O_RDWR, 0644);
  if (fd == -1) {
    perror("open");
    return STATUS_ERROR;
  }

  return fd;
}
