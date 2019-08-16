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
        Includes.
############################################################################################################*/
  
  #include <ESP8266WiFi.h> // Link for ESP8266 library download: https://github.com/esp8266/Arduino
  #include <ESP8266WebServer.h>
  #include <ESP8266mDNS.h>
  
  #include <SPI.h>
  #include <EEPROM.h>
  
  #include "Web_Page.h"

/*############################################################################################################
        Global variables.
############################################################################################################*/
  
  const char *ssid = "ConditioningCage";
  const char *password = "0112358132"; // Fibonacci
  IPAddress myIP;
  ESP8266WebServer server ( 80 );

/*############################################################################################################
        Function declarations.
############################################################################################################*/
  
  void handleRoot();
  void Programar();
  void handleNotFound();

/*############################################################################################################
        Setup.
############################################################################################################*/
  void setup ( void ) 
  {
    // Uncommenting Serial.begin() and all Serial.println() lines may help troubleshooting this sketch.
    // Configure UART communication
	  //Serial.begin ( 115200 );

    pinMode ( ArduinoDueSkinner->pinABORT, OUTPUT );
    digitalWrite ( ArduinoDueSkinner->pinABORT, LOW );
    pinMode ( ArduinoDueSkinner->pinSOUND, OUTPUT );
    digitalWrite ( ArduinoDueSkinner->pinSOUND, LOW );
    //pinMode ( ArduinoDueSkinner->pinSOUNDnot, OUTPUT );
    //digitalWrite ( ArduinoDueSkinner->pinSOUNDnot, HIGH );
    pinMode ( ArduinoDueSkinner->pinSHOCK, OUTPUT );
    digitalWrite ( ArduinoDueSkinner->pinSHOCK, LOW );

    // Waiting 1 second before reset Arduino
    delay(1000);
    pinMode (ArduinoDueSkinner->pinRSTarduino,OUTPUT);
    digitalWrite ( ArduinoDueSkinner->pinRSTarduino, LOW );
    delay(100);
    digitalWrite ( ArduinoDueSkinner->pinRSTarduino, HIGH );
  
    // Configure SPI communication.
    SPI.begin ();
    SPI.setBitOrder(MSBFIRST);
    SPI.setDataMode(SPI_MODE0); 
    SPI.setClockDivider(SPI_CLOCK_DIV16);
    //SPI.setClockDivider(SPI_CLOCK_DIV2); 
                                         /* SPI_CLOCK_DIV2    0x00101001 //8 MHz
                                            SPI_CLOCK_DIV4    0x00241001 //4 MHz
                                            SPI_CLOCK_DIV8    0x004c1001 //2 MHz
                                            SPI_CLOCK_DIV16   0x009c1001 //1 MHz
                                            SPI_CLOCK_DIV32   0x013c1001 //500 KHz
                                            SPI_CLOCK_DIV64   0x027c1001 //250 KHz
                                            SPI_CLOCK_DIV128  0x04fc1001 //125 KHz */
    SPI.setHwCs(true); // setHwCs(true) sets ESP8266 as MASTER. setHwCs(false) sets ESP8266 as SLAVE.

    // Configure WiFI
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);
    myIP = WiFi.softAPIP();

    //Domain Name Server. 
    if ( MDNS.begin ( "esp8266" ) ) 
    {
    //  Serial.println ( "MDNS responder started" );
    }
    
    /*
  	Serial.println("Configuring access point...");
    Serial.println();
    Serial.print("Access point name is: ");
    Serial.println(ssid);
    Serial.print("Access Point IP address: ");
    Serial.println(myIP);
  
     // Print the IP address
      Serial.print("Use this URL to connect: ");
      Serial.print("http://");
      Serial.print(myIP);
      Serial.println("/");
  */
  	server.on ( "/", handleRoot );
  	server.on ( "/Programar", Programar );
  	server.on ( "/inline", []() 
  	{
  		server.send ( 200, "text/plain", "this works as well" );
  	} );
  	server.onNotFound ( handleNotFound );
  	server.begin();
  //	Serial.println ( "HTTP server started" );
  }


/*############################################################################################################
        loop.
############################################################################################################*/
  
  void loop ( void ) 
  {
  	server.handleClient();
  }

/*############################################################################################################
        handleRoot.
############################################################################################################*/
  
  void handleRoot() 
  {
    // Publish webpage.
      server.send ( 200, "text/html", ArduinoDueSkinner->PaginaHTML );
  }

/*############################################################################################################
       Experiment code.
############################################################################################################*/
  
  void Programar()
  {
      String argname,argvalue,strResponse;
      uint8_t numofargs = (uint8_t) (server.args());
      Serial.print("numofargs: "); 
      Serial.println(numofargs);
      bool bSenha=false;

      strResponse="";
      
      // For all the arguments...
      for (int i = 0; i < numofargs; i++ )
      {
          // Get argument name, which is a String, and turn it into an array of chars.
          argname = server.argName(i);
          //Serial.print(argname);
          //Serial.print(": ");
          // Get argument value, which is a String, and turn it into an array of chars.
          argvalue = server.arg(i);
          //Serial.println(argvalue);

          // switch structure on the first character of parameter name (indicates the type)
          
          if (argname=="Comando1")
          {
            strResponse+="O comando1 =" + argvalue + "; ";
          }

          if (argname=="Comando2")
          {
            strResponse+="O comando2 =" + argvalue + "; ";
          }

          if (argname=="Comando3")
          {
            strResponse+="O comando3 =" + argvalue + "; ";
          }
          
          if (argname=="Comando4")
          {
            uint16_t uDataIn;
            uDataIn=SPI.transfer16(0xA0A0); //Pode processar
            char *chTemp; chTemp=(char *) &uDataIn;
            strResponse+=String("O comando4 =") + uDataIn + "; ";
          }

          if (argname=="EscrevaBytes")
          {
            
           
            char *c = (char *) ArduinoDueSkinner->BufferFloat;
            c[0]='A';
            c[1]='B';
            c[2]='C';
            c[3]='D';

            uint16_t *varX;
            
            strResponse+="Request sending (" + argvalue + ") bytes on SPI bus; ";

            for (int zxc=0;zxc<argvalue.toInt();zxc+=2) 
              {
                varX=(uint16_t *)(c +zxc);
                while (SPI1CMD & SPIBUSY) {};
                SPI.write16(*varX,true);
              }
            
    
          }      
          if (argname=="Senha")
          {
            if (argvalue=="12345")
              {
                strResponse+="** CORRECT PASSWORD ** \n";
                bSenha=true;
              } else strResponse+="** INCORRECT PASSWORD ** \n";
            
          }

          if (argname=="ProgramarTrials")
          {
            if (i==(numofargs-1)) strResponse+="SOME PARAMETERS ARE MISSING";
              else if ((server.argName(i+1)=="strTrials")&&(bSenha))
                {
                  i++;
                  ArduinoDueSkinner->ProgramaVariaveis(argvalue.toInt(),server.arg(i));
                } else strResponse+="STRING PARAMETERS IS MISSING";
          }

          if ((argname=="START")&&bSenha)
            {
              ArduinoDueSkinner->StartTrials();
              strResponse+="\nSTART COMMAND\n";
            }
            
          if ((argname=="ABORT")&&bSenha)
            {
              ArduinoDueSkinner->Abort();
              strResponse+="\nABORT COMMAND\n";
            }
          
          
     } // end for structure (end sending parameters)
     //Serial.println("end variables");
     server.send ( 200, "application/x-www-form-urlencoded; charset=UTF-8",strResponse);
  }


/*############################################################################################################
        handleNotFound.
############################################################################################################*/

void handleNotFound() 
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) 
  {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }

  server.send ( 404, "text/plain", message );
}
