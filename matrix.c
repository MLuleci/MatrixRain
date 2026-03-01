#include <curses.h>
#include <locale.h>
#include <wchar.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define HEAD_PAIR 1
#define BODY_PAIR 2

#define FPS 10
#define MESSAGE "press 'q' to quit"
#define MESSAGE_LENGTH 17

#define MIN_HEIGHT(h) (h / 10)
#define MAX_HEIGHT(h) (h / 2)
#define MIN_SPEED 1
#define MAX_SPEED 3

typedef struct _line_t {
  wchar_t head;
  int x;
  int y; 
  int speed;
  int height;
} line_t;

static int g_screen_width = 0;
static int g_screen_height = 0;
static line_t* g_lines = NULL;
static wchar_t g_characters[0x42];

int random_range(int lo, int hi)
{
  int range = hi - lo;
  return (rand() % range) + lo;
}

wchar_t random_character()
{
  int index = random_range(0, wcslen(g_characters));
  return g_characters[index];
}

void new_line(line_t* line)
{
  if (line == NULL) return;
  
  line->y = 0;
  line->speed = random_range(MIN_SPEED, MAX_SPEED);
  line->height = random_range(MIN_HEIGHT(g_screen_height), MAX_HEIGHT(g_screen_height));
  line->head = random_character();
}

bool is_visible(line_t* line)
{
  return (line != NULL) && (line->y - line->height < g_screen_height);
}

void update_line(line_t* line)
{
  if (!is_visible(line)) {
    return new_line(line);
  }
  int screen_height = g_screen_height - 1;
  
  // Overwrite old head and draw the new body characters:
  attron(COLOR_PAIR(BODY_PAIR));
  for (int i = 0; i < line->speed; ++i)
  {
    int y = line->y + i;
    if (y < 0 || y >= screen_height) continue;
    mvprintw(y, line->x, "%lc", line->head);
    line->head = random_character();
  }
  attroff(COLOR_PAIR(BODY_PAIR));
  
  // Overwrite the tail characters:
  for (int i = 0; i < line->speed; ++i)
  {
    int y = line->y - line->height - i;
    if (y < 0 || y >= screen_height) continue;
    mvprintw(y, line->x, " ");
  }
  
  // Advance the head:
  line->y += line->speed;
  
  // Draw the head character:
  if (line->y >= screen_height) return;
  attron(COLOR_PAIR(HEAD_PAIR));
  mvprintw(line->y, line->x, "%lc", line->head);
  attroff(COLOR_PAIR(HEAD_PAIR));
}

void die(const char* message)
{
  fputs(message, stderr);
  free(g_lines);
  endwin();
  exit(EXIT_FAILURE);
}

void on_resized()
{
  int previous_width = g_screen_width;
  getmaxyx(stdscr, g_screen_height, g_screen_width);
  clear();
  
  // (Re-)initialize lines:
  line_t* p = (line_t*)realloc(g_lines, g_screen_width * sizeof(line_t));
  if (p == NULL) die("could not allocate memory");
  
  g_lines = p;
  for (int i = previous_width; i < g_screen_width; ++i)
  {
    line_t* line = &g_lines[i];
    line->x = i;
    new_line(line);
  }
}

void setup()
{
  setlocale(LC_ALL, "C.UTF-8");
  initscr();
  
  if (has_colors() == FALSE) {
    endwin();
    fprintf(stderr, "Terminal does not support colour.\n");
    exit(EXIT_FAILURE);
  }
  start_color();
  
  init_pair(HEAD_PAIR, COLOR_WHITE, COLOR_BLACK);
  init_pair(BODY_PAIR, COLOR_GREEN, COLOR_BLACK);
  
  srand(time(NULL));
  timeout(1000 / FPS); // Set input timeout, effectively limiting FPS
  curs_set(0); // Make cursor invisible
  raw(); // Disable line buffering, and also intercept signals
  noecho(); // Don't print typed characters
  
  // Digits 0-9 followed by Katakana unicode block 0xFF66-0xFF9D:
  for (int i = 0; i < 10; ++i)
  {
    g_characters[i] = (wchar_t)('0' + i);
  }
  for (int i = 0; i < 0x37; ++i)
  {
    g_characters[i + 10] = (wchar_t)(0xFF66 + i);
  }
  g_characters[0x41] = (wchar_t)'\0';
}

int main(int argc, char **argv)
{
  setup();
  on_resized(); // Initialize screen
  
  char time_buffer[32];
  bool is_running = true;
  while (is_running)
  {
    switch (getch())
    {
      case 'q':
      is_running = false;
      break;
      case KEY_RESIZE:
      on_resized();
      break;
    }
    
    for (int i = 0; i < g_screen_width; ++i)
    {
      update_line(&g_lines[i]);
    }
    
    // Draw status line:
    time_t t = time(NULL);
    struct tm *lt = localtime(&t);
    if (lt == NULL) {
      die("could not get local time");
    }
    
    if (strftime(time_buffer, sizeof(time_buffer), "%F %r", lt) == 0) {
      die("could not format time");
    }
    
    mvprintw(g_screen_height - 1, 0, "%s", time_buffer);
    mvprintw(g_screen_height - 1, g_screen_width - MESSAGE_LENGTH, MESSAGE);
    
    refresh();
  }
  
  free(g_lines);
  endwin();
  return EXIT_SUCCESS;
}