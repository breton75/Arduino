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
IPAddress ip(192, 168, 44, 44);

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

// Подключаем библиотеку iarduino_Encoder_tmr для работы с энкодерами через аппаратный таймер 
iarduino_Encoder_tmr enc (pinEncA, pinEncB);

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
  
}

void loop()
{
  funcEncoderRead();
  funcTcpRead();
}
        
void funcEncoderRead(void)
{
     if(TURN_ANGLE)
     {
       int i = enc.read();
       current_angle += sq(i);
       Serial.println(String(current_angle));
       if(current_angle * 18 >= TURN_ANGLE)
       {
         current_angle = 0;
         analogWrite(pinEnginePw, LOW);
        }
     }
     else if(TURN_COUNT)
     {
       int i = enc.read();
       current_turn += sq(i);
         
       if(floor(current_turn / 20) >= TURN_COUNT)
       {
         current_turn = 0;
         analogWrite(pinEnginePw, LOW);
       }
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
        TURN_ANGLE = 0;
        client.write("OK");
      }
      else if (cmd.startsWith("SET:TURN:ANGLE:"))
      {
        TURN_ANGLE = getCmdValue(cmd);
        TURN_COUNT = 0;
        client.write("OK");
      }
      
      else if (cmd.startsWith("SET:ENGINE:PW:"))
      {
        ENGINE_PW = getCmdValue(cmd);
        client.write("OK");
      }
      
      else if (cmd == "START")
      {
        analogWrite(pinEnginePw, ENGINE_PW);
        client.write("OK");
      }
      
      else if (cmd == "STOP")
      {
        analogWrite(pinEnginePw, LOW);
        client.write("OK");
      }
      
      else
        client.write("Unkcnown command");

    Serial.println("sssss");
    delay(10);
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
