#include <LiquidCrystal.h>
#include <Servo.h>
#define POTENCIOMETRO A0
#define BOTAO_ENVIA 8
#define BOTAO_RESET 2
#define BUZZER 9
#define FOTO_RES A1
#define CONST_LUZ 110
#define PINO_SERVO 3
#define SERVO_ABERTO 90
#define SERVO_FECHADO 0

LiquidCrystal lcd(12,11,7,6,5,4);
Servo servo;
bool tranca = false;
bool botao_anterior = false;
char tranca_str[8];
int senha_pos = 0;
int senha_cofre[] = {6,9,6,9};
int senha_digitada[5];
int valor_pot = 0;
int valor_map = 0;
int tentativas = 0;
volatile bool resetar = false;

void ISR_reset(){
  resetar = true;
}

void status_trava(bool aberto){
  if(aberto){
    servo.write(SERVO_ABERTO);
  } else {
    servo.write(SERVO_FECHADO);
  }
}

void verifica_luz(){
  int luz = analogRead(FOTO_RES);
  if(luz < CONST_LUZ){
    sistema_bloqueado();
  }
}

void sistema_bloqueado(){
  status_trava(false);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("INVASOR");
  lcd.setCursor(0,1);
  lcd.print("DETECTADO!");
  for(int i = 0; i < 3; i++){
    tone(BUZZER, 150, 300); delay(400);
  }
  noTone(BUZZER);
  while(true){
    if(resetar) reset_sistema_ISR(); //reset via attach to inte
  }
}

void mensagem_inicial(){
  if (tranca){
    strcpy(tranca_str, "ABERTO");
  } else {
    strcpy(tranca_str, "FECHADO");
  }
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Seja bem-vindo!");
  lcd.setCursor(0,1);
  char buffer[16];
  sprintf(buffer, "STATUS: %s", tranca_str);
  lcd.print(buffer);
}

void mostrar_digito(int digito){
  lcd.clear();
  lcd.setCursor(0,0);
  char buffer[16];
  sprintf(buffer, "DIGITO: %d", digito);
  lcd.print(buffer);
  lcd.setCursor(0,1);
  for(int i = 0; i < senha_pos; i++){
    lcd.print("*");
  }
}

void buzzer_sucesso(){
  tone(BUZZER, 523, 100); delay(120);
  tone(BUZZER, 659, 100); delay(120);
  tone(BUZZER, 784, 100); delay(120);
  tone(BUZZER, 1047, 300); delay(350);
  noTone(BUZZER);
}

void buzzer_erro(){
  tone(BUZZER, 300, 200); delay(250);
  tone(BUZZER, 200, 400); delay(450);
  noTone(BUZZER);
}

bool valida_senha(){
  for(int i = 0; i < 4; i++){
    if(senha_digitada[i] != senha_cofre[i]){
      return false;
    }
  }
  return true;
}

void senha_correta(){
  tranca = true;
  status_trava(true);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Senha correta!");
  lcd.setCursor(0,1);
  lcd.print("Cofre aberto :)");
  buzzer_sucesso();
  delay(3000);
}

void senha_errada(){
  tentativas++;
  Serial.print("Tentativas restantes: ");
  Serial.println(3 - tentativas);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Senha errada!");
  lcd.setCursor(0,1);
  char buffer[16];
  sprintf(buffer, "Tentativas: %d/3", tentativas);
  lcd.print(buffer);
  buzzer_erro();
  senha_pos = 0;
  memset(senha_digitada, 0, sizeof(senha_digitada));
  if(tentativas >= 3){
    sistema_bloqueado();
  }
}

void envia_num(int digito){
  bool botao_atual = (digitalRead(BOTAO_ENVIA) == HIGH);
  if(botao_atual == true && botao_anterior == false){
    senha_digitada[senha_pos] = digito;
    Serial.print("Digito registrado: ");
    Serial.print(digito);
    Serial.print(" | Posicao: ");
    Serial.println(senha_pos);
    senha_pos++;
    if(senha_pos == 4){
      Serial.print("Senha digitada: ");
      for(int i = 0; i < 4; i++){
        Serial.print(senha_digitada[i]);
      }
      Serial.println();
      if(valida_senha()){
        senha_correta();
      } else {
        senha_errada();
      }
    }
  }
  botao_anterior = botao_atual;
}

void reset_sistema_ISR(){
  resetar = false;
  tranca = false;
  tentativas = 0;
  senha_pos = 0;
  memset(senha_digitada, 0, sizeof(senha_digitada));
  status_trava(false);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Sistema");
  lcd.setCursor(0,1);
  lcd.print("resetado!");
  delay(2000);
  mensagem_inicial();
}

void setup(){
  servo.attach(PINO_SERVO);
  status_trava(false); // começa fechado
  lcd.begin(16,2);
  mensagem_inicial();
  delay(2000);
  pinMode(BUZZER, OUTPUT);
  pinMode(BOTAO_ENVIA, INPUT);
  pinMode(BOTAO_RESET, INPUT);
  attachInterrupt(digitalPinToInterrupt(BOTAO_RESET), ISR_reset, RISING);
  
  Serial.begin(9600);
}

void loop(){
  if(resetar) reset_sistema_ISR();
  verifica_luz();
  if(tranca) return;
  status_trava(false);
  valor_pot = analogRead(POTENCIOMETRO);
  valor_map = map(valor_pot, 0, 1023, 0, 9);
  mostrar_digito(valor_map);
  envia_num(valor_map);
  delay(50);
}
