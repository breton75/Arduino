/*
  Web Server

 A simple web server that shows the value of the analog input pins.
 using an Arduino Wiznet Ethernet shield.

 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13
 * Analog inputs attached to pins A0 through A5 (optional)

 created 18 Dec 2009
 by David A. Mellis
 modified 9 Apr 2012
 by Tom Igoe

 */

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
  
}

void loop()
{
  funcEncoderRead();
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
   current_turn = 0;
   current_angle = 0;
}

void funcEncoderRead(void)
{
  if(!started) return;
  
  int i = enc.read();
//  Serial.println(String(TURN_ANGLE));
     if(TURN_ANGLE)
     {
       current_angle += sq(i);
       Serial.println(String(current_angle * 18));
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
        
void funcTcpRead()
{
    // listen for incoming clients
  EthernetClient client = server.available();
  
  String s = "";
  if (client)
  {
    Serial.println("new client");

//      unsigned long avail = ;
      String cmd = "";      
      
      while(client.available())
      {
        char c = client.read();
        cmd += c;
        
        if(c == '\n') break;
      }
      
      cmd.trim();
      
      if(cmd == "") return;

      /* начинаем проверку команды */
      if(cmd.startsWith("SET:TURN:COUNT:"))
      {
        TURN_COUNT = getCmdValue(cmd);
        if(TURN_COUNT) TURN_ANGLE = 0;
        client.write("OK");
      }
      else if (cmd.startsWith("SET:TURN:ANGLE:"))
      {
        TURN_ANGLE = getCmdValue(cmd);
        if(TURN_ANGLE) TURN_COUNT = 0;
        client.write("OK");
      }
      
      else if (cmd.startsWith("SET:ENGINE:PW:"))
      {
        ENGINE_PW = getCmdValue(cmd);
        client.write("OK");
      }
      
      else if (cmd == "SET:DIRECTION:CLOCKWISE")
      {
        spin_direction_clockwise = true;
        client.write("OK");
      }
      
      else if (cmd == "SET:DIRECTION:ANTICLOCKWISE")
      {
        spin_direction_clockwise = false;
        client.write("OK");
      }      
      
      else if (cmd == "START")
      {
        start_engine();
        client.write("OK");
      }
      
      else if (cmd == "STOP")
      {
        stop_engine();
        client.write("OK");
      }
      
      else
        client.write("Unknown command");


    delay(1);
    client.stop();
    Serial.println("client disconnected");
        
  }
}

uint16_t getCmdValue(String cmd)
{
  String s_val = cmd.substring(cmd.lastIndexOf(":") + 1);
  uint16_t val = s_val.toInt();;

  return val;
  
}
