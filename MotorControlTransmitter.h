#include <Wire.h>

const int MOTOR_CONTROLLER_I2C_ADDRESS = 8;

// Function to send a 5-digit number as two bytes
void sendCostAndAlarmToDisplay(float cost, bool alarmMode) {

  int costAsInt = static_cast<int>(cost * 100);
  
  Wire.beginTransmission(MOTOR_CONTROLLER_I2C_ADDRESS);

  // Split the cost into a high byte and low byte
  byte highByte = (costAsInt >> 8) & 0xFF; // Higher byte
  byte lowByte = costAsInt & 0xFF;         // Lower byte

  Wire.write(highByte);            
  Wire.write(lowByte);  
  
  if (alarmMode) {

    Wire.write(1);

  } else {

    Wire.write(0);

  }             

  Wire.endTransmission();
                 
  Serial.print("Setting Cost To: ");
  Serial.println(cost);
}