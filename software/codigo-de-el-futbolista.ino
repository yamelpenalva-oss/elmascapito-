

#include <Ps3Controller.h>


const int PIN_AIN1 = 18;
const int PIN_PWMA = 4;
const int PIN_AIN2 = 5;
const int PIN_BIN1 = 19;
const int PIN_BIN2 = 21;
const int PIN_PWMB = 22;

const int PWM_FREQ        = 5000;  
const int PWM_RESOLUTION  = 8;      
const int CH_PWMA         = 0;      
const int CH_PWMB         = 1;      

const int DEADZONE        = 15;     
const int VEL_NORMAL_MAX  = 180;   
const int VEL_TURBO_MAX   = 255;    
const int VEL_CRUCETA     = 200;    


bool turboActivo   = false;
bool paroEmergencia = false;
bool ultimoEstadoCross = false; 
bool ultimoEstadoCircle = false; 
void motorA(int vel) {
  vel = constrain(vel, -255, 255);
  if (vel > 0) {
    digitalWrite(PIN_AIN1, HIGH);
    digitalWrite(PIN_AIN2, LOW);
  } else if (vel < 0) {
    digitalWrite(PIN_AIN1, LOW);
    digitalWrite(PIN_AIN2, HIGH);
  } else {
    digitalWrite(PIN_AIN1, LOW);
    digitalWrite(PIN_AIN2, LOW);
  }
  ledcWrite(CH_PWMA, abs(vel));
}


void motorB(int vel) {
  vel = constrain(vel, -255, 255);
  if (vel > 0) {
    digitalWrite(PIN_BIN1, HIGH);
    digitalWrite(PIN_BIN2, LOW);
  } else if (vel < 0) {
    digitalWrite(PIN_BIN1, LOW);
    digitalWrite(PIN_BIN2, HIGH);
  } else {
    digitalWrite(PIN_BIN1, LOW);
    digitalWrite(PIN_BIN2, LOW);
  }
  ledcWrite(CH_PWMB, abs(vel));
}


void detenerMotores() {
  digitalWrite(PIN_AIN1, LOW);
  digitalWrite(PIN_AIN2, LOW);
  digitalWrite(PIN_BIN1, LOW);
  digitalWrite(PIN_BIN2, LOW);
  ledcWrite(CH_PWMA, 0);
  ledcWrite(CH_PWMB, 0);
}

void driverEnable(bool habilitado) {
  digitalWrite(PIN_STBY, habilitado ? HIGH : LOW);
}


void aplicarZonaMuerta(int &valor) {
  if (abs(valor) < DEADZONE) valor = 0;
}

void manejarConStick() {
  
  int ejeY = -Ps3.data.analog.stick.ly; 
  int ejeX =  Ps3.data.analog.stick.lx; 

  aplicarZonaMuerta(ejeY);
  aplicarZonaMuerta(ejeX);

  if (ejeY == 0 && ejeX == 0) {
    detenerMotores();
    return;
  }

  int velMax = turboActivo ? VEL_TURBO_MAX : VEL_NORMAL_MAX;

  
  int velIzq = ejeY + ejeX;
  int velDer = ejeY - ejeX;

  
  velIzq = map(constrain(velIzq, -128, 127), -128, 127, -velMax, velMax);
  velDer = map(constrain(velDer, -128, 127), -128, 127, -velMax, velMax);

  motorA(velIzq); 
  motorB(velDer); 
}

void manejarConCruceta() {
  int velMax = turboActivo ? VEL_TURBO_MAX : VEL_CRUCETA;

  bool arriba   = Ps3.data.button.up;
  bool abajo    = Ps3.data.button.down;
  bool izquierda = Ps3.data.button.left;
  bool derecha  = Ps3.data.button.right;

  if (arriba && izquierda) {
    motorA(velMax / 3);
    motorB(velMax);
  } else if (arriba && derecha) {
    motorA(velMax);
    motorB(velMax / 3);
  } else if (abajo && izquierda) {
    motorA(-velMax / 3);
    motorB(-velMax);
  } else if (abajo && derecha) {
    motorA(-velMax);
    motorB(-velMax / 3);
  } else if (arriba) {
    motorA(velMax);
    motorB(velMax);
  } else if (abajo) {
    motorA(-velMax);
    motorB(-velMax);
  } else if (izquierda) {
    motorA(-velMax);
    motorB(velMax);
  } else if (derecha) {
    motorA(velMax);
    motorB(-velMax);
  } else {
    detenerMotores();
  }
}

bool stickActivo() {
  int ejeY = Ps3.data.analog.stick.ly;
  int ejeX = Ps3.data.analog.stick.lx;
  return (abs(ejeY) >= DEADZONE) || (abs(ejeX) >= DEADZONE);
}

bool crucetaActiva() {
  return Ps3.data.button.up || Ps3.data.button.down ||
         Ps3.data.button.left || Ps3.data.button.right;
}

void manejarBotonesEspeciales() {

  bool crossAhora = Ps3.data.button.cross;
  if (crossAhora && !ultimoEstadoCross) {
    turboActivo = !turboActivo;
    Serial.print("Turbo: ");
    Serial.println(turboActivo ? "ACTIVADO" : "desactivado");
  }
  ultimoEstadoCross = crossAhora;

  bool circleAhora = Ps3.data.button.circle;
  if (circleAhora && !ultimoEstadoCircle) {
    paroEmergencia = !paroEmergencia;
    Serial.print("Paro de emergencia: ");
    Serial.println(paroEmergencia ? "ACTIVADO (motores bloqueados)" : "liberado");
    if (paroEmergencia) {
      detenerMotores();
    }
  }
  ultimoEstadoCircle = circleAhora;
}

void onPs3Notify() {
  manejarBotonesEspeciales();

  if (paroEmergencia) {
    detenerMotores();
    return;
  }

  if (stickActivo()) {
    manejarConStick();
  } else if (crucetaActiva()) {
    manejarConCruceta();
  } else {
    detenerMotores();
  }
}

void onPs3Connect() {
  Serial.println(">> Mando PS3 CONECTADO");
  driverEnable(true);
  detenerMotores();
}

void onPs3Disconnect() {
  Serial.println(">> Mando PS3 DESCONECTADO - deteniendo motores por seguridad");
  detenerMotores();
  driverEnable(false);
}

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(PIN_AIN1, OUTPUT);
  pinMode(PIN_AIN2, OUTPUT);
  pinMode(PIN_BIN1, OUTPUT);
  pinMode(PIN_BIN2, OUTPUT);
  

  ledcSetup(CH_PWMA, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(PIN_PWMA, CH_PWMA);
  ledcSetup(CH_PWMB, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(PIN_PWMB, CH_PWMB);

  driverEnable(false); 
  detenerMotores();

  Ps3.attach(onPs3Notify);
  Ps3.attachOnConnect(onPs3Connect);
  Ps3.attachOnDisconnect(onPs3Disconnect);
  Ps3.begin();

}

void loop() {
  if (!Ps3.isConnected()) {
    detenerMotores();
  }
  delay(20);
}
