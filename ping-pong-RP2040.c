#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "libs/ssd1306/ssd1306.h"
#include "libs/ssd1306/font.h"

// Screen dimensions
#define WIDTH 128
#define HEIGHT 64

// Paddle dimensions
#define PADDLE_WIDTH 4
#define PADDLE_HEIGHT 16
#define PADDLE_SPEED 2

// Ball parameters
#define BALL_SIZE 4
#define BALL_SPEED 2

// GPIO definitions
#define SDA_PIN 14
#define SCL_PIN 15
#define JOYSTICK_Y 27

// Display initialization
ssd1306_t disp;

// Game state structure
struct GameState {
    // Player paddle (left)
    int player_y;
    
    // AI paddle (right)
    int ai_y;
    int ai_dir;
    
    // Ball position and direction
    int ball_x;
    int ball_y;
    int ball_dx;
    int ball_dy;
    
    // Score
    int player_score;
    int ai_score;
};

void initialize_display() {
    i2c_init(i2c1, 400000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    ssd1306_init(&disp, WIDTH, HEIGHT, false, 0x3C, i2c1);
    ssd1306_config(&disp);
    ssd1306_fill(&disp, false);  // Use fill instead of clear
    ssd1306_send_data(&disp);     // Use send_data instead of show
}

void initialize_game(struct GameState* state) {
    state->player_y = (HEIGHT - PADDLE_HEIGHT) / 2;
    state->ai_y = (HEIGHT - PADDLE_HEIGHT) / 2;
    state->ball_x = WIDTH / 2;
    state->ball_y = HEIGHT / 2;
    state->ball_dx = -BALL_SPEED;
    state->ball_dy = BALL_SPEED;
    state->ai_dir = 1;
    state->player_score = 0;
    state->ai_score = 0;
}

void read_joystick(struct GameState* state) {
    adc_select_input(1); // Read from ADC channel 1 (GPIO27)
    uint16_t adc_value = adc_read();
    //state->player_y = (adc_value * (HEIGHT - PADDLE_HEIGHT)) / 4096;
    state->player_y = (HEIGHT - PADDLE_HEIGHT) - (adc_value * (HEIGHT - PADDLE_HEIGHT)) / 4096;

}

void update_ai(struct GameState* state) {
    // Centraliza a raquete da IA na posição da bola
    if (state->ai_y + PADDLE_HEIGHT / 2 < state->ball_y) {
        state->ai_y += PADDLE_SPEED;
    } else if (state->ai_y + PADDLE_HEIGHT / 2 > state->ball_y) {
        state->ai_y -= PADDLE_SPEED;
    }

    // Mantém a raquete dentro dos limites da tela
    if (state->ai_y < 0) state->ai_y = 0;
    if (state->ai_y > HEIGHT - PADDLE_HEIGHT) state->ai_y = HEIGHT - PADDLE_HEIGHT;
}

//rotina move a raquete da ia de maneira aleatória
/*
void update_ai(struct GameState* state) {
    // Simple AI with random direction changes
    if (rand() % 100 < 2) { // 2% chance to change direction
        state->ai_dir = (rand() % 2) * 2 - 1;
    }
    
    state->ai_y += state->ai_dir * PADDLE_SPEED;
    
    // Keep AI paddle within bounds
    if (state->ai_y < 0) state->ai_y = 0;
    if (state->ai_y > HEIGHT - PADDLE_HEIGHT) state->ai_y = HEIGHT - PADDLE_HEIGHT;
}
*/
void update_ball(struct GameState* state) {
    // Update ball position
    state->ball_x += state->ball_dx;
    state->ball_y += state->ball_dy;

    // Top/bottom collision
    if (state->ball_y <= 0 || state->ball_y >= HEIGHT - BALL_SIZE) {
        state->ball_dy *= -1;
    }

    // Player paddle collision
    if (state->ball_x <= PADDLE_WIDTH && 
        state->ball_y + BALL_SIZE >= state->player_y && 
        state->ball_y <= state->player_y + PADDLE_HEIGHT) {
        state->ball_dx *= -1;
    }

    // AI paddle collision
    if (state->ball_x >= WIDTH - PADDLE_WIDTH - BALL_SIZE && 
        state->ball_y + BALL_SIZE >= state->ai_y && 
        state->ball_y <= state->ai_y + PADDLE_HEIGHT) {
        state->ball_dx *= -1;
    }

    // Score detection
    if (state->ball_x < 0) {
        state->ai_score++;
        initialize_game(state);
    }
    if (state->ball_x > WIDTH) {
        state->player_score++;
        initialize_game(state);
    }
}

void draw_game(struct GameState* state) {
    ssd1306_fill(&disp, false);  // Clear screen
    
    // Draw paddles using rect (x, y, width, height, value, fill)
    ssd1306_rect(&disp, state->player_y, 0, PADDLE_WIDTH, PADDLE_HEIGHT, true, true);
    ssd1306_rect(&disp, state->ai_y, WIDTH - PADDLE_WIDTH, PADDLE_WIDTH, PADDLE_HEIGHT, true, true);
    
    // Draw bordas do campo
    ssd1306_rect(&disp, 0, 0, WIDTH, HEIGHT, true, false);

    // Desenha a linha tracejada vertical no centro
    for (int x = 0; x < WIDTH; x += 8) {
        ssd1306_rect(&disp, x, HEIGHT / 1 - 1, 3, 2, true, true); // Linha horizontal (para testar)
    }

    // Draw ball
    ssd1306_rect(&disp, state->ball_y, state->ball_x, BALL_SIZE, BALL_SIZE, true, true);
    
    // Draw scores
    char score_str[16];
    snprintf(score_str, sizeof(score_str), "%d - %d", state->player_score, state->ai_score);
    ssd1306_draw_string(&disp, score_str, WIDTH/2 - 16, 0);
    
    ssd1306_send_data(&disp);  // Update display
}

int main() {
    stdio_init_all();
    initialize_display();
    adc_init();
    adc_gpio_init(JOYSTICK_Y);
    
    struct GameState state;
    initialize_game(&state);
    
    while (1) {
        read_joystick(&state);
        update_ai(&state);
        update_ball(&state);
        draw_game(&state);
        sleep_ms(16); // ~60 FPS
    }
    
    return 0;
}