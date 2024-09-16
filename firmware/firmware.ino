

extern "C" {
#include <llcp.h>
}

#include "msgs.h"
// Dados que estavam no "msgs.h", se for rodar no arduino direto sem o resto dos arquivos do projeto do github
// utilizar o trecho abaixo e comentar a linha 10
/*
#define CMD_MSG_ID 10
#define HEARTBEAT_MSG_ID 11

struct __attribute__((__packed__)) cmd_msg
{
  uint8_t  id;
  uint8_t  set_out_a;
  uint8_t  set_out_b;
  bool  set_out_vbat;
};

struct __attribute__((__packed__)) heartbeat_msg
{
  uint8_t  id;
  bool     is_ok;
  uint16_t cmds_received;
};  */

#define F_CPU 8000000L

#define EN_12V A3
#define OUT_A 9     // PWM output A
#define OUT_B 10    // PWM output B (not used for PWM control)
#define PWM_RESOLUTION 100  // PWM resolution (0-100%)

#define TX_BUFFER_LEN 255
uint8_t tx_buffer[TX_BUFFER_LEN];
LLCP_Receiver_t llcp_receiver;
uint16_t num_cmds_received = 0;
long last_hb = millis();

uint8_t out_A = 0;  // PWM value for OUT_A
uint8_t out_B = 0;  // Stored value for OUT_B (not used for PWM control)
bool out_VBAT = false;

void setup() {
  pinMode(EN_12V, OUTPUT);
  pinMode(OUT_A, OUTPUT);
  pinMode(OUT_B, OUTPUT); // OUT_B is still defined
  pinMode(OUT_VBAT, OUTPUT);
  digitalWrite(OUT_A, LOW);
  digitalWrite(OUT_B, LOW);
  digitalWrite(EN_12V, HIGH);
  digitalWrite(OUT_VBAT, LOW);

  // Set up PWM for OUT_A
  TCCR1A = _BV(COM1A0) | _BV(COM1B0) |  _BV(COM1A1) | _BV(COM1B1) | _BV(WGM11);
  TCCR1B = _BV(WGM13) | _BV(WGM12)
           | _BV(CS11) | _BV(CS10);
  ICR1 = PWM_RESOLUTION;

  setPWM(out_A);  // Initialize PWM output for OUT_A

  Serial.begin(57600);
  llcp_initialize(&llcp_receiver);
}

void loop() {
  if (receive_message()) {
    // Update only PWM output for OUT_A
    setPWM(out_A);
    // Set vbat based on the message received from the ROS driver
    digitalWrite(OUT_VBAT, out_VBAT);

    // OUT_B is received but not used for PWM control
  }

  if (millis() - last_hb >= 1000) {
    last_hb = millis();
    send_heartbeat();
  }
  delay(1);
}

/**
 * Receives a message through serial communication using the LLCP protocol.
 * The function processes the received message and updates the PWM output (OUT_A).
 *
 * @return true if a valid message is received, false otherwise.
 */
bool receive_message() {
  uint16_t msg_len;
  bool got_valid_msg = false;
  LLCP_Message_t* llcp_message_ptr;

  while (Serial.available() > 0) {
    bool checksum_matched;
    uint8_t char_in = Serial.read();
    // Process individual characters through the LLCP protocol
    if (llcp_processChar(char_in, &llcp_receiver, &llcp_message_ptr, &checksum_matched)) {
      if (checksum_matched) {
        switch (llcp_message_ptr->payload[0]) {
          case CMD_MSG_ID: {
              // Process command message
              cmd_msg* received_msg = (cmd_msg*)llcp_message_ptr;

              // Update the PWM value for OUT_A
              out_A = received_msg->set_out_a;

              // Receive and store the value for OUT_B, but don't use it for PWM
              out_B = received_msg->set_out_b;

              // Update the PWM value for =out_VBAT
              out_VBAT = received_msg->set_out_vbat;

              got_valid_msg = true;
              num_cmds_received++;
              break;
            }
        }
        return true;
      }
    }
  }
  return got_valid_msg;
}

/**
 * Sets the PWM output on pin OUT_A.
 *
 * @param pwm_a The duty cycle for OUT_A (0-100%).
 */
void setPWM(uint8_t pwm_a) {
  if (pwm_a > PWM_RESOLUTION) {
    pwm_a = PWM_RESOLUTION;
  }

  // Set PWM duty cycle for OUT_A
  OCR1A = PWM_RESOLUTION - pwm_a;
}

/**
 * Sends a heartbeat message over the serial line using the LLCP protocol.
 */
void send_heartbeat() {
  heartbeat_msg my_msg;
  uint16_t msg_len;

  // Fill the heartbeat message with data
  my_msg.id = HEARTBEAT_MSG_ID;
  my_msg.is_ok = true;
  my_msg.cmds_received = num_cmds_received;

  // Prepare and send the heartbeat message
  msg_len = llcp_prepareMessage((uint8_t*)&my_msg, sizeof(my_msg), tx_buffer);
  for (int i = 0; i < msg_len; i++) {
    Serial.write(tx_buffer[i]);
  }
}
