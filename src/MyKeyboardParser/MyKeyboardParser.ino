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
int keysStored = 0;
bool hasWrapped = false;

void storeKey(uint8_t key);

void commandAndControl(char command)
{
  switch(command)
 {
   case '?':
    Serial1.println("Help screen");
    Serial1.println(" r = reset");
    Serial1.println(" i = info");
    break;
    
   case 'r':
     Serial1.println("Reset"); 
 } 
}

class KbdRptParser : public KeyboardReportParser
{
        void PrintKey(uint8_t mod, uint8_t key);
        
protected:
	virtual void OnKeyDown	(uint8_t mod, uint8_t key);
	virtual void OnKeyUp	(uint8_t mod, uint8_t key);
	virtual void OnKeyPressed(uint8_t key);
};

void KbdRptParser::PrintKey(uint8_t m, uint8_t key)	
{
    MODIFIERKEYS mod;
    *((uint8_t*)&mod) = m;
    Serial.print((mod.bmLeftCtrl   == 1) ? "C" : " ");
    Serial.print((mod.bmLeftShift  == 1) ? "S" : " ");
    Serial.print((mod.bmLeftAlt    == 1) ? "A" : " ");
    Serial.print((mod.bmLeftGUI    == 1) ? "G" : " ");
    
    Serial.print(" >");
    PrintHex<uint8_t>(key);
    Serial.print("< ");

    Serial.print((mod.bmRightCtrl   == 1) ? "C" : " ");
    Serial.print((mod.bmRightShift  == 1) ? "S" : " ");
    Serial.print((mod.bmRightAlt    == 1) ? "A" : " ");
    Serial.println((mod.bmRightGUI    == 1) ? "G" : " ");
};

void KbdRptParser::OnKeyDown(uint8_t mod, uint8_t key)	
{
    uint8_t buffer[3];
    
    buffer[0] = 0xFF;
    buffer[1] = KEY_DOWN;
    buffer[2] = key;
    Serial.write(buffer, 3);
    return;
    
}

void KbdRptParser::OnKeyUp(uint8_t mod, uint8_t key)	
{
    uint8_t buffer[3];
    
    buffer[0] = 0xFF;
    buffer[1] = KEY_UP;
    buffer[2] = key;
    Serial.write(buffer, 3);
    
    storeKey(key);
    
    return;
}

void storeKey(uint8_t key)
{
   if ( (key >= HID_KEYBOARD_SC_A) &&
        (key <= HID_KEYBOARD_SC_Z) )
   {
     Serial1.write('a' + key - HID_KEYBOARD_SC_A);     
   }
  else
 {
  
    Serial1.println("Invalid key pressed");
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

