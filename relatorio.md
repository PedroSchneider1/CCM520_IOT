# Relatório Técnico — Sistema de Controle de Acesso para Cofre Inteligente

**Disciplina:** CCM520  
**Projeto:** Controle de Acesso com Arduino  

---

## 1. Objetivo

O projeto tem como objetivo projetar e implementar um sistema embarcado de controle de acesso para um compartimento seguro, utilizando a plataforma Arduino. O sistema deve ser capaz de gerenciar a entrada de uma senha pelo usuário, validar o acesso, atuar sobre uma trava física representada por um servo motor e monitorar continuamente condições de segurança do ambiente.

---

## 2. Descrição Geral do Sistema

O sistema foi desenvolvido em linguagem C para Arduino e integra os seguintes subsistemas:

- **Interface de entrada:** potenciômetro analógico para seleção de dígitos e botão digital para confirmação.
- **Interface visual:** display LCD 16x2 para comunicação com o usuário em todas as etapas.
- **Interface de saída:** servo motor representando a trava do cofre e buzzer para feedback sonoro.
- **Subsistema de segurança:** sensor LDR (fotorresistor) para detecção de abertura indevida.
- **Subsistema de reset:** botão de emergência tratado via interrupção de hardware.

---

## 3. Recursos de Hardware

### 3.1 Microcontrolador

Utilizado o **Arduino Uno**.

### 3.2 Periféricos e Justificativas

| Periférico    | Pino(s)               | Justificativa                                  |
|---------------|-----------------------|------------------------------------------------|
| LCD 16x2      | 4, 5, 6, 7, 11, 12    | Modo 4 bits, não precisa de pinos digitais     |
| Potenciômetro | A0                    | Leitura analógica contínua, mapeada para 0–9   |
| Botão Enviar  | D8                    | Botão como input                               |
| Botão Reset   | D2                    | Pino com interrupção externa (INT0)            |
| Servo Motor   | D3 (PWM)              | Controle de posição angular via PWM            |
| Buzzer        | D9                    | Geração de tom via função `tone()`             |
| LDR           | A1                    | Leitura analógica de ambiente                  |

---

## 4. Recursos de Software e Bibliotecas

- **LiquidCrystal.h** (nativa Arduino): controle do display LCD em modo 4 bits.
- **Servo.h** (nativa Arduino): abstração do controle PWM do servo motor.
- **Funções nativas C:** `memset`, `strcpy`, `sprintf` para manipulação de strings e memória.
- **Funções Arduino:** `analogRead`, `digitalRead`, `tone`, `noTone`, `attachInterrupt`, `map`.

---

## 5. Funcionalidades Implementadas

### 5.1 Interface Visual (LCD)

O display LCD guia o usuário em todas as etapas do sistema. Foram implementadas as seguintes telas:

- **Tela inicial:** exibe "Seja bem-vindo!" e o status atual da trava (ABERTO/FECHADO).
- **Tela de digitação:** exibe o dígito selecionado pelo potenciômetro e asteriscos indicando os dígitos já confirmados.
- **Tela de sucesso:** confirma a abertura do cofre.
- **Tela de erro:** informa o número de tentativas usadas (formato `X/3`).
- **Tela de bloqueio:** exibe "INVASOR DETECTADO!" durante o estado de alarme.
- **Tela de reset:** confirma a reinicialização do sistema.

### 5.2 Entrada de Dados via Potenciômetro

A seleção de cada dígito da senha é realizada pelo potenciômetro conectado à entrada analógica A0. O valor bruto (0–1023) é mapeado para o intervalo inteiro 0–9 pela função `map()`. A confirmação de cada dígito é feita pelo botão de envio (pino D8).

A detecção do pressionamento do botão utiliza detecção de **borda de subida por software** (transição `LOW → HIGH`), comparando o estado atual com o estado anterior armazenado na variável `botao_anterior`. Esta abordagem elimina registros duplicados sem uso de outros temporizadores.

### 5.3 Validação de Senha

A senha é composta por 4 dígitos inteiros armazenados no array `senha_cofre[]`. A validação compara elemento a elemento o array `senha_digitada[]` com `senha_cofre[]` na função `valida_senha()`, retornando `true` somente se todos os 4 dígitos coincidirem.

A senha padrão definida é `{6, 9, 6, 9}` e pode ser modificada diretamente no código-fonte.

### 5.4 Atuação sobre a Trava (Servo Motor)

O servo motor representa a trava física do cofre. A função `status_trava(bool aberto)` centraliza o controle:

- **Cofre aberto:** servo posicionado a **90°** (`SERVO_ABERTO`).
- **Cofre fechado:** servo posicionado a **0°** (`SERVO_FECHADO`).

O servo é inicializado na posição fechada no `setup()` e retorna ao estado fechado em qualquer situação de erro ou bloqueio.

### 5.5 Feedback Sonoro (Buzzer)

Foram implementadas duas funções de feedback sonoro:

- **`buzzer_sucesso()`:** toca uma sequência de 4 notas ascendentes (Dó, Mi, Sol, Dó5) simulando um jingle de desbloqueio.
- **`buzzer_erro()`:** emite dois bipes graves e descendentes, sinalizando falha.
- **Alarme de invasão:** em `sistema_bloqueado()`, o buzzer toca 3 pulsos de 150 Hz durante 300 ms cada, simulando um alarme de segurança.

### 5.6 Sistema de Bloqueio por Tentativas

O contador `tentativas` é incrementado a cada senha incorreta. Ao atingir **3 tentativas**, a função `sistema_bloqueado()` é chamada, que:

1. Fecha o servo imediatamente.
2. Exibe a mensagem de alerta no LCD.
3. Dispara o alarme sonoro.
4. Entra em laço infinito, bloqueando qualquer nova entrada.

A saída desse estado só é possível via botão de reset (interrupção).

### 5.7 Monitoramento de Segurança por LDR

A função `verifica_luz()` é chamada a cada iteração do `loop()`. Ela realiza a leitura analógica do LDR no pino A1 e compara com o limiar `CONST_LUZ` (110). Se o valor lido for inferior ao limiar — indicando alta luminosidade, como quando alguém tenta abrir o cofre à força — e a senha correta não foi inserida, o sistema aciona `sistema_bloqueado()` imediatamente.

Este mecanismo simula o arrombamento ou abertura forçada do cofre.

### 5.8 Interrupção de Hardware para Reset

O botão de reset está conectado ao pino D2, que suporta a interrupção externa INT0 do ATmega328P. A interrupção é configurada para a **borda de subida** (`RISING`):

```c
attachInterrupt(digitalPinToInterrupt(BOTAO_RESET), ISR_reset, RISING);
```

A ISR `ISR_reset()` apenas levanta a flag `volatile bool resetar`. A lógica completa de reset é executada no contexto do `loop()`, pela função `reset_sistema_ISR()`. Esta separação é necessária porque funções como `delay()`, `tone()` e métodos do LCD utilizam temporizadores e interrupções internas do Arduino que **não devem ser chamados dentro de uma ISR**.

O uso de `volatile` na declaração da flag garante que o compilador não otimize a leitura da variável, respeitando a atualização feita pelo contexto de interrupção.

---

## 6. Fluxo do Software
```
[INÍCIO]
    │
    ▼
[Tela Inicial] ──── LDR detecta luz alta sem senha correta  ──► [SISTEMA BLOQUEADO]
    │
    ▼
[Aguarda dígito via potenciômetro]
    │
    ▼
[Botão Enviar pressionado] ──► registra dígito
    │
    ▼
[4 dígitos completos?]
    ├── Não → volta ao passo anterior
    └── Sim
         │
         ▼
    [Senha válida?]
         ├── Sim → [COFRE ABERTO] → servo 90° → aguarda reset
         └── Não → [tentativas < 3?]
                        ├── Sim → [Senha Errada] → volta ao início
                        └── Não → [SISTEMA BLOQUEADO]
```
### Controle de Fluxo Crítico

A flag `volatile bool resetar` e a função `verifica_luz()` são os dois mecanismos de interrupção do fluxo normal. Ambos têm prioridade sobre a lógica de entrada de senha, garantindo resposta rápida a eventos de segurança.

---

## 7. Estrutura do Código

```
projeto.c
│
├── setup()               → inicializa pinos, LCD, servo e interrupção
├── loop()                → ciclo principal: reset, LDR, leitura do pot, exibição e envio
│
├── envia_num()           → detecta borda de subida do botão e registra dígito
├── valida_senha()        → compara senha_digitada[] com senha_cofre[]
├── senha_correta()       → abre servo, exibe mensagem, toca melodia de sucesso
├── senha_errada()        → incrementa tentativas, exibe erro, aciona bloqueio se >= 3
│
├── verifica_luz()        → lê LDR; aciona sistema_bloqueado() se luz < CONST_LUZ
├── sistema_bloqueado()   → fecha servo, exibe alerta, loop de alarme infinito
│
├── status_trava()        → move servo para ABERTO (90°) ou FECHADO (0°)
├── mensagem_inicial()    → exibe tela de boas-vindas com status atual
├── mostrar_digito()      → exibe dígito atual e asteriscos no LCD
│
├── buzzer_sucesso()      → melodia de 4 notas ascendentes
├── buzzer_erro()         → dois bipes graves descendentes
│
├── ISR_reset()           → interrupção: levanta flag volatile resetar
└── reset_sistema_ISR()   → executa o reset completo do sistema
```

### Snippets Relevantes

**Mapeamento do potenciômetro para dígito:**
```c
valor_pot = analogRead(POTENCIOMETRO);        // leitura bruta: 0–1023
valor_map = map(valor_pot, 0, 1023, 0, 9);   // mapeado para: 0–9
```

**Detecção de borda de subida por software (debounce):**
```c
bool botao_atual = (digitalRead(BOTAO_ENVIA) == HIGH);
if (botao_atual == true && botao_anterior == false) {
    // borda detectada: registra o dígito
    senha_digitada[senha_pos] = digito;
    senha_pos++;
}
botao_anterior = botao_atual;
```

**Validação da senha elemento a elemento:**
```c
bool valida_senha() {
    for (int i = 0; i < 4; i++) {
        if (senha_digitada[i] != senha_cofre[i]) return false;
    }
    return true;
}
```

**Separação ISR / lógica de reset:**
```c
// ISR: apenas levanta a flag — nenhuma função bloqueante aqui
void ISR_reset() {
    resetar = true;
}

// Loop principal: executa o reset quando seguro
void loop() {
    if (resetar) reset_sistema_ISR();
    // ...
}
```

**Configuração da interrupção de hardware:**
```c
attachInterrupt(digitalPinToInterrupt(BOTAO_RESET), ISR_reset, RISING);
```
