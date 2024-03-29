/*

Ao realizar testes de INPUT e OUTPUT nos pinos, obtivemos os seguintes resultados:
INPUT não funcionou apenas no GPIO0.
OUTPUT não funcionou apenas nos pinos GPIO34 e GPIO35, que são VDET1 e VDET2 respectivamente.

*/

//bibliotecas
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>
#include "RTClib.h"
#include <EEPROM.h>
#include <dht.h>

//memória
#define EEPROM_SIZE 2

//dimmer ac 127v da serpentina
#define PINO_DIM 13
#define PINO_ZC 12

//sensor de temperatura e umidade
#define dht_pin A5

//dimmer dc 12v dos collers
#define PINO_COLLERS A6

//pino de saida para relé LUZ
#define PINO_LUZ 27

//pino de saida para relé LUZ
#define PINO_BOMBA 26

//pino sensor umidade
#define PINO_UMIDADE A8

//Declaração de Objetos
dht   my_dht;
RTC_DS3231 rtc;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;

//strings 
String estado = "";
String v1 = "";
String v2 = "";
String valor = "";

//variaveis de programa
int resultado = 0;
int ajustehora = 0;
int ajusteminuto = 0;
int borda1 = 00;
int borda2 = 00;

//variaveis de controle
int umidadereal = 0;
int tempreal = 0;
int tempmax = 40; // começo em 40 para evitar bugs
int tempmin = 20; // começo em 20 para evitar bugs
int irri1hora = 0;
int irri1minuto = 0;
int irri2hora = 1; // começa em 1 para evitar bug no começo de programação
int irri2minuto = 0;
int lumligadohora = 0;
int lumligadominuto = 1; // começa em 1 para não ficar ligando/desligando no começo
int lumdesligadohora = 0;
int lumdesligadominuto = 1; // começa em 1 para não ficar ligando/desligando no começo
int tempobomba = 0;

//variaveis de tempo de iluminação
int soma_horas_lum_ligado = 0;
int soma_horas_lum_desligado = 0;
int soma_minutos_lum_ligado = 0;
int soma_minutos_lum_desligado = 0;

//variaveis de bomba ligado e desligado
int estado_tempo_bomba = 0;

//variaveis de controle manual
int manualcoller = 0;
int manualserpentina = 0;

//variaveis de bomba e iluminação ( Essa função está funcionando ao contrario 0 é ligado e 1 é desligado)
int estadoluz = 0;
int estadobomba = 0;


//tempo bomba
int save_tempo_bomba = 0;

//flags
int lum = 0;
int bomba = 0;
int coller = 0;
int serpentina = 0;

//variaveis de controle de iluminação
int calc_lum_ligado = 0;
int save_calc_horas_ligado = 0;
int save_calc_minutos_ligado = 0;
int save_calc_horas_desligado = 0;
int save_calc_minutos_desligado = 0;
int estado_calc_lum = 0;
int startlum = 0; //estado de inicio da lampada

//intensidade da serpentina
volatile long luminosidade = 0;  // 0 a 100 

// --- Variáveis Globais ---
int temperatura = 0x00,   //armazena a temperatura em inteiro
    umidade     = 0x00;   //armazena a umidade em inteiro

#define SERVICE_UUID           "ab0828b1-198e-4351-b779-901fa0e0371e" // UART service UUID
#define CHARACTERISTIC_UUID_RX "4ac8a682-9736-4e5d-932b-e9b31405049c"
#define CHARACTERISTIC_UUID_TX "0972EF8C-7613-4075-AD52-756F33D4DA91"


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++){
			
		//mostra a string inteira
        Serial.print(rxValue[i]);
		}
		
	    Serial.print(" / ");
		
		
	    //mostra a função recebida
		estado = rxValue[0];
		Serial.print(estado);
		
		Serial.print(" / ");
		
		v1 = rxValue[1];
		v2 = rxValue[2];
		
		valor = v1+v2;
		
		//mostra os valores recebidos
		Serial.print(valor);
		
		Serial.print(" / ");
		
		//transforma texto em número
		resultado = valor.toInt();
		Serial.println(resultado);
		
        if (rxValue.find("H") != -1) { 
		 ajustehora = resultado;
		 rtc.adjust(DateTime(2014, 1, 21, (ajustehora),(ajusteminuto), 0));
		 estado_calc_lum = 0;
        }
        
        if (rxValue.find("M") != -1) { 
         ajusteminuto = resultado;
		 rtc.adjust(DateTime(2014, 1, 21, (ajustehora),(ajusteminuto), 0));
		 estado_calc_lum = 0;
        }
		
        if (rxValue.find("X") != -1) { 
		 serpentina = 0;
		 coller = 0;
         tempmax = resultado;
		 EEPROM.write(0, resultado);
		 EEPROM.commit();
        }
		
        if (rxValue.find("U") != -1) {
		 serpentina = 0;
		 coller = 0;			
         tempmin = resultado;
		 EEPROM.write(1, resultado);
		 EEPROM.commit();
        }
		
        if (rxValue.find("A") != -1) {
         bomba = 0;			
         irri1hora = resultado;
		 EEPROM.write(2, resultado);
		 EEPROM.commit();
        }
		
		if (rxValue.find("B") != -1) { 
		 bomba = 0;
         irri1minuto = resultado;
		 EEPROM.write(3, resultado);
		 EEPROM.commit();
        }
		
		if (rxValue.find("C") != -1) {
         bomba = 0;			
         irri2hora = resultado;
		 EEPROM.write(4, resultado);
		 EEPROM.commit();
        }
		
		if (rxValue.find("D") != -1) {
         bomba = 0;			
         irri2minuto = resultado;
		 EEPROM.write(5, resultado);
		 EEPROM.commit();
        }
		
		if (rxValue.find("E") != -1) {
         bomba = 0;			
         tempobomba = resultado;
		 EEPROM.write(10, resultado);
		 EEPROM.commit();
        }
		
		//verificar caso de não conseguir ajustar novamente para começar desligado.
		if (rxValue.find("F") != -1) { 
		 startlum = 1;
		 EEPROM.write(13, startlum);
	     EEPROM.commit();
        }
		
		if (rxValue.find("G") != -1) {
         lum = 0;			
         lumligadohora = resultado;
		 EEPROM.write(6, resultado);
		 EEPROM.commit();
        }
		
		if (rxValue.find("I") != -1) {
         lum = 0;			
         lumligadominuto = resultado;
		 EEPROM.write(7, resultado);
		 EEPROM.commit();
        }
		
		if (rxValue.find("J") != -1) {
         lum = 0;			
         lumdesligadohora = resultado;
		 EEPROM.write(8, resultado);
		 EEPROM.commit();
        }
		
		if (rxValue.find("K") != -1) {
         lum = 0;			
         lumdesligadominuto = resultado;
		 EEPROM.write(9, resultado);
		 EEPROM.commit();
        }
		
		if (rxValue.find("L") != -1) {
         lum = 1;
         estadoluz = 1;		 
         //manual iluminação ligado
        }
		
		if (rxValue.find("M") != -1) { 
		 lum = 1;
		 estadoluz = 0;
         //manual iluminação desligado
        }
		
		if (rxValue.find("N") != -1) {
         bomba = 1;	
         estadobomba = 1;		 
         //manual bomba ligado
        }
		
		if (rxValue.find("O") != -1) { 
		 bomba = 1;
		 estadobomba = 0;
         //manual bomba desligado
        }
		
		if (rxValue.find("P") != -1) {
         serpentina = 1;			
         manualserpentina = resultado;
        }
		
		if (rxValue.find("Q") != -1) {
         coller = 1;			
         manualcoller = resultado;
        }
		
        Serial.println();
        Serial.println("*********");
      }
    }
};

void zeroCross()  {
  if (luminosidade>100) luminosidade=100;
  if (luminosidade<0) luminosidade=0;
  long t1 = 8200L * (100L - luminosidade) / 100L;      
  delayMicroseconds(t1);   
  digitalWrite(PINO_DIM, HIGH);  
  delayMicroseconds(6);      // t2
  digitalWrite(PINO_DIM, LOW);   
}

void setup() {
	
  rtc.begin();
  EEPROM.begin(EEPROM_SIZE);
  
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  
  //carregamento de memória
  tempmax = EEPROM.read(0);
  tempmin = EEPROM.read(1);
  irri1hora = EEPROM.read(2);
  irri1minuto = EEPROM.read(3);
  irri2hora = EEPROM.read(4);
  irri2minuto = EEPROM.read(5);
  lumligadohora = EEPROM.read(6);
  lumligadominuto = EEPROM.read(7);
  lumdesligadohora = EEPROM.read(8);
  lumdesligadominuto = EEPROM.read(9);
  tempobomba = EEPROM.read(10);
  save_calc_horas_ligado = EEPROM.read(11);
  save_calc_minutos_ligado = EEPROM.read(12);
  startlum = EEPROM.read(13);
  save_calc_horas_desligado = EEPROM.read(14);
  save_calc_minutos_desligado = EEPROM.read(15);
  estado_calc_lum = EEPROM.read(16);

  //setup para o dimmer AC 127v
  pinMode(PINO_DIM, OUTPUT);
  attachInterrupt(PINO_ZC, zeroCross, RISING);
  
  //setup para sensor umidade
  pinMode(PINO_UMIDADE, INPUT);
  
  //setup para collers
  pinMode(PINO_COLLERS, OUTPUT);
  
  //setup para luz
  pinMode(PINO_LUZ, OUTPUT);
  digitalWrite(PINO_LUZ, HIGH);
  
  //setup para bomba
  pinMode(PINO_BOMBA, OUTPUT);
  digitalWrite(PINO_BOMBA, HIGH);
  
  // ativando e ajustando leitura serial
  Serial.begin(115200);
   
  // Create the BLE Device
  BLEDevice::init("BLE-KV");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(
										CHARACTERISTIC_UUID_TX,
										BLECharacteristic::PROPERTY_NOTIFY
									);
                      
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
											 CHARACTERISTIC_UUID_RX,
											BLECharacteristic::PROPERTY_WRITE
										);

  pRxCharacteristic->setCallbacks(new MyCallbacks());

 
 // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
 
}


void loop() {

my_dht.read11(dht_pin);
DateTime now = rtc.now();

//leitura da temperatura;
tempreal = my_dht.temperature;

//leitura de umidade do solo
umidadereal = map(analogRead(PINO_UMIDADE),0,1023,0,100);

//CONTROLE DE TEMPERATURA AUTOMATICO
if(serpentina==0 && coller==0){
	 //COLLERS
	 if(tempmax > tempreal){
		 analogWrite(PINO_COLLERS, 0); 
	    }
	 if(tempmax <= tempreal < (tempmax+2)){
		 analogWrite(PINO_COLLERS, 20); 
	    }
	 if((tempmax+2) <= tempreal < (tempmax+4)){
		 analogWrite(PINO_COLLERS, 40); 
	    }
	 if((tempmax+4) <= tempreal < (tempmax+6)){
		 analogWrite(PINO_COLLERS, 60); 
	    }
	 if((tempmax+6) <= tempreal < (tempmax+8)){
		 analogWrite(PINO_COLLERS, 80); 
	    }
	 if((tempmax+8) <= tempreal <= (tempmax+10)){
		 analoglWrite(PINO_COLLERS, 100); 
	    }
	 
	 //SERPENTINA
     if(tempmin < tempreal){
         luminosidade = 0;
	    }
     if(tempmin >= tempreal > (tempmin-2)){
         luminosidade = 20;
	    }
	 if((tempmin-2) >= tempreal > (tempmin-4)){
         luminosidade = 40;
	    }
	 if((tempmin-4) >= tempreal > (tempmin-6)){
         luminosidade = 60;
	    }
	 if((tempmin-6) >= tempreal > (tempmin-8)){
         luminosidade = 80;
	    }
     if((tempmin-8) >= tempreal >= (tempmin-10)){
         luminosidade = 100;
	    }
    } 		
	
//CONTROLE DE COLLERS E SERPENTINA MANUAL
if(coller==1 && serpentina==0){
     analogWrite(PINO_COLLERS, manualcoller);
	 luminosidade = 0;
	}
	
if(serpentina==1 && coller==0){
     analogWrite(PINO_COLLERS, 0);
	 luminosidade = manualserpentina;
	}

if(serpentina==1 && coller==1){
     analogWrite(PINO_COLLERS, manualcoller);
	 luminosidade = manualserpentina;
    }

	
//CONTROLE DE ILUMINAÇÃO AUTOMATICO
if(lum==0){
	 if(startlum==1){
		 
		 digitalWrite(PINO_LUZ, LOW);
		 
		 if(estado_calc_lum==0){
			 lumligadohora = (lumligadohora * 3600);
		     save_calc_horas_ligado = (lumligadohora + now.unixtime());
		     EEPROM.write(11, save_calc_horas_ligado);
			 EEPROM.commit();
			 
			 lumligadominuto = (lumligadominuto * 60);
		     save_calc_minutos_ligado = (lumligadominuto + now.unixtime());
		     EEPROM.write(12, save_calc_minutos_ligado);
			 EEPROM.commit();
			 
			 estado_calc_lum = 1;
			 EEPROM.write(16, estado_calc_lum);
			 EEPROM.commit();
			 lumligadohora = EEPROM.read(6);
             lumligadominuto = EEPROM.read(7);
		    }
			
		 if((save_calc_horas_ligado+save_calc_minutos_ligado)<=now.unixtime()){
			 startlum=0;
			 estado_calc_lum = 0;
			 EEPROM.write(13, startlum);
			 EEPROM.commit();
			 EEPROM.write(16, estado_calc_lum);
			 EEPROM.commit();
		    }
	    } 
		
	 if(startlum==0){
		 
		 digitalWrite(PINO_LUZ, HIGH);
		 
		 if(estado_calc_lum==0){
			 lumdesligadohora = (lumdesligadohora * 3600);
		     save_calc_horas_desligado = (lumdesligadohora + now.unixtime());
		     EEPROM.write(14, save_calc_horas_desligado);
			 EEPROM.commit();
			 
			 lumdesligadominuto = (lumdesligadominuto * 60);
		     save_calc_minutos_desligado = (lumdesligadominuto + now.unixtime());
		     EEPROM.write(15, save_calc_minutos_desligado);
			 EEPROM.commit();
			 
			 estado_calc_lum = 1;
			 EEPROM.write(16, estado_calc_lum);
			 EEPROM.commit();
			 lumdesligadohora = EEPROM.read(8);
             lumdesligadominuto = EEPROM.read(9);
		    }

		 if((save_calc_horas_desligado+save_calc_minutos_desligado)<=now.unixtime()){
			 startlum=1;
			 estado_calc_lum = 0;
			 EEPROM.write(13, startlum);
			 EEPROM.commit();
			 EEPROM.write(16, estado_calc_lum);
			 EEPROM.commit();
		    }
	    } 	 
	 
	}

//CONTROLE DE ILUMINAÇÃO MANUAL
if(lum==1){
	if(estadoluz==1){
	 digitalWrite(PINO_LUZ, LOW);
	}
	if(estadoluz==0){
	 digitalWrite(PINO_LUZ, HIGH);
	}
}

//CONTROLE DE BOMBA AUTOMATICO
if(bomba==0){
	 //irrigação 1
	 if(now.hour()==irri1hora && now.minute()==irri1minuto){
		 digitalWrite(PINO_BOMBA, LOW);
		 save_tempo_bomba = (tempobomba + now.unixtime());
		 estado_tempo_bomba=1;
	    }
		
	 //irrigação 2	
	 if(now.hour()==irri2hora && now.minute()==irri2minuto){
		 digitalWrite(PINO_BOMBA, LOW);
		 save_tempo_bomba = (tempobomba + now.unixtime());
		 estado_tempo_bomba=1;
	    }	
	 
	 //tempo de bomba ligado
	 if(estado_tempo_bomba==1){
		 if(save_tempo_bomba <= now.unixtime()){
			 digitalWrite(PINO_BOMBA, HIGH);
			 estado_tempo_bomba=0;
		    }
	    }
		
    }

//CONTROLE DE BOMBA MANUAL
if(bomba==1){
	 if(estadobomba==1){
	     digitalWrite(PINO_BOMBA, LOW);
	    }
	 if(estadobomba==0){
	     digitalWrite(PINO_BOMBA, HIGH);
	    } 
	}
	
	
//envio de dados para esp32
	if (deviceConnected) {
		
      char txString[60]; // make sure this is big enuffz;
      sprintf(txString,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",borda1,now.hour(),now.minute(),now.second(),tempreal,tempmax,tempmin,irri1hora,irri1minuto,irri2hora,irri2minuto,tempobomba,lumligadohora,lumligadominuto,lumdesligadohora,lumdesligadominuto,umidadereal,borda2); // string de transmição
    
        pTxCharacteristic->setValue(txString);
        pTxCharacteristic->notify();
  
        Serial.print("*** Sent Value: ");
        Serial.print(txString);
		Serial.println(" ***");

	}

}
