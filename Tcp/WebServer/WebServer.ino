#include <OneWire.h>
#include <DallasTemperature.h>

#include <SPI.h>
#include <Ethernet.h>
#include <iarduino_Encoder_tmr.h>   

// MAC address and IP
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(169, 254, 44, 44);

// Initialize the Ethernet server library
EthernetServer server(35580);

/* константа с номером вывода для силового ключа */
const uint8_t pinEnginePw = 9;
      uint8_t ENGINE_PW = HIGH;

/* константа с номером вывода для энкодера */
const uint8_t pinEncBTN = 4;

/* Определяем константу с указанием № вывода Arduino 
 к которому подключён выводы «A» и "B" энкодера */
const uint8_t   pinEncA = 7;
const uint8_t   pinEncB = 8;


/* переменные количества оборотов */
uint16_t TURN_COUNT = 0;
uint16_t current_turn = 0;

/* переменные угла поворота */
uint16_t TURN_ANGLE = 0;
uint16_t current_angle = 0;

/* напрвление вращения */
bool spin_direction_clockwise = true;
const uint8_t   pinRelay = 5;

// Подключаем библиотеку iarduino_Encoder_tmr для работы с энкодерами через аппаратный таймер 
iarduino_Encoder_tmr enc (pinEncA, pinEncB);

bool started = false;

/* Температура */
#define ONE_WIRE_BUS 6 // номер пина к которому подключен DS18B20

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// arrays to hold device address
DeviceAddress insideThermometer;

float current_temp = 0;
unsigned long timer = 0;

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }


  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
  
  pinMode(pinEnginePw,   OUTPUT);
  digitalWrite(pinEnginePw, LOW);
  
  enc.begin();   
  pinMode(pinEncBTN,   INPUT);
  
  pinMode(pinRelay,   OUTPUT);
  digitalWrite(pinRelay, LOW);
  
  sensors.begin();
  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0");
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures();
  
  timer = millis();
  
}

void loop()
{
  funcEncoderRead();
  funcTempRead();
  funcTcpRead();
}

void start_engine()
{
   current_turn = 0;
   current_angle = 0;
   
   digitalWrite(pinEnginePw, LOW);
   
   if(spin_direction_clockwise) digitalWrite(pinRelay, HIGH);
   else digitalWrite(pinRelay, LOW);
     
   digitalWrite(pinEnginePw, ENGINE_PW);
   
   started = true;
   
}

void stop_engine()
{
   digitalWrite(pinEnginePw, LOW);
   started = false;
   
   int i = enc.read();
}

void funcEncoderRead(void)
{
  if(!started) return;
  
  int i = enc.read();

  if(TURN_ANGLE)
  {
    current_angle += sq(i);
  Serial.println(String(current_angle));
    if(current_angle * 18 >= TURN_ANGLE)
      stop_engine();
         
  }
  else if(TURN_COUNT)
  {
    current_turn += sq(i);
         
    if(floor(current_turn / 20) >= TURN_COUNT)
      stop_engine();
  }
}

void funcTempRead()
{
  if(!started) return;
  if(millis() - timer < 1000) return;
    
  timer = millis();
  
  current_temp = sensors.getTempC(insideThermometer);
  
  sensors.requestTemperatures();
  Serial.println(String(current_temp));
}
        
void funcTcpRead()
{
    // listen for incoming clients
  EthernetClient client = server.available();
  
  if (client)
  {
    Serial.println("new client");

//      unsigned long avail = ;
      String cmd = "";      
      String cmdList = "";
      String state = "STATE;";
      
      while(client.available())
      {
        char c = client.read();
        cmdList += c;
        
        if(c == '\n') break;
      }

//String(45, HEX);
      
      /** разбираем список полученных команд **/
      while(cmdList.length() > 0)
      {
        int pos = cmdList.indexOf(';');
        if(pos == -1) {
          cmd = cmdList;
          cmdList = "";
        }
        else {
          cmd = cmdList.substring(0, pos);
          cmdList.remove(0, pos + 1);
        }
          
        cmd.trim();  
  
        Serial.println(cmd);
        

        /* проверяем очередную команду */
        if(cmd.startsWith("SET:TURN:"))
        {
          TURN_COUNT = getCmdValue(cmd);
          if(TURN_COUNT) TURN_ANGLE = 0;
          state += ("TURN:" + String(TURN_COUNT) + ';');
        }
        else if (cmd.startsWith("SET:ANGLE:"))
        {
          TURN_ANGLE = getCmdValue(cmd);
          if(TURN_ANGLE) TURN_COUNT = 0;
          state += ("ANGLE:" + String(TURN_ANGLE) + ';');
        }
      
        else if (cmd.startsWith("SET:ENGINE:"))
        {
          ENGINE_PW = getCmdValue(cmd);
          state += ("ENGINE:" + String(ENGINE_PW) + ';');
        }
      
        else if (cmd == "SET:DIRECTION:CLOCKWISE")
        {
          spin_direction_clockwise = true;
          state += "DIRECTION:CLOCKWISE;";
        }
      
        else if (cmd == "SET:DIRECTION:ANTICLOCKWISE")
        {
          spin_direction_clockwise = false;
          state += "DIRECTION:ANTICLOCKWISE;";
        }      
      
        else if (cmd == "START")
        {
          start_engine();
          state += "STARTED;";
        }
      
        else if (cmd == "STOP")
        {
          stop_engine();
          state += "STOPPED;";
        }
        
        else if (cmd == "STATE") {
          state += started ? "STARTED;" : "STOPPED;";
          state += ("CURRENT:TEMP:" + String(current_temp) + ';');
          state += ("CURRENT:TURN:" + String(floor(current_turn / 20)) + ';');
          state += ("CURRENT:ANGLE:" + String(current_angle * 18) + ';');
        }
      
        else {
          state = "UNKNOWN_COMMAND:" + cmd + ";";
          break;
        }
      }

    delay(1);
    
    if(state != "STATE;") 
      client.write(state.c_str(), state.length());

      
//    client.stop();
//    Serial.println("client disconnected");
        
  }
}

uint16_t getCmdValue(String cmd)
{
  String s_val = cmd.substring(cmd.lastIndexOf(":") + 1);
  uint16_t val = s_val.toInt();

  return val;
  
}
