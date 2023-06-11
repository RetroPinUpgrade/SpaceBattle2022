#ifndef WOS_H

#define WOS_MAJOR_VERSION 1
#define WOS_MINOR_VERSION 0


#define SW_SELF_TEST_SWITCH           0x7F
#define SOL_NONE                      0x0F
#define SWITCH_STACK_EMPTY            0xFF
#define CONTSOL_DISABLE_FLIPPERS      0x40
#define CONTSOL_DISABLE_COIN_LOCKOUT  0x20
#define WOS_MAX_LAMPS                 64



// Lamp Functions
void WOS_SetDimDivisor(byte level=1, byte divisor=2);
void WOS_ApplyFlashToLamps(unsigned long curTime);
void WOS_FlashAllLamps(unsigned long curTime); // Self-test function
void WOS_TurnOffAllLamps();
void WOS_SetLampState(int lampNum, byte s_lampState, byte s_lampDim=0, int s_lampFlashPeriod=0);

// Solenoid Functions
//   Solenoids
void WOS_PushToSolenoidStack(byte solenoidNumber, byte numPushes, boolean disableOverride = false);
void WOS_SetCoinLockout(boolean lockoutOn = false, byte solNum = 15);
void WOS_SetContinuousSolenoid(boolean solOn, byte solNum = 15);
void WOS_SetDisableFlippers(boolean disableFlippers = true);
void WOS_DisableSolenoidStack();
void WOS_EnableSolenoidStack();
boolean WOS_PushToTimedSolenoidStack(byte solenoidNumber, byte numPushes, unsigned long whenToFire, boolean disableOverride = false);
void WOS_UpdateTimedSolenoidStack(unsigned long curTime);

// Sound Functions
void WOS_SetSoundValueLimits(unsigned short lowerLimit, unsigned short upperLimit);
void WOS_PushToSoundStack(unsigned short soundNumber, byte numPushes);
boolean WOS_PushToTimedSoundStack(unsigned short soundNumber, byte numPushes, unsigned long whenToPlay);
void WOS_UpdateTimedSoundStack(unsigned long curTime);
#ifdef WOS_11_MPU
void WOS_PlayWOS11Sound(byte soundNum);
void WOS_PlayWOS11Music(byte songNum);
#endif
 
// Display Functions
byte WOS_SetDisplay(int displayNumber, unsigned long value, boolean blankByMagnitude=false, byte minDigits=2);
#ifdef WOS_11_MPU
byte WOS_SetDisplayText(int displayNumber, char *text, boolean blankByLength=true);
#endif 
void WOS_SetDisplayBlank(int displayNumber, byte bitMask);
void WOS_SetDisplayCredits(int value, boolean displayOn = true, boolean showBothDigits=true);
void WOS_SetDisplayMatch(int value, boolean displayOn = true, boolean showBothDigits=true);
void WOS_SetDisplayBallInPlay(int value, boolean displayOn = true, boolean showBothDigits=true);
void WOS_SetDisplayFlash(int displayNumber, unsigned long value, unsigned long curTime, int period=500, byte minDigits=2);
void WOS_SetDisplayFlashCredits(unsigned long curTime, int period=100);
void WOS_CycleAllDisplays(unsigned long curTime, byte digitNum=0); // Self-test function
byte WOS_GetDisplayBlank(int displayNumber);

// Switch Functions
byte WOS_PullFirstFromSwitchStack();
boolean WOS_ReadSingleSwitchState(byte switchNum);
boolean WOS_GetUpDownSwitchState();
void WOS_ClearUpDownSwitchState();

// Initialization and Utility
void WOS_InitializeMPU(byte creditResetButton = 0xFF); 
void WOS_SetBoardLEDs(boolean LED1, boolean LED2, byte BCDValue=0xFF);
void WOS_DataWrite(int address, byte data);
byte WOS_DataRead(int address);
boolean WOS_DiagnosticModeRequested();
byte WOS_GetSwitchesNow(byte switchCol);
void WOS_Update(unsigned long CurrentTime);

// EEProm Helper Functions
byte WOS_ReadByteFromEEProm(unsigned short startByte);
void WOS_WriteByteToEEProm(unsigned short startByte, byte value);
unsigned long WOS_ReadULFromEEProm(unsigned short startByte, unsigned long defaultValue=0);
void WOS_WriteULToEEProm(unsigned short startByte, unsigned long value);

#endif
#define WOS_H
