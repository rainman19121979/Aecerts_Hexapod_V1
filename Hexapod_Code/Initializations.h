Servo coxa1;
Servo femur1;
Servo tibia1;

Servo coxa2;
Servo femur2;
Servo tibia2;

Servo coxa3;
Servo femur3;
Servo tibia3;

Servo coxa4;
Servo femur4;
Servo tibia4;

Servo coxa5;
Servo femur5;
Servo tibia5;

Servo coxa6;
Servo femur6;
Servo tibia6;

// Servo-Arrays für optimierten Zugriff
Servo* coxaServos[6] = {&coxa1, &coxa2, &coxa3, &coxa4, &coxa5, &coxa6};
Servo* femurServos[6] = {&femur1, &femur2, &femur3, &femur4, &femur5, &femur6};
Servo* tibiaServos[6] = {&tibia1, &tibia2, &tibia3, &tibia4, &tibia5, &tibia6};

const int coxa1Pin = 22;
const int femur1Pin = 23;
const int tibia1Pin = 24;

const int coxa2Pin = 25;
const int femur2Pin = 26;
const int tibia2Pin = 27;

const int coxa3Pin = 28;
const int femur3Pin = 29;
const int tibia3Pin = 30;

const int coxa4Pin = 31;
const int femur4Pin = 32;
const int tibia4Pin = 33;

const int coxa5Pin = 34;
const int femur5Pin = 35;
const int tibia5Pin = 36;

const int coxa6Pin = 37;
const int femur6Pin = 38;
const int tibia6Pin = 39;

// Pin-Arrays für optimierten Zugriff
const int coxaPins[6] = {coxa1Pin, coxa2Pin, coxa3Pin, coxa4Pin, coxa5Pin, coxa6Pin};
const int femurPins[6] = {femur1Pin, femur2Pin, femur3Pin, femur4Pin, femur5Pin, femur6Pin};
const int tibiaPins[6] = {tibia1Pin, tibia2Pin, tibia3Pin, tibia4Pin, tibia5Pin, tibia6Pin};

const float a1 = 46;  //Coxa Length
const float a2 = 108; //Femur Length
const float a3 = 200; //Tibia Length   
float legLength = a1+a2+a3;

Vector3 currentPoints[6];
Vector3 cycleStartPoints[6];

Vector3 currentRot(180, 0, 180);
Vector3 targetRot(180, 0, 180);

float strideMultiplier[6] = {1, 1, 1, -1, -1, -1};
float rotationMultiplier[6] = {-1, 0, 1, -1, 0 , 1};

Vector3 ControlPoints[10];
Vector3 RotateControlPoints[10];

Vector3 AttackControlPoints[10];

bool servosAttached = false;

void attachServos(){
  for(int i = 0; i < 6; i++){
    coxaServos[i]->attach(coxaPins[i], 500, 2500);
    femurServos[i]->attach(femurPins[i], 500, 2500);
    tibiaServos[i]->attach(tibiaPins[i], 500, 2500);
  }
  servosAttached = true;
  Serial.println("Servos Attached");
}

void detachServos() {
  for(int i = 0; i < 6; i++){
    coxaServos[i]->detach();
    femurServos[i]->detach();
    tibiaServos[i]->detach();
  }
  servosAttached = false;
  Serial.println("Servos Detached");
}

