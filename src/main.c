#include <termios.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <stdlib.h>
#include <curses.h>
#include <ctype.h>
#include <ncurses/form.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define TEXT_WINDOW_HEIGHT 21
#define TEXT_WINDOW_WIDTH 80
#define MENU_WINDOW_HEIGHT 3
#define MENU_WINDOW_WIDTH 80

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

int OpenFile(WINDOW *text_window, FORM *text_form)
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

    form_driver(text_form, REQ_CLR_FIELD);
    while ((bytes_read = read(fd, internal_data, 30000)) > 0) {
      mvwprintw(text_window, current_y, current_x, "%s", internal_data);
      current_x = (current_x + bytes_read) % 16;
      current_y = (current_y + bytes_read) / 16;
    }
    if (bytes_read == -1) { 
      error_found = 1;
    }
    else {

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

int main(void)
{
  /*  Data */
  WINDOW *text_window,
         *command_field,
         *text_subwindow;
  FORM *text_form;
  FIELD *text_fields[2];
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

  text_window = newwin(TEXT_WINDOW_HEIGHT, TEXT_WINDOW_WIDTH, 0, 0);
  text_subwindow = subwin(text_window, TEXT_WINDOW_HEIGHT - 2,
                          TEXT_WINDOW_WIDTH - 2, 1, 1);
  keypad(text_subwindow, 1);

  text_fields[0] = new_field(TEXT_WINDOW_HEIGHT - 2, TEXT_WINDOW_WIDTH - 2,
                             0, 0, 0, 0);
  text_fields[1] = NULL;
  field_opts_off(text_fields[0], O_STATIC);
  text_form = new_form(text_fields);
  
  set_form_win(text_form, text_window);
  set_form_sub(text_form, text_subwindow);
  
  box(text_window, 0, 0);
  wrefresh(text_window);
  refresh();

  post_form(text_form);
  wrefresh(text_window);

  command_field = CreateMenu();

  refresh();

  wmove(text_window, 1, 1);
  while (1) {
    symbol = wgetch(text_subwindow); 
    if (isalpha(symbol)) {
      form_driver(text_form, symbol);
      getyx(text_window, current_y, current_x);
      
    } else {
      switch (symbol) {
        case KEY_F(2):
          if (OpenFile(text_subwindow, text_form) != -1)
            form_driver(text_form, REQ_BEG_FIELD);
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
          form_driver(text_form, REQ_PREV_CHAR);
            --current_x;
          break;
        case KEY_RIGHT:
          if (current_x == last_x) {
            
          }
          form_driver(text_form, REQ_NEXT_CHAR);
            ++current_x;
          break;
        case KEY_UP:
          form_driver(text_form, REQ_PREV_LINE);
          /* if (current_y != minY)
            --current_y; */
          break;
        case KEY_DOWN:
          form_driver(text_form, REQ_NEXT_LINE);
          /* if (current_y != maxY - 1)
            ++current_y; */
          break;
        case KEY_BACKSPACE:
          form_driver(text_form, REQ_DEL_PREV);
          break;
        case '\n':
          form_driver(text_form, REQ_NEW_LINE);
          ++current_y;
          current_x = 0;
          break;
        default:
          form_driver(text_form, symbol);
      }
    }
    if (need_exit)
      break;
  }

  unpost_form(text_form);
  free_form(text_form);
  free_field(text_fields[0]);
  delwin(command_field);

  endwin();

  return 0;
}
