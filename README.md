# Semáforo Inteligente com Modo Noturno e Acessibilidade

## Objetivo

Este projeto implementa um sistema de semáforo inteligente utilizando FreeRTOS no microcontrolador RP2040, com foco em acessibilidade por meio de sinais visuais e sonoros. O projeto faz parte da atividade prática da disciplina de Sistemas Embarcados Multitarefas.

## Funcionalidades

- Dois modos de operação:
  - **Modo Normal**: Verde → Amarelo → Vermelho com sinais sonoros distintos para cada cor.
  - **Modo Noturno**: Luz amarela piscando com sinal sonoro lento.
- Alternância de modo pelo botão A.
- Feedback sonoro com buzzer para acessibilidade:
  - **Verde**: 1 beep curto a cada segundo.
  - **Amarelo**: Beeps rápidos intermitentes.
  - **Vermelho**: Tom contínuo (500ms ligado e 1.5s desligado).
  - **Noturno**: Beep lento a cada 2 segundos.
- Uso da matriz de LEDs e display OLED para representação visual do estado.

## Componentes Utilizados (BitDogLab)

- LED RGB
- Matriz de LED 5x5
- Display OLED SSD1306
- Buzzer
- Botões A e B

## Estrutura do Código

O código é baseado exclusivamente em tarefas do FreeRTOS, sem uso de filas ou semáforos. As principais tarefas incluem:

- Controle do semáforo (modo normal e noturno)
- Controle do buzzer
- Exibição no display OLED
- Controle da matriz de LEDs
- Leitura do botão para alternar modos

## Execução

1. Compile o código com o SDK do RP2040 e FreeRTOS.
2. Grave o firmware na placa BitDogLab (RP2040).
3. Ao iniciar, o sistema entra no Modo Normal.
4. Pressione o botão A para alternar para o Modo Noturno e vice-versa.

## Links

- **Vídeo de Demonstração**:
