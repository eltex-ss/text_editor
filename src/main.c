#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
  char string_to_write[] = "Hello, world!";
  char string_to_read[15];
  int fd;

  /*  Writing data */
  if ((fd = open("temp_file", O_CREAT | O_WRONLY, 0666)) == -1) {
    perror("Error, can't create file");
    exit(1);
  }
  if ((write(fd, string_to_write, strlen(string_to_write))) == -1) {
    perror("Error, can't write to file");
    exit(1);
  }
  if ((close(fd)) == -1) {
    perror("Error, can't close the file");
    exit(1);
  }
  
  /*  Reading data */
  if ((fd = open("temp_file", O_RDONLY, 0666)) == -1) {
    perror("Error, can't open file to read");
    exit(1);
  }
  if ((read(fd, string_to_read, strlen(string_to_write))) == -1) {
    perror("Error, can't read the file");
    exit(1);
  }
  if ((close(fd)) == -1) {
    perror("Error, can't close reading file");
    exit(1);
  }

  printf("Successfully!\n");
  printf("Read: %s\n", string_to_read);
  return 0;
}
