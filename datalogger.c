#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "hardware/adc.h"
#include "hardware/rtc.h"


#include "ff.h"
#include "diskio.h"
#include "f_util.h"
#include "hw_config.h"
#include "my_debug.h"
#include "rtc.h"
#include "sd_card.h"

#include "lib/Cartao_FatFS_SPI.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "ssd1306.h"
#include "font.h"



// Variáveis globais de estado
bool mount = false; // Indica se o cartão SD está montado
bool read = false;  // Indica se está em modo de gravação

// Definições dos pinos dos botões
#define botaoB 6 // Botão B: inicia/para gravação
#define botaoA 5 // Botão A: monta/desmonta SD

// Definições dos pinos dos LEDs e buzzer
#define LED_PIN_GREEN 11 // LED verde: pronto
#define LED_PIN_BLUE 12  // LED azul: acesso SD
#define LED_PIN_RED 13   // LED vermelho: erro/estado
#define BUZZER_A 21      // Buzzer

 

// Definições de I2C para sensores e display
#define I2C_PORT i2c0               // I2C0: pinos 0 e 1 (MPU6050)
#define I2C_SDA 0                   // SDA do MPU6050
#define I2C_SCL 1                   // SCL do MPU6050
#define I2C_PORT_DISP i2c1          // I2C1: pinos 14 e 15 (Display OLED)
#define I2C_SDA_DISP 14             // SDA do display
#define I2C_SCL_DISP 15             // SCL do display
#define ENDERECO_DISP 0x3C          // Endereço I2C do display SSD1306
#define DISP_W 128                  // Largura do display
#define DISP_H 64                   // Altura do display

 
 // O endereço padrao deste IMU é o 0x68
 static int addr = 0x68;
 
 static void mpu6050_reset() {
     // Two byte reset. First byte register, second byte data
     // There are a load more options to set up the device in different ways that could be added here
     uint8_t buf[] = {0x6B, 0x80};
     i2c_write_blocking(I2C_PORT, addr, buf, 2, false);
     sleep_ms(100); // Allow device to reset and stabilize
 
     // Clear sleep mode (0x6B register, 0x00 value)
     buf[1] = 0x00;  // Clear sleep mode by writing 0x00 to the 0x6B register
     i2c_write_blocking(I2C_PORT, addr, buf, 2, false); 
     sleep_ms(10); // Allow stabilization after waking up
 }
 
 static void mpu6050_read_raw(int16_t accel[3], int16_t gyro[3], int16_t *temp) {
     // For this particular device, we send the device the register we want to read
     // first, then subsequently read from the device. The register is auto incrementing
     // so we don't need to keep sending the register we want, just the first.
 
     uint8_t buffer[6];
 
     // Start reading acceleration registers from register 0x3B for 6 bytes
     uint8_t val = 0x3B;
     i2c_write_blocking(I2C_PORT, addr, &val, 1, true); // true to keep master control of bus
     i2c_read_blocking(I2C_PORT, addr, buffer, 6, false);
 
     for (int i = 0; i < 3; i++) {
         accel[i] = (buffer[i * 2] << 8 | buffer[(i * 2) + 1]);
     }
 
     // Now gyro data from reg 0x43 for 6 bytes
     // The register is auto incrementing on each read
     val = 0x43;
     i2c_write_blocking(I2C_PORT, addr, &val, 1, true);
     i2c_read_blocking(I2C_PORT, addr, buffer, 6, false);  // False - finished with bus
 
     for (int i = 0; i < 3; i++) {
         gyro[i] = (buffer[i * 2] << 8 | buffer[(i * 2) + 1]);;
     }
 
     // Now temperature from reg 0x41 for 2 bytes
     // The register is auto incrementing on each read
     val = 0x41;
     i2c_write_blocking(I2C_PORT, addr, &val, 1, true);
     i2c_read_blocking(I2C_PORT, addr, buffer, 2, false);  // False - finished with bus
 
     *temp = buffer[0] << 8 | buffer[1];
 }



// Função para iniciar o buzzer
void iniciar_buzzer(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin); // Obtém o slice correspondente

    pwm_set_clkdiv(slice_num, 125); // Define o divisor de clock
    pwm_set_wrap(slice_num, 1000);  // Define o valor máximo do PWM

    pwm_set_gpio_level(pin, 100); //Para um som mais baixo foi colocado em 100
    pwm_set_enabled(slice_num, true);
}

// Função para parar o buzzer
void parar_buzzer(uint pin) {
    uint slice_num = pwm_gpio_to_slice_num(pin); // Obtém o slice correspondente
    pwm_set_enabled(slice_num, false); // Desabilita o slice PWM
    gpio_put(pin, 0); // Coloca o pino em nível para garantir que o buzzer está desligado
}

void gpio_irq_handler(uint gpio, uint32_t events)
{
    absolute_time_t agora = get_absolute_time();
    if (gpio == botaoB)
    {
        static absolute_time_t ultimo_acionamentoB = 0;
        // Debounce: só aceita novo comando após 300ms
        if (absolute_time_diff_us(ultimo_acionamentoB, agora) > 300000) {
            if (read){
                printf("Parando leitura\n");
                read = false;
            } else {
                printf("Iniciando Leitura\n");
                read = true;
            }
            ultimo_acionamentoB = agora;
        }
    } else if (!read){
        static absolute_time_t ultimo_acionamentoA = 0;
        // Debounce: só aceita novo comando após 300ms
        if (absolute_time_diff_us(ultimo_acionamentoA, agora) > 300000) {
            if (mount){
                printf("Desmontando cartão\n");
                mount = false;
            } else {
                printf("Montando cartão\n");
                mount = true;
            }
            ultimo_acionamentoA = agora;
        }
    }
}

int main()
{
    // Inicialização dos botões e interrupções
    gpio_init(botaoB);
    gpio_set_dir(botaoB, GPIO_IN);
    gpio_pull_up(botaoB);
    gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
   
    gpio_init(botaoA);
    gpio_set_dir(botaoA, GPIO_IN);
    gpio_pull_up(botaoA);
    gpio_set_irq_enabled_with_callback(botaoA, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // Inicialização dos LEDs
    gpio_init(LED_PIN_GREEN);
    gpio_set_dir(LED_PIN_GREEN, GPIO_OUT);
    gpio_init(LED_PIN_BLUE);
    gpio_set_dir(LED_PIN_BLUE, GPIO_OUT);
    gpio_init(LED_PIN_RED);
    gpio_set_dir(LED_PIN_RED, GPIO_OUT);

    // Inicialização da UART/USB
    stdio_init_all();
    sleep_ms(5000);

    // Inicialização do display OLED
    i2c_init(I2C_PORT_DISP, 400 * 1000);
    gpio_set_function(I2C_SDA_DISP, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_DISP, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_DISP);
    gpio_pull_up(I2C_SCL_DISP);

    ssd1306_t ssd;
    ssd1306_init(&ssd, DISP_W, DISP_H, false, ENDERECO_DISP, I2C_PORT_DISP);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);

    // Mensagem inicial no display
    ssd1306_fill(&ssd, false);
    ssd1306_rect(&ssd, 3, 3, 122, 58, true, false);
    ssd1306_draw_string(&ssd, "Inicializando", 10, 6);
    ssd1306_send_data(&ssd);
    // LED amarelo (verde+vermelho) indica inicialização
    gpio_put(LED_PIN_GREEN, true);
    gpio_put(LED_PIN_BLUE, false);
    gpio_put(LED_PIN_RED, true);





 
     printf("Hello, MPU6050! Reading raw data from registers...\n");
 
     // This example will use I2C0 on the default SDA and SCL pins (4, 5 on a Pico)
     i2c_init(I2C_PORT, 400 * 1000);
     gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
     gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
     gpio_pull_up(I2C_SDA);
     gpio_pull_up(I2C_SCL);
     // Make the I2C pins available to picotool
     printf("Antes do bi_decl...\n");
     bi_decl(bi_2pins_with_func(I2C_SDA, I2C_SCL, GPIO_FUNC_I2C));
     printf("Antes do reset MPU...\n");
     mpu6050_reset();
 
    int16_t acceleration[3], gyro[3], temp;


    // Configuração do SD
    sleep_ms(5000);
    time_init();



    // Variáveis de controle de estado
    bool last_mount = false;      // Estado anterior da montagem
    static uint numero_amostra = 1; // Contador de amostras
    static bool azul = false;        // Estado do LED azul

    // Loop principal
    while (1) {
        // Feedback visual/sonoro para montagem/desmontagem do SD
        if (mount != last_mount) {
            if (mount) {
                printf("Montando cartão\n");
                // LED amarelo (verde+vermelho)
                gpio_put(LED_PIN_GREEN, true);
                gpio_put(LED_PIN_BLUE, false);
                gpio_put(LED_PIN_RED, true);
                iniciar_buzzer(BUZZER_A); // Beep curto
                sleep_ms(100);
                parar_buzzer(BUZZER_A);
                run_mount();
                gpio_put(LED_PIN_RED, false); // Volta para verde se pronto
            } else {
                printf("Desmontando cartão\n");
                // Dois beeps curtos
                for (int i = 0; i < 2; i++) {
                    iniciar_buzzer(BUZZER_A);
                    sleep_ms(80);
                    parar_buzzer(BUZZER_A);
                    sleep_ms(80);
                }
                run_unmount();
            }
            last_mount = mount;
        }

        // Gravação de dados do IMU
        static bool was_read = false;
        if (mount && read) {
            // Lê dados do sensor
            mpu6050_read_raw(acceleration, gyro, &temp);
            char amostra_str[10];
            snprintf(amostra_str, sizeof(amostra_str), "%d", numero_amostra);

            // Atualiza display: status de gravação
            ssd1306_fill(&ssd, false);
            ssd1306_rect(&ssd, 3, 3, 122, 58, true, false);
            ssd1306_draw_string(&ssd, "Gravando...", 10, 6);
            ssd1306_draw_string(&ssd, "Amostra:", 10, 20);
            ssd1306_draw_string(&ssd, amostra_str, 75, 20);
            ssd1306_send_data(&ssd);

            // LED azul piscando indica acesso ao SD
            azul = !azul;
            gpio_put(LED_PIN_GREEN, false);
            gpio_put(LED_PIN_RED, false);
            gpio_put(LED_PIN_BLUE, azul);

            // Salva dados no arquivo CSV
            bool ok = save_imu_data_to_csv(numero_amostra++, acceleration[0], acceleration[1], acceleration[2],
                                gyro[0], gyro[1], gyro[2]);
            if (!ok) {
                // LED roxo piscando indica erro de gravação
                for (int i = 0; i < 3; i++) {
                    gpio_put(LED_PIN_RED, true);
                    gpio_put(LED_PIN_BLUE, true);
                    sleep_ms(150);
                    gpio_put(LED_PIN_RED, false);
                    gpio_put(LED_PIN_BLUE, false);
                    sleep_ms(150);
                }
                // Mensagem de erro no display
                ssd1306_fill(&ssd, false);
                ssd1306_rect(&ssd, 3, 3, 122, 58, true, false);
                ssd1306_draw_string(&ssd, "ERRO!", 10, 20);
                ssd1306_send_data(&ssd);
                sleep_ms(1000);
            }
            was_read = true;
        } else if (mount && !read) {
            // Só mostra 'Dados Salvos!' se estava gravando antes
            if (was_read) {
                ssd1306_fill(&ssd, false);
                ssd1306_rect(&ssd, 3, 3, 122, 58, true, false);
                ssd1306_draw_string(&ssd, "Dados Salvos!", 10, 20);
                ssd1306_send_data(&ssd);
                sleep_ms(300);
                was_read = false;
            }
            // Pronto para gravar: LED verde e display aguardando
            gpio_put(LED_PIN_GREEN, true);
            gpio_put(LED_PIN_BLUE, false);
            gpio_put(LED_PIN_RED, false);

            ssd1306_fill(&ssd, false);
            ssd1306_rect(&ssd, 3, 3, 122, 58, true, false);
            ssd1306_draw_string(&ssd, "Aguardando", 10, 6);
            ssd1306_send_data(&ssd);
        } else {
            // Sistema parado/desmontado: LEDs e display desligados
            gpio_put(LED_PIN_GREEN, false);
            gpio_put(LED_PIN_BLUE, false);
            gpio_put(LED_PIN_RED, false);
            ssd1306_fill(&ssd, false);
            ssd1306_rect(&ssd, 3, 3, 122, 58, true, false);
            ssd1306_draw_string(&ssd, "Aguardando", 10, 6);
            ssd1306_send_data(&ssd);
            was_read = false;
        }

        sleep_ms(500); // Intervalo entre leituras (500ms)
    }
    return 0;
}
