#include <SoftwareSerial.h> //블루투스 라이브러리
#include <Servo.h>  // 서보 라이브러리
#include <SPI.h>    // RFID를 위한 SPI 라이브러
#include <MFRC522.h>// RFID 라이브러

#define SERVO_PIN 2 //서보 PIN
Servo myservo;      //서보 라이브러리

#define SS_PIN 10   //RFID SS(SDA:ChipSelect) PIN
#define RST_PIN 9   //RFID Reset PIN
MFRC522 rfid(SS_PIN, RST_PIN); //RFID 라이브러리

/* 등록된 RF CARD ID */
#define PICC_0 0xF3
#define PICC_1 0xA5
#define PICC_2 0xA0
#define PICC_3 0x27

SoftwareSerial BTSerial(3, 4);
int echo = 8;
int trig = 7;
int flag = 1;
char buffer[100];

//노래
int val=0;
int speakerPin = 6;
int sw = 5;
//Jingle Bells
int length = 26;
char notes[] = "eeeeeeegcde fffffeeeeddedg";
int beats[] = { 1, 1, 2, 1, 1, 2, 1, 1, 1, 1, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2};
int tempo = 300;
 
void playTone(int tone, int duration) {
  for (long i = 0; i < duration * 1000L; i += tone * 2) {
    digitalWrite(speakerPin, HIGH);
    delayMicroseconds(tone);
    digitalWrite(speakerPin, LOW);
    delayMicroseconds(tone);
  }
}
 
void playNote(char note, int duration) {
  
char names[] = { 'c', 'd', 'e', 'f', 's', 'g', 'a', 'v', 'b', 'C', 'D', 'E' };
int tones[] = { 1915, 1700, 1519, 1432, 1352, 1275, 1136, 1073, 1014, 956, 852, 758 };
 
  // play the tone corresponding to the note name
  for (int i = 0; i < 8; i++) {
    if (names[i] == note) {
      playTone(tones[i], duration);
    }
  }
}
 
void setup() { 
  //시리얼 모니터 시작
  pinMode(speakerPin, OUTPUT);
  pinMode(sw, INPUT_PULLUP);
  Serial.begin(9600);
  BTSerial.begin(9600);
  pinMode(trig, OUTPUT); 
  pinMode(echo, INPUT);
  SPI.begin(); // SPI 시작
  rfid.PCD_Init(); // RF 모듈 시작
  attachInterrupt(digitalPinToInterrupt(2), sdistance, RISING);
  //2번 서보모터 핀 작동할때(신호가 LOW->HIGH로 변경시) 인터럽트 실행(초음파센서 LOW)
  Serial.println("Initialized");
  myservo.attach(SERVO_PIN); //서보 시작
  myservo.write(180); //초기 서보 모터를 180도로 위치 시킴
}
 
void loop() {
  val=digitalRead(sw);
  if(val == LOW){//스위치 누르면 노래 시작
    for (int i = 0; i < length; i++) {
     if (notes[i] == ' ') {
       delay(beats[i] * tempo); // rest
     } else {
       playNote(notes[i], beats[i] * tempo);
     }
     // pause between notes
     delay(tempo / 2); 
    }
  }
  else
  {
    noTone(speakerPin);
  }

  //초음파 센서
  digitalWrite(trig, LOW);
  digitalWrite(echo, LOW);
  delayMicroseconds(5);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  unsigned long duration = pulseIn(echo, HIGH);
  float distance = ((float)(340 * duration) / 10000) /2;

  Serial.print(distance);
  Serial.println(" cm");
  
  dtostrf(distance, 5, 2, buffer);
  strcat(buffer,"\r");
  BTSerial.write(buffer);
  delay(1000);
  
  //카드가 인식 안되었다면 더이상 진행하지 말고 빠져나감
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  //RF카드의 ID가 인식 안되었다면 더이상 진행하지 말고 빠져나감
  if ( ! rfid.PICC_ReadCardSerial())
    return;  


  Serial.print(F("PICC type: "));
  //PICC 타입 읽어오기
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));



  //MIFARE 방식이 아니라면 더이상 진행하지 말고 빠져나감
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
    piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
    piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  //ID가 등록된 ID와 동일하다면
  if (rfid.uid.uidByte[0] == PICC_0 && 
      rfid.uid.uidByte[1] == PICC_1 && 
      rfid.uid.uidByte[2] == PICC_2 && 
      rfid.uid.uidByte[3] == PICC_3 ) {
      
      Serial.println(F("This is a confirmed Card."));   
      Serial.println(F("Motor On!!"));

      if(flag%2 == 1)
      {
        myservo.write(90);
        flag++;
        delay(1000);
      }
      else
      {
        myservo.write(180);
        flag--;
        delay(1000);
      }
  }
  
  else{
    //등록된 카드가 아니라면 시리얼 모니터로 ID 표시
    Serial.println(F("This is an unconfirmed Card."));
    Serial.print(F("In hex: "));
    printHex(rfid.uid.uidByte, rfid.uid.size);    
  }
}

void sdistance(){//인터럽트 호출됐을때 서보모터 돌아갈동안 초음파stop
  digitalWrite(trig, LOW);
  digitalWrite(echo, LOW);
}

//ID값 16진수 표시
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}
