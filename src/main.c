#include <termios.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <stdlib.h>
#include <curses.h>
#include <ctype.h>

#include <netinet/in.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "list.h"

#define TEXT_WINDOW_HEIGHT 21
#define TEXT_WINDOW_WIDTH 80
#define MENU_WINDOW_HEIGHT 3
#define MENU_WINDOW_WIDTH 80

enum Direction {
  UP,
  DOWN,
  RIGHT,
  LEFT
};

static char file_name[100];
struct List *lines;

void SigWinch(int signo)
{
  struct winsize size;
  ioctl(stdout, TIOCGWINSZ, (char *)&size);
  resizeterm(size.ws_row, size.ws_col);
  refresh();
}

void initialize(void)
{
  initscr();
  signal(SIGWINCH, SigWinch);
  cbreak();
  noecho();
  curs_set(1);
  keypad(stdscr, 1);
  refresh();

  lines = CreateList();
}

WINDOW *CreateWindow(int h, int w, int start_y, int start_x)
{
  WINDOW *local_window = newwin(h, w, start_y, start_x);
  box(local_window, 0, 0);
  wrefresh(local_window);

  return local_window;
}

WINDOW *CreateMenu(void)
{
  WINDOW *menu = CreateWindow(MENU_WINDOW_HEIGHT, MENU_WINDOW_WIDTH,
                              TEXT_WINDOW_HEIGHT, 0);
  mvwprintw(menu, 1, 1, "F2)Open F3)Save F4)Find F6)Quit");
  wrefresh(menu);
  return menu;
}

int OpenFile(WINDOW *text_window)
{
  WINDOW *open_window = NULL;
  WINDOW *open_subwindow = NULL;
  int ch = 0;
  int length = 0;
  int fd = 0;
  int current_x = 0,
      current_y = 0;
  int error_found = 0;

  /*  Creating the window*/
  open_window = newwin(10, 30, 6, 15);
  open_subwindow = derwin(open_window, 8, 28, 1, 1);
  keypad(open_subwindow, 1);
  box(open_window, 0, 0);

  wrefresh(open_window);
  wrefresh(open_subwindow);

  /*  Logic */
  mvwprintw(open_subwindow, 0, 0, "Please, write filename");
  wmove(open_subwindow, 1, 0);
  while (1) {
    ch = wgetch(open_subwindow);
    if (ch  == '\n') {
      break;
    } else if (isalpha(ch)) {
      waddch(open_subwindow, ch);
      file_name[length++] = ch;
    } else if (ch == KEY_BACKSPACE) {
      --length;
      mvwdelch(open_subwindow, 1, length);
    }
  }
  file_name[length] = '\0';
  if ((fd = open(file_name, O_RDWR, 0666)) == -1) {
    mvwprintw(open_subwindow, 0, 0, "Can't open the file!!!");
    mvwprintw(open_subwindow, 1, 0, "Press any key to continue");
    curs_set(0);
    error_found = 1;
    file_name[0] = '\0';
  }
  else {
    char internal_data[30000]; /*  30 Kb */
    int bytes_read;

    wclear(text_window);
    while ((bytes_read = read(fd, internal_data, 30000)) > 0) {
      mvwprintw(text_window, current_y, current_x, "%s", internal_data);
      current_x = (current_x + bytes_read) % 16;
      current_y = (current_y + bytes_read) / 16;
    }
    if (bytes_read == -1) { 
      error_found = 1;
    }
    close(fd);
  }
  
  if (error_found) {
    wgetch(open_subwindow);
    /* wmove(text_window, 0, 0);
    curs_set(1); */
  }
  delwin(open_subwindow);
  delwin(open_window);

  return -error_found;
}

int SaveFile(WINDOW *text_window, int max_y)
{
  WINDOW *save_window,
         *save_subwindow;
  char line[79];
  char file_name[50];
  char ch;
  int current_pos = 0;
  int current_line = 0;
  int fd = 0;
  int length = 0;
  int error_found = 0;
  int current_x,
      current_y;

  getyx(text_window, current_y, current_x);

  save_window = newwin(10, 30, 6, 15);
  save_subwindow = derwin(save_window, 8, 28, 1, 1);
  keypad(save_subwindow, 1);
  box(save_window, 0, 0);

  wrefresh(save_window);
  wrefresh(save_subwindow);

  /*  Logic */
  mvwprintw(save_subwindow, 0, 0, "Please, write filename");
  wmove(save_subwindow, 1, 0);
  while (1) {
    ch = wgetch(save_subwindow);
    if (ch == '\n') {
      break;
    } else if (isalpha(ch)) {
      waddch(save_subwindow, ch);
      file_name[length++] = ch;
    } else if (ch == '\a') {
      /*  There is have to be (ch == KEY_BACKSPACE), but
       *  it's now working and I don't know why. It works
       *  in OpenFile, but not here*/
      --length;
      mvwdelch(save_subwindow, 1, length);
    }
  }
  file_name[length] = '\0';
  if ((fd = open(file_name, O_CREAT | O_EXCL | O_WRONLY, 0666)) == -1) {
    if ((fd = open(file_name, O_WRONLY, 0666)) == -1) { 
      mvwprintw(save_subwindow, 0, 0, "Can't open the file!");
      mvwprintw(save_subwindow, 1, 0, "Press any key to continue");
      curs_set(0);
      error_found = 1;
      file_name[0] = '\0';
    }
  }
  if (!error_found) {
    for (current_line = 0; current_line < max_y; ++current_line) {
      mvwinnstr(text_window, current_line, 0, line, 79);
      if (write(fd, line, 79) == -1) { 
        mvwprintw(save_subwindow, 0, 0, "Can't write into the file!");
        mvwprintw(save_subwindow, 1, 0, "Press any key to continue");
        curs_set(0);
        error_found = 1;
      }
    }

    close(fd);
  }
  wmove(text_window, current_y, current_x);
  if (error_found) {
    wgetch(save_subwindow);
    curs_set(1);
  }
  delwin(save_subwindow);
  delwin(save_window);

  return -error_found; 
}

void GetNextVerticalPos(WINDOW *text_window, enum Direction dir, int *y, int *x,
                       int delta)
{
  char next_line[TEXT_WINDOW_WIDTH - 1];
  int key_pos = 0;
  int current_symbol_pos = 0;
  int current_x = *x;
  int current_y = *y;

  wmove(text_window, current_y + delta, current_x);
  winnstr(text_window, next_line, TEXT_WINDOW_WIDTH - 1); 
  wmove(text_window, current_y - delta, current_x);
  for (current_symbol_pos = TEXT_WINDOW_WIDTH - 2;
      current_symbol_pos > -1; --current_symbol_pos) {
    if (next_line[current_symbol_pos] != '\0' &&
        next_line[current_symbol_pos] != '\n') {
      key_pos = current_symbol_pos;
      break;
    }
  }
  if (key_pos < current_x)
    current_x = key_pos;

  *y = current_y + delta;
  *x = current_x;
}

void GetNextSymbol(WINDOW *text_window, enum Direction dir, int *y, int *x)
{
  int current_x,
      current_y;
  int delta = 0;

  getyx(text_window, current_y, current_x);
  switch (dir) {
    case UP:
    case DOWN:
      if (dir == UP)
        delta = -1;
      else if (dir == DOWN)
        delta = 1;
      GetNextVerticalPos(text_window, dir, &current_y, &current_x, delta);
      break;
    case LEFT:
      if (current_x == 0) {
        current_x = TEXT_WINDOW_WIDTH - 2;
        --current_y;
      } else {
        --current_x;
      }
      break;
    case RIGHT:
      if (current_x == TEXT_WINDOW_WIDTH - 1) {
        current_x = 0;
        ++current_y;
      } else {
        ++current_x;
      }
      break;
  }
  *y = current_y;
  *x = current_x;
}

void RemoveChar(WINDOW *text_window)
{
  int current_x,
      current_y;

  getyx(text_window, current_y, current_x);
  if (current_x != 0)
    mvwdelch(text_window, current_y, current_x - 1);
  else if (current_y != 0) {
    mvwdelch(text_window, current_y - 1, 78); 
  }
}

int main(void)
{
  /*  Data */
  WINDOW *text_window,
         *command_field,
         *text_subwindow;
  unsigned int symbol;
  int current_x,
      current_y;
  int need_exit = 0;
  int last_y,
      last_x;
  
  initialize();
  current_x = 0;
  current_y = 0;
  last_y = 1;

  /* ====================================== */
  /*                                        */
  /*  Creating all windows                  */
  /*                                        */
  text_window = newwin(TEXT_WINDOW_HEIGHT, TEXT_WINDOW_WIDTH, 0, 0);
  /* text_subwindow = subwin(text_window, 10,
                          10, 1, 1); */
  text_subwindow = subwin(text_window, TEXT_WINDOW_HEIGHT - 2,
                          TEXT_WINDOW_WIDTH - 2, 1, 1);
  keypad(text_subwindow, 1);
  scrollok(text_subwindow, 1);
  idlok(text_subwindow, 1);

  box(text_window, 0, 0);
  wrefresh(text_window);
  command_field = CreateMenu();
  refresh();

  /* ====================================== */
  /*                                        */
  /*  Logic                                 */
  /*                                        */
  wmove(text_subwindow, 0, 0);
  while (1) {
    symbol = wgetch(text_subwindow); 
    if (isalpha(symbol)) {
      waddch(text_subwindow, symbol);
      getyx(text_subwindow, current_y, current_x);
      if (current_x == 0)
        ++last_y;
    } else {
      switch (symbol) {
        case KEY_F(2):
          if (OpenFile(text_subwindow) != -1)
            wmove(text_subwindow, 0, 0);
          redrawwin(text_subwindow);
          wmove(text_subwindow, 0, 0);
          curs_set(1);
          break;
        case KEY_F(3):
          if (SaveFile(text_subwindow, last_y) != -1) {
            redrawwin(text_subwindow);
          }
          break;
        case KEY_F(4):
          /*  TODO: find */
          break;
        case KEY_F(6):
          /*  Exit */
          need_exit = 1;
          break;
        case KEY_LEFT:
          GetNextSymbol(text_subwindow, LEFT, &current_y, &current_x);
          wmove(text_subwindow, current_y, current_x);
          break;
        case KEY_RIGHT:
          GetNextSymbol(text_subwindow, RIGHT, &current_y, &current_x);
          wmove(text_subwindow, current_y, current_x);
          break;
        case KEY_UP:
          GetNextSymbol(text_subwindow, UP, &current_y, &current_x);
          wmove(text_subwindow, current_y, current_x);
          break;
        case KEY_DOWN:
          GetNextSymbol(text_subwindow, DOWN, &current_y, &current_x);
          wmove(text_subwindow, current_y, current_x);
          break;
        case KEY_BACKSPACE:
          RemoveChar(text_subwindow);
          break;
        case '\n':
          ++current_y;
          ++last_y;
          current_x = 0;
        default:
          waddch(text_subwindow, symbol);
      }
    }
    if (need_exit)
      break;
  }

  delwin(command_field);
  delwin(text_subwindow);
  delwin(text_window);
  RemoveList(lines);
  endwin();

  return 0;
}
