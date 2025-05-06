#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "lib/matriz_led.h"
#include "lib/buzzer.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include <stdio.h>

//Variáveis do display
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

//Pinos GPIO
#define LED_PIN_RED 13
#define LED_PIN_GREEN 11
#define BUZZER_PIN 21
#define BUTTON_PIN_A 5

//Variáveis globais
bool modoNoturno = false;
enum corSemaforo{
    VERDE,
    VERMELHO, 
    AMARELO,
    DESLIGADO
};
enum corSemaforo corAtual = VERDE; 
static volatile uint32_t last_interrupt_time_A = 0; 
volatile bool estadoMudou = false;     


//Tarefa que implementao modo normal do semáforo
void vModoNormalTask()
{
    gpio_init(LED_PIN_RED);
    gpio_set_dir(LED_PIN_RED, GPIO_OUT);
    gpio_init(LED_PIN_GREEN);
    gpio_set_dir(LED_PIN_GREEN, GPIO_OUT);

    while (true){

        if (!modoNoturno){ //Verificaçãp da varíavel global de modo noturno (presente em todas as tarefas)
            corAtual = VERDE;
            estadoMudou = true; 
            gpio_put(LED_PIN_RED, 0);
            gpio_put(LED_PIN_GREEN, 1);
            vTaskDelay(pdMS_TO_TICKS(5000));

            if (!modoNoturno) {
                corAtual = AMARELO; 
                estadoMudou = true; 
                gpio_put(LED_PIN_GREEN, 1);
                gpio_put(LED_PIN_RED, 1);
                vTaskDelay(pdMS_TO_TICKS(2000));
            }

            if (!modoNoturno) {
                corAtual = VERMELHO;
                estadoMudou = true; 
                gpio_put(LED_PIN_GREEN, 0);
                gpio_put(LED_PIN_RED, 1);
                vTaskDelay(pdMS_TO_TICKS(5000));
            }

        } else {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

//Tarefa que plementa o modo nortuno
void vModoNoturnoTask()
{
    gpio_init(LED_PIN_RED);
    gpio_set_dir(LED_PIN_RED, GPIO_OUT);
    gpio_init(LED_PIN_GREEN);
    gpio_set_dir(LED_PIN_GREEN, GPIO_OUT);

    while (true){
        if (modoNoturno){
            corAtual = AMARELO; 
            estadoMudou = true; 
            gpio_put(LED_PIN_GREEN, 1);
            gpio_put(LED_PIN_RED, 1);
            vTaskDelay(pdMS_TO_TICKS(1000));

            if (modoNoturno) { //Veificação dupla do modo noturno para evitar conflitos de tarefa. 
                corAtual = DESLIGADO;
                estadoMudou = true; 
                gpio_put(LED_PIN_GREEN, 0);
                gpio_put(LED_PIN_RED, 0); 
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

//Tarefa para imentar o buzzer
void vBuzzerTask()
{
    buzzer_init(BUZZER_PIN); 
    printf("Buzzer inicializado\n");

    enum corSemaforo ultimaCor = DESLIGADO;
    uint32_t ultimoToque = 0;

    while (true){
        uint32_t tempoAtual = to_ms_since_boot(get_absolute_time());

        // Se a cor mudou, toca imediatamente - Esse primeiro toque acontece imediatamente após a mudança de cor, sons intermitentes são feitos depos para evitar delays.
        if (corAtual != ultimaCor) {
            ultimaCor = corAtual;
            ultimoToque = tempoAtual;

            if (modoNoturno) { // Buzzer para modo noturno
                if (corAtual == AMARELO) {
                    tocar_frequencia(1000, 300); 
                }
            } else {
                switch (corAtual) { //Switch para modo normal
                    case VERDE:
                        tocar_frequencia(1200, 1000); 
                        ultimoToque = tempoAtual + 99999; 
                        break;
                    case AMARELO:
                        tocar_frequencia(1000, 250);
                        break;
                    case VERMELHO:
                        tocar_frequencia(800, 500); 
                        break;
                    default:
                        break;
                }
            }
        }

        // Sons intermitentes
        if (!modoNoturno) {
            switch (corAtual) {
                case AMARELO:
                    if (tempoAtual - ultimoToque >= 500) {
                        tocar_frequencia(1000, 250);
                        ultimoToque = tempoAtual;
                    }
                    break;
                case VERMELHO:
                    if (tempoAtual - ultimoToque >= 2000) {
                        tocar_frequencia(800, 500);
                        ultimoToque = tempoAtual;
                    }
                    break;
                default:
                    break;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

//Tarefa que implementa matriz LEDs
void vMatrizLEDTask()
{
    // Iniciar matriz de LEDs
    printf("Inicializando matriz de LEDs...\n");
    iniciar_matriz_leds(pio0, 0, led_matrix_pin);
    clear_matrix(pio0, 0);
    update_leds(pio, sm);

    while (true){
        if (modoNoturno){ //Matriz para modo noturno
            if (corAtual == AMARELO) {
                clear_matrix(pio0, 0);
        
                for (int y = 0; y < 5; y++) {
                    for (int x = 0; x < 5; x++) {
                        if (padrao_alerta[y][x] == 1) {
                            uint8_t led_pos = matriz_posicao_xy(x, y);
                
                            // Exclamação: 
                            //Haste em uma cor
                            if (x == 2 && (y == 0 || y == 1 || y == 2)) {
                                leds[led_pos] = create_color(10, 40, 0); 
                            } 
                            // Ponto inferior da exclamação destacado em outra cor
                            else if (x == 2 && y == 4) {
                                leds[led_pos] = create_color(10, 40, 0); 
                            }
                            // Resto do fundo da exclamação
                            else {
                                leds[led_pos] = create_color(30, 30, 0); 
                            }
                        }
                    }
                }
            
                update_leds(pio0, 0);
            } else {
                clear_matrix(pio0, 0);
                update_leds(pio0, 0);
            }
        } else {  // Matriz para modo normal
            clear_matrix(pio0, 0);
            
            switch (corAtual) {
                case VERDE:
                    // Simbolo de aprovado
                    for (int y = 0; y < 5; y++) {
                        for (int x = 0; x < 5; x++) {
                            if (padrao_verde[y][x] == 1) {
                                uint8_t led_pos = matriz_posicao_xy(x, y);
                                leds[led_pos] = create_color(40, 0, 0);
                            }
                        }
                    }
                    break;
                    
                case AMARELO:
                    // Simbolo de alerta
                    for (int y = 0; y < 5; y++) {
                        for (int x = 0; x < 5; x++) {
                            if (padrao_amarelo[y][x] == 1) {
                                uint8_t led_pos = matriz_posicao_xy(x, y);
                                leds[led_pos] = create_color(30, 30, 0); 
                            }
                        }
                    }
                    break;
                    
                case VERMELHO:
                    // Simbolo reprovado(X)
                    for (int y = 0; y < 5; y++) {
                        for (int x = 0; x < 5; x++) {
                            if (padrao_vermelho[y][x] == 1) {
                                uint8_t led_pos = matriz_posicao_xy(x, y);
                                leds[led_pos] = create_color(0, 40, 0); // Vermelho (GRB)
                            }
                        }
                    }
                    break;
                    
                default:
                    break;
            }
            update_leds(pio0, 0);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

//Tarefa que atualiza o display
void vDisplayTask()
{
    //Inicializa o display
    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);                    
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);                    
    gpio_pull_up(I2C_SDA);                                        
    gpio_pull_up(I2C_SCL);                                        

    ssd1306_t ssd;                                                
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); 
    ssd1306_config(&ssd);                                         
    ssd1306_send_data(&ssd);                                      
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    //Variáveis para display
    char modo_str[20];
    char cor_str[20];
    char instrucao_str[25];

    while (true)
    {

        sprintf(modo_str, "Modo: %s", modoNoturno ? "Noturno" : "Normal");

        if (modoNoturno) {
            if (corAtual == AMARELO) {
                sprintf(cor_str, "Estado: ALERTA");
                sprintf(instrucao_str, "Atencao!");
            } else {
                sprintf(cor_str, "Estado: -");
                sprintf(instrucao_str, "Atencao!");
            }
        } else {
            switch (corAtual) {
                case VERDE:
                    sprintf(cor_str, "Estado: LIVRE");
                    sprintf(instrucao_str, "Seguro!");
                    break;
                case AMARELO:
                    sprintf(cor_str, "Estado: ALERTA");
                    sprintf(instrucao_str, "Atencao!");
                    break;
                case VERMELHO:
                    sprintf(cor_str, "Estado: PARADO");
                    sprintf(instrucao_str, "Pare!");
                    break;
                default:
                    sprintf(cor_str, "Estado: -");
                    sprintf(instrucao_str, "-");
                    break;
            }
        }

        ssd1306_fill(&ssd, false);
        ssd1306_rect(&ssd, 3, 3, 122, 60, true, false);
        ssd1306_line(&ssd, 3, 20, 122, 20, true);
        ssd1306_line(&ssd, 3, 40, 122, 40, true);
        
        ssd1306_draw_string(&ssd, "SEMAFORO", 16, 8);
        ssd1306_draw_string(&ssd, modo_str, 10, 25);
        ssd1306_draw_string(&ssd, cor_str, 10, 33);
        ssd1306_draw_string(&ssd, instrucao_str, 10, 48);
        
        ssd1306_send_data(&ssd);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

//Tarefa que implemnta botão para alternar modos
void vBotaoTask()
{
    gpio_init(BUTTON_PIN_A);
    gpio_set_dir(BUTTON_PIN_A, GPIO_IN);
    gpio_pull_up(BUTTON_PIN_A);

    bool botao_anterior = true; 

    while (true){
        uint32_t current_time = to_us_since_boot(get_absolute_time()); 
        
        bool estado_atual = gpio_get(BUTTON_PIN_A);
        
        if (estado_atual == false && botao_anterior == true && 
            (current_time - last_interrupt_time_A > 200000)) { // 200ms de debounce
            
            last_interrupt_time_A = current_time;
            modoNoturno = !modoNoturno; 
            printf("Modo alterado: %s\n", modoNoturno ? "Modo Noturno" : "Modo Normal");
        }
        
        botao_anterior = estado_atual; 
        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}

// Trecho para modo BOOTSEL com botão B
#include "pico/bootrom.h"
#define botaoB 6
void gpio_irq_handler(uint gpio, uint32_t events)
{
    reset_usb_boot(0, 0);
}

int main()
{
    // Para ser utilizado o modo BOOTSEL com botão B
    gpio_init(botaoB);
    gpio_set_dir(botaoB, GPIO_IN);
    gpio_pull_up(botaoB);
    gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    // Fim do trecho para modo BOOTSEL com botão B

    stdio_init_all();
    printf("Iniciando Semaforo Inteligente com Modo Noturno\n");

    xTaskCreate(vModoNormalTask, "Semáforo Modo Normal", configMINIMAL_STACK_SIZE,
         NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vModoNoturnoTask, "Semaforo Modo Noturno", configMINIMAL_STACK_SIZE, 
        NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vBuzzerTask, "Task Buzzer", configMINIMAL_STACK_SIZE, 
        NULL, tskIDLE_PRIORITY + 2, NULL);
    xTaskCreate(vMatrizLEDTask, "Task Matriz de LED", configMINIMAL_STACK_SIZE * 2, // Aumentei o stack para garantir
        NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vDisplayTask, "Task Display", configMINIMAL_STACK_SIZE, 
        NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(vBotaoTask, "Task Botao", configMINIMAL_STACK_SIZE, 
        NULL, tskIDLE_PRIORITY + 2, NULL);
    vTaskStartScheduler();
    panic_unsupported();
}
