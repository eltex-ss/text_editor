#include <termios.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <stdlib.h>
#include <curses.h>
#include <ctype.h>

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

void ClearWindow(WINDOW *text_window)
{
  int max_x,
      max_y,
      current_pos;
  char empty_line[79];

  getmaxyx(text_window, max_y, max_x);
  for (current_pos = max_y; current_pos > -1; --current_pos)
    wprintw(text_window, "%s", empty_line);
  wmove(text_window, 0, 0);
}

int OpenFile(WINDOW *text_window)
{
  WINDOW *open_window;
  int ch;
  int length = 0;
  int fd;
  int current_x = 0,
      current_y = 0;
  int error_found = 0;

  /*  Creating the window*/
  open_window = newwin(10, 30, 6, 15);
  box(open_window, 0, 0);

  wrefresh(open_window);

  /*  Logic */
  mvwprintw(open_window, 1, 1, "Please, write filename");
  wmove(open_window, 2, 1);
  while ((ch = wgetch(open_window)) != '\n') {
    waddch(open_window, ch);
    file_name[length++] = ch;
  }
  file_name[length] = '\0';
  if ((fd = open(file_name, O_RDWR, 0666)) == -1) {
    mvwprintw(open_window, 1, 1, "Can't open the file!!!");
    mvwprintw(open_window, 2, 1, "Press any key to continue");
    curs_set(0);
    error_found = 1;
    file_name[0] = '\0';
  }
  else {
    char internal_data[30000]; /*  30 Kb */
    int bytes_read;

    ClearWindow(text_window);
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
    wgetch(open_window);
    curs_set(1);
  }
  delwin(open_window); 

  return -error_found;
}
/*
void SaveFile(WINDOW *text_window)
{
  char line[79];
  
  
}
*/

void GetNewVerticalPos(WINDOW *text_window, enum Direction dir, int *y, int *x,
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
  if (key_pos > current_x)
    current_x = key_pos;

  *y = current_y;
  *x = current_x;
}

void GetSymbol(WINDOW *text_window, enum Direction dir, int *y, int *x)
{
  int current_x = *x;
  int current_y = *y;
  int delta = 0;

  if (dir == UP)
    delta = -1;
  else if (dir == DOWN)
    delta = 1;

  switch (dir) {
    case UP:
    case DOWN:
      GetNewVerticalPos(text_window, dir, &current_y, &current_x, delta);
      break;
    case LEFT:
      if (current_x == 0) {
        current_x = TEXT_WINDOW_WIDTH - 2;
        --current_y;
      }
      break;
    case RIGHT:
      if (current_x == TEXT_WINDOW_WIDTH - 1) {
        current_x = 0;
        ++current_y;
      }
      break;
  }
  *y = current_y;
  *x = current_x;
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
  last_y = 0;

  /* ====================================== */
  /*                                        */
  /*  Creating all windows                  */
  /*                                        */
  text_window = newwin(TEXT_WINDOW_HEIGHT, TEXT_WINDOW_WIDTH, 0, 0);
  text_subwindow = subwin(text_window, TEXT_WINDOW_HEIGHT - 2,
                          TEXT_WINDOW_WIDTH - 2, 1, 1);
  keypad(text_subwindow, 1);
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
    int key_pos = 0;
    
    symbol = wgetch(text_subwindow); 
    if (isalpha(symbol)) {
      waddch(text_subwindow, symbol);
    } else {
      switch (symbol) {
        case KEY_F(2):
          if (OpenFile(text_subwindow) != -1)
            wmove(text_subwindow, 0, 0);
          wrefresh(text_window);
          break;
        case KEY_F(3):
          /*  TODO: save = create subwin, read filename, write and close */
          break;
        case KEY_F(4):
          /*  TODO: find */
          break;
        case KEY_F(6):
          /*  Exit */
          need_exit = 1;
          break;
        case KEY_LEFT:
          
          break;
        case KEY_RIGHT:
          break;
        case KEY_UP:
          break;
        case KEY_DOWN:
          break;
        case KEY_BACKSPACE:
          /* form_driver(text_form, REQ_DEL_PREV); */
          break;
        case '\n':
          /* form_driver(text_form, REQ_NEW_LINE); */
          ++current_y;
          current_x = 0;
          break;
        default:
          waddch(text_window, symbol);
          /* form_driver(text_form, symbol); */
      }
    }
    if (need_exit)
      break;
  }

  delwin(command_field);
  delwin(text_subwindow);
  delwin(text_window);
  endwin();

  return 0;
}
