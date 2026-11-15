# Gerador de Sinais Sweep com DAC, DMA e Timer 6 (STM32H7)

Firmware em C desenvolvido para microcontroladores **STM32H7**, implementando a geração contínua de ondas senoidais via **DAC (Digital-to-Analog Converter)** atualizadas por **DMA** acionado por hardware pelo **Timer 6**, com uma máquina de estados para execução de **Sweep Linear de Frequência** em múltiplos estágios em loop infinito.

---

## 📋 Funcionalidades

- **Geração de Senoide por DMA/DAC:** Utiliza uma tabela de 128 pontos de 8 bits transferidos via DMA para o DAC sem sobrecarregar a CPU.
- **Varredura de Frequência (Sweep) Automática:** O sistema executa um perfil programado de 5 estágios sequenciais cobrindo frequências de **5 Hz até 2 kHz** ao longo de um ciclo total de 20 minutos.
- **Controle Preciso por Timer (TIM6):** Atualização dinâmica dos registradores `ARR` (Auto-Reload Register) e `PSC` (Prescaler) do Timer 6 para rampa linear exata de frequência.
- **Gerenciamento de Cache (STM32H7):** Limpeza e sincronização do Data Cache (`SCB_CleanDCache_by_Addr`) para garantir leituras corretas do DMA na memória.

---

## 📊 Estrutura dos Estágios de Sweep

O perfil de frequência é executado de forma cíclica e contínua através de 5 estágios:

| Estágio | Frequência Inicial | Frequência Final | Duração | Prescaler (PSC) | Clock Efetivo |
| :---: | :---: | :---: | :---: | :---: | :---: |
| **1** | 5.0 Hz | 22.5 Hz | 6 minutos (360 s) | `9` | 20 MHz |
| **2** | 22.5 Hz | 110.0 Hz | 5 minutos (300 s) | `0` | 200 MHz |
| **3** | 110.0 Hz | 500.0 Hz | 4 minutos (240 s) | `0` | 200 MHz |
| **4** | 500.0 Hz | 900.0 Hz | 3 minutos (180 s) | `0` | 200 MHz |
| **5** | 900.0 Hz | 2000.0 Hz | 2 minutos (120 s) | `0` | 200 MHz |

*(Após o término do Estágio 5, o sistema reinicia automaticamente no Estágio 1).*

---

## 🛠️ Especificações Técnicas e Periféricos

- **Clock do Sistema (SYSCLK):** Configurado a 400 MHz (HSE Bypass com PLL).
- **Periférico DAC:** Canal configurado com buffer de saída ativado e gatilho de disparo por transbordamento do Timer 6 (`DAC_TRIGGER_T6_TRGO`).
- **Buffer de Onda:** Tabela estática de 128 pontos de senoide de 8 bits atenuada para evitar saturação do DAC.
- **Base de Tempo:** `HAL_GetTick()` utilizado para amostragem e cálculo de rampa a cada 100 ms no loop principal.

---

## ⚙️ Arquitetura do Software

1. **Inicialização (`main`):**
   - Configuração da MPU e ativação das Caches L1 (Instruction e Data Cache).
   - Inicialização da HAL, clocks do sistema (400 MHz) e LED de status.
   - Configuração do Timer 6 e inicialização do DAC com o primeiro estágio de 5 Hz.
2. **Loop Principal (`while(1)`):**
   - Executa continuamente a função `Processar_Sweep_Senoide()`.
   - A cada 100 ms, calcula a fração de tempo decorrida no estágio atual, determina a frequência instantânea da rampa linear e recalcula o valor do `ARR` do Timer 6.
   - Ao estourar a duração do estágio, avança para o próximo nível ou reinicia o ciclo completo.

---

## 📁 Estrutura do Projeto

```text
├── Src/
│   └── main.c              # Configuração do sistema, DAC, Timer 6 e lógica de Sweep
├── Inc/
│   └── main.h              # Definições de protótipos e includes da HAL
└── README.md               # Documentação do projeto




