#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <BleCombo.h>
//#include <BleMouse.h>
//#include <BleKeyboard.h>

//#define MOUSE_MIDDLE 4
//const uint8_t KEY_LEFT_CTRL = 0x80;
// Initialize BLE devices
//BleKeyboard bleKeyboard("Luva MICSA Keyboard");
//BleMouse bleMouse("Luva MICSA");
BleCombo bleCombo("Luva MICSA");

Adafruit_MPU6050 mpu;

// Pin Definitions
const int PINO_TOQUE_PINCA = 5;
const int PINO_FLEX_MEDIO  = 15;
const int PINO_SDA = 21;
const int PINO_SCL = 22;

// Calibration Parameters
int THRESHOLD_FLEX_MEDIO = 2000, MAX_FLEX = 1000, MIN_FLEX = 3000;
const float DEADZONE_GYRO = 0.20;
const float DEADZONE_ACC = 1.80;
const float SENSIVIDADE_ORBIT = 15.0;
const float SENSIVIDADE_PAN = 6.0;
const float SENSIVIDADE_ZOOM = 2.0;


float offsetAX = 0, offsetAY = 0;
float offsetGX = 0, offsetGY = 0;
bool mouseConnected = false;
bool keyboardConnected = false;

void calibrarSensores() {
  Serial.println("Calibrando... MANTENHA A LUVA PARADA!");
  const int leituras = 100;
  for (int i = 0; i < leituras; i++) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    offsetAX += a.acceleration.x;
    offsetAY += a.acceleration.y;
    offsetGX += g.gyro.x;
    offsetGY += g.gyro.y;
    delay(10);
  }
  offsetAX /= leituras;
  offsetAY /= leituras;
  offsetGX /= leituras;
  offsetGY /= leituras;
  Serial.println("Calibração Concluída!");
}

const int N = 5;

    float samples[N];
    int indexyl = 0;
    
    float movingAverage(float newValue)
    {
        samples[indexyl] = newValue;
        indexyl = (indexyl + 1) % N;
    
        float sum = 0;
        
        for(int i = 0; i < N; i++)
            sum += samples[i];
    
        return sum / N;
    }

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("--- Inicializando Luva Controladora BLE ---");

  // Initialize I2C
  Wire.begin(PINO_SDA, PINO_SCL);

  // Initialize MPU6050
  if (!mpu.begin()) {
    Serial.println("ERRO: Sensor MPU6050 não encontrado!");
    while (1) { delay(10); }
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  calibrarSensores();
  pinMode(PINO_TOQUE_PINCA, INPUT);
  pinMode(PINO_FLEX_MEDIO, INPUT);

  // Start BLE devices    
  /*Serial.println("Iniciando BLE Mouse...");
  bleMouse.begin();
  if(bleMouse.isConnected()){
      Serial.println("success!");
  }else{
    Serial.println("Fuck me");  
  }
  bleKeyboard.begin();
  Serial.println("Iniciando BLE Keyboard...");
  if(bleKeyboard.isConnected()){
      Serial.println("success!");
  }else{
    Serial.println("Fuck me");  
  }*/

  Serial.println("Inicializando Módulos...");
  bleCombo.begin();

  Serial.println("Aguardando conexão Bluetooth...");
  Serial.println("Procure por 'Luva MICSA' no Windows Bluetooth");
}

void loop() {
  // Check connection status
  /*if (!mouseConnected && bleMouse.isConnected()) {
    mouseConnected = true;
    Serial.println("Mouse BLE Conectado!");
  }*/

  /*static bool lastState = false;

    bool currentState = bleMouse.isConnected();
    
    if (currentState != lastState) {
        Serial.print("Mouse state changed to: ");
        Serial.println(currentState);
        lastState = currentState;
    }*/

  /*if (!keyboardConnected && bleKeyboard.isConnected()) {
    keyboardConnected = true;
    Serial.println("Teclado BLE Conectado!");
  }*/
  /*if(bleCombo.isConnected()){
      Serial.println("all done, go fuck yourself");
    }*/
  
    

  // Only process if at least one device is connected

  // Nah, process it whole
  if (bleCombo.isConnected()){//bleMouse.isConnected() || bleKeyboard.isConnected()) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    // Apply calibration
    float accX = a.acceleration.x - offsetAX;
    float accY = a.acceleration.y - offsetAY;
    float gyroX = g.gyro.x - offsetGX;
    float gyroY = g.gyro.y - offsetGY;

    bool pincaAtiva = (digitalRead(PINO_TOQUE_PINCA) == HIGH);
    int flexValue = analogRead(PINO_FLEX_MEDIO);
    
    int fling = movingAverage(flexValue);

    // 1. Check if an error or extreme stretch occurred, then reset safely
    if (MAX_FLEX - MIN_FLEX > 1050) {
        MAX_FLEX = fling + 100;
        MIN_FLEX = fling - 100; 
    } 
    // 2. Otherwise, perform normal boundary expansion
    else {
        if (fling > MAX_FLEX) {
            MAX_FLEX = fling;
        }
        if (fling < MIN_FLEX) {
            MIN_FLEX = fling;
        }
    }
    
    // 3. Update Threshold
    THRESHOLD_FLEX_MEDIO = (MAX_FLEX + MIN_FLEX) / 2;
   /* 
    if(flexValue > MAX_FLEX || (MAX_FLEX - MIN_FLEX > 950)){
        MAX_FLEX = flexValue;// + 150;
    }
    if(flexValue < MIN_FLEX || (MAX_FLEX - MIN_FLEX > 950)){
        MIN_FLEX = flexValue; // 150;  //((MAX_FLEX- 200) + flexValue)/2
    }
    THRESHOLD_FLEX_MEDIO = (MAX_FLEX + MIN_FLEX)/2;
    */
    bool medioFletido = (flexValue < THRESHOLD_FLEX_MEDIO);
    Serial.print("Min: ");Serial.print(MIN_FLEX);Serial.print(" Median: ");Serial.print(THRESHOLD_FLEX_MEDIO);Serial.print(" Max: ");Serial.print(MAX_FLEX);Serial.print(" Value: ");Serial.println(flexValue);

    // MODE 1: PAN (Translate) - Pinça + Dedo Médio Dobrado
    if (pincaAtiva && medioFletido) {
      bleCombo.press(KEY_LEFT_CTRL);
      bleCombo.pressMouse(MOUSE_MIDDLE);
      int moveX = 0, moveY = 0;
      if (abs(accX) > DEADZONE_ACC) moveX = -(int)(accX * SENSIVIDADE_PAN); 
      if (abs(accY) > DEADZONE_ACC) moveY = (int)(accY * SENSIVIDADE_PAN);

      if (moveX != 0 || moveY != 0) {
        bleCombo.move(moveX, moveY);
        Serial.print("PAN: dx="); Serial.print(moveX);
        Serial.print(" dy="); Serial.println(moveY);
      }
    }
    // MODE 2: ORBIT (Rotate) - Only Pinça
    else if (pincaAtiva) {
      bleCombo.release(KEY_LEFT_CTRL);
      bleCombo.pressMouse(MOUSE_MIDDLE);

      int rotX = 0, rotY = 0;
      if (abs(gyroX) > DEADZONE_GYRO) rotY = (int)(gyroX * SENSIVIDADE_ORBIT);
      if (abs(gyroY) > DEADZONE_GYRO) rotX = (int)(gyroY * SENSIVIDADE_ORBIT);

      if (rotX != 0 || rotY != 0) {
        bleCombo.move(rotX, rotY);
        Serial.print("ORBIT: rx="); Serial.print(rotX);
        Serial.print(" ry="); Serial.println(rotY);
      }
    }
    // MODE 3: ZOOM (aproximação) - only flexão
    else if (medioFletido) { 
      bleCombo.press(KEY_LEFT_SHIFT);
      bleCombo.pressMouse(MOUSE_MIDDLE);

      int moveX = 0, moveY = 0;
      //if (abs(accX) > DEADZONE_ACC) moveX = -(int)(accX * SENSIVIDADE_PAN);
      if (abs(accY) > DEADZONE_ACC) moveY = (int)(accY * SENSIVIDADE_ZOOM);
      /*int rotX = 0, rotY = 0;
      if (abs(gyroX) > DEADZONE_GYRO) rotY = (int)(gyroX * SENSIVIDADE_ORBIT);
      if (abs(gyroY) > DEADZONE_GYRO) rotX = (int)(gyroY * SENSIVIDADE_ORBIT); 

      if (rotX != 0 || rotY != 0) {
        bleCombo.move(rotX, rotY);
        Serial.print("ORBIT: rx="); Serial.print(rotX);moveX
        Serial.print(" ry="); Serial.println(rotY);
      }*/
      if (/*moveX != 0 || */moveY != 0) {
        bleCombo.move(0, moveY);
        Serial.print("ZOOM: dy="); Serial.print(moveY);
        //Serial.print(" dy="); Serial.println(moveY);
      }
    }
    // IDLE: Release all keysmoveXmoveX
    else {
      bleCombo.releaseAll();
    }
  } else {
    // Print status every 5 seconds while waiting for connectionmoveX
    static unsigned long lastMsg = 0;
    if (millis() - lastMsg > 5000) {
      Serial.println("Aguardando conexão Bluetooth...");
      lastMsg = millis();
    }
  } 
  //Serial.print("Mouse: ");moveXmoveXmoveX
  //Serial.print(bleMouse.isConnected());
  
  //Serial.print("  Keyboard: ");
  //Serial.println(bleKeyboard.isConnected());
  //Serial.println(analogRead(PINO_FLEX_MEDIO));
  delay(20); // Slightly increased for stability
}
