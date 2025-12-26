/* Jumping dot game for arduino uno r4 wifi with the led 12x8 matrix
  Includes A-Z chars and digits rendering, scrolling text, 4 scenes.
  Every loop there's a 1 ms delay which then corresponds nicely to 1 tick == 1 ms.
  Game is play-tested by family, this is generation 4 of the original concept.
  Game is hard.
  High score so far: 58.
*/


#include "Arduino_LED_Matrix.h"
#include <EEPROM.h>


#define DISPLAY_WIDTH 12
#define DISPLAY_HEIGHT 8
#define LETTER_WIDTH 3
#define LETTER_PADDING 1
#define LETTER_HEIGHT 5
#define INPUT_BUTTON 8
#define LETTER_COUNT 26
#define MUL 10000
#define MAX_SPEED 200

ArduinoLEDMatrix matrix;
static uint8_t display_frame[96];

static uint8_t letter_mat[LETTER_COUNT][15] = {
  {1,1,1, 1,0,1, 1,1,1, 1,0,1, 1,0,1}, // A
  {1,1,1, 1,0,1, 1,1,1, 1,0,1, 1,1,1}, // B
  {1,1,1, 1,0,0, 1,0,0, 1,0,0, 1,1,1}, // C
  {1,1,0, 1,0,1, 1,0,1, 1,0,1, 1,1,0}, // D
  {1,1,1, 1,0,0, 1,1,0, 1,0,0, 1,1,1}, // E
  {1,1,1, 1,0,0, 1,1,0, 1,0,0, 1,0,0}, // F
  {1,1,1, 1,0,0, 1,0,0, 1,0,1, 1,1,1}, // G
  {1,0,1, 1,0,1, 1,1,1, 1,0,1, 1,0,1}, // H
  {1,1,1, 0,1,0, 0,1,0, 0,1,0, 1,1,1}, // I
  {1,1,1, 0,0,1, 0,0,1, 1,0,1, 1,1,1}, // J
  {1,0,1, 1,0,1, 1,1,0, 1,0,1, 1,0,1}, // K
  {1,0,0, 1,0,0, 1,0,0, 1,0,0, 1,1,1}, // L
  {1,0,1, 1,1,1, 1,1,1, 1,0,1, 1,0,1}, // M
  {0,0,0, 1,1,0, 1,0,1, 1,0,1, 1,0,1}, // N
  {1,1,1, 1,0,1, 1,0,1, 1,0,1, 1,1,1}, // O
  {1,1,1, 1,0,1, 1,1,1, 1,0,0, 1,0,0}, // P
  {1,1,1, 1,0,1, 1,0,1, 1,1,1, 0,0,1}, // Q
  {1,1,1, 1,0,1, 1,1,1, 1,1,0, 1,0,1}, // R
  {1,1,1, 1,0,0, 1,1,1, 0,0,1, 1,1,1}, // S
  {1,1,1, 0,1,0, 0,1,0, 0,1,0, 0,1,0}, // T
  {1,0,1, 1,0,1, 1,0,1, 1,0,1, 1,1,1}, // U
  {1,0,1, 1,0,1, 1,0,1, 1,0,1, 0,1,0}, // v
  {1,0,1, 1,0,1, 1,1,1, 1,1,1, 1,1,1}, // W
  {1,0,1, 1,0,1, 0,1,0, 1,0,1, 1,0,1}, // X
  {1,0,1, 1,0,1, 1,1,1, 0,1,0, 0,1,0}, // Y
  {1,1,1, 0,0,1, 0,1,0, 1,0,0, 1,1,1}, // Z
};

// 10 digits of 3x5 digits
static uint8_t digit_mat[10][15] = {
  {0,1,0, 1,0,1, 1,0,1, 1,0,1, 0,1,0}, // 0
  {0,1,0, 1,1,0, 0,1,0, 0,1,0, 1,1,1}, // 1
  {1,1,1, 0,0,1, 0,1,0, 1,0,0, 1,1,1}, // 2
  {1,1,1, 0,0,1, 0,1,1, 0,0,1, 1,1,1}, // 3
  {1,0,0, 1,0,1, 1,1,1, 0,0,1, 0,0,1}, // 4
  {1,1,1, 1,0,0, 1,1,1, 0,0,1, 1,1,1}, // 5
  {1,1,1, 1,0,0, 1,1,1, 1,0,1, 1,1,1}, // 6
  {1,1,1, 1,0,1, 0,0,1, 0,0,1, 0,0,1}, // 7
  {1,1,1, 1,0,1, 1,1,1, 1,0,1, 1,1,1}, // 8
  {1,1,1, 1,0,1, 1,1,1, 0,0,1, 1,1,1}, // 9
};

#define OBSTACLE_SHAPE_COUNT 8
static uint8_t obstacle_shapes[OBSTACLE_SHAPE_COUNT][8] = {
  {0,0,0,0,1,1,1,1},
  {1,0,0,0,0,1,1,1},
  {1,0,0,0,0,1,1,1},
  {1,1,0,0,0,0,1,1},
  {1,1,0,0,0,0,1,1},
  {1,1,1,0,0,0,0,1},
  {1,1,1,0,0,0,0,1},
  {1,1,1,0,1,0,0,0},
};

typedef struct {
  int shape;
  int pos_x;
} s_obstacle;

typedef struct {
  int pos_x;
  int pos_y;
  int width;
  int height;
  int speed_x;
  int speed_y;
  int gravity;
} s_player;

typedef struct {
  char* text;
  int len;
  int pos_x;
  int pos_y;
} s_text_object;

typedef enum {
  HOME_SCREEN,
  GAME,
  GAME_OVER,
  SHOW_SCORE
} e_game_state;


void clear_buffer(uint8_t* buffer, uint8_t val)
{
  for (int i = 0; i < DISPLAY_HEIGHT * DISPLAY_WIDTH; i++)
  {
    buffer[i] = val;
  }
}


void draw_char(char c, int x, int y, uint8_t* buffer)
{
  uint8_t stopdraw_width = LETTER_WIDTH;
  uint8_t stopdraw_height = LETTER_HEIGHT;
  int startdraw_width = 0;
  int startdraw_height = 0;
  bool is_digit = (c >= '0' && c <= '9');

  if (x >= DISPLAY_WIDTH) return;
  if (x + LETTER_WIDTH <= 0) return;

  if (x < 0)
  {
    startdraw_width = 0 - x;
  } 
  else if (x + LETTER_WIDTH > DISPLAY_WIDTH)
  {
    stopdraw_width = DISPLAY_WIDTH - x;
  }

  if (y + LETTER_HEIGHT > DISPLAY_HEIGHT)
  {
    stopdraw_height = DISPLAY_HEIGHT - y;
    return;
  }

  char draw_char_idx = c - 'A';
  for (int i = 0; i < stopdraw_height; i++)
  {
    for (int j = startdraw_width; j < stopdraw_width; j++)
    {
      if (is_digit)
      {
        draw_char_idx = c - '0';
        buffer[((i+y) * DISPLAY_WIDTH) + x + j] = digit_mat[draw_char_idx][(i*LETTER_WIDTH) + j];
      }
      else
      {
        buffer[((i+y) * DISPLAY_WIDTH) + x + j] = letter_mat[draw_char_idx][(i*LETTER_WIDTH) + j];
      }
    }
  }
}

void draw_text_object(s_text_object* to, uint8_t* buffer)
{
  for (int i = 0; i < to->len; i++)
  {
    char c = to->text[i];
    if (c == 0) break;
    if (c == ' ') continue;
    else {
      if (c >= '0' && c <= '9')
      {
        draw_char(c, to->pos_x + (i*(LETTER_WIDTH + LETTER_PADDING)), to->pos_y, buffer);
      }
      else if (c >= 'A' && c <= 'Z')
      {
        draw_char(c, to->pos_x + (i*(LETTER_WIDTH + LETTER_PADDING)), to->pos_y, buffer);
      }
      else if (c >= 'a' && c <= 'z')
      {
        c -= 32;
        draw_char(c - 32, to->pos_x + (i*(LETTER_WIDTH + LETTER_PADDING)), to->pos_y, buffer);
      }
    }
  }
}


void init_player(s_player* p)
{
  p->height = 1;
  p->width = 2;
  p->speed_x = 0;
  p->speed_y = 0;
  p->pos_x = 0;
  p->pos_y = 0;
  p->gravity = 10;
}




static uint8_t x_ctr = 0;
static uint8_t letter_ctr = 0;
static uint8_t button_timeout = 0;
static bool button_pressed = false;

void draw_player(s_player* p, uint8_t* buffer)
{
  int x = (int) (p->pos_x / MUL);
  int y = (int) (p->pos_y / MUL);


  if (y == DISPLAY_HEIGHT)
  {
    y = DISPLAY_HEIGHT - 1;
  }

  //Serial.print("draw X: "); Serial.print(y); Serial.print("\n");

  for (int i = 0; i < p->height; i++)
  {
    for (int j = 0; j < p->width; j++)
    {
      buffer[((i+y) * DISPLAY_WIDTH) + x + j] = 1;
    }
  }
}

void draw_obstacle(s_obstacle* ob, uint8_t* buffer)
{
  if (ob->pos_x >= DISPLAY_WIDTH)
  {
    return;
  }
  if (ob->pos_x < 0)
  {
    return;
  }
  for (int i = 0; i < DISPLAY_HEIGHT; i++)
  {
    buffer[(i * DISPLAY_WIDTH) + ob->pos_x] = obstacle_shapes[ob->shape][i];
  }
}

bool check_player_hit_obstacle(s_player* p, s_obstacle* ob)
{
  int y = (int) (p->pos_y / MUL);


  if (y == DISPLAY_HEIGHT)
  {
    y = DISPLAY_HEIGHT - 1;
  }
  int obstacle_check = obstacle_shapes[ob->shape][y];

  return obstacle_check == 1;
}

static bool is_air_time = false;
static int air_time = 120; // ticks, so ms? 
static int air_time_ctr = 0;
static int speed_before_air_time = 0;

void process_player_movement(s_player* p, uint8_t* buffer)
{
  
  int speed_before = p->speed_y;
  //Serial.print("Speed: "); Serial.print(p->speed_y); Serial.print("\n");
  if (p->speed_y == 0 && !is_air_time)
  {
    is_air_time = true;
    air_time_ctr = 220;
  }

  if (is_air_time)
  {
    air_time_ctr--;
    if (air_time_ctr <= 0)
    {
      is_air_time = false;
    }
  }

  if (is_air_time)
  {
    p->speed_y = 0;
  }
  else {
    p->speed_y += p->gravity;
  }

  if (p->speed_y >= MAX_SPEED)
  {
    p->speed_y = MAX_SPEED;
  }


  int speed_after = p->speed_y;

  p->pos_y += p->speed_y;

  if (p->pos_y >= (DISPLAY_HEIGHT * MUL))
  {
    p->pos_y = DISPLAY_HEIGHT * MUL;
  }
  if (p->pos_y <= 0)
  {
    p->pos_y = 0;
  }
}

void reset_button()
{
  button_timeout = 0;
  button_pressed = false;
}

void check_button_press()
{
  bool pressed = digitalRead(INPUT_BUTTON) == LOW;
  if (button_timeout == 0)
  {
    button_pressed = false;
    if (pressed)
    {
      button_pressed = true;
      button_timeout = 20; // ticks so ms
    }
  }
  else {
    if (pressed)
    {
      button_timeout = 2;
      button_pressed = true;
    }
    else
    {
      button_timeout--;
    }
  }
}

int strlen(char* s)
{
  int len = 0;
  for(int i = 0;s[i] != 0;i++)
  {
    len++;
  }
  return len;
}
static char text_home_screen[] = "PRESS TO START";
static char text_game_over[] = "GAME OVER";
static char text_score[] = "SCORE";
static char text_score_buffer[5] = {0,0,0,0,0};


void fill_score_buffer(int score)
{
  int mod = score % 10;
  text_score_buffer[2] = mod + '0';
  mod = score / 10;
  text_score_buffer[1] = mod + '0';
  mod = score / 100;
  text_score_buffer[0] = mod + '0';
}

static s_obstacle ob1;
static s_player g_player;
static e_game_state g_game_state;
static s_text_object textob_home_screen;
static s_text_object textob_gameover;
static s_text_object textob_score;
static s_text_object textob_score_number;

static int game_score = 0;

static bool letter_set = false;
static bool action_taken = false;

static int obstacle_update_ctr = 80; // ms?
static int obstacle_update_speed = 80; // ms
static int text_scroll_ctr = 1000; // ms?
static int gameover_timeout_ctr = 2000;
static int general_counter_1 = 0;
static int general_counter_2 = 0;

void setup() {
  pinMode(INPUT_BUTTON, INPUT_PULLUP);
  Serial.begin(115200);
  matrix.begin();
  init_player(&g_player);
  ob1.pos_x = 14;
  ob1.shape = 0;
  g_game_state = HOME_SCREEN;
  button_pressed = false;
  general_counter_1 = 0;

  textob_home_screen.len = strlen(text_home_screen);
  textob_home_screen.text = text_home_screen;
  textob_home_screen.pos_x = 0;
  textob_home_screen.pos_y = 1;

  textob_gameover.len = strlen(text_game_over);
  textob_gameover.text = text_game_over;
  textob_gameover.pos_x = 0;
  textob_gameover.pos_y = 1;

  textob_score.len = strlen(text_score);
  textob_score.text = text_score;
  textob_score.pos_x = 0;
  textob_score.pos_y = 1;
}

void fill_framing(uint8_t* frame_buffer)
{
  for(int i = 0; i < DISPLAY_WIDTH; i++)
  {
    frame_buffer[i] = 1;
    frame_buffer[(DISPLAY_WIDTH * (DISPLAY_HEIGHT - 1)) + i] = 1;
  }
  for (int i = 0; i < DISPLAY_HEIGHT; i++)
  {
    frame_buffer[i * DISPLAY_WIDTH] = 1;
    frame_buffer[i * DISPLAY_WIDTH + (DISPLAY_WIDTH - 1)] = 1;
  }
}

void reinit_game()
{
  game_score = 0;
  init_player(&g_player);
  ob1.pos_x = 14;
  ob1.shape = random() % OBSTACLE_SHAPE_COUNT;
  g_game_state = HOME_SCREEN;
  button_pressed = false;
  textob_home_screen.pos_x = 0;
  textob_home_screen.pos_y = 1;
  textob_gameover.pos_x = 0;
  textob_gameover.pos_y = 1;
  textob_score.pos_x = 0;
  textob_score.pos_y = 1;
  
  letter_set = false;
  action_taken = false;

  obstacle_update_ctr = 80; // ms?
  obstacle_update_speed = 80;
  text_scroll_ctr = 1000;
  reset_button();
}

void loop() {

  switch (g_game_state) {
    /*----------------------------------------- HOME */
    case HOME_SCREEN:
      if (general_counter_1 > 0)
      {
        general_counter_1--;
      }
      else
      {
        check_button_press();
      }

      clear_buffer(display_frame, 0);

      if (button_pressed)
      {
        g_game_state = GAME;
        break;
      }

      if (text_scroll_ctr-- < 0)
      {
        text_scroll_ctr = 100;
        textob_home_screen.pos_x -= 1;
        if (textob_home_screen.pos_x + ((LETTER_WIDTH + LETTER_PADDING)*textob_home_screen.len) < 0)
        {
          textob_home_screen.pos_x = DISPLAY_WIDTH;
        }
      }

      draw_text_object(&textob_home_screen, display_frame);
      matrix.loadPixels(display_frame, 96);
      break;

    /*----------------------------------------- GAME */
    case GAME:
      check_button_press();

      clear_buffer(display_frame, 0);
      if (general_counter_2 > 0)
      {
        fill_framing(display_frame);
        general_counter_2--;
      }

      if (button_pressed && !action_taken)
      {
        g_player.speed_y = -750;
        is_air_time = false;
        action_taken = true;
      }
      else if (!button_pressed)
      {
        action_taken = false;
      }

      process_player_movement(&g_player, display_frame);
      draw_obstacle(&ob1, display_frame);
      draw_player(&g_player, display_frame);

      if (ob1.pos_x == g_player.width - 1)
      {
        bool hit = check_player_hit_obstacle(&g_player, &ob1);
        if (hit)
        {
          // PLAYER HIT WALL

          Serial.print("Score: "); Serial.print(game_score); Serial.print("\n");
          g_game_state = GAME_OVER;
          text_scroll_ctr = 500;
          clear_buffer(display_frame, 1);
          matrix.loadPixels(display_frame, 96);
          delay(100);
          reset_button();
          gameover_timeout_ctr = 1000;
          break;
        }
      }

      if (game_score < 10)
      {
        obstacle_update_speed = 80;
      }
      else if (game_score < 20)
      {
        obstacle_update_speed = 70;
      }
      else if (game_score < 30)
      {
        obstacle_update_speed = 65;
      }
      else if (game_score < 40)
      {
        obstacle_update_speed = 60;
      }
      else
      {
        obstacle_update_speed = 50;
      }

      if (obstacle_update_ctr <= 0)
      {
        obstacle_update_ctr = obstacle_update_speed;
        ob1.pos_x--;
        if (ob1.pos_x < 0)
        {
          game_score++;
          general_counter_2 = 50;
          fill_framing(display_frame);
          ob1.pos_x = 14;
          ob1.shape = random() % OBSTACLE_SHAPE_COUNT;
        }
      }
      else
      {
        obstacle_update_ctr --;
      }

      matrix.loadPixels(display_frame, 96);
      break;


    /*----------------------------------------- GAMEOVER */
    case GAME_OVER:
      if (gameover_timeout_ctr > 0)
      {
        gameover_timeout_ctr--;
      }
      else
      {
        check_button_press();
      }

      clear_buffer(display_frame, 0);

      if (button_pressed)
      {
        g_game_state = SHOW_SCORE;
        general_counter_1 = 500;
        reset_button();
        break;
      }

      if (text_scroll_ctr-- < 0)
      {
        text_scroll_ctr = 100;
        textob_gameover.pos_x -= 1;
        if (textob_gameover.pos_x + ((LETTER_WIDTH + LETTER_PADDING)*textob_gameover.len) < 0)
        {
          textob_gameover.pos_x = DISPLAY_WIDTH;
        }
      }

      draw_text_object(&textob_gameover, display_frame);
      matrix.loadPixels(display_frame, 96);
      break;

    case SHOW_SCORE:
      if (general_counter_1 > 0)
      {
        general_counter_1--;
      }
      else
      {
        check_button_press();
      }
      clear_buffer(display_frame, 0);
      fill_score_buffer(game_score);

      textob_score_number.len = strlen(text_score_buffer);
      textob_score_number.text = text_score_buffer;
      textob_score_number.pos_x = 0;
      textob_score_number.pos_y = 2;

      draw_text_object(&textob_score_number, display_frame);
      matrix.loadPixels(display_frame, 96);

      if (button_pressed)
      {
        general_counter_1 = 500;
        reinit_game();
        break;
      }


      break;
  }
  


  delay(1);
}
