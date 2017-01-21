#include <Wire.h>
int mpu = 0x68;
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;
const int led = 13;
double filteredAngle = 0;
const double dt = 0.033;
const double a = 0.96;
const int arraySize = 45;
double pitchAngle[arraySize];
double time[arraySize];
double* timePtr;
double gyAngle;
double accelAngle;
bool freezingStarts = false;
bool stillFreezing = false;
int pauseTime = 90;
int count = 0;
int adaptThreshCounter = 0;
const int threshSize = 150;
int adaptThreshArray[threshSize];
int threshSum = 0;
int thresh = 0;
const int MOTOR = 9;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println('Start');
  Wire.begin();
  Wire.beginTransmission(mpu);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
  pinMode(led, OUTPUT);
  pinMode(MOTOR, OUTPUT);
  
  for (int i = 0; i < arraySize; i++) {
   time[i] = i*dt; 
  }
  double* timePrt = (double*)time;
}

void loop() {
  // put your main code here, to run repeatedly:
  Wire.beginTransmission(mpu);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(mpu,14,true);
  AcX = Wire.read()<<8|Wire.read();
  AcY = Wire.read()<<8|Wire.read();
  AcZ = Wire.read()<<8|Wire.read();
  Tmp = Wire.read()<<8|Wire.read();
  GyX = Wire.read()<<8|Wire.read();
  GyY = Wire.read()<<8|Wire.read();
  GyZ = Wire.read()<<8|Wire.read();
  delay(33);
  
  GyZ = GyZ/131;
  gyAngle = filteredAngle + GyZ*dt;
  if (AcZ == 0) AcZ = 1;
  accelAngle = atan(sqrt((pow(AcX, 2) + pow(AcY, 2)))/AcZ);
  filteredAngle = a*gyAngle + (1-a)*accelAngle;
  // Shift over the elements in the array of angle values
  for (int i = 0; i < arraySize - 1; i++) {
   pitchAngle[i] = pitchAngle[i+1];
  }
  pitchAngle[arraySize-1] = filteredAngle;
  double* pitchAnglePtr = (double*)pitchAngle;
  
  count = peakCount(pitchAnglePtr, timePtr, arraySize);
  if (adaptThreshCounter < threshSize) {
    // add to the array
    adaptThreshArray[adaptThreshCounter] = count;
    Serial.println("Adding to the thresh array");
    adaptThreshCounter++;
  }
  else if (adaptThreshCounter == threshSize) {
   // determine the threshold
   for (int i = 0; i < threshSize; i++) {
     threshSum += adaptThreshArray[i];
   }
   int avg = threshSum / threshSize;
   thresh = 3*avg;
   thresh = 5;
   Serial.print("The new thresh is ");
   Serial.println(thresh);
   adaptThreshCounter++;
  }
  else {
    if (!stillFreezing){
      if (count > thresh) { //this 5 is just a guess
        stillFreezing = true;
        digitalWrite(led, HIGH);
        turnMotorOnOrOff(true);
      }
    }
    else{
      if (count < thresh - 1) {
        stillFreezing = false;
        digitalWrite(led, LOW);
        turnMotorOnOrOff(false);
      }
    }
    
    Serial.print(count);
    Serial.print(" ");
    Serial.print(GyZ);
    Serial.print(" ");
    Serial.print(stillFreezing);
    Serial.print(" ");
    Serial.print(AcZ);
    Serial.print(" ");
    Serial.print(accelAngle);
    Serial.print(" ");
    Serial.println(filteredAngle);
    count = 0;
  }
}

int peakCount(double* data_, double* trial_, int arrSize){
  double PksLocsGrid_[2][arrSize];
  int counter = 0;
	
  for(int i=1; i < arrSize; i++){
    if(data_[i] > data_[i-1] + 0.2 && data_[i] > data_[i+1] + 0.2) {
      PksLocsGrid_[0][counter]=data_[i];
      PksLocsGrid_[1][counter]=trial_[i];
      counter++;
    }
  }
  return counter;
}

void turnMotorOnOrOff(bool onOrOff) {
 int motorSpeed = 0;
 if (onOrOff) motorSpeed = 250;
 analogWrite(MOTOR, motorSpeed);
}
