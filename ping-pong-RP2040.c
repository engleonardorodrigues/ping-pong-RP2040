/*****************************************************************************************************
 * Título        : Jogo de Ping Pong para Raspberry Pi Pico W
 * Desenvolvedor : Leonardo Rodrigues
 * Versão        : 1.0.0
 * 
 * Descrição:
 * 
 * Este programa implementa um jogo de Ping Pong interativo utilizando a placa Raspberry Pi Pico W, um 
 * display OLED SSD1306 e um joystick analógico. O jogador controla uma raquete enquanto a 
 * raquete adversária é controlada por uma Inteligência Artificial. O objetivo do jogo é rebater a bola e marcar pontos, 
 * com dificuldade progressiva ao longo da partida.
 * 
 * Materiais utilizados:
 * 
 * 1 - Raspberry Pi Pico W (BitDogLab)
 * 1 - Display OLED SSD1306 (128x64, comunicação I²C)
 * 1 - Joystick analógico
  ******************************************************************************************************/


#include <stdio.h>                                                                                              // Biblioteca padrão de entrada/saída
#include <stdlib.h>                                                                                             // Biblioteca padrão de funções gerais
#include <string.h>                                                                                             // Biblioteca para manipulação de strings
#include "pico/stdlib.h"                                                                                        // Biblioteca específica do Raspberry Pi Pico
#include "hardware/i2c.h"                                                                                       // Biblioteca para comunicação I2C
#include "hardware/adc.h"                                                                                       // Biblioteca para conversão analógica-digital
#include "libs/ssd1306/ssd1306.h"                                                                               // Biblioteca do display SSD1306
#include "libs/ssd1306/font.h"                                                                                  // Biblioteca de fontes para o display

// Dimensões da tela
#define LARGURA 128                                                                                             // Largura total do display em pixels
#define ALTURA 64                                                                                               // Altura total do display em pixels

// Dimensões da raquete
#define LARGURA_RAQUETE 4                                                                                       // Largura horizontal da raquete
#define ALTURA_RAQUETE 16                                                                                       // Altura vertical da raquete  
#define VELOCIDADE_RAQUETE 2                                                                                    // Velocidade de movimento das raquetes

// Parâmetros da bola
#define TAMANHO_BOLA 4                                                                                          // Tamanho do lado do quadrado da bola
#define VELOCIDADE_BOLA 2                                                                                       // Velocidade base de movimento da bola

// Definições de GPIO
#define PINO_SDA 14                                                                                             // Pino GPIO para SDA (dados I2C)
#define PINO_SCL 15                                                                                             // Pino GPIO para SCL (clock I2C)
#define JOYSTICK_Y 27                                                                                           // Pino ADC para eixo Y do joystick

// Inicialização do display
ssd1306_t disp;                                                                                                 // Estrutura de controle do display OLED

// Estrutura do estado do jogo
struct EstadoJogo {
    int jogador_y;                                                                                              // Posição Y da raquete do jogador
    int ia_y;                                                                                                   // Posição Y da raquete da IA
    int direcao_ia;                                                                                             // Direção atual do movimento da IA
    int bola_x;                                                                                                 // Posição X atual da bola
    int bola_y;                                                                                                 // Posição Y atual da bola
    int bola_dx;                                                                                                // Direção horizontal da bola (+/- velocidade)
    int bola_dy;                                                                                                // Direção vertical da bola (+/- velocidade)
    int pontuacao_jogador;                                                                                      // Pontos acumulados pelo jogador
    int pontuacao_ia;                                                                                           // Pontos acumulados pela IA
};

void inicializar_display() {
    i2c_init(i2c1, 400000);                                                                                     // Inicializa interface I2C a 400kHz
    gpio_set_function(PINO_SDA, GPIO_FUNC_I2C);                                                                 // Configura pino SDA para função I2C
    gpio_set_function(PINO_SCL, GPIO_FUNC_I2C);                                                                 // Configura pino SCL para função I2C
    gpio_pull_up(PINO_SDA);                                                                                     // Habilita resistor pull-up no SDA
    gpio_pull_up(PINO_SCL);                                                                                     // Habilita resistor pull-up no SCL

    ssd1306_init(&disp, LARGURA, ALTURA, false, 0x3C, i2c1);                                                    // Inicializa display com endereço 0x3C
    ssd1306_config(&disp);                                                                                      // Configuração adicional do display
    ssd1306_fill(&disp, false);                                                                                 // Limpa o display (preenche com preto)
    ssd1306_send_data(&disp);                                                                                   // Envia buffer para o display
}

void inicializar_jogo(struct EstadoJogo* estado) {
    estado->jogador_y = (ALTURA - ALTURA_RAQUETE) / 2;                                                          // Centraliza raquete do jogador verticalmente
    estado->ia_y = (ALTURA - ALTURA_RAQUETE) / 2;                                                               // Centraliza raquete da IA verticalmente
    estado->bola_x = LARGURA / 2;                                                                               // Posiciona bola no centro horizontal
    estado->bola_y = ALTURA / 2;                                                                                // Posiciona bola no centro vertical
    estado->bola_dx = -VELOCIDADE_BOLA;                                                                         // Define direção inicial da bola para esquerda
    estado->bola_dy = VELOCIDADE_BOLA;                                                                          // Define direção vertical inicial para baixo
    estado->direcao_ia = 1;                                                                                     // Direção inicial da IA
    estado->pontuacao_jogador = 0;                                                                              // Zera pontuação do jogador
    estado->pontuacao_ia = 0;                                                                                   // Zera pontuação da IA
}

void ler_joystick(struct EstadoJogo* estado) {
    adc_select_input(1);                                                                                        // Seleciona canal ADC1 (GPIO27)
    uint16_t valor_adc = adc_read();                                                                            // Lê valor bruto do ADC (0-4095)
    estado->jogador_y = (ALTURA - ALTURA_RAQUETE) - (valor_adc * (ALTURA - ALTURA_RAQUETE)) / 4096;             // Mapeia valor ADC para posição Y
}

void atualizar_ia(struct EstadoJogo* estado) {
    if (estado->ia_y + ALTURA_RAQUETE / 2 < estado->bola_y) {                                                   // Se centro da raquete está abaixo da bola
        estado->ia_y += VELOCIDADE_RAQUETE;                                                                     // Move raquete para baixo
    } else if (estado->ia_y + ALTURA_RAQUETE / 2 > estado->bola_y) {                                            // Se centro da raquete está acima da bola
        estado->ia_y -= VELOCIDADE_RAQUETE;                                                                     // Move raquete para cima
    }

    if (estado->ia_y < 0) estado->ia_y = 0;                                                                     // Limita movimento superior da raquete
    if (estado->ia_y > ALTURA - ALTURA_RAQUETE) estado->ia_y = ALTURA - ALTURA_RAQUETE;                         // Limita movimento inferior
}

void atualizar_bola(struct EstadoJogo* estado) {
    estado->bola_x += estado->bola_dx;                                                                          // Atualiza posição horizontal da bola
    estado->bola_y += estado->bola_dy;                                                                          // Atualiza posição vertical da bola

    if (estado->bola_y <= 0 || estado->bola_y >= ALTURA - TAMANHO_BOLA) {                                       // Verifica colisão com bordas superior/inferior
        estado->bola_dy *= -1;                                                                                  // Inverte direção vertical
    }

    if (estado->bola_x <= LARGURA_RAQUETE &&                                                                    // Verifica colisão com raquete do jogador
        estado->bola_y + TAMANHO_BOLA >= estado->jogador_y && 
        estado->bola_y <= estado->jogador_y + ALTURA_RAQUETE) {
        estado->bola_dx *= -1;                                                                                  // Inverte direção horizontal
    }

    if (estado->bola_x >= LARGURA - LARGURA_RAQUETE - TAMANHO_BOLA &&                                           // Verifica colisão com raquete da IA
        estado->bola_y + TAMANHO_BOLA >= estado->ia_y && 
        estado->bola_y <= estado->ia_y + ALTURA_RAQUETE) {
        estado->bola_dx *= -1;                                                                                  // Inverte direção horizontal
    }

    if (estado->bola_x < 0) {                                                                                   // Bola passou pela raquete esquerda
        estado->pontuacao_ia++;                                                                                 // Incrementa pontuação da IA
        estado->jogador_y = (ALTURA - ALTURA_RAQUETE) / 2;                                                      // Reposiciona raquete do jogador
        estado->ia_y = (ALTURA - ALTURA_RAQUETE) / 2;                                                           // Reposiciona raquete da IA
        estado->bola_x = LARGURA / 2;                                                                           // Reposiciona bola no centro X
        estado->bola_y = ALTURA / 2;                                                                            // Reposiciona bola no centro Y
        estado->bola_dx = -VELOCIDADE_BOLA;                                                                     // Reinicia direção horizontal
        estado->bola_dy = VELOCIDADE_BOLA;                                                                      // Reinicia direção vertical
    }
    if (estado->bola_x > LARGURA) {                                                                             // Bola passou pela raquete direita
        estado->pontuacao_jogador++;                                                                            // Incrementa pontuação do jogador
        estado->jogador_y = (ALTURA - ALTURA_RAQUETE) / 2;                                                      // Reposiciona raquete do jogador
        estado->ia_y = (ALTURA - ALTURA_RAQUETE) / 2;                                                           // Reposiciona raquete da IA
        estado->bola_x = LARGURA / 2;                                                                           // Reposiciona bola no centro X
        estado->bola_y = ALTURA / 2;                                                                            // Reposiciona bola no centro Y
        estado->bola_dx = VELOCIDADE_BOLA;                                                                      // Reinicia direção horizontal
        estado->bola_dy = VELOCIDADE_BOLA;                                                                      // Reinicia direção vertical
    }
}

void desenhar_tela_inicial() {
    ssd1306_fill(&disp, false);                                                                                 // Limpa o buffer do display
    
    ssd1306_draw_string(&disp, "EMBARCATECH", (LARGURA/2) - 44, (ALTURA/2) - 10);                               // Desenha texto centralizado
    ssd1306_draw_string(&disp, "GAME", (LARGURA/2) - 16, (ALTURA/2) + 2);                                       // Desenha subtítulo
    
    ssd1306_send_data(&disp);                                                                                   // Atualiza display com novo buffer
}

void desenhar_jogo(struct EstadoJogo* estado) {
    ssd1306_fill(&disp, false);                                                                                 // Limpa o buffer do display
    
    ssd1306_rect(&disp, estado->jogador_y, 0, LARGURA_RAQUETE, ALTURA_RAQUETE, true, true);                     // Desenha raquete do jogador
    ssd1306_rect(&disp, estado->ia_y, LARGURA - LARGURA_RAQUETE, LARGURA_RAQUETE, ALTURA_RAQUETE, true, true);  // Desenha raquete da IA
    
    ssd1306_rect(&disp, 0, 0, LARGURA, ALTURA, true, false);                                                    // Desenha borda do campo

    for (int x = 0; x < LARGURA; x += 8) {                                                                      // Desenha linha central tracejada
        ssd1306_rect(&disp, x, ALTURA / 1 - 1, 3, 2, true, true); 
    }

    ssd1306_rect(&disp, estado->bola_y, estado->bola_x, TAMANHO_BOLA, TAMANHO_BOLA, true, true);                // Desenha bola
    
    char placar[16];                                                                                            // Buffer para texto do placar
    snprintf(placar, sizeof(placar), "%d - %d", estado->pontuacao_jogador, estado->pontuacao_ia);               // Formata placar
    ssd1306_draw_string(&disp, placar, LARGURA/2 - 16, 5);                                                      // Desenha placar na posição calculada
    
    ssd1306_send_data(&disp);                                                                                   // Atualiza display com novo buffer
}

int main() {
    stdio_init_all();                                                                                           // Inicializa todas as interfaces padrão
    inicializar_display();                                                                                      // Configura hardware do display
    adc_init();                                                                                                 // Inicializa módulo ADC
    adc_gpio_init(JOYSTICK_Y);                                                                                  // Configura pino do joystick como ADC

    desenhar_tela_inicial();                                                                                    // Exibe tela inicial
    sleep_ms(3000);                                                                                             // Aguarda 3 segundos
    
    struct EstadoJogo estado;                                                                                   // Cria instância do estado do jogo
    inicializar_jogo(&estado);                                                                                  // Inicializa valores do jogo
    
    while (1) {                                                                                                 // Loop principal do jogo
        ler_joystick(&estado);                                                                                  // Lê entrada do jogador
        atualizar_ia(&estado);                                                                                  // Atualiza IA
        atualizar_bola(&estado);                                                                                // Atualiza física da bola
        desenhar_jogo(&estado);                                                                                 // Renderiza cena
        sleep_ms(16);                                                                                           // Controla FPS (~60Hz)
    }
    
    return 0;
}