# ClinicSync - Monitor de Ocupação de Clinicas com FreeRTOS

Projeto embarcado desenvolvido na Raspberry Pi Pico (RP2040) com FreeRTOS, que gerencia e exibe em tempo real o número de pacientes dentro de uma clínica. Utilizando **botões físicos, LEDs RGB, buzzer e display OLED**, o sistema oferece **sinalização visual e sonora** da ocupação atual. O modo de reset pode ser acionado via botão joystick.

## Funcionalidades Principais

- **Contagem de pacientes:**  
  - Capacidade máxima definida **(10 pacientes)**
  - **Botão A**: incrementa a contagem
  - **Botão B**: decrementa a contagem
  - **Botão Joystick**: reseta o contador (zera ocupação)

- **Sinalização visual com LEDs RGB:**  
  - **Azul** : nenhuma caga ocupada
  - **Verde**: vagas disponíveis
  - **Amarelo**: última vaga restante
  - **Vermelho**: clínica 

- **Feedback no display OLED SSD1306:**

  - Mostra número de pacientes ocupando as vagas
  - Mensagem de alerta "CLINICA LOTADA" quando atingir capacidade máxima

- **Feedback sonoro via buzzer** 

- **Interface multitarefa com FreeRTOS:**
  - Utilização de **semáforos** e **mutexes** para controle seguro de acesso concorrente
  

## Componentes Utilizados

| Componente               | GPIO/Pino         | Função                                                       |
|--------------------------|-------------------|--------------------------------------------------------------|
| LED RGB (Vermelho)       | 13                | Sinalização de clínica cheia                                |
| LED RGB (Verde)          | 11                | Sinalização de clínica vazia/parcial                        |
| LED RGB (Azul)           | 12                | Usado na combinação para cor amarela                        |
| Botão A                  | 5                 | Incrementa número de pacientes                              |
| Botão B                  | 6                 | Decrementa número de pacientes                              |
| Botão Joystick           | 22                | Reseta ocupação                                              |
| Display OLED SSD1306     | 14 (SDA), 15 (SCL)| Mostra quantidade de pacientes                              |
| Buzzer A                 | 21                | Emite som para ações                                        |
| Buzzer B                 | 10                | Pode ser usado para sons alternativos                       |
| Raspberry Pi Pico        | -                 | Plataforma embarcada principal                              |

---

## Organização do Código

- **`main.c`**  
  Arquivo principal que inicializa os periféricos e define as tasks do FreeRTOS.

- **Tarefas principais**  
  - `vBotaoATask()` – Trata o botão A (incrementa o número de pacientes)  
  - `vBotaoBTask()` – Trata o botão B (decrementa o número de pacientes)  
  - `vResetTask()` – Reseta o número de pacientes para zero  
  - `vAtualizaDisplayTask()` – Atualiza o display OLED com a situação atual  
  - `vAtualizaLedTask()` – Controla os LEDs RGB de acordo com a ocupação  
  - `vBuzzerTask()` – Gera sons correspondentes às ações do sistema  

- **Funções auxiliares**  
  - `inicializa_sistema()` – Mostra uma animação no OLED na inicialização  
  - `atualizaDisplay()` – Exibe o número de pacientes e o estado no display  
  - `atualizaLeds()` – Altera o estado dos LEDs RGB conforme a ocupação da clínica  



## ⚙️ Instalação e Uso

1. **Pré-requisitos**
   - Clonar o repositório:
     ```bash
     git clone https://github.com/JotaPablo/ClinicSync.git
     cd ClinicSync
     ```
   - Instalar o **Visual Studio Code** com as extensões:
     - **C/C++**
     - **Raspberry Pi Pico SDK**
     - **Compilador ARM GCC**
     - **CMake Tools**

2. **Ajuste do caminho do FreeRTOS**
   - Abra o arquivo `CMakeLists.txt` na raiz do projeto e ajuste a variável `FREERTOS_KERNEL_PATH` para o diretório onde você instalou o FreeRTOS Kernel.  
     Exemplo:
     ```cmake
     set(FREERTOS_KERNEL_PATH "C:/FreeRTOS-Kernel")
     ```
     → Substitua `"C:/FreeRTOS-Kernel"` pelo caminho correto na sua máquina.

3. **Compilação**
   - Compile o projeto manualmente via terminal:
     ```bash
     mkdir build
     cd build
     cmake ..
     make
     ```
   - Ou utilize a opção **Build** da extensão da Raspberry Pi Pico no VS Code.

4. **Execução**
   - Conecte o Raspberry Pi Pico ao computador mantendo o botão **BOOTSEL** pressionado.
   - Copie o arquivo `.uf2` gerado na pasta `build` para o dispositivo `RPI-RP2`.