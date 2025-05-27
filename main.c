#include <stdio.h>
#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include <string.h>

#include "lib/ssd1306.h"
#include "lib/neopixel.h"
#include "lib/buzzer.h"

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "semphr.h"

// Pinos
#define RED_PIN       13
#define GREEN_PIN     11
#define BLUE_PIN      12
#define BUTTON_A      5
#define BUTTON_B      6
#define BUTTON_JOYSTICK 22
#define BUZZER_A    21
#define BUZZER_B    10

static ssd1306_t ssd; // Estrutura do display OLED

uint64_t last_time_button_a = 0;
uint64_t last_time_button_b = 0;
uint64_t last_time_button_joystick = 0;

// Váriaveis do Programa

#define max 10 // Constante de valor máximo de vagas

// Contador de quantas vagas estão preenchidas na clínica
// Deve ser 'volatile' porque pode ser acessado por várias tasks
volatile uint16_t vagasPreenchidas = 0; 

// Semaforos e Multex

SemaphoreHandle_t xContadorSem;

SemaphoreHandle_t xDisplayMutex;

SemaphoreHandle_t xButtonASem;
SemaphoreHandle_t xButtonBSem;
SemaphoreHandle_t xResetSem; // Em referência ao botão do joystick

// String auxiliar para exibição de quantas vagas foram preenchidas
char mensagemDisplay[20]; 

// Mostra uma animação simples de inicialização no display OLED
void inicializa_sistema(bool taskHablitada){

    ssd1306_fill(&ssd, 0);
    ssd1306_draw_string(&ssd, "Habilitando", 18, 20);
    ssd1306_draw_string(&ssd, "Sistema", 34, 30);
    ssd1306_send_data(&ssd);
    taskHablitada ? vTaskDelay(pdMS_TO_TICKS(500)) : sleep_ms(500);

    ssd1306_fill(&ssd, 0);
    ssd1306_draw_string(&ssd, "Habilitando", 18, 20);
    ssd1306_draw_string(&ssd, "Sistema", 34, 30);
    ssd1306_draw_string(&ssd, ".", 54, 40);
    ssd1306_send_data(&ssd);
    taskHablitada ? vTaskDelay(pdMS_TO_TICKS(1000)) : sleep_ms(1000);

    ssd1306_fill(&ssd, 0);
    ssd1306_draw_string(&ssd, "Habilitando", 18, 20);
    ssd1306_draw_string(&ssd, "Sistema", 34, 30);
    ssd1306_draw_string(&ssd, "..", 50, 40);
    ssd1306_send_data(&ssd);
    taskHablitada ? vTaskDelay(pdMS_TO_TICKS(1000)) : sleep_ms(1000);

    ssd1306_fill(&ssd, 0);
    ssd1306_draw_string(&ssd, "Habilitando", 18, 20);
    ssd1306_draw_string(&ssd, "Sistema", 34, 30);
    ssd1306_draw_string(&ssd, "...", 46, 40);
    ssd1306_send_data(&ssd);
    taskHablitada ? vTaskDelay(pdMS_TO_TICKS(1000)) : sleep_ms(1000);

}

// Atualiza o conteúdo do display OLED com o número atual de pacientes
void atualizaDisplay(){

    ssd1306_fill(&ssd, 0);
    
    snprintf(mensagemDisplay, sizeof(mensagemDisplay), "%d / %d", vagasPreenchidas, max);

    // Exibe aviso se a clínica estiver lotada    
    if(vagasPreenchidas == max) ssd1306_draw_string(&ssd, "CLINICA LOTADA", 6, 10);

    ssd1306_draw_string(&ssd, "PACIENTES", 26, 30);
    ssd1306_draw_string(&ssd, mensagemDisplay, (124 - strlen(mensagemDisplay) * 8) / 2, 45);

    ssd1306_send_data(&ssd);
}

// Atualiza os LEDs RGB de acordo com o número de pacientes
void atualizaLeds(){

    if(vagasPreenchidas == max){
        gpio_put(RED_PIN, true);
        gpio_put(GREEN_PIN, false);
        gpio_put(BLUE_PIN, false);
    }
    else if(vagasPreenchidas == max - 1){
        gpio_put(RED_PIN, true);
        gpio_put(GREEN_PIN, true);
        gpio_put(BLUE_PIN, false);
    }
    else if(vagasPreenchidas == 0){
        gpio_put(RED_PIN, false);
        gpio_put(GREEN_PIN, false);
        gpio_put(BLUE_PIN, true);
    }
    else{
        gpio_put(RED_PIN, false);
        gpio_put(GREEN_PIN, true);
        gpio_put(BLUE_PIN, false);
    }
}

// Emite um sinal sonoro simples utilizando o buzzer
void beep(){
    buzzer_turn_on(BUZZER_A, 1000);
    vTaskDelay(200);
    buzzer_turn_off(BUZZER_A);
    vTaskDelay(200);
}

// Task que trata a entrada de um novo paciente (Botão A)
void vTaskEntrada(){

    // Espera o botão A ser pressionado
    while(true){
        if(xSemaphoreTake(xButtonASem, portMAX_DELAY) == pdTRUE){
            // Verifica se há ainda vagas no contador
            if(xSemaphoreTake(xContadorSem, 0) == pdTRUE){

                // Verifica se o display ainda está sendo utilizado
                if(xSemaphoreTake(xDisplayMutex, portMAX_DELAY) == pdTRUE){
                    vagasPreenchidas++; // Aumenta a quantidade de vagas já preenchidas (Coloquei aqui dentro pra evitar race condition)
                    atualizaDisplay();
                    atualizaLeds();
                    xSemaphoreGive(xDisplayMutex); // Finalizou com o display, devolve o mutex.
                }

            }
            // Caso não haja mais vagas
            else{
                // Verifica se o display ainda está sendo utilizado
                if(xSemaphoreTake(xDisplayMutex, portMAX_DELAY) == pdTRUE){
                    atualizaDisplay();
                    atualizaLeds();
                    beep();
                    xSemaphoreGive(xDisplayMutex); // Finalizou com o display, devolve o mutex.
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50)); // Delay para não cobrar muito do processador
    }

}

// Task que trata a saída de um paciente (Botão B)
void vTaskSaida(){

    while(true){
        // Espera o botão do joystick ser pressionado
        if(xSemaphoreTake(xButtonBSem, portMAX_DELAY) == pdTRUE){
            // Verifica se há vagas que possams er retiradas
            if(vagasPreenchidas > 0){

                // Verifica se o display ainda está sendo utilizado
                if(xSemaphoreTake(xDisplayMutex, portMAX_DELAY) == pdTRUE){
                    vagasPreenchidas--; // Diminui a quantidade de vagas já preenchidas (Coloquei aqui dentro pra evitar race condition)
                    xSemaphoreGive(xContadorSem);
                    atualizaDisplay();
                    atualizaLeds();
                    xSemaphoreGive(xDisplayMutex); // Finalizou com o display, devolve o mutex.
                }

            }
            // Se não, emite um beep
            else{
                beep();
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50)); // Delay para não cobrar muito do processador

    }

}

// Task que trata o reset do sistema (Botão Joystick)
void VTaskReset(){

    while(true){

        if(xSemaphoreTake(xResetSem, portMAX_DELAY) == pdTRUE){

            // Desabilita as interrupções dos botões para evitar interferência durante o reset
            gpio_set_irq_enabled(BUTTON_A, GPIO_IRQ_EDGE_FALL, false);
            gpio_set_irq_enabled(BUTTON_B, GPIO_IRQ_EDGE_FALL, false);
            gpio_set_irq_enabled(BUTTON_JOYSTICK, GPIO_IRQ_EDGE_FALL, false);

            // Reinicia o contador e devolve os semáforos correspondentes
            while(vagasPreenchidas > 0){
                vagasPreenchidas--;
                xSemaphoreGive(xContadorSem);
            }

            if(xSemaphoreTake(xDisplayMutex, portMAX_DELAY) == pdTRUE){
                inicializa_sistema(true);
                atualizaDisplay();
                atualizaLeds();
                beep();
                beep();
                xSemaphoreGive(xDisplayMutex); // Finalizou com o display, devolve o mutex.
            }

            // Reabilita as interrupções dos botões
            gpio_set_irq_enabled(BUTTON_A, GPIO_IRQ_EDGE_FALL, true);
            gpio_set_irq_enabled(BUTTON_B, GPIO_IRQ_EDGE_FALL, true);
            gpio_set_irq_enabled(BUTTON_JOYSTICK, GPIO_IRQ_EDGE_FALL, true);

        }

        vTaskDelay(pdMS_TO_TICKS(50)); // Delay para não cobrar muito do processador

    }

}

// Função de callback chamada quando um botão é pressionado
static void gpio_button_handler(uint gpio, uint32_t events) {

    uint64_t current_time = to_ms_since_boot(get_absolute_time());

    // Verifica qual botão gerou a interrupção e aplica debounce (200ms)
    if(gpio == BUTTON_A && current_time - last_time_button_a >= 200){
        last_time_button_a = current_time;
        // Libera o semáforo associado ao botão A
        xSemaphoreGiveFromISR(xButtonASem, NULL);
    }
    else if(gpio == BUTTON_B && current_time - last_time_button_b >= 200){
        last_time_button_b = current_time;
        // Libera o semáforo associado ao botão B
        xSemaphoreGiveFromISR(xButtonBSem, NULL);
    }
    else if(gpio == BUTTON_JOYSTICK && current_time - last_time_button_joystick >= 200){
        last_time_button_joystick = current_time;
        // Libera o semáforo para resetar o número de pacientes
        xSemaphoreGiveFromISR(xResetSem, NULL);
    }
}



// Inicializa o sistema: configura GPIOs, display, buzzer e interrupções dos botões
void setup(){
    stdio_init_all();

    display_init(&ssd);

    buzzer_init(BUZZER_A);

    // Configuração dos LEDs
    gpio_init(RED_PIN);
    gpio_set_dir(RED_PIN, GPIO_OUT);
    gpio_init(GREEN_PIN);
    gpio_set_dir(GREEN_PIN, GPIO_OUT);
    gpio_init(BLUE_PIN);
    gpio_set_dir(BLUE_PIN, GPIO_OUT);

    // Configuração dos Botões
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);

    gpio_init(BUTTON_JOYSTICK);
    gpio_set_dir(BUTTON_JOYSTICK, GPIO_IN);
    gpio_pull_up(BUTTON_JOYSTICK);

    // Configura interrupções
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_button_handler);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &gpio_button_handler);
    gpio_set_irq_enabled_with_callback(BUTTON_JOYSTICK, GPIO_IRQ_EDGE_FALL, true, &gpio_button_handler);


}

int main()
{
    setup();

    inicializa_sistema(false);
    atualizaDisplay();
    atualizaLeds();

    // Inicializa Semaforos Binários do Botões
    xButtonASem = xSemaphoreCreateBinary();
    xButtonBSem = xSemaphoreCreateBinary();
    xResetSem = xSemaphoreCreateBinary();

    // Incializa Mutex do Display
    xDisplayMutex = xSemaphoreCreateMutex();

    // Inicializa Semaforo do Contador
    xContadorSem = xSemaphoreCreateCounting(max, max);

    // Cria tarefas
    xTaskCreate(vTaskEntrada, "Task Entrada", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    xTaskCreate(vTaskSaida, "Task Saida", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);
    xTaskCreate(VTaskReset, "Task Reset", configMINIMAL_STACK_SIZE + 128, NULL, 1, NULL);

    vTaskStartScheduler();
    panic_unsupported();
}
