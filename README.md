# open_the_door
SIM connected door opener with stepper motor on Raspberry PI

Links :
https://raspberry-pi.fr/sim800l-gsm-gps-raspberry/
https://raspberrypi-tutorials.fr/controle-de-moteur-stepper-raspberry-pi-avec-l293d-uln2003a/
https://datasheetspdf.com/pdf-file/1006817/Kiatronics/28BYJ-48/1

http://www.microchip.ua/simcom/SIM800x/Application%20Notes/SIM800%20Series_IP_Application%20Note_V1.05.pdf
https://cdn-shop.adafruit.com/product-files/2637/SIM800+Series_AT+Command+Manual_V1.09.pdf
https://simcom.ee/documents/SIM800x/SIM800%20Series_GSM%20Location_Application%20Note_V1.01.pdf

NodeMCU :
https://www.instructables.com/Quick-Start-to-Nodemcu-ESP8266-on-Arduino-IDE/
https://www.makerguides.com/28byj-48-stepper-motor-arduino-tutorial/
https://simple-circuit.com/esp8266-nodemcu-stepper-motor-control/
https://randomnerdtutorials.com/esp32-dht11-dht22-temperature-humidity-web-server-arduino-ide/


## TODO FOR THE MOTOR

- Make the top gear/wheel for the rope
- Change the number of steps to the exact one
- Don't put the motor on pin2 it is reserved for the second LED

## Commands I use to communicate with the SIM card

Command to enter communication mode with the SIM800L :
```sudo minicom -b 115000 -o -D /dev/serial0```

``` 
> AT+CPIN?
< +CPIN: READY
<
< OK

> AT+CMFG?
< +CMFG: 0
< 
< OK

> AT+CMGF=1
< OK

To send a SMS you can use the command below and terminate it with a CTRL+Z :

> AT+CMGS="+33601020304"
> Hello World!
< +CMGS: 1
<
< OK
```

## HTTP REQUEST
AT+SAPBR=3,1,"Contype","GPRS"
AT+SAPBR=3,1,"APN","TM"
AT+SAPBR=1,1
AT+SAPBR=2,1

AT+HTTPINIT
AT+HTTPSSL=1
AT+HTTPPARA="CID",1
AT+HTTPPARA="URL","www.sim.com" 
AT+HTTPACTION=0
AT+HTTPREAD

AT+HTTPTERM
AT+SAPBR=0,1 // To close the GPRS context


AT+CFUN=1 To enable all functionality of the module
AT+CSTT="TM","",""
AT+CIICR // Start wireless connection with the GPRS
AT+CIPSHUT

AT+CIFSR // Get IP address

AT+CIPSTART="TCP","86.238.129.207",8080
AT+CIPCLOSE

AT+CIPSEND=63
GET exploreembedded.com/wiki/images/1/15/Hello.txt HTTP/1.0

AT+CIPSTART="TCP","api.ipify.org",80
AT+CIPCLOSE
AT+CIPSEND=47
GET http://api.ipify.org?format=json HTTP/1.0

AT+CIPMUX // To start up multi ip connection



All commands manual : https://www.elecrow.com/wiki/images/2/20/SIM800_Series_AT_Command_Manual_V1.09.pdf



AT+CIPSTART="TCP","api.ipify.org",80
AT+CIPSEND=46
GET http://api.ipify.org?format=json HTTP/1.0

https://stackoverflow.com/questions/49292130/raspberry-pi-python-send-sms-using-sim800l
