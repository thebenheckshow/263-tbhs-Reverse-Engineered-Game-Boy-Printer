//Gameboy Printer Test Program

#include "pictures.h"
#include "letters.h"

unsigned char res[] = {0, 0};                         //The 2 byte response we always get

//Commands:

unsigned char cFrameEnd0[] = {8, 136, 51, 4, 0, 0, 0, 4, 0};
unsigned char wordPrint[20]; // = {65, 66, 67, 0, 69, 70, 71, 72, 73, 74};

int dataIn = 6;         //Transmit mode by default unless button pressed on boot
int dataOut = 7;
int clock = 8;

unsigned short checksum = 0;
int whichChar = 0;
  
#define ledLight  13

void setup() {

  Serial.begin(9600);
  Serial.println("GAMEBOY PRINTER FUN");
  Serial.println("");
  Serial.println("Type a word followed by / to print a banner");  

  pinMode(ledLight, 1);

  pinMode(4, INPUT);
  digitalWrite(4, 1);           //Set internal pullup

  pinMode(clock, OUTPUT);
  digitalWrite(clock, 1);

  if (digitalRead(4) == 0) {    //Button pressed? Flip variables in/out to receive mode

    dataIn = 7;
    dataOut = 6;
        
  }
  
  pinMode(dataIn, INPUT);       //Set directions
  pinMode(dataOut, OUTPUT);  
  
}

void loop() {

  if (Serial.available() > 0) {     //Something came in?

    wordPrint[whichChar] = Serial.read();

    if (wordPrint[whichChar] == '/') {
      
      if (wordPrint[0] < 58) {
          printShape(wordPrint[0] - 48);
      }
      else {
          printWord();
      }      
             
      whichChar = 0;
    }
    else {
      whichChar += 1;
    }
 
  }

  //printShape(0);
  
  //printWord();
  
  //printLetter('q');

}

void printWord() {

  int run = 1;

  Serial.print("Making banner: '");

  for (int x = 0 ; x < whichChar ; x++) {
    
    Serial.write(wordPrint[x]);
  }

  Serial.println("'");

  whichChar = 0;
  
  while (run) {
  
    if (wordPrint[whichChar + 1] == '/') {       //This is the last actual letter?
        printLetter(wordPrint[whichChar], 3);   //Extra tear-off margin
        run = 0; 
    }
    else {
        printLetter(wordPrint[whichChar], 1);   //Normal letter    
    }
    
    whichChar += 1;
    
  }
  
  Serial.println("Banner complete!");  
    
}

void printLetter(unsigned char whichLetter, int margin) {

  Serial.print("Sending '");
  Serial.write(whichLetter);
  Serial.print("'"); 
  
  if (whichLetter == 32) {
    whichLetter = 26;
  }
  else {    
    if (whichLetter > 96) {
      whichLetter -= 97;  
    }
    else {      
      whichLetter -= 65;  
    }
  }
 
  frameStart();

  delay(50);
  
  int index = whichLetter * 30;
  
  int lineCount = 0;
  
  //Letters are 5 fat blocks wide, 6 fat blocks tall
  //Each fat block is made of 8x8 blocks
  //4 blocks wide, 3 blocks tall
  
  for (int line = 0 ; line < 6 ; line++) {            //Total of 6 blocks tall (6x18 = total height)

    Serial.print(".");
    
    digitalWrite(ledLight, line & 1);
    
    for (int fatTall = 0 ; fatTall < 3; fatTall++) {
 
      if ((lineCount & 1) == 0) {           //An even line?     
        lineStart();       
      }
 
      for (int fatWide = 0 ; fatWide < 5; fatWide++) {
      
        int blockColor = letters[index++] * 255;      //Get color of the 4x1 slab
        
        for (int count = 0 ; count < 4 ; count++) {   //Send out 4 fat blocks in a row        
          send8x8(blockColor);     
        }
                            
      }
      
      if (lineCount & 1) {                          //Is this an odd line? (meaning we've sent a pair)
      
        sendChecksum();
        delay(50);
        
      }

      lineCount += 1;      
      
      index -= 5;                                     //Repeat this line 3 times (go back in memory)
      
    }
    
    index += 5;
       
  }
  
  delayMicroseconds(1500);
  
  sendCommand(cFrameEnd0);
  getResponse();
  
  delay(20);
  
  printPage(0, margin); 
   
}

void printShape(unsigned char whichShape) {

  if (whichShape > 2) {
    return;
  }

  Serial.print("Sending picture");

  frameStart();

  delay(50);
  
  int index = whichShape * 360;

  for (int line = 0 ; line < 9 ; line++) {

    Serial.print(".");
  
    digitalWrite(ledLight, line & 1);
    
    lineStart();

    for (int block = 0 ; block < 40 ; block++) {

      int blockColor = picture[index] * 255;
        
      //Send an 8x8 block
      send8x8(blockColor);
      
      index += 1;
     
    }
        
    sendChecksum();

    delay(50);
    
  }
  
  delayMicroseconds(1500);
  
  sendCommand(cFrameEnd0);
  getResponse();
  
  delay(20);
  
  printPage(0, 0);

  Serial.println("Picture complete!");
  
}

void sendLine(unsigned char whatColor) {
   
  for (int x = 0 ; x < 640 ; x++) {

    sendByte(whatColor);
    
  }  

}

void sendCommand(unsigned char *charPointer) {
  
  int numBytes = *charPointer++;    //First char is the number of bytes in this command
  
  for (int x = 0 ; x < numBytes ; x++) {
    
    //Serial.println(*charPointer, DEC);

    sendByte(*charPointer++);
    
  }

}

unsigned char sendByte(unsigned char byteToSend) {

    checksum += byteToSend;              //Always build this, even if not used
  
    for (int x = 0 ; x < 8 ; x++) {

      if (byteToSend & B10000000) {       
        PORTD |= B10000000;       
      }
      else {        
        PORTD &= ~B10000000;        
      }
      
      PORTB &= ~1;        //Pulse clock active low   
      delayMicroseconds(55);      //Approximate Gameboy timing (1ms per byte)   
      PORTB |= 1;         //Back to high

      byteToSend <<= 1;
     
      delayMicroseconds(55);     
      
    }
    
    delayMicroseconds(200);

}

unsigned char getResponse() {

  res[0] = 0;
  res[1] = 0;
  
  for (int xx = 0 ; xx < 2 ; xx++) {
    
    for (int x = 0 ; x < 8 ; x++) {
      
      res[xx] <<= 1;              //Shift to the left making room for next bit. DO this first so we don't lose the last one
           
      PORTB &= ~1;                //Pulse clock active low 
      
      delayMicroseconds(25);      //Approximate Gameboy timing (1ms per byte)
      
      //Check the bit halfway through
      if (PIND & 64) {            //A bit is there?
          res[xx] |= 1;           //Set it       
      }
      
      delayMicroseconds(25);      //Approximate Gameboy timing (1ms per byte)
      
      PORTB |= 1;                //Back to high

      delayMicroseconds(55);     
      
    }
    
    delayMicroseconds(200);
 
  }

}


void send8x8(unsigned char blockColor) {

  for (int count = 0 ; count < 16 ; count++) {        
    sendByte(blockColor);        
  }   

}

void sendHeader() {       //Sends the header in front of every command
  
  sendByte(136);
  sendByte(51);
  checksum = 0;

}

void frameStart() {
  
  sendHeader();       //Resets checksum
  sendByte(1);
  sendByte(0);
  sendByte(0);
  sendByte(0);  
  sendByte(0);   
  sendByte(1);
  getResponse();
  
}

void lineStart() {
  
  sendHeader();       //Resets checksum
  sendByte(4);
  sendByte(0);
  sendByte(128);
  sendByte(2);
  
}

void printPage(unsigned char topMargin, unsigned char bottomMargin) {

  Serial.print(" Printing");
  
  sendHeader();
  sendByte(2);
  sendByte(0);
  sendByte(4);
  sendByte(0);
  sendByte(1);
  sendByte((topMargin << 4) | bottomMargin);
  sendByte(228);
  sendByte(127);
  sendChecksum();
  getResponse();

  int printing = (B10000001 << 8) | B00000110;
  int response = printing;

  delay(1000);
  
  while (response == printing) {

      Serial.print(".");
  
      delay(250);
      digitalWrite(ledLight, 1);
      delay(250);
      digitalWrite(ledLight, 0);

      sendHeader();
      sendByte(0x0F);
      sendByte(0);
      sendByte(0);
      sendByte(0);
      sendByte(0x0F);
      sendByte(0);      
      
      getResponse();
      
      response = (res[0] << 8) | res[1] & 0xFF;
 
  }

  Serial.println("done!"); 
  
}

void sendChecksum() {

  unsigned short lineChecksum = checksum;                //Grab the state of checksum so we don't change it while sending
  
  sendByte(lineChecksum & 0xFF);                        //Send checksum low byte
  sendByte(lineChecksum >> 8);                          //Send checksum high byte

  getResponse();
  
}







