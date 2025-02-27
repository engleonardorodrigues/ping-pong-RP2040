#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "libs/ssd1306/ssd1306.h"
#include "libs/ssd1306/font.h"

// Dimensões da tela
#define LARGURA 128
#define ALTURA 64

// Dimensões da raquete
#define LARGURA_RAQUETE 4
#define ALTURA_RAQUETE 16
#define VELOCIDADE_RAQUETE 2

// Parâmetros da bola
#define TAMANHO_BOLA 4
#define VELOCIDADE_BOLA 2

// Definições de GPIO
#define PINO_SDA 14
#define PINO_SCL 15
#define JOYSTICK_Y 27

// Inicialização do display
ssd1306_t disp;

// Estrutura do estado do jogo
struct EstadoJogo {
    // Raquete do jogador (esquerda)
    int jogador_y;
    
    // Raquete da IA (direita)
    int ia_y;
    int direcao_ia;
    
    // Posição e direção da bola
    int bola_x;
    int bola_y;
    int bola_dx;
    int bola_dy;
    
    // Pontuação
    int pontuacao_jogador;
    int pontuacao_ia;
};

void inicializar_display() {
    i2c_init(i2c1, 400000);
    gpio_set_function(PINO_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PINO_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PINO_SDA);
    gpio_pull_up(PINO_SCL);

    ssd1306_init(&disp, LARGURA, ALTURA, false, 0x3C, i2c1);
    ssd1306_config(&disp);
    ssd1306_fill(&disp, false);
    ssd1306_send_data(&disp);
}

void inicializar_jogo(struct EstadoJogo* estado) {
    estado->jogador_y = (ALTURA - ALTURA_RAQUETE) / 2;
    estado->ia_y = (ALTURA - ALTURA_RAQUETE) / 2;
    estado->bola_x = LARGURA / 2;
    estado->bola_y = ALTURA / 2;
    estado->bola_dx = -VELOCIDADE_BOLA;
    estado->bola_dy = VELOCIDADE_BOLA;
    estado->direcao_ia = 1;
    estado->pontuacao_jogador = 0;
    estado->pontuacao_ia = 0;
}

void ler_joystick(struct EstadoJogo* estado) {
    adc_select_input(1);
    uint16_t valor_adc = adc_read();
    estado->jogador_y = (ALTURA - ALTURA_RAQUETE) - (valor_adc * (ALTURA - ALTURA_RAQUETE)) / 4096;
}

void atualizar_ia(struct EstadoJogo* estado) {
    if (estado->ia_y + ALTURA_RAQUETE / 2 < estado->bola_y) {
        estado->ia_y += VELOCIDADE_RAQUETE;
    } else if (estado->ia_y + ALTURA_RAQUETE / 2 > estado->bola_y) {
        estado->ia_y -= VELOCIDADE_RAQUETE;
    }

    if (estado->ia_y < 0) estado->ia_y = 0;
    if (estado->ia_y > ALTURA - ALTURA_RAQUETE) estado->ia_y = ALTURA - ALTURA_RAQUETE;
}

void atualizar_bola(struct EstadoJogo* estado) {
    estado->bola_x += estado->bola_dx;
    estado->bola_y += estado->bola_dy;

    if (estado->bola_y <= 0 || estado->bola_y >= ALTURA - TAMANHO_BOLA) {
        estado->bola_dy *= -1;
    }

    // Colisão com a raquete do jogador
    if (estado->bola_x <= LARGURA_RAQUETE && 
        estado->bola_y + TAMANHO_BOLA >= estado->jogador_y && 
        estado->bola_y <= estado->jogador_y + ALTURA_RAQUETE) {
        estado->bola_dx *= -1;
    }

    // Colisão com a raquete da IA
    if (estado->bola_x >= LARGURA - LARGURA_RAQUETE - TAMANHO_BOLA && 
        estado->bola_y + TAMANHO_BOLA >= estado->ia_y && 
        estado->bola_y <= estado->ia_y + ALTURA_RAQUETE) {
        estado->bola_dx *= -1;
    }

    // Verificação de pontuação (mantém os pontos)
    if (estado->bola_x < 0) {
        estado->pontuacao_ia++;
        // Reinicia apenas a posição da bola e raquetes
        estado->jogador_y = (ALTURA - ALTURA_RAQUETE) / 2;
        estado->ia_y = (ALTURA - ALTURA_RAQUETE) / 2;
        estado->bola_x = LARGURA / 2;
        estado->bola_y = ALTURA / 2;
        estado->bola_dx = -VELOCIDADE_BOLA;
        estado->bola_dy = VELOCIDADE_BOLA;
    }
    if (estado->bola_x > LARGURA) {
        estado->pontuacao_jogador++;
        // Reinicia apenas a posição da bola e raquetes
        estado->jogador_y = (ALTURA - ALTURA_RAQUETE) / 2;
        estado->ia_y = (ALTURA - ALTURA_RAQUETE) / 2;
        estado->bola_x = LARGURA / 2;
        estado->bola_y = ALTURA / 2;
        estado->bola_dx = VELOCIDADE_BOLA;
        estado->bola_dy = VELOCIDADE_BOLA;
    }
}

void desenhar_tela_inicial() {
    ssd1306_fill(&disp, false);
    
    // Primeira linha: "Embarcatech" (centralizado vertical e horizontal)
    ssd1306_draw_string(&disp, "EMBARCATECH", (LARGURA/2) - 44, (ALTURA/2) - 10);
    
    // Segunda linha: "Game" (centralizado abaixo da primeira linha)
    ssd1306_draw_string(&disp, "GAME", (LARGURA/2) - 16, (ALTURA/2) + 2);
    
    ssd1306_send_data(&disp);
}

void desenhar_jogo(struct EstadoJogo* estado) {
    ssd1306_fill(&disp, false);
    
    // Desenha raquetes
    ssd1306_rect(&disp, estado->jogador_y, 0, LARGURA_RAQUETE, ALTURA_RAQUETE, true, true);
    ssd1306_rect(&disp, estado->ia_y, LARGURA - LARGURA_RAQUETE, LARGURA_RAQUETE, ALTURA_RAQUETE, true, true);
    
    // Desenha bordas do campo
    ssd1306_rect(&disp, 0, 0, LARGURA, ALTURA, true, false);

    // Linha central tracejada
    for (int x = 0; x < LARGURA; x += 8) {
        ssd1306_rect(&disp, x, ALTURA / 1 - 1, 3, 2, true, true);
    }

    // Desenha bola
    ssd1306_rect(&disp, estado->bola_y, estado->bola_x, TAMANHO_BOLA, TAMANHO_BOLA, true, true);
    
    // Desenha placar centralizado e mais abaixo
    char placar[16];
    snprintf(placar, sizeof(placar), "%d - %d", estado->pontuacao_jogador, estado->pontuacao_ia);
    
    // Ajuste na posição Y do placar (5 pixels abaixo do topo)
    ssd1306_draw_string(&disp, placar, LARGURA/2 - 16, 5);
    
    ssd1306_send_data(&disp);
}

int main() {
    stdio_init_all();
    inicializar_display();
    adc_init();
    adc_gpio_init(JOYSTICK_Y);

    // Mostra tela inicial por 2 segundos
    desenhar_tela_inicial();
    sleep_ms(5000);
    
    struct EstadoJogo estado;
    inicializar_jogo(&estado);
    
    while (1) {
        ler_joystick(&estado);
        atualizar_ia(&estado);
        atualizar_bola(&estado);
        desenhar_jogo(&estado);
        sleep_ms(16);
    }
    
    return 0;
}