//Adding libraries
#include <UbidotsMicroESP8266.h>
#include <SparkFun_MAG3110.h>
 
//Instantiating a "MAG3110" (magnetic sensor)
MAG3110 mag = MAG3110(); //Instantiate MAG3110 (magnetic sensor)
 
#define TOKEN  "A1E-m6TFNlkJVzsYvmTgNdgo82hdKcRts9"  // Put here your Ubidots TOKEN
#define WIFISSID "BELL098" // Put here your Wi-Fi SSID
#define PASSWORD "35DCED9E" // Put here your Wi-Fi password
 
Ubidots client(TOKEN);
 
int ledPin = 2;        // GPIO2 of ESP8266
int relayPin = 13; //Pin for the relay signal
bool shutoffValveState;
int refreshRate = 12.5;  //Put refresh rate (milliseconds) here
 
//Creating function to get the total of a set of values to calibrate with
int getTotal(int base, int total) {
  total += base;
  return total;
}
 
//Creating function to create deadzone to prevent false alarms
int countIfChanging(int delta, int counter){
  if ((delta > 10) || (delta < -10)){
    counter++;
    return (counter);
  } else {
    return 0;
  }
}
 
//Creating function to wait 300x refresh rate before sounding alarm
int isChanging(int counter){
  if(counter > 300){
    return 1;
  } else {
    return 0;
  }
}
 
//Creating variables for getTotal
int calX, calY, calZ, totX, totY, totZ;
 
void setup() {
  Serial.begin(115200); //Enter upload speed here
  client.setDataSourceName("Water_meter"); //Enter name of device here
  client.wifiConnection(WIFISSID, PASSWORD); //Connecting to WiFi
 
  mag.initialize(); //Initializes the mag sensor
  digitalWrite(ledPin, LOW); // Turn on LED
  pinMode(relayPin, OUTPUT); // Setting relay pin to output to send instead of receive signals
  //Resetting total(axis) values
  totX = 0;
  totY = 0;
  totZ = 0;
  //Measuring 30 values to calibrate
  for(int j=0;j<30;j++){
    mag.triggerMeasurement(); //Triggers measurement of sensor; must instantiate command to initiate measurement
    mag.readMag(&calX, &calY, &calZ); //Set 3 sensor values (x, y, z) to 3 values entered
    //Finding total of each set of 30 values
    totX = getTotal(calX, totX);
    totY = getTotal(calY, totY);
    totZ = getTotal(calZ, totZ);
  }
  //Finding averages of values
  calX = totX/30;
  calY = totY/30;
  calZ = totZ/30;
}
 
int timeStamp = 0;
//Initializing magnetic axes as 0
int x = 0;
int y = 0;
int z = 0;
//Initializing other variables
int lastX, lastY, lastZ, xCounter, yCounter, zCounter;
//Initializing boolean variables for whether each variable is changing
 
void loop() {
  //pinMode(ledPin, OUTPUT);  //Setting LED to output
  //Setting the previous magnetic values to the current ones
  lastX = x;
  lastY = y;
  lastZ = z;
  if (millis() - timeStamp > 12.5) //Making sure only one measurement happens each second
  {
    timeStamp = millis(); //Keeping the 1 measurement/second
    mag.triggerMeasurement(); //Triggering measurement
  }
 
  if (mag.dataReady()) {  //Making sure the data has been collected before updating variables
    //Read the data
    mag.readMag(&x, &y, &z); //Setting variables x, y and z to the values collected by the sensor
  }
 
  //Calibrating x, y, and z values using previously collected variables
  x -= calX;
  y -= calY;
  z -= calZ;
 
  //Instantiating variables to record changes in x, y, and z
  int deltaX, deltaY, deltaZ;
 
  //Setting "change in" variables to the current value of the axis minus the previous value
  deltaX = x - lastX;
  deltaY = y - lastY;
  deltaZ = z - lastZ;
 
  //Using countIfChanging function to count consecutive seconds during which each axis is changing outside of deadzone
  xCounter = countIfChanging(deltaX, xCounter);
  yCounter = countIfChanging(deltaY, yCounter);
  zCounter = countIfChanging(deltaZ, zCounter);
  //Printing values for debugging reasons
  Serial.println(xCounter);
  Serial.println(yCounter);
  Serial.println(zCounter);
 
  //Printing values so you know what is happening
  Serial.print("X: ");
  Serial.print(x);
  Serial.print(", Y: ");
  Serial.print(y);
  Serial.print(", Z: ");
  Serial.println(z);
  Serial.print("Change in X: ");
  Serial.print(deltaX);
  Serial.print(", Change in Y: ");
  Serial.print(deltaY);
  Serial.print(", Change in Z: ");
  Serial.println(deltaZ);
  //Printing whether x, y, and z are changing
  if(isChanging(xCounter)){
    Serial.print("X Changing, ");
  } else {
    Serial.print("X Not Changing, ");
  }
  if(isChanging(yCounter)){
    Serial.print("Y Changing, ");
  } else {
    Serial.print("Y Not Changing, ");
  }
  if(isChanging(zCounter)){
    Serial.print("Z Changing");
  } else {
    Serial.println("Z Not Changing");
  }
  Serial.println("--------");
  shutoffValveState = client.getValue("5a9c7626c03f973522262884"); //Call value for whether the shutoff is turned on or off
  if(shutoffValveState == true){ //Checks whether the shutoff switch is turned on
    digitalWrite(relayPin, LOW); //Closes switch if shutoff switch is turned on, activating shutoff
  } else {
    digitalWrite(relayPin, HIGH); //Opens switch to save power when shutoff switch is turned off
  }
  //Adding to Ubidots (cloud)
  client.add("change-in-x", deltaX); //Add variable "x"
  client.add("change-in-y", deltaY); //Add variable "y"
  client.add("change-in-z", deltaZ); //Add variable "z"
  client.sendAll(true); //Send variables to client
  delay(refreshRate); //Refresh
}
