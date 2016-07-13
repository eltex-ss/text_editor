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
  char symbol_to_read;
  int fd;
  int file_length;
  int current_pos;

  /*  Writing data */
  if ((fd = open("temp_file", O_CREAT | O_WRONLY, 0666)) == -1) {
    perror("Error, can't create file");
    exit(1);
  }
  if (write(fd, string_to_write, strlen(string_to_write)) == -1) {
    perror("Error, can't write to file");
    exit(1);
  }
  if (close(fd) == -1) {
    perror("Error, can't close the file");
    exit(1);
  }
  
  /*  Reading data */
  if ((fd = open("temp_file", O_RDONLY, 0666)) == -1) {
    perror("Error, can't open file to read");
    exit(1);
  }
  file_length = lseek(fd, -1, SEEK_END);
  if (file_length == -1) {
    perror("Error, can't change pointer to read");
    exit(1);
  }
  for (current_pos = file_length; current_pos > -1; --current_pos) {
    read(fd, &symbol_to_read, 1);
    lseek(fd, -2, SEEK_CUR);
    printf("%c", symbol_to_read);
  }
  printf("\n");
  if (close(fd) == -1) {
    perror("Error, can't close reading file");
    exit(1);
  }

  printf("Successfully!\n");
  return 0;
}
