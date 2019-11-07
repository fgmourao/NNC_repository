/*
 * Núcleo de Neurociências - UFMG/Brazil (Universidade Federal de Minas Gerais)
 * 
 * Custom Conditioning Chamber for Classical Fear Conditioning
 * 
 * Sketch for Arduino Due (slave SPI). It receives parameters from ESP8266 (master SPI) and generates configurable sound and shock stimuli patterns.
 * Timer4 and Timer5 are used to control sound and shock temporal patterns.
 * Sound signal is produced by DAC1.
 * Shock signals are produced in 8 digital pins (configurable via code).
 * 
 * Authors: Paulo Aparecido Amaral Júnior  - amaraljr.paulo@gmail.com
 *          Flávio Afonso Gonçalves Mourão - mourao.fg@gmail.com
 *          Márcio Flávio Dutra Moraes     - mfdm@icb.ufmg.br
 *          
 *          2019
 */
 
/*############################################################################################################
        includes.
############################################################################################################*/

#include "Waveforms.h"
#include "DueTimer.h"
#include <SPI.h>
#include <avr/interrupt.h>

/*############################################################################################################
        Defines.
############################################################################################################*/

static BitOrder bitOrder = MSBFIRST;
static int pinSS = 10;    // Slave Select pin for Arduino.
static int pinABORT = 48; // Pin used to Abort experiment (Abort).
static int pinSOUND = 50; // Input pin by which ESP8266 controls SOUND.
static int pinSHOCK = 52; // Input pin by which ESP8266 controls SHOCK.
static int pinTrigger = 53; // Output pin used to generate a reference signal (a square wave) that represents sound modulator.


bool bpinSOUND = false;  // variable used to determine if it's a rising or a falling edge on pinSOUND.
bool bpinSHOCK = false;  // variable used to determine if it's a rising or a falling edge on pinSHOCK.
bool bBarStatus = false; // variable used to track the status of SHOCK (if shock is on or if it's off).

/*  ATENTION!!
    freqModulating must be less than (clock freqModulating)/maxSampleNumModulating
*/
/*############################################################################################################
        Global variables.
############################################################################################################*/
// Variables that ESP8266 will send to ArduinoDue via SPI bus.
/*  ATENTION!!
  carrier_freq must be less than (clock carrier_freq)/maxSampleNumCarrier
  REMEMBER! : maximum feasible frequency is given by the conversor limit: 125kHz
  */
float carrier_freq   = 1000;   // defauilt value for carrier frequency (in Hertz). This variable can be programmed by ESP8266 via webpage -> SPI.
float modulator_freq = 53.71; // default value for modulator frequency (in Hertz). This variable can be programmed by ESP8266 via webpage -> SPI.
float volume_sound   = 100;     // default value for volume sound (in percent). This variable can be programmed by ESP8266 via webpage -> SPI.
int iC = 0;   // Carrier wave counter.
int iM = 0;   // Modulating wave counter
uint32_t DACbuffer[maxSamplesNumCarrier * maxSamplesNumModulating]; // Template of the signal with modulator already calculated.

// SHOCK variables.
float pulse_shockHigh = 10; // default value for the time the bar is tied to GND (in ms). This variable can be programmed by ESP8266 via webpage -> SPI.
float pulse_shockLow  = 10;  // default value for the time that should be waited until reach next bar (in ms). This variable can be programmed by ESP8266 via webpage -> SPI.
float fDeltaT;

int ipulse_shockHigh; // in ms.
int ipulse_shockLow; // in ms.
int iDeltaT;

int iBarPin = 22;
static int iInitialBarPin = 23; // Initial pin for the first bar.
static int MaxNumBarPin   = 8;    // Number of bars.
static int iBarPinStep    = 2;     // Step for bar pins (so we can choose only odd or even pins, for example).
int iCountShockIntervals;

bool bSPIprocessdataOUT = false;
bool bSPIprocessdataIn  = false;
int iSPIdataCountIN  = 0;
int iSPIdataCountOUT = 0;
int iSPIdataMaxOUT   = 0;
byte byteBufferDataIN[500];
byte byteBufferDataOUT[500];
uint32_t uData; // A variable where we copy the content of the SPI Receive Data Register when we have an SPI interrupt.
uint32_t *pointerData;

String strTEMP;

/*############################################################################################################
        Function declarations.
############################################################################################################*/

void slaveBegin(uint8_t _pin);
void FUNCAO_INTERRUPT();
void ProcessIncomingData();
void bars(bool bStatusiBarPin);
void carrier();
void modulating();
void pinSOUND_rising();
void pinSOUND_falling();
void pinSHOCK_rising();
void pinSHOCK_falling();
void ProgramSound();
void ProgramShock();
void Abort();

/*############################################################################################################
        Setup
############################################################################################################*/

void setup()
{
  // Configure SPI communication.
  slaveBegin(pinSS);

  pinMode(pinABORT, INPUT);
  pinMode(pinSOUND, INPUT);
  pinMode(pinSHOCK, INPUT);
  pinMode(pinTrigger, OUTPUT);
  digitalWrite(pinTrigger, LOW);
  
  // Programming pins that control bars potentials
  for (int i = 0; i < MaxNumBarPin; i++)
  {
    pinMode((iInitialBarPin + i * iBarPinStep), OUTPUT);
    digitalWrite((iInitialBarPin + i * iBarPinStep), LOW);
  }

  // Timer4 produces carrier signal and timer5 modulates carrier signal
  Timer4.attachInterrupt(carrier);
  Timer5.attachInterrupt(modulating);

  analogWriteResolution(12);
  analogWrite(DAC1, 0); // Configure DAC communication with analogWrite's built-in parameters

  ProgramSound(); //Program template signal for SOUND.
}

/*############################################################################################################
        Loop
############################################################################################################*/

void loop()
{
  if (bpinSOUND != digitalRead(pinSOUND)) // check if pinSOUND has changed state.
  {
    if (bpinSOUND) pinSOUND_falling(); // check if it was a falling edge.
    else pinSOUND_rising();            

    bpinSOUND = !bpinSOUND; // toggle bpinsSOUND, since pinSOUND changed state.
  }

  if (bpinSHOCK != digitalRead(pinSHOCK)) // check if pinSHOCK has changed state.
  {
    if (bpinSHOCK) pinSHOCK_falling();    // check if it was a falling edge.
      else pinSHOCK_rising();             
    if (ipulse_shockHigh > 0) bpinSHOCK = !bpinSHOCK; // we check if pulse_shockHigh > 0 before writing TRUE/FALSE to bpinSHOCK. Else, bpinSHOCK will always be false.
  }

  if (bpinSHOCK) bars(bBarStatus); // if bpinSHOCK is TRUE, shock must be turned on (this is made in bars() function).

  //SerialComandCheck();
  if (bSPIprocessdataIn) ProcessIncomingData();

}

/*############################################################################################################
        slaveBegin. Programm Arduino Due as a slave SPI.
############################################################################################################*/

void slaveBegin(uint8_t _pin)
{
  SPI.begin(_pin);
  REG_SPI0_CR = SPI_CR_SWRST;      // reset SPI
  REG_SPI0_CR = SPI_CR_SPIEN;      // enable SPI
  REG_SPI0_MR = SPI_MR_MODFDIS;    // slave and no modefault
  REG_SPI0_CSR = SPI_MODE0|(0x80); // DLYBCT=0, DLYBS=0, SCBR=0, 16 bit transfer

  //REG_SPI0_IER=SPI_IER_RDRF; // Setting Interrupt Enable Flag Register to 1???

  delay(1000);
  
  // Configures interruption whenever master SPI sends a message.
  int intNum = digitalPinToInterrupt(_pin); // _pin (SPI slave select) is used to generate interrupts.
  if (intNum != NOT_AN_INTERRUPT) 
  {
    SPI.usingInterrupt(intNum); // uses slave select pin to trigger an interrupt service routine (FUNCAO_INTERRUPT).
    attachInterrupt(intNum, FUNCAO_INTERRUPT, RISING); // Slave select rising indicates that SPI transaction in done and data can be processed.
  }
}

/*############################################################################################################
    SPI Transmission/reception complete ISR
    
    SPI COMUNICATION FROM/TO MASTER: Our protocol.

        0xA0A0 is the command word sent by ESP8266 that indicates SPI data should be processed.
        0x0010 is the command word sent by ESP8266 that indicates control variables should be programmed.
          (values for carrier_freq, modulator_freq, pulse_shockHigh and pulse_shockLow should be sent by ESP8266 after 0x0010 command)
        0x10FF is the command word sent by Arduino indicating that the values for the control variables were all received.
        0x1000 is the command word sent by Arduino indicating that the values for the control variables were not received.
        0x1A00 is the command word sent by ESP8266 indicating to ABORT experiment.

    SPI Registers and Commands:
        REG_SPI0_SR - SPI Status Register
        REG_SPI0_RDR - SPI Receive Data Register
        SPI_SR_RDRF - Receive Data Register Full
        SPI_SR_TDRE - Transmit Data Register Empty
        
############################################################################################################*/

void FUNCAO_INTERRUPT()
{
    while ((REG_SPI0_SR & SPI_SR_RDRF) != 0) // wait for SPI data is available
      {
      uData = REG_SPI0_RDR; // The registry is a uint32_t 32bit word
        if (bitOrder == LSBFIRST) uData = __REV(__RBIT(uData));
      byteBufferDataIN[iSPIdataCountIN]= (uData & 0xFF);        // copy the first byte received to byteBufferDataIN.
      byteBufferDataIN[iSPIdataCountIN+1]= (uData & 0xFF00)>>8; // copy the second byte received to byteBufferDataIN.
      iSPIdataCountIN+=2;

      if (uData==0xA0A0) bSPIprocessdataIn=true; // status of bSPIprocessdataIn variable is checked in main loop. Fim de comando.

      if (((REG_SPI0_SR & SPI_SR_TDRE) != 0) && bSPIprocessdataOUT) // checking if we have to send data
        {
          pointerData= (uint32_t *) (byteBufferDataOUT +iSPIdataCountOUT);
          REG_SPI0_TDR = *pointerData;
          iSPIdataCountOUT+=2;
          if (iSPIdataCountOUT>=iSPIdataMaxOUT) // If at the end, finished reading.
              {
                iSPIdataCountOUT=0;
                bSPIprocessdataOUT=false;
                iSPIdataMaxOUT=0;
              }
        }
      }
}

void ProcessIncomingData()
{
  //Serial.println(iSPIdataCountIN);
  if ((byteBufferDataIN[0]==0x10)&&((byteBufferDataIN[1]==0x00))) // Command 0x0010
      // This is the command that tells Arduino to program experiment control variables.
    {
      float *fTemp= (float *)(byteBufferDataIN+2);
      carrier_freq=fTemp[0];    //+ 4bytes = 6
      modulator_freq=fTemp[1];  //+ 4bytes = 10
      pulse_shockHigh=fTemp[2]; //+ 4bytes = 14
      pulse_shockLow=fTemp[3];  //+ 4bytes = 18
      volume_sound=fTemp[4];    //+ 4bytes = 22
                                //+ 2bytes = 24. This is the process command: 0xA0A0;
      
      if (iSPIdataCountIN==24) // Succeeded in SPI communication (received expected number of bytes). Don't forget 0xA0A0 in order to process SPI data.
        {          
          byteBufferDataOUT[iSPIdataCountOUT]=0x10; // 0x10FF indicates success in receiving data.
          byteBufferDataOUT[iSPIdataCountOUT+1]=0xFF; // 0x10FF indicates success in receiving data.
          ProgramSound();
          //ProgramShock();
          //Serial.println(carrier_freq);
          //Serial.println(modulator_freq);
          //Serial.println(pulse_shockHigh);
          //Serial.println(pulse_shockLow);
          //Serial.println(volume_sound);
        } else 
          {
          byteBufferDataOUT[iSPIdataCountOUT]=0x10; // 0x1000 indicates fail in receiving data.
          byteBufferDataOUT[iSPIdataCountOUT+1]=0x00; // 0x1000 indicates fail in receiving data.
          }
      // Anyway,
      iSPIdataMaxOUT+=2;
      bSPIprocessdataOUT=true; 
      //Serial.println(String("Mandando Sinal de Volta: ") + iSPIdataMaxOUT);
    }

  if ((byteBufferDataIN[0]==0x1A)&&((byteBufferDataIN[1]==0x00))) // ABORT Command 0x1A00.
    {
        Abort();
    }
    
  //Independent of what was processed, erase and restart.
  iSPIdataCountIN=0;
  bSPIprocessdataIn=false;
}

/*############################################################################################################
        Functions for PIN control
############################################################################################################*/

  void pinSOUND_rising() // pinSOUND rises when ESP8266 indicates Arduino Due should start producing SOUND.
  {
    iC = 0; iM = 0;
  }
  
  void pinSOUND_falling() // pinSOUND falls when ESP8266 indicates Arduino Due should stop producing SOUND.
  {
    analogWrite(DAC1, 0);
    iC = 0; iM = 0;
    PIO_SetOutput( g_APinDescription[pinTrigger].pPort, g_APinDescription[pinTrigger].ulPin, LOW, 0, PIO_DEFAULT );
  }
  
  void pinSHOCK_rising() // pinSHOCK rises when ESP8266 indicates Arduino Due should start producing shock patterns (control signals).
  {
    iBarPin = iInitialBarPin;
    iCountShockIntervals = 0;
  }
  
  void pinSHOCK_falling() // pinSHOCK rises when ESP8266 indicates Arduino Due should stop producing shock patterns (control signals).
  {
    PIO_SetOutput(g_APinDescription[iBarPin].pPort, g_APinDescription[iBarPin].ulPin, LOW, 0, PIO_DEFAULT) ;
    iBarPin = iInitialBarPin;
    bBarStatus = false;
    iCountShockIntervals = 0;
  }

/*############################################################################################################
        Function: bars
############################################################################################################*/

  void bars(bool bStatusiBarPin) //When the interrupt is called those sequence of commands are executed
  {
  // This function will execute when the processor is idle ... within the loop ... the priority FOR TIMING is with the wave generation.
    // If Count is between zero and ipulse_shockHigh, SHOCK is suposed to be on.
    // If Count is between ipulse_shockHigh and ipulse_shockLow, SHOCK is suposed to be off.
    // If Count is greater than ipulse_shockHigh + ipulse_shockLow, should SHOCK next bar.
    if (bStatusiBarPin) // SHOCK is on => let's see if we have to turn it off.
    {
      if (iCountShockIntervals >= ipulse_shockHigh)
      {
        PIO_SetOutput( g_APinDescription[iBarPin].pPort, g_APinDescription[iBarPin].ulPin, LOW, 0, PIO_DEFAULT );
        bBarStatus = false;
      }
  
    } else // SHOCK is off => let's see if we have to turn it on.
    {
      if ((iCountShockIntervals >= 0) && (iCountShockIntervals<ipulse_shockHigh))
      {
        PIO_SetOutput( g_APinDescription[iBarPin].pPort, g_APinDescription[iBarPin].ulPin, HIGH, 0, PIO_DEFAULT );
        bBarStatus = true;
      }
      
      // SHOCK is off => let's see if we have to change to next bar.
      if (iCountShockIntervals >= (ipulse_shockHigh + ipulse_shockLow))
          {
            iCountShockIntervals = 0;
            iBarPin += iBarPinStep;
            // If we were in the last bar, return to the first one.
              if (iBarPin == (iInitialBarPin + MaxNumBarPin * iBarPinStep)) iBarPin = iInitialBarPin; 
          }
    }
  }

/*############################################################################################################
        Function: carrier
############################################################################################################*/

void carrier()
{
  // note that the DACbuffer was already calculated in ProgramSound(). Here, we only check which value we have to write on DAC.
  if (bpinSOUND) dacc_write_conversion_data(DACC_INTERFACE, DACbuffer[iC + iM * maxSamplesNumCarrier]); // writes data directly on the pin (analogWrite is too slow); with analogWrite() the maximum frequency achieved was 28kHz, whereas that implementation can reach up to 125kHz
  iC++;
  // check if carrier completed one period.
    if (iC == maxSamplesNumCarrier) iC = 0;
  // 
  if (bpinSHOCK) iCountShockIntervals += iDeltaT; // We are using the same clock to count shock and sound intervals.

  //IFs must be only bools or INTs ... working with floats delays things
}

/*############################################################################################################
        Function: modulating
############################################################################################################*/

void modulating()
{
  // The next 2 if's below are used to generate a trigger sinal that will be used as a reference for the modulation phase.
    if ((iM == 0) && (bpinSOUND)) PIO_SetOutput( g_APinDescription[pinTrigger].pPort, g_APinDescription[pinTrigger].ulPin, LOW, 0, PIO_DEFAULT );
    if ((iM == (maxSamplesNumModulating/2)) && (bpinSOUND)) PIO_SetOutput( g_APinDescription[pinTrigger].pPort, g_APinDescription[pinTrigger].ulPin, HIGH, 0, PIO_DEFAULT );
  iM++;
  if (iM == maxSamplesNumModulating) iM = 0;// Modulator completes one period
}

/*############################################################################################################
        Function: ProgramSound

        This is where we program a sound buffer that will be played during experiment.

        Timer4 is used to control the carrier wave and the shock.
        Timer5 is used to control trigger wave.
        
############################################################################################################*/

void ProgramSound()
{
  //Stop Everything
  Timer4.stop();
  Timer5.stop();
  bpinSOUND = false;

  // First program sound buffer with new parameters.
  for (int i = 0; i < maxSamplesNumModulating; i++)
    for (int j = 0; j < maxSamplesNumCarrier; j++)
      // check if sound is modulated of if it's a pure tune.
      if (modulator_freq > 0) DACbuffer[j + (i * maxSamplesNumCarrier)] = ((1 + waveformsTableCarrier[j] * waveformsTableModulating[i]) * 4095 * volume_sound/100) / 2;
      else DACbuffer[j + (i * maxSamplesNumCarrier)] = ((1 + waveformsTableCarrier[j]) * 4095 * volume_sound/100) / 2;

  iC = 0;
  iM = 0;
  fDeltaT = (1 / (carrier_freq * maxSamplesNumCarrier)) * 1000; // Pacote de cada Delta T do interrupt de SOM ... Em millisegundos.

  float fTemp;
  fTemp = fDeltaT * 1000; // In multiples of 10 microsseconds;
  iDeltaT = floor(fTemp);
  iCountShockIntervals = 0;

  fTemp = pulse_shockHigh * 1000; // In multiples of 10 microsseconds;
  ipulse_shockHigh = floor(fTemp);

  fTemp = pulse_shockLow * 1000; // In multiples of 10 microsseconds;
  ipulse_shockLow = floor(fTemp);

  // The line below can be a problem if carrier_freq is zero. Shock will have no clock to control its temporal patterns.
  Timer4.setFrequency(carrier_freq * maxSamplesNumCarrier);
  if (modulator_freq > 0)
  {
    Timer5.setFrequency(modulator_freq * maxSamplesNumModulating);
    Timer5.start();
  }
  Timer4.start();
}

/*############################################################################################################
        Function: Abort
############################################################################################################*/

void Abort()
{
  // Disable all timers.
}
