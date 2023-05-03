//////////////////////////////////////////////
//        RemoteXY include library          //
//////////////////////////////////////////////

// RemoteXY select connection mode and include library
#define REMOTEXY_MODE__ESP32CORE_WIFI_CLOUD
#include <WiFi.h>
#include <RemoteXY.h>

// RemoteXY connection settings
#define REMOTEXY_WIFI_SSID "your SSID" /// put your credentials here
#define REMOTEXY_WIFI_PASSWORD "your password"/// put your credentials here
#define REMOTEXY_CLOUD_SERVER "cloud.remotexy.com"
#define REMOTEXY_CLOUD_PORT 6376
#define REMOTEXY_CLOUD_TOKEN "your API Key"/// put your credentials here

// wifi credentials
const char* ssid = REMOTEXY_WIFI_SSID;
const char* password = REMOTEXY_WIFI_PASSWORD;

// remoteXY vars
uint8_t stepSize = 1;
int16_t reqPos = 0;  // required position
int16_t limit = 270;
int16_t curPos = 0;  // current capacitor position

// RemoteXY configurate
#pragma pack(push, 1)
uint8_t RemoteXY_CONF[] =  // 192 bytes
  { 255, 4, 0, 14, 0, 185, 0, 16, 31, 1, 72, 20, 11, 19, 43, 43, 1, 6, 140, 38,
    0, 0, 0, 0, 0, 128, 132, 67, 0, 0, 0, 0, 1, 0, 3, 63, 12, 12, 1, 31,
    68, 101, 99, 114, 101, 97, 115, 101, 0, 129, 0, 3, 3, 56, 6, 6, 77, 97, 103, 76,
    111, 111, 112, 32, 67, 111, 110, 116, 114, 111, 108, 108, 101, 114, 0, 129, 0, 10, 91, 46,
    6, 6, 70, 105, 110, 101, 32, 65, 100, 106, 117, 115, 116, 109, 101, 110, 116, 0, 1, 0,
    48, 63, 12, 12, 12, 31, 73, 110, 99, 114, 101, 97, 115, 101, 0, 129, 0, 21, 66, 23,
    6, 6, 80, 111, 115, 105, 116, 105, 111, 110, 0, 129, 0, 8, 10, 44, 6, 6, 98, 121,
    32, 84, 101, 107, 109, 97, 107, 101, 114, 85, 75, 0, 1, 0, 14, 76, 12, 12, 2, 31,
    45, 45, 0, 1, 0, 35, 76, 12, 12, 4, 31, 43, 43, 0, 70, 16, 28, 36, 9, 9,
    26, 37, 0, 67, 4, 23, 53, 20, 5, 1, 16, 11 };

// this structure defines all the variables and events of your control interface
struct {
  uint8_t button_left;    // =1 if button pressed, else =0
  uint8_t button_right;   // =1 if button pressed, else =0
  uint8_t button_left2;   // =1 if button pressed, else =0
  uint8_t button_right2;  // =1 if button pressed, else =0

  // output variables
  int16_t circularbar_1;  // from 0 to limit
  uint8_t led_1;          // led state 0
  char text_1[11];        // string UTF8 end zero


  // other variable
  uint8_t connect_flag;  // =1 if wire connected, else =0
} RemoteXY;
#pragma pack(pop)

/////////////////////////////////////////////
//           END RemoteXY include          //
/////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//  Declare variables for the motor pins
//////////////////////////////////////////////////////////////////////////////
uint8_t motorPin1 = 27;  // Blue - 28BYJ48 pin 1 (Arduino)
uint8_t motorPin2 = 26;  // Pink - 28BYJ48 pin 2
uint8_t motorPin3 = 25;  // Yellow - 28BYJ48 pin 3
uint8_t motorPin4 = 33;  // Orange - 28BYJ48 pin 4
//                                                                                      // Red - 28BYJ48 pin 5 (VCC)
int motorSpeed = 3600;                                                               // 800 - 4800 defaultvariable to set stepper speed, ***higher is slower***
int lookup[8] = { B01000, B01100, B00100, B00110, B00010, B00011, B00001, B01001 };  // used for stepping the motor

//////////////////////////////////////////////////////////////////////////////
// Digital encoder
//////////////////////////////////////////////////////////////////////////////
int8_t pinA = 12;  // also knows as data
int8_t pinB = 14;  // also known as clk or clock
int8_t pinSW = 27;
int16_t bounceMsec = 20;

// track the last state of decoder inputs
bool stateA = true;
bool stateB = false;
uint32_t msec = 0;

//////////////////////////////////////////////////////////////////////////////
//  remoteXY handler, sub code generated by remoteXY with custom code added
//////////////////////////////////////////////////////////////////////////////
void RemoteXYHandler() {
  RemoteXY_Handler();

  RemoteXY.led_1 = 0;

  // right button pressed
  if (RemoteXY.button_right == 1) {
    reqPos = reqPos + (stepSize * 5);
    RemoteXY.led_1 = 1;
  }
  // left button pressed
  if (RemoteXY.button_left == 1) {
    reqPos = reqPos - (stepSize * 5);
    RemoteXY.led_1 = 1;
  }

  // right button2 pressed, fine control
  if (RemoteXY.button_right2 == 1) {
    reqPos = reqPos + stepSize;
    RemoteXY.led_1 = 1;
  }
  // left button2 pressed, fine control
  if (RemoteXY.button_left2 == 1) {
    reqPos = reqPos - stepSize;
    RemoteXY.led_1 = 1;
  }

  // limit positional values
  if (reqPos <= 0) {
    reqPos = 0;
  } else {
    if (reqPos >= limit) {
      reqPos = limit;
    }
  }
  Serial.println(reqPos);
  RemoteXY.circularbar_1 = reqPos;
  String str = String(reqPos);
  uint8_t len = str.length() + 1;
  char char_array[len];
  str.toCharArray(char_array, 11); // display on reqPos app
  //strcpy  (RemoteXY.text_1, str);
  sprintf(RemoteXY.text_1, char_array);
  
  Serial.println();
  delay(250);
}

//////////////////////////////////////////////////////////////////////////////
//  Manage the digital encoder values
//////////////////////////////////////////////////////////////////////////////
void getEncoderPosition() {

  bool decSW = digitalRead(pinSW);

  if (!decSW) {
    if (stepSize == 8) {
      stepSize = 1;
      Serial.println("Stepsize 1");
      delay(500);
    } else {
      if (stepSize == 1) {
        stepSize = 8;
        Serial.println("Stepsize 8");
        delay(500);
      }
    }
    return;
  }

  int limitDelay = 250;  //prevents false reads, hopefully
  bool decA = digitalRead(pinA);
  bool decB = digitalRead(pinB);

  if (stateA != decA && millis() > msec + bounceMsec) {
    if (stateB != decA) {
      reqPos += stepSize;
      if (reqPos >= limit) {
        reqPos = limit;
        delay(limitDelay);
      }
    } else {
      reqPos -= stepSize;
      if (reqPos < 0) {
        reqPos = 0;
        delay(limitDelay);
      }
    }

    msec = millis();  // get the time
    stateA = decA;    // store last decoder states
    stateB = decB;
  }
}

//////////////////////////////////////////////////////////////////////////////
//  run the motor from end to end and back. It is a stepper so it will not harm
//  it and no limit switches are needed
//  if it does not do 180 degrees then the value of countsPerHalfRev is wrong,
//  refer to documentation for your specific motor, try increasing or decreasing
//  as needed you can reverse antoclockwise and clockwise statements to ensure
//  the cap is closed
//////////////////////////////////////////////////////////////////////////////
void initMotor() {
  delay(500);
  Serial.println("Init ACW");
  reqPos = limit;
  moveMotor();
  delay(500);
  Serial.println("Init CW");
  reqPos = 0;
  moveMotor();
}


//////////////////////////////////////////////////////////////////////////////
// move the motor
//////////////////////////////////////////////////////////////////////////////
// set pins to ULN2003 high in sequence from 1 to 4
// delay "motorSpeed" between each pin setting (to determine speed)
void moveMotor() {
  if (curPos != reqPos) {  // if same then do nothing

    while (reqPos > curPos) {
      for (int i = 7; i >= 0; i--) {
        setOutput(i);
        delayMicroseconds(motorSpeed);
      }
      curPos++;
      Serial.print("curPos: ");
      Serial.println(curPos);
    }
    while (reqPos < curPos) {
      for (int i = 0; i < 8; i++) {
        setOutput(i);
        delayMicroseconds(motorSpeed);
      }
      curPos--;
      Serial.print("curPos: ");
      Serial.println(curPos);
    }
  }
}

//////////////////////////////////////////////////////////////////////////////
// drive the outputs sequentially to get the desired rotation
//////////////////////////////////////////////////////////////////////////////
void setOutput(int out) {
  digitalWrite(motorPin1, bitRead(lookup[out], 0));
  digitalWrite(motorPin2, bitRead(lookup[out], 1));
  digitalWrite(motorPin3, bitRead(lookup[out], 2));
  digitalWrite(motorPin4, bitRead(lookup[out], 3));
}

//////////////////////////////////////////////////////////////////////////////
// WiFi setup and report connected
//////////////////////////////////////////////////////////////////////////////
void get_network_info() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[*] Network information for ");
    Serial.println(ssid);
    Serial.println("[+] BSSID : " + WiFi.BSSIDstr());
    Serial.print("[+] Gateway IP : ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("[+] Subnet Mask : ");
    Serial.println(WiFi.subnetMask());
    Serial.println((String) "[+] RSSI : " + WiFi.RSSI() + " dB");
    Serial.print("[+] ESP32 IP : ");
    Serial.println(WiFi.localIP());
  }
}

//////////////////////////////////////////////////////////////////////////////
//  setup
//////////////////////////////////////////////////////////////////////////////
void setup() {
  // setup digital encoder pins as inputs, they will be HIGH by default
  pinMode(pinSW, INPUT_PULLUP);  // saves need for external pullup resistors
  pinMode(pinA, INPUT_PULLUP);
  pinMode(pinB, INPUT_PULLUP);

  //declare the motor pins as outputs
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(motorPin3, OUTPUT);
  pinMode(motorPin4, OUTPUT);

  Serial.begin(115200);
  initMotor();  //do this to run motor from end to end once to calibrate without using end stops as it won't hurt it.
  Serial.println("Motor Ready!");

  bool wifiEnabled = true;

  if (wifiEnabled) {
    WiFi.persistent(false);
    WiFi.begin(ssid, password);
    Serial.println("Connecting WiFI");

    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(100);
    }

    Serial.println("\nConnected to the WiFi network");
    get_network_info();
    Serial.println("All ready!");
  } else {
    Serial.println("WiFi disabled");
  }

  delay(1000);
  RemoteXY_Init();  // get ready to use remoteXY controls
}


//////////////////////////////////////////////////////////////////////////////
//  main prog loop
//////////////////////////////////////////////////////////////////////////////
void loop() {

  bool remote = false;

  RemoteXYHandler();  // use WiFi AP
  //getEncoderPosition();  // if using digital encoder

  moveMotor();
}
