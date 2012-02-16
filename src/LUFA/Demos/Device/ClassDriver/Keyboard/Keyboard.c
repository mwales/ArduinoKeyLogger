/*
             LUFA Library
     Copyright (C) Dean Camera, 2010.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2010  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Main source file for the Keyboard demo. This file contains the main tasks of
 *  the demo and is responsible for the initial application hardware configuration.
 */

#include "Keyboard.h"

/** Buffer to hold the previously generated Keyboard HID report, for comparison purposes inside the HID class driver. */
uint8_t PrevKeyboardHIDReportBuffer[sizeof(USB_KeyboardReport_Data_t)];

/**** My Code ****/

// Message structure
//  0xFF Sync Byte
//  0 for Up, 1 for Down
//  Keycode Byte
//  Modifier Byte


uint8_t LatchedMessage[4];
uint8_t IncomingMessage[4];
const int MAX_SIMULTANEOUS_KEYS = 6;
uint8_t KeysPressed[6];
uint8_t globalModCode;
int incomingMessageIndex;

#define KEY_UP 0
#define KEY_DOWN 1

static inline void Buttons_Init(void)
{
	memset(KeysPressed, 0, MAX_SIMULTANEOUS_KEYS);
	
}

//static inline uint8_t Buttons_GetStatus(void) ATTR_WARN_UNUSED_RESULT;
//static inline uint8_t Buttons_GetStatus(void)
//{
//	return (LatchedMessage[2]);
//}

static inline void Joystick_Init(void)
{
	Serial_Init(115200, true);
    UCSR1B = ((1 << RXCIE1) | (1 << TXEN1) | (1 << RXEN1));
}

static inline void removeKey(uint8_t keyCode)
{
   bool keyFound = false;
   for(int i = 0; i < (MAX_SIMULTANEOUS_KEYS - 1); i++)
   {      
      
      if (KeysPressed[i] == keyCode)
      {
         keyFound = true;
      } 
      
      if (keyFound)
      {
         // This character should be moved up from the last character
         KeysPressed[i] = KeysPressed[i+1];
         KeysPressed[i+1] = 0;
      }
   }
}

static inline void addKey(uint8_t keyCode, uint8_t modCode)
{
   for(int i = 0; i < MAX_SIMULTANEOUS_KEYS; i++)
   {
      if (KeysPressed[i] == 0)
      {
         // This is the first empty spot in the list
         KeysPressed[i] = keyCode;
         globalModCode = modCode;
         break;
      }
   }
}

static inline void processNewMessage(uint8_t upDownCode, uint8_t keyCode, uint8_t modCode)
{
   if (upDownCode == KEY_UP)
   {
      removeKey(keyCode);
   }
   else
   {
      addKey(keyCode, modCode);
   }
}

/** ISR to manage the reception of data from the serial port, placing received bytes into a circular buffer
 *  for later transmission to the host.
 */
ISR(USART1_RX_vect, ISR_BLOCK)
{
    uint8_t ReceivedByte = UDR1;
    
    IncomingMessage[incomingMessageIndex] = ReceivedByte;
    incomingMessageIndex++;

    if (incomingMessageIndex == 4)
    {
        //Received the whole message
        processNewMessage(IncomingMessage[1], IncomingMessage[2], IncomingMessage[3]);
        globalModCode = IncomingMessage[3];
        //LatchedMessage[1] = IncomingMessage[1];
        //LatchedMessage[2] = IncomingMessage[2];
        incomingMessageIndex = 0;
    }
    
    if ( (incomingMessageIndex == 1) && 
         (IncomingMessage[0] != 0xFF) )
    {
        incomingMessageIndex = 0;
    }
         
        

    //if ((USB_DeviceState == DEVICE_STATE_Configured) &&
	//    !RingBuffer_IsFull(&USARTtoUSB_Buffer)) {
	//RingBuffer_Insert(&USARTtoUSB_Buffer, ReceivedByte);
    //}
}

//static inline uint8_t Joystick_GetStatus(void) ATTR_WARN_UNUSED_RESULT;
//static inline uint8_t Joystick_GetStatus(void)
//{
//	return (LatchedMessage[1]);
//}

/**** End My Code ****/

/** LUFA HID Class driver interface configuration and state information. This structure is
 *  passed to all HID Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_HID_Device_t Keyboard_HID_Interface =
 	{
		.Config =
			{
				.InterfaceNumber              = 0,

				.ReportINEndpointNumber       = KEYBOARD_EPNUM,
				.ReportINEndpointSize         = KEYBOARD_EPSIZE,
				.ReportINEndpointDoubleBank   = false,

				.PrevReportINBuffer           = PrevKeyboardHIDReportBuffer,
				.PrevReportINBufferSize       = sizeof(PrevKeyboardHIDReportBuffer),
			},
    };

/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void)
{
	SetupHardware();

	//LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
	sei();

	for (;;)
	{
		HID_Device_USBTask(&Keyboard_HID_Interface);
		USB_USBTask();
	}
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware()
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/* Hardware Initialization */
	Joystick_Init();
	//LEDs_Init();
	Buttons_Init();
	USB_Init();
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
	//LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
	//LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= HID_Device_ConfigureEndpoints(&Keyboard_HID_Interface);

	USB_Device_EnableSOFEvents();

	//LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	HID_Device_ProcessControlRequest(&Keyboard_HID_Interface);
}

/** Event handler for the USB device Start Of Frame event. */
void EVENT_USB_Device_StartOfFrame(void)
{
	HID_Device_MillisecondElapsed(&Keyboard_HID_Interface);
}

/** HID class driver callback function for the creation of HID reports to the host.
 *
 *  \param[in]     HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in,out] ReportID    Report ID requested by the host if non-zero, otherwise callback should set to the generated report ID
 *  \param[in]     ReportType  Type of the report to create, either HID_REPORT_ITEM_In or HID_REPORT_ITEM_Feature
 *  \param[out]    ReportData  Pointer to a buffer where the created report should be stored
 *  \param[out]    ReportSize  Number of bytes written in the report (or zero if no report is to be sent
 *
 *  \return Boolean true to force the sending of the report, false to let the library determine if it needs to be sent
 */
bool CALLBACK_HID_Device_CreateHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo, uint8_t* const ReportID,
                                         const uint8_t ReportType, void* ReportData, uint16_t* const ReportSize)
{
	USB_KeyboardReport_Data_t* KeyboardReport = (USB_KeyboardReport_Data_t*)ReportData;

	//uint8_t JoyStatus_LCL    = Joystick_GetStatus();
	//uint8_t ButtonStatus_LCL = Buttons_GetStatus();

	uint8_t UsedKeyCodes = 0;
	int i;
	for(i = 0; i < MAX_SIMULTANEOUS_KEYS; i++)
   {
      if (KeysPressed[i] != 0)
      {
         // This is the first empty spot in the list
         KeyboardReport->KeyCode[UsedKeyCodes++] = KeysPressed[i];
      }
      else
      {
         break;
      }
   }

	//static int counter = 0;
	//counter++;

	//if (counter > 100)
	//{
   //   counter = 0;
      //KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_G + incomingMessageIndex;
      //KeyboardReport->KeyCode[UsedKeyCodes++] = HID_KEYBOARD_SC_A + i;
	//}
	
	
	

	if (UsedKeyCodes)
	  KeyboardReport->Modifier = globalModCode;
   
	*ReportSize = sizeof(USB_KeyboardReport_Data_t);
	return false;
}

/** HID class driver callback function for the processing of HID reports from the host.
 *
 *  \param[in] HIDInterfaceInfo  Pointer to the HID class interface configuration structure being referenced
 *  \param[in] ReportID    Report ID of the received report from the host
 *  \param[in] ReportType  The type of report that the host has sent, either HID_REPORT_ITEM_Out or HID_REPORT_ITEM_Feature
 *  \param[in] ReportData  Pointer to a buffer where the created report has been stored
 *  \param[in] ReportSize  Size in bytes of the received HID report
 */
void CALLBACK_HID_Device_ProcessHIDReport(USB_ClassInfo_HID_Device_t* const HIDInterfaceInfo,
                                          const uint8_t ReportID,
                                          const uint8_t ReportType,
                                          const void* ReportData,
                                          const uint16_t ReportSize)
{
	/*uint8_t  LEDMask   = LEDS_NO_LEDS;
	uint8_t* LEDReport = (uint8_t*)ReportData;

	if (*LEDReport & HID_KEYBOARD_LED_NUMLOCK)
	  LEDMask |= LEDS_LED1;

	if (*LEDReport & HID_KEYBOARD_LED_CAPSLOCK)
	  LEDMask |= LEDS_LED3;

	if (*LEDReport & HID_KEYBOARD_LED_SCROLLLOCK)
	  LEDMask |= LEDS_LED4;

	LEDs_SetAllLEDs(LEDMask);*/
}

