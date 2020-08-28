// Global functions.
  int parseStringToFloat(String str,char delimiter);
  int parseStringToFloat(String str,char delimiter, float *floatArray);

// Timers and their interrupt service routines.
  os_timer_t trial_timer, sound_timer, shock_timer;

  void isr_trial(void *pArg);
  void isr_sound(void *pArg);
  void isr_shock(void *pArg);

/* Function parseStringToFloat, used in 2 different ways (2 implementations below)
 *  
 *  
 */
   int parseStringToFloat(String str,char delimiter)
   {
      int floatsEscritos=0;
      for (int k=0;k<str.length();k++) if (str[k]==delimiter) floatsEscritos++;
      return (floatsEscritos+1);
   }

int parseStringToFloat(String str,char delimiter, float *floatArray)
{
  int floatsEscritos=0;
  int lastPos=0;
  for (int i=0;i<str.length();i++)
    if (str[i]==delimiter)
      {
        floatArray[floatsEscritos]=str.substring(lastPos,i).toFloat();
        lastPos=i+1;
        floatsEscritos++;
      }
  floatArray[floatsEscritos]=str.substring(lastPos).toFloat();
  return (floatsEscritos+1);
}

    
class ControleDueSkinner
{
  public:
  //Construtor
  ControleDueSkinner();

 //Class functions 
  void StartTrials();
  void Abort();
  bool ProgramaArduinoSPI(int iQualTrial);
  bool ProgramaVariaveis(int QuantosTrials, String strVar);
  bool bReady=0;
  
 //Control GPIOs
  const int pinABORT = 4;       // Pin used to tell Arduino to abort experiment (Abort).
  const int pinSOUND = 0;       // Pin used to tell Arduino that the sound (modulator + carrier) must be played.
  const int pinSOUNDnot = 5;    // PinSOUNDnot = not(pinSOUND)
  const int pinSHOCK = 2;       // Pin used to tell Arduino to shock rat's paw.
  const int pinRSTarduino = 16; // Pin used to reset Arduino.
  
 // Variables related to experiment parameters. Data comes from WEBPAGE.
  float BufferFloat[1800];    // a buffer to hold the values configured by user via webpage. Values are copied from strVar.
  float *silence;             // in seconds.
  float current_trial_length; // in seconds
  float *onset_shock;         // in seconds.
  float *onset_sound;         // in seconds.
  float *sound_duration;      // in seconds.
  float *shock_duration;      // in seconds.
  float *carrier_freq;        // in Hertz.
  float *modulator_freq;      // in Hertz.
  float *pulse_shockHigh;     // in ms. 
  float *pulse_shockLow;      // in ms.
  float *volume_sound;        // in percent (%).

  int iCountTrials=0;
  int iNumberOfTrials=0;
  
/*############################################################################################################ 
// Webpage content. -> Need to convert html to string and then past here below
############################################################################################################*/

 String PaginaHTML =  
"<!DOCTYPE html>\n\
<html>\n\
<body>\n\
\n\
<body style=\"background-color:powderblue;\">\n\
\n\
<center><h2>Conditioning Cage V1.0</h2></center>\n\
<!-- \n\
<button type=\"button\" onclick=\"loadDoc()\">Request data</button></br>\n\
-->\n\
\n\
Cage Control Password: <input id=\"senha\" type=\"password\" value=\"\" />\n\
<button type=\"button\" onclick=\"enviarESP()\">Send Parameters</button>\n\
<button type=\"button\" onclick=\"enviarESPSTART()\">START</button>\n\
<button type=\"button\" onclick=\"enviarESPABORT()\">ABORT</button></br>\n\
<!-- \n\
<button type=\"button\" onclick=\"testeESP()\">TESTANDO COISAS</button></br>\n\
-->\n\
<div id=\"entrada\">\n\
 <p>TRIAL CONFIGURATION</p></br>\n\
  Inter-trial silence or initial silence (seconds)<input id=\"Silence\" type=\"text\" value=\"\" /><br>\n\
  Sound onset (seconds)<input id=\"SoundOnset\" type=\"text\" value=\"\" />  \n\
  Sound duration (seconds)<input id=\"SoundDuration\" type=\"text\" value=\"\" /><br>\n\
  Shock onset (seconds)<input id=\"ShockOnset\" type=\"text\" value=\"\" />  \n\
  Shock duration (seconds)<input id=\"ShockDuration\" type=\"text\" value=\"\" /><br>\n\
  </br>\n\
  Carrier Frequency (Hz)<input id=\"CarrierFreq\" type=\"text\" value=\"\" />  \n\
  Modulator Frequency (Hz)<input id=\"ModFreq\" type=\"text\" value=\"\" /><br>\n\
  ShockPulse High (ms)<input id=\"ShockPulseHigh\" type=\"text\" value=\"\" />  \n\
  ShockPulse Low (ms)<input id=\"ShockPulseLow\" type=\"text\" value=\"\" /><br>\n\
  Sound Volume (%)<input id=\"SoundVolume\" type=\"text\" value=\"\" /><br>\n\
  </br>\n\
  <input id=\"SubmitTrial\" type=\"button\" onclick=\"adicionarTrial()\" value=\"Add Trial\" /><br>\n\
</div>\n\
<hr>\n\
<textarea id=\"expTexto\" rows=\"4\" cols=\"50\"></textarea>\n\
<button type=\"button\" onclick=\"programaExperimento()\">LOAD TRIAL(S)</button></br>\n\
<hr><br>\n\
<div id=\"saidaHTML\">\n\
</div>\n\
\n\
<p id=\"demo\"></p>\n\
 \n\
<script>\n\
//Variáveis Globais\n\
xhr=new XMLHttpRequest();\n\
\n\
var Trials = [];\n\
var varSet = [];\n\
\n\
varSet[0]=\"Silence\";\n\
varSet[1]=\"SoundOnset\";\n\
varSet[2]=\"SoundDuration\";\n\
varSet[3]=\"ShockOnset\";\n\
varSet[4]=\"ShockDuration\";\n\
varSet[5]=\"CarrierFreq\";\n\
varSet[6]=\"ModFreq\";\n\
varSet[7]=\"ShockPulseHigh\";\n\
varSet[8]=\"ShockPulseLow\";\n\
varSet[9]=\"SoundVolume\";\n\
for (var i=0;i<varSet.length;i++) Trials[i]=[];\n\
\n\
function adicionarTrial()\n\
{\n\
var indice=0;\n\
if (typeof Trials[0] != 'undefined') indice=Trials[0].length;\n\
for (var i=0;i<varSet.length;i++) Trials[i][indice]=document.getElementById(varSet[i]).value;\n\
ShowRecordedTrials();\n\
};\n\
\n\
function ShowRecordedTrials()\n\
{\n\
var strTemp=\"<center><p> ALL RECORDED TRIALS </p></center>\";\n\
\n\
for (var i=0;i<Trials[0].length;i++)\n\
  {\n\
  strTemp+=\"<hr>\";\n\
  strTemp+=\"<p>RECORDED TRIAL - \" + i + \" Start at (seg): \" + Trials[0][i] + \"s. \";\n\
  strTemp+=\"<br> To delete this trial click ====> <button id=\\\"reg\" + i + \"\\\" type=\\\"button\\\" onclick=\\\"ClickDeleteReg(this)\\\">Delete</button></p>\";\n\
\n\
  strTemp+=varSet[1] + \" = \" + Trials[1][i] + \" e \";\n\
  strTemp+=varSet[2] + \" = \" + Trials[2][i];\n\
  strTemp+=\"<br>\" + varSet[3] + \" = \" + Trials[3][i] + \" e \";\n\
  strTemp+=varSet[4] + \" = \" + Trials[4][i];\n\
  strTemp+=\"<br>\" + varSet[5] + \" = \" + Trials[5][i] + \" e \";\n\
  strTemp+=varSet[6] + \" = \" + Trials[6][i];\n\
  strTemp+=\"<br>\" + varSet[7] + \" = \" + Trials[7][i] + \" e \";\n\
  strTemp+=varSet[8] + \" = \" + Trials[8][i];\n\
  strTemp+=\"<br>\" + varSet[9] + \" = \" + Trials[9][i]; \n\
  strTemp+=\"<hr>\";\n\
  }\n\
//strTemp+=\"Sort = \" + Trials[0].sort();  ele faz o sort e fode tudo  \n\
document.getElementById(\"saidaHTML\").innerHTML = strTemp;\n\
\n\
strTemp=\"\";\n\
for (var j=0;j<varSet.length;j++)\n\
  {\n\
  //strTemp+=\"|\";\n\
  for (var i=0;i<((Trials[0].length)-1);i++)\n\
    strTemp+=Trials[j][i] + \";\";\n\
  strTemp+=Trials[j][i] + \"|\";\n\
  }\n\
strTemp=strTemp.substring(0, strTemp.length-1); \n\
document.getElementById(\"expTexto\").value=strTemp;\n\
}\n\
\n\
function programaExperimento()\n\
{\n\
//Trials = [];\n\
\n\
var linha=document.getElementById(\"expTexto\").value;\n\
var linha2=linha.split(\"|\");\n\
var linha3;\n\
\n\
for (var i=0; i<linha2.length ;i++)\n\
  {\n\
  linha3=linha2[i].split(\";\");\n\
  for (var j=0; j<linha3.length ;j++)\n\
    Trials[i][j]=linha3[j];\n\
  }\n\
  \n\
ShowRecordedTrials();\n\
}\n\
\n\
\n\
function ClickDeleteReg(ele)\n\
{\n\
  var iQual=ele.id.substring(3);\n\
  \n\
  if (confirm(\"Tem certeza? Voce vai deletar o TRIAL: \" + iQual))\n\
    for (var i=0;i<varSet.length;i++) Trials[i].splice(iQual,1);\n\
    \n\
  \n\
  ShowRecordedTrials();\n\
}\n\
\n\
function trataResposta()\n\
{\n\
if(xhr.readyState == 4 && xhr.status == 200) \n\
  {\n\
      alert(xhr.responseText);\n\
  }\n\
};\n\
\n\
\n\
function enviarESP() \n\
{\n\
//MUITO CUIDADO, FALTA SORT E OUTRAS COISITAS MAS\n\
var strTemp=\"\";\n\
\n\
\n\
strTemp+=\"ProgramarTrials=\" + Trials[0].length + \"&\";\n\
strTemp+=\"strTrials=\";\n\
\n\
for (var j=0;j<varSet.length;j++)\n\
  for (var i=0;i<Trials[0].length;i++)\n\
    strTemp+=Trials[j][i] + \";\";\n\
\n\
strTemp=strTemp.substring(0,strTemp.length-1);\n\
\n\
var strTemp2=\"Senha=\" + document.getElementById(\"senha\").value + \"&\";\n\
\n\
xhr.open(\"POST\", \"http://192.168.4.1/Programar\", true);\n\
xhr.setRequestHeader(\"Content-Type\", \"application/x-www-form-urlencoded; charset=UTF-8\");\n\
xhr.onreadystatechange = trataResposta;\n\
xhr.send(strTemp2+strTemp);\n\
\n\
};\n\
\n\
function enviarESPSTART() \n\
{\n\
//MUITO CUIDADO, FALTA SORT E OUTRAS COISITAS MAS\n\
//var strTemp=\"EscrevaBytes=32\";\n\
var strTemp=\"Senha=\" + document.getElementById(\"senha\").value + \"&START=1\";\n\
xhr.open(\"POST\", \"http://192.168.4.1/Programar\", true);\n\
xhr.setRequestHeader(\"Content-Type\", \"application/x-www-form-urlencoded; charset=UTF-8\");\n\
xhr.onreadystatechange = trataResposta;\n\
xhr.send(strTemp);\n\
};\n\
\n\
function enviarESPABORT() \n\
{\n\
//MUITO CUIDADO, FALTA SORT E OUTRAS COISITAS MAS\n\
//var strTemp=\"EscrevaBytes=32\";\n\
var strTemp=\"Senha=\" + document.getElementById(\"senha\").value + \"&ABORT=1\";\n\
xhr.open(\"POST\", \"http://192.168.4.1/Programar\", true);\n\
xhr.setRequestHeader(\"Content-Type\", \"application/x-www-form-urlencoded; charset=UTF-8\");\n\
xhr.onreadystatechange = trataResposta;\n\
xhr.send(strTemp);\n\
};\n\
\n\
function testeESP() \n\
{\n\
//MUITO CUIDADO, FALTA SORT E OUTRAS COISITAS MAS\n\
//var strTemp=\"EscrevaBytes=32\";\n\
var strTemp=\"Comando4=1\";\n\
\n\
\n\
\n\
\n\
xhr.open(\"POST\", \"http://192.168.4.1/Programar\", true);\n\
xhr.setRequestHeader(\"Content-Type\", \"application/x-www-form-urlencoded; charset=UTF-8\");\n\
xhr.onreadystatechange = trataResposta;\n\
xhr.send(strTemp);\n\
\n\
};\n\
function loadDoc() \n\
{\n\
xhr.open(\"POST\", \"http://192.168.4.1/Programar\", true);\n\
xhr.setRequestHeader(\"Content-Type\", \"application/x-www-form-urlencoded; charset=UTF-8\");\n\
xhr.onreadystatechange = trataResposta;\n\
xhr.send(\"Comando4=Este foi o Primeiro&teste1=Depois este&teste2=LIGA ESTA PORRA\");\n\
\n\
//ProgramarTrials=5&strTrials=2.14.314;2342342342;2423423;234234;...\n\
//\n\
};\n\
\n\
</script>\n\
\n\
</body>\n\
</html>";


// End Webpage.


  private:




  protected:



  
};
extern ControleDueSkinner *ArduinoDueSkinner =new ControleDueSkinner();


/*############################################################################################################
        Funções da CLASSE
############################################################################################################*/

ControleDueSkinner::ControleDueSkinner()
{
  os_timer_setfn(&trial_timer, isr_trial, NULL); // sets callback function for sound_timer.
  os_timer_setfn(&sound_timer, isr_sound, NULL); // sets callback function for sound_timer.
  os_timer_setfn(&shock_timer, isr_shock, NULL); // sets callback function for shock_timer.
}

/*############################################################################################################
    SPI COMUNICATION FROM/TO MASTER: Our protocol.

        0xA0A0 is the command word sent by ESP8266 that indicates SPI data should be processed.
        0x0010 is the command word sent by ESP8266 that indicates control variables should be programmed.
          (values for carrier_freq, modulator_freq, pulse_shockHigh and pulse_shockLow should be sent by ESP8266 after 0x0010 command)
        0x10FF is the command word sent by Arduino indicating that the values for the control variables were all received.
        0x1000 is the command word sent by Arduino indicating that the values for the control variables were not received.
        0x1A00 is the command word sent by ESP8266 indicating to ABORT experiment.
        
############################################################################################################*/

bool ControleDueSkinner::ProgramaArduinoSPI(int iQualTrial)
{
  uint16_t *uTemp;

  SPI.write16(0x0010,true); // Command sent to Arduino indicating that parameters are goind to be sent through SPI.
  
  uTemp=(uint16_t *)(carrier_freq+iQualTrial);
  SPI.write16(*uTemp,true); SPI.write16(*(uTemp+1),true); // sending first float variable (every 2 bytes)

  uTemp=(uint16_t *)(modulator_freq+iQualTrial);
  SPI.write16(*uTemp,true); SPI.write16(*(uTemp+1),true); // sending second float variable (every 2 bytes)

  uTemp=(uint16_t *)(pulse_shockHigh+iQualTrial);
  SPI.write16(*uTemp,true); SPI.write16(*(uTemp+1),true); // sending third float variable (every 2 bytes)

  uTemp=(uint16_t *)(pulse_shockLow+iQualTrial);
  SPI.write16(*uTemp,true); SPI.write16(*(uTemp+1),true); // sending fourth float variable (every 2 bytes)

  uTemp=(uint16_t *)(volume_sound+iQualTrial);
  SPI.write16(*uTemp,true); SPI.write16(*(uTemp+1),true); // sending fifth float variable (every 2 bytes)

  SPI.write16(0xA0A0,true); // Telling Arduino to process data.
}

void ControleDueSkinner::Abort()
{
  // Disarm timers and put GPIOs in LOW state (except pinRSTarduino and pinSOUNDnot, which goes HIGH), indicating not to play sound and not to shock anymore.
    os_timer_disarm(&trial_timer);
    os_timer_disarm(&sound_timer);
    os_timer_disarm(&shock_timer);
    
    digitalWrite(pinSOUND, LOW); 
    digitalWrite(pinSOUNDnot, HIGH);
    digitalWrite(pinSHOCK, LOW);

    
    SPI.write16(0x001A,true); //Comando para programar parametros
    SPI.write16(0xA0A0,true); //Pode processar
   // Web page response.
  //  server.send ( 200, "text/html", temp );
}

void ControleDueSkinner::StartTrials()
{
  //Vamos zerar o contador
  if (bReady==0) return;

  //Colocar os pinos de controle em LOW
  digitalWrite ( pinSOUND, LOW );
  digitalWrite ( pinSOUNDnot, HIGH ); 
  digitalWrite ( pinSHOCK, LOW );

  //Vamos desarmar os interrupts:
  os_timer_disarm(&trial_timer);
  os_timer_disarm(&sound_timer);
  os_timer_disarm(&shock_timer);

  void *nada;
  iCountTrials=-1; //Vai acrescentar assim que entrar no interrupt de timer
  if (silence[0]==0) isr_trial(nada); //Começar direto
    else os_timer_arm(&(trial_timer), (uint32_t)(silence[0]*1000), 0);  //Ligar o timer
}



bool ControleDueSkinner::ProgramaVariaveis(int QuantosTrials, String strVar)
{

            //Formato strVar: iNumberOfTrials;todos os silence; todos os onset_sound ...
            
            iNumberOfTrials=QuantosTrials;
            parseStringToFloat(strVar,';',BufferFloat);
            
            silence = BufferFloat;  
            onset_sound = BufferFloat+iNumberOfTrials*1;  
            sound_duration = BufferFloat+iNumberOfTrials*2; 
            onset_shock = BufferFloat+iNumberOfTrials*3; 
            shock_duration = BufferFloat+iNumberOfTrials*4; 
                        
            carrier_freq = BufferFloat+iNumberOfTrials*5;      
            modulator_freq = BufferFloat+iNumberOfTrials*6; 
            pulse_shockHigh = BufferFloat+iNumberOfTrials*7; 
            pulse_shockLow = BufferFloat+iNumberOfTrials*8; 
            volume_sound = BufferFloat+iNumberOfTrials*9; 

            bReady=1;          
}
/*############################################################################################################
        Funções de Timmer Interrupt - FORA DA CLASSE
############################################################################################################*/
  
//extern void ControleDueSkinner::isr_trial(void *pArg)
void isr_trial(void *pArg)
{
  ArduinoDueSkinner->iCountTrials++; //Vamos acrscentar o trial para o Próximo, este já deu início e foi programado.
  
  //Colocar os pinos de controle em LOW
  digitalWrite ( ArduinoDueSkinner->pinSOUND, LOW ); 
  digitalWrite ( ArduinoDueSkinner->pinSOUNDnot, HIGH ); 
  digitalWrite ( ArduinoDueSkinner->pinSHOCK, LOW );

  //Vamos desarmar os interrupts:
  os_timer_disarm(&(sound_timer));
  os_timer_disarm(&(shock_timer));

  ArduinoDueSkinner->ProgramaArduinoSPI(ArduinoDueSkinner->iCountTrials); //Programar o que tiver que ser programado
  delay(10); //Esperar um pouco antes de começar

  //Vamos programar os sons e choques deste trial
  if (ArduinoDueSkinner->onset_shock[ArduinoDueSkinner->iCountTrials]==0) 
  {
    digitalWrite ( ArduinoDueSkinner->pinSHOCK, HIGH );
    os_timer_arm(&(shock_timer), (uint32_t)(ArduinoDueSkinner->shock_duration[ArduinoDueSkinner->iCountTrials]*1000), 0);
  }    else if (ArduinoDueSkinner->onset_shock[ArduinoDueSkinner->iCountTrials]>0) os_timer_arm(&(shock_timer), (uint32_t)(ArduinoDueSkinner->onset_shock[ArduinoDueSkinner->iCountTrials]*1000), 0);
  
  if (ArduinoDueSkinner->onset_sound[ArduinoDueSkinner->iCountTrials]==0) 
  {
    digitalWrite ( ArduinoDueSkinner->pinSOUND, HIGH );
    digitalWrite ( ArduinoDueSkinner->pinSOUNDnot, LOW );
    os_timer_arm(&(sound_timer), (uint32_t)(ArduinoDueSkinner->sound_duration[ArduinoDueSkinner->iCountTrials]*1000), 0);
  }    else if (ArduinoDueSkinner->onset_sound[ArduinoDueSkinner->iCountTrials]>0) os_timer_arm(&(sound_timer), (uint32_t)(ArduinoDueSkinner->onset_sound[ArduinoDueSkinner->iCountTrials]*1000), 0);


  /* Let's program next trial (if there is one). Note that the trial_timer is programmed for the next trial
   * just after starting the current trial (programming SOUND and SHOCK timers for the current timer).
   * First, we have to calculate the current trial length in order to program the next trial.
   */
  if ( (ArduinoDueSkinner->onset_sound[ArduinoDueSkinner->iCountTrials] + ArduinoDueSkinner->sound_duration[ArduinoDueSkinner->iCountTrials])
        >=
       (ArduinoDueSkinner->onset_shock[ArduinoDueSkinner->iCountTrials] + ArduinoDueSkinner->shock_duration[ArduinoDueSkinner->iCountTrials]) )
          ArduinoDueSkinner->current_trial_length = ArduinoDueSkinner->onset_sound[ArduinoDueSkinner->iCountTrials] + ArduinoDueSkinner->sound_duration[ArduinoDueSkinner->iCountTrials];
  else
          ArduinoDueSkinner->current_trial_length = ArduinoDueSkinner->onset_shock[ArduinoDueSkinner->iCountTrials] + ArduinoDueSkinner->shock_duration[ArduinoDueSkinner->iCountTrials];
          
  if ( ArduinoDueSkinner->iCountTrials<(ArduinoDueSkinner->iNumberOfTrials-1) ) // Checking if there are more trials to program.
    os_timer_arm( &(trial_timer), (uint32_t)((ArduinoDueSkinner->silence[ArduinoDueSkinner->iCountTrials+1] + ArduinoDueSkinner->current_trial_length) * 1000), 0 ); //Ligar o timer
  else os_timer_disarm(&(trial_timer));    //Na verdade não precisaria desta linha, mas, em todo caso ....                                                    
  
  
}

//extern void ControleDueSkinner::isr_sound(void *pArg)
void isr_sound(void *pArg)
{

if (digitalRead (ArduinoDueSkinner->pinSOUND)) 
  {
    digitalWrite ( ArduinoDueSkinner->pinSOUND, LOW );
    digitalWrite ( ArduinoDueSkinner->pinSOUNDnot, HIGH );
  }
  else 
  {
    digitalWrite ( ArduinoDueSkinner->pinSOUND, HIGH );
    digitalWrite ( ArduinoDueSkinner->pinSOUNDnot, LOW );
    os_timer_arm(&(sound_timer), (uint32_t)(ArduinoDueSkinner->sound_duration[ArduinoDueSkinner->iCountTrials]*1000), 0);
  }

}

//extern void ControleDueSkinner::isr_shock(void *pArg)
void isr_shock(void *pArg)
{

if (digitalRead (ArduinoDueSkinner->pinSHOCK)) digitalWrite ( ArduinoDueSkinner->pinSHOCK, LOW );
  else 
  {
    digitalWrite ( ArduinoDueSkinner->pinSHOCK, HIGH );
    os_timer_arm(&(shock_timer), (uint32_t)(ArduinoDueSkinner->shock_duration[ArduinoDueSkinner->iCountTrials]*1000), 0);
  }

}
