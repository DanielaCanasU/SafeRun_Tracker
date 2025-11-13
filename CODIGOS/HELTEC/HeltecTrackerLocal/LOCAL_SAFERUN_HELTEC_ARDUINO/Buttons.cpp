#include "Buttons.h"
#include "Arduino.h"

#include "AppState.h"

#include "Config.h"

Botones::Botones() {
  // Constructor implementation
}

void Botones::init(){
  pinMode(BTN_UP_PIN, INPUT_PULLUP);
  pinMode(BTN_DOWN_PIN, INPUT_PULLUP);
  pinMode(BTN_OK_PIN, INPUT_PULLUP);
  pinMode(BTN_BACK_PIN, INPUT_PULLUP);
  /*
  // Interrupciones botones
  attachInterrupt(digitalPinToInterrupt(BUTTON_LEFT), handleLeftInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(BUTTON_RIGHT), handleRightInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(BUTTON_SELECT), handleSelectInterrupt, CHANGE);

  leftButton = {0,0,false,false,0,0,0};
  rightButton = {0,0,false,false,0,0,0};
  selectButton = {0,0,false,false,0,0,0};
  */
  Serial.println("--------------------------------");
  Serial.println("BOTONES INICIADO CORRECTAMENTE");
  Serial.println("--------------------------------");
}