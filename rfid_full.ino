#include <Wire.h> // Library for I2C communication
#include <LiquidCrystal_I2C.h> // Library for LCD
#include <SPI.h>      //include the SPI bus library
#include <MFRC522.h>  //include the RFID reader library
#include <EEPROM.h> //include embedded EEPROM library

#define SS_PIN 10  //slave select pin
#define RST_PIN 5  //reset pin

MFRC522 mfrc522(SS_PIN, RST_PIN);  // instatiate a MFRC522 reader object.
MFRC522::MIFARE_Key key;          //create a MIFARE_Key struct named 'key', which will hold the card information
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2); // Change to (0x27,16,2) for 16x2 LCD.

int blocks[2]= {4,5}; // Blocks that would hold the student name and student number
byte studentName[16]= {"Nana Amponsah"}; // array of bytes containing student name
byte studentNumber[16]={"PS/CSC/16/003"}; // array of bytes containing student indext number

byte readStudentName[18]; // array used to read back student name block
byte readStudentNumber[18]; // array used to read back student number block


void setup() {
  Serial.begin(9600);        // Initialize serial communications with the PC
  SPI.begin();               // Init SPI bus
  mfrc522.PCD_Init();        // Init MFRC522 card (in case you wonder what PCD means: proximity coupling device)
  
  // Initiate the LCD:
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0); // Set the cursor on the first column and first row.
  lcd.print("Student ID Check"); 
  lcd.setCursor(0, 1); // Set the cursor on the first column and first row.
  lcd.print("Swipe ID Card"); 
  Serial.println("Scan your student card");
  
  // Prepare the security key for the read and write functions.
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;  //keyByte is defined in the "MIFARE_Key" 'struct' definition in the .h file of the library
  }

//  Write 

}

void loop() {
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return;
  }
  Serial.println("Student card identified");
    // Clear EEPROM for the purpose of this test
//   for (int i = 0 ; i < EEPROM.length() ; i++) {
//    EEPROM.write(i, 0);
//  }
  //  Read from memory if there's the need to write to memory again
  int writeFlag = EEPROM.read(0);
  
  Serial.println(writeFlag);
  if (writeFlag != 1){
      // Write student details to card
      Serial.println("Write student details");
      writeToMemory(); 
   }

  //  Scan identified card for student details
  String *Id; // Pointer to string
  Id = getStudentID(readStudentName,readStudentNumber); //execute function to read data blocks
  Serial.println("Printing serial details");
  lcd.clear();
  for(int j=0;j<2;j++){
    lcd.setCursor(0,j);
    lcd.print(*(Id + j ));
    }
//  mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
  delay(3000);
}

bool writeToMemory(){
  for(int j=0;j<2;j++){
        if (j == 0){
            writeBlock(blocks[j],studentName); 
          }
         else{
            writeBlock(blocks[j],studentNumber);  
        }
    }
  EEPROM.write(0,1);
  
  return true;  
}


String * getStudentID(byte studentName[],byte studentNumber[])
{
  static String studentId[2]= {"",""};
    
  // Read the student details one by one
  
  int nameResponse = readBlock(blocks[0],studentName); // Read student name
  if (nameResponse == 0){
  //  append the student name to the student id array
    for(int i=0;i<16;i++){
      Serial.write(studentName[i]);
//      Serial.println(studentName[i],BIN);
      studentId[0]+= (char) studentName[i];  // Add student name to student id array
    }
    Serial.println("");
    Serial.println("Name reading successful");
  }
  else
  {
    Serial.println("Name response failed");
//    return "Failed";
  }
   
  int numberResponse = readBlock(blocks[1],studentNumber); // Read student number
  if (numberResponse == 0){
    //  append the student number to the student id array
    for(int j=0;j<16;j++){
      Serial.write(studentNumber[j]);
//      Serial.println(studentNumber[j],BIN);
      studentId[1]+= (char) studentNumber[j];  // Add student name to student id array
    }
    Serial.println("");
    Serial.println("Number reading successful");
    
    }
  else 
  {
    Serial.println("Number response failed");
//    return "Failed";
  }
  // Stop card from reading again
  mfrc522.PICC_HaltA();
  
  return studentId;
}



//Write specific block    
int writeBlock(int blockNumber, byte arrayAddress[]) 
{
  //this makes sure that we only write into data blocks. Every 4th block is a trailer block for the access/security info.
  int largestModulo4Number=blockNumber/4*4;
  int trailerBlock=largestModulo4Number+3;//determine trailer block for the sector
  if (blockNumber > 2 && (blockNumber+1)%4 == 0){Serial.print(blockNumber);Serial.println(" is a trailer block:");return 2;}
  Serial.print("Trailer block");
  Serial.println(trailerBlock);
  Serial.print(blockNumber);
  Serial.println(" is a data block:");
  
  //authentication of the desired block for access
  byte status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  Serial.print("Write Status: ");
  Serial.println(status);
  if (status != MFRC522::STATUS_OK) {
         Serial.print("PCD_Authenticate() failed: ");
         Serial.println(mfrc522.GetStatusCodeName(status));
         return 3;//return "3" as error message
  }
  
  //writing the block 
  status = mfrc522.MIFARE_Write(blockNumber, arrayAddress, 16);
  //status = mfrc522.MIFARE_Write(9, value1Block, 16);
  if (status != MFRC522::STATUS_OK) {
           Serial.print("MIFARE_Write() failed: ");
           Serial.println(mfrc522.GetStatusCodeName(status));
           return 4;//return "4" as error message
  }
  Serial.println("block was written");
}

//Read specific block
int readBlock(int blockNumber, byte arrayAddress[]) 
{
  int largestModulo4Number=blockNumber/4*4;
  int trailerBlock=largestModulo4Number+3;//determine trailer block for the sector

  //authentication of the desired block for access
  byte status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));

  if (status != MFRC522::STATUS_OK) {
         Serial.print("PCD_Authenticate() failed (read): ");
         Serial.println(mfrc522.GetStatusCodeName(status));
         return 3;//return "3" as error message
  }

//reading a block
byte buffersize = 18;//we need to define a variable with the read buffer size, since the MIFARE_Read method below needs a pointer to the variable that contains the size... 
status = mfrc522.MIFARE_Read(blockNumber, arrayAddress, &buffersize);//&buffersize is a pointer to the buffersize variable; MIFARE_Read requires a pointer instead of just a number
  if (status != MFRC522::STATUS_OK) {
          Serial.print("MIFARE_read() failed: ");
          Serial.println(mfrc522.GetStatusCodeName(status));
          return 4;//return "4" as error message
  }
  Serial.println("block was read");
  return 0;
}
