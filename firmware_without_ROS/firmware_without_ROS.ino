#define F_CPU 8000000L

#define EN_12V A3
#define OUT_A 9     // PWM output A
#define OUT_B 10    // PWM output B (not used for PWM control)
#define PWM_RESOLUTION 100  // PWM resolution (0-100%)

#define MIN_VOLTAGE 27
#define MAX_VOLTAGE 34

#define MIN_PWM (PWM_RESOLUTION * 0.8)  // 80% da resolução máxima
#define MAX_PWM PWM_RESOLUTION

uint8_t out_A = 0;  // PWM value for OUT_A

long last_update = millis();
long last_hb = millis();
float voltage = MIN_VOLTAGE;

void setup() {
  pinMode(EN_12V, OUTPUT);
  pinMode(OUT_A, OUTPUT);
  pinMode(OUT_B, OUTPUT);

  digitalWrite(OUT_A, LOW);
  digitalWrite(OUT_B, LOW);
  digitalWrite(EN_12V, HIGH);

  // Set up PWM for OUT_A
  TCCR1A = _BV(COM1A0) | _BV(COM1B0) | _BV(COM1A1) | _BV(COM1B1) | _BV(WGM11);
  TCCR1B = _BV(WGM13) | _BV(WGM12) | _BV(CS11) | _BV(CS10);
  ICR1 = PWM_RESOLUTION;

  setPWM(out_A);  // Initialize PWM output for OUT_A

  Serial.begin(57600);
}

void loop() {
  if (millis() - last_update >= 10000) {  // Aumenta 1V a cada 10 segundos
    last_update = millis();
    
    voltage += 1.0;
    
    if (voltage > MAX_VOLTAGE) {
      voltage = MIN_VOLTAGE;  // Reinicia quando passa do máximo
    }
    
    // Mapeia o valor da tensão para o valor de PWM entre 80% e 100% da resolução
    out_A = map(voltage, MIN_VOLTAGE, MAX_VOLTAGE, MIN_PWM, MAX_PWM);
    
    setPWM(out_A);  // Aplica o valor de PWM ajustado
  }

  if (millis() - last_hb >= 1000) {
    last_hb = millis();
  }
  delay(1);
}

/**
 * Define o valor de PWM para o pino OUT_A.
 */
void setPWM(uint8_t pwm_a) {
  if (pwm_a > PWM_RESOLUTION) {
    pwm_a = PWM_RESOLUTION;
  }

  // Ajusta a taxa de duty cycle de PWM para OUT_A
  OCR1A = PWM_RESOLUTION - pwm_a;
}

