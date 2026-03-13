// EEPROM

#include <EEPROM.h>

// Variabili
unsigned int motorSpeed = 0;

//Define Variables we'll be connecting to
double Setpoint = 30, Input, Output;

const int ADDR_SETPOINT = 0;   // Indirizzo per la temperatura
const int ADDR_MOTOR = 4;      // Indirizzo per la velocità motore (offset di 4 byte per float/int)
unsigned long lastStorageTime = 0;
bool pendingSave = false;

void loadSettings() {
  float savedSetpoint;
  int savedMotor;
  
  EEPROM.get(ADDR_SETPOINT, savedSetpoint);
  EEPROM.get(ADDR_MOTOR, savedMotor);

  // Verifica che i dati siano validi (non 0 o NaN) prima di caricarli
  if (savedSetpoint >= 0 && savedSetpoint <= 300) Setpoint = savedSetpoint;
  if (savedMotor >= 0 && savedMotor <= 255) motorSpeed = savedMotor;
}

void saveSettings() {
  EEPROM.put(ADDR_SETPOINT, Setpoint);
  EEPROM.put(ADDR_MOTOR, motorSpeed);
  pendingSave = false;
}

// PID
#include <PID_v1.h>

#define PWM_OUTPUT 3



//Specify the links and initial tuning parameters
double Kp = 40, Ki = 3, Kd = 80;
PID temperature_pid(&Input, &Output, &Setpoint, Kp, Ki, Kd, DIRECT);

#define temp_resistor A0

#define motor_forward_pwm 11
#define motor_backwards_pwm 5

bool heating_enabled = false;

void disable_heating() {
  heating_enabled = false;
  analogWrite(motor_forward_pwm, 0);

}

void enable_heating() {
  heating_enabled = true;
  temperature_pid.SetMode(AUTOMATIC);  // Does this reset?
  analogWrite(motor_forward_pwm, motorSpeed);

}


void set_motor_speed(unsigned int speed){
  motorSpeed = speed;
  analogWrite(motor_forward_pwm, motorSpeed);
}

// Display
#include <U8g2lib.h>
#include <Wire.h>

// Usa questa versione specifica per molti display 128x32 generici
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0);

// Se non va, prova questo (SH1106, a volte usato su moduli stretti)
// U8G2_SH1106_128X32_VISION_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// Buffer grafico (60 campioni per lasciare spazio a sinistra)
byte history[60] = {0}; 

// Variabili di stato per la logica visuale
unsigned long lastActivity = 0;  // Timestamp ultimo movimento encoder
const int activityTimeout = 3000; // Tempo (ms) prima di tornare alla visualizzazione reale
bool isSetting = false;          // Flag per switchare tra dial impostazione e reale

void updateHistory(float temp) {
  for (int i = 0; i < 59; i++) {
    history[i] = history[i + 1]; // Sposta tutto a sinistra
  }
  // Mappa la temperatura (es. 0-300°C) sui 32 pixel di altezza del display
  history[59] = map(constrain(temp, 0, 300), 0, 300, 31, 0); 
}

unsigned long lastUI = 0;
void drawUI() {
  u8g2.clearBuffer();

  // --- PARTE DESTRA: GRAFICO STORICO (60px) ---
  // Coordinate: x da 68 a 128
  for (int i = 0; i < 59; i++) {
    u8g2.drawLine(68 + i, history[i], 69 + i, history[i+1]);
  }

  // --- PARTE SINISTRA: IL DIAL (x=32, y=16, r=14) ---
  int centerX = 32;
  int centerY = 18;
  int radius = 16;

  if (!heating_enabled && millis() - lastActivity < activityTimeout) {
    // STATO IMPOSTAZIONE: Semicerchio superiore (0-300°C)
    int arcAngle = map(constrain(Setpoint, 0, 300), 0, 300, 0, 180);
    // Disegno arco semplice (approssimato con punti per velocità)
    for(int a = 180; a <= 180 + arcAngle; a += 5) {
      float rad = a * 0.01745;
      u8g2.drawPixel(centerX + cos(rad)*radius, centerY + sin(rad)*radius);
    }
    
    u8g2.setFont(u8g2_font_helvB12_tf);
    u8g2.setCursor(centerX - 12, centerY + 5);
    u8g2.print((int)Setpoint);
  } 
  else if (millis() - lastActivity < activityTimeout) {
    // STATO ATTIVO: Cerchio spesso (Temperatura Reale)
    //u8g2.drawCircle(centerX, centerY, radius);
    //u8g2.drawCircle(centerX, centerY, radius - 1); // Spessore
    
    // Semicerchio inferiore per Velocità Motore (0-255)
    int arcAngle = map(motorSpeed, 0, 255, 0, 180);
    // Disegno arco semplice (approssimato con punti per velocità)
    for(int a = 180; a <= 180 + arcAngle; a += 5) {
      float rad = a * 0.01745;
      u8g2.drawPixel(centerX + cos(rad)*radius, centerY + sin(rad)*radius);
    }

    u8g2.setFont(u8g2_font_helvB10_tf);
    u8g2.setCursor(centerX - 10, centerY + 4);
    u8g2.print((int)Input);
  } 
  else {
    // STATO SPENTO: Solo testo temperatura letta
    u8g2.setFont(u8g2_font_helvB14_tf);
    u8g2.setCursor(centerX - 14, centerY + 7);
    u8g2.print((int)Input);
    u8g2.setFont(u8g2_font_5x7_tr);
    u8g2.drawStr(centerX - 10, centerY + 14, "OFF");
  }

  // --- STATO TESTUALE LATO GRAFICO ---
  u8g2.setFont(u8g2_font_5x7_tr);
  u8g2.setCursor(68, 7);
  u8g2.print(heating_enabled ? "HEATING" : "IDLE");
  u8g2.setCursor(110, 7);
  u8g2.print((int)Input); u8g2.print("C");

  u8g2.sendBuffer();
}

// Encoder

#define ENC_A 7
#define ENC_B 8
#define ENC_BTN 6

int lastStateA;
unsigned long lastBtnPress = 0;
const int debounceDelay = 200; // Protezione contro i rimbalzi del tasto

void setupEncoder() {
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  pinMode(ENC_BTN, INPUT_PULLUP);
  lastStateA = digitalRead(ENC_A);
}

void handleEncoder() {
  // 1. GESTIONE ROTAZIONE
  int currentStateA = digitalRead(ENC_A);

  if (currentStateA != lastStateA && currentStateA == LOW) {
    lastActivity = millis(); // Risveglia la UI (mostra il dial)
    
    if (digitalRead(ENC_B) != currentStateA) {
      // Rotazione ORARIA
      if (heating_enabled) {
        motorSpeed = constrain(motorSpeed + 5, 0, 255);
      } else {
        Setpoint = constrain(Setpoint + 1, 0, 300);
      }

    } else {
      // Rotazione ANTIORARIA
      if (heating_enabled) {
        motorSpeed = constrain(motorSpeed - 5, 0, 255);
      } else {
        Setpoint = constrain(Setpoint - 1, 0, 300);
      }

    }

    set_motor_speed(motorSpeed);
          // All'interno di handleEncoder(), dove modifichi Setpoint o motorSpeed:
      pendingSave = true;
      lastStorageTime = millis();
  }
  lastStateA = currentStateA;

  // 2. GESTIONE PRESSIONE TASTO (Toggle ON/OFF)
  if (digitalRead(ENC_BTN) == LOW) {
    if (millis() - lastBtnPress > debounceDelay) {
      heating_enabled = !heating_enabled;
      
      if (heating_enabled) {
        enable_heating(); // La funzione che resetta il PID
      } else {
        disable_heating();
      }
      
      lastActivity = millis();
      lastBtnPress = millis();
    }
  }
}

void setup() {
  // Using AREF 3.3v
  analogReference(EXTERNAL);

  Serial.begin(115200);
  pinMode(PWM_OUTPUT, OUTPUT);
  analogWrite(PWM_OUTPUT, 0);

  setupEncoder();

  loadSettings();

  pinMode(motor_forward_pwm, OUTPUT);
  set_motor_speed(0);
  temperature_pid.SetMode(AUTOMATIC);

  u8g2.begin();


}

void loop() {

if (Serial.available() > 0) {
    char firstChar = Serial.peek(); // Sbircia il primo carattere

    if (firstChar == 'h') {
        Serial.read(); // Rimuove 'h' dal buffer
        enable_heating();
    } 
    else if (firstChar == 's') {
        Serial.read(); // Rimuove 's' dal buffer
        disable_heating();
    } 
    else if (isdigit(firstChar)) {
        // Se è un numero, leggi tutto l'intero (es. "260")
        Setpoint = Serial.parseInt();
    } 
    else {
        // Pulisce caratteri spazzatura (spazi, invio, ecc.)
        Serial.read();
    }

    lastActivity = millis();
}


  handleEncoder();

  Input = getCelsius(analogRead(temp_resistor));
  Serial.print("SetPoint:");
  Serial.print(Setpoint);
  Serial.print(",degrees:");
  Serial.print(Input);
  if (heating_enabled) {
    temperature_pid.Compute();
    analogWrite(PWM_OUTPUT, Output);
  } else{
    Output = 0;
    analogWrite(PWM_OUTPUT, 0);
  }
  Serial.print(",Output:");
  Serial.println(Output);

  if (millis() - lastUI > 200) {
    updateHistory(Input);
    drawUI();
    lastUI = millis();
  }

  // Salvataggio ritardato per proteggere la EEPROM
  /*
  if (pendingSave && (millis() - lastStorageTime > 5000)) {
    saveSettings();
  }
  */
}

double getCelsius(int rawADC) {
  double resistance = 100000.0 * (1023.0 / rawADC - 1.0);
  double temp;
  // Equazione di Steinhart-Hart semplificata (B-coefficient)
  temp = resistance / 100000.0;   // (R/Ro)
  temp = log(temp);               // ln(R/Ro)
  temp /= 3950.0;                 // 1/B * ln(R/Ro)
  temp += 1.0 / (25.0 + 273.15);  // + (1/To)
  temp = 1.0 / temp;              // Inverti
  temp -= 273.15;                 // Converti in Celsius
  return temp;
}
