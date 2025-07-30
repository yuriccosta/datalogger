# Datalogger de Movimento com IMU – BitDogLab + Raspberry Pi Pico W

Este projeto implementa um datalogger embarcado de movimento, utilizando a plataforma **BitDogLab com Raspberry Pi Pico W**. O sistema realiza a leitura de aceleração e giroscópio em tempo real usando o sensor MPU6050, armazena os dados em formato `.csv` em um cartão SD e fornece feedback ao usuário por meio de um display OLED, LED RGB e buzzer.

---

## 🎯 Objetivo

Desenvolver um sistema embarcado capaz de capturar e armazenar dados de aceleração e giroscópio com base em um sensor MPU6050, fornecendo interface de status e controle ao usuário localmente através de recursos visuais e sonoros da plataforma BitDogLab.

---

## ⚙️ Funcionalidades

- **Leitura de Sensor IMU MPU6050 via I²C**  
  Captura contínua dos valores de aceleração e giroscópio nos eixos X, Y e Z.

- **Armazenamento no cartão SD via SPI**  
  Os dados são armazenados em um arquivo `.csv` com cabeçalho:  
  `numero_amostra,accel_x,accel_y,accel_z,giro_x,giro_y,giro_z`.

- **Display OLED SSD1306 (I²C)**  
  Exibe mensagens como:  
  - “Inicializando”  
  - “Aguardando”  
  - “Gravando...”  
  - “Dados Salvos!”  
  - “ERRO!”  
  Também mostra o número da amostra durante a gravação.

- **LED RGB (Status do Sistema)**  
  - **Amarelo**: Inicialização ou montagem do cartão SD  
  - **Verde**: Sistema pronto para captura  
  - **Azul piscando**: Acesso ao cartão SD durante gravação  
  - **Roxo piscando**: Erro na gravação

- **Buzzer (Feedback Sonoro)**  
  - 1 beep curto: Montagem do cartão SD  
  - 2 beeps curtos: Desmontagem do cartão SD

- **Botões Físicos (GPIO com interrupção e debounce)**  
  - **Botão A**: Monta ou desmonta o cartão SD com segurança  
  - **Botão B**: Inicia ou para a gravação de dados

---

## 🧩 Componentes Utilizados

| Componente            | Função                                                                 |
|-----------------------|------------------------------------------------------------------------|
| MPU6050 (I²C)         | Leitura de aceleração e giroscópio                                     |
| Cartão MicroSD (SPI)  | Armazenamento dos dados em `.csv`                                      |
| Display OLED SSD1306  | Exibição de status do sistema e número da amostra                      |
| LED RGB               | Indicação dos estados operacionais                                     |
| Buzzer                | Feedback sonoro em eventos de montagem e desmontagem do cartão         |
| Botões GPIO           | Controle total do sistema via interrupções com debounce                |
| Raspberry Pi Pico W   | Processamento e controle central                                       |

---

## 📂 Estrutura do Arquivo `.csv`

```csv
numero_amostra,accel_x,accel_y,accel_z,giro_x,giro_y,giro_z
1,2952,-4,17428,661,795,-173
2,3024,-980,17376,760,161,-3160
...
