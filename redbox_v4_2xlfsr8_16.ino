// Hardware random number generator by Richard Jelbert - 1st January 2020
// assumes four random sources with div/2 counters
// runs serial at 115200 which is up to 14K BYTES a second max rate
// currenly running at 11k BYTES PER SECOND

byte buffersize = 64;
byte buffer[64];
int vin = A0;
unsigned long time_now = 0;

unsigned int start_state16;
unsigned int lfsr16;
unsigned int counter16;

byte lfsr8;
byte start_state8;
byte counter8;

// some useful linux commands...
// cat /dev/cu.usbmodem14101 | head -c 1000 > rndcapture.bin
// screen /dev/cu.usbmodem14101 115200
// stty -a /dev/cu.usbmodem14101
// od -x < /dev/cu.usbmodem14101
// dieharder -g 201 -f 2_2_ghost_random.bin -a
// dieharder -g 201 -f /media/pi/INZURA/redbox20-20.bin -a -Y 1 -D test_name -D psamples -D assessment -c ','
// dieharder -g 201 -f /media/pi/INZURA/redbox20-20-ghost_rot1.bin -a -Y 1 -D show_rng -D test_name -D ntuple -D psamples -D assessment -D show_num -D pvalues -D tsamples -c ',' > /media/pi/INZURA/redbox20-20-ghost_rot1.tests

// set things up...
void setup() {
  Serial.begin(115200, SERIAL_8N1);
  Serial.flush();
  pinMode(2, INPUT); // noise generator input 1
  pinMode(3, INPUT); // noise generator input 2
  pinMode(4, INPUT); // noise generator input 3
  pinMode(5, INPUT); // noise generator input 4
  pinMode(6, OUTPUT); // status LED
  pinMode(7, OUTPUT); // debug output pin to show data tx data frame for scope

// blip the green led to show power has been turned on..
  digitalWrite(6,1); // turn green LED on
  delay(40);
  digitalWrite(6,0); // turn green LED off

  digitalWrite(7,0);
// go into calibrate mode if 12v PSU disconnected on power up
// probably betteer to put this feature on a button...
  int val = analogRead(vin);
  if (val < 400) { calibrate(); }  
  self_test();
  digitalWrite(6,1); // turn green LED on
  time_now = millis();
  seed_lfsr16();
  seed_lfsr8();
}

// the main loop gets random nibbles, builds a byte and then sends it over serial!
void loop(){
  
// fill up a 64 byte buffer reading two random nibbles at a time
  for (int i=0;i<=(buffersize-1);i++) {
    buffer[i] = (getnibble() >> 2); // add first nibble
    buffer[i] = (buffer[i] | (getnibble() << 2)); // add seccond nibble  
}

// whiten the raw entropy bytes with a a couple of LFSRs...

// 1st do the 8 bit lfsr jumble
int c = 0;
for (int i = 0;i<=(buffersize-1);i++) {
//for (int c = 0;c<=1;c++) {  
// clock the 8 bit lfsr x times
lfsr8 ^= lfsr8 >> 3;
lfsr8 ^= lfsr8 << 5;
lfsr8 ^= lfsr8 >> 7;
counter8+=1;
if (lfsr8 == start_state8) { seed_lfsr8(); c = 2;}
//}
buffer[i] = buffer[i] ^ lfsr8;
//buffer[i] = lfsr8;
 // Serial.println(buffer[i], DEC);
}

// 8 bit taps
// 2,5,7 = period of 217
// 3,5,7 = period of 255 (slowest but best)
// 3,5 = period of 217 (quite fast, long period in case of zero value seed)
// 1,7 = period of 63
// 1,3 = period of 63 (fastest, seed only used twice in one dump of the buffer)

// now mash things up with a maximal period 16 bit LFSR
int d = 0;
for (int i = 0;i<=(buffersize-1);i++) {
//for (d = 0;d<=1;d++) {  
// clock the 16 bit lfsr x times
lfsr16 ^= lfsr16 >> 7;
lfsr16 ^= lfsr16 << 9;
lfsr16 ^= lfsr16 >> 13;
counter16+=1;
if (lfsr16 == start_state16) { seed_lfsr16(); d = 2;}
//}
buffer[i] = buffer[i] ^ (lfsr16 & 0b11111111);
//buffer[i] = (lfsr16 & 0b11111111);
// Serial.println(buffer[i], DEC);
}
// 16 bit taps 
// 7,9, 13 65535 (from example)
// 3,5,11 65535 (I worked out)

// now dump the whitened entropy buffer into the 64byte UART buffer
// do this so the hardware UART can send the data while new entropy is loaded

digitalWrite(7, 1);
for (int i = 0;i<=(buffersize-1);i++) { 
  Serial.write(buffer[i]);
}
digitalWrite(7, 0);

// every hour run self test
if (millis() > (time_now + 3600000)){
  digitalWrite(6,0); // turn green LED off to show self test is underway and output has stopped
  self_test();
  digitalWrite(6,1); // turn green LED on because all tests must have passed
  time_now = millis();}
}
////////////////////////////////////// FUNCTIONS //////////////////////////////////////

// initialise lfsr16 with 16 random bits (non zero)
void seed_lfsr16(){
lfsr16 = 0;
 do {
  lfsr16 = lfsr16 | getnibble();
  lfsr16 = lfsr16 << 4;
  lfsr16 = lfsr16 | getnibble();
  lfsr16 = lfsr16 << 6;
  lfsr16 = lfsr16 | (getnibble() << 2);
  lfsr16 = lfsr16 | (getnibble() >> 2);
 } while (lfsr16 == 0);
start_state16 = lfsr16;
//Serial.println(start_state16, BIN);
//Serial.println(counter16, DEC);
counter16=0;
}

void seed_lfsr8(){
lfsr8 = 0;
 do {
  lfsr8 = lfsr8 | (getnibble() << 2);
  lfsr8 = lfsr8 | (getnibble() >> 2);
 } while (lfsr8 == 0);
start_state8 = lfsr8;
//Serial.println(start_state8, BIN);
//Serial.println(counter8, DEC);
counter8=0;
}

// function to get a random nibble
// Note: Instructions typically take 62.5nS on a 16MHz Arduino. Noise pulse is 800nS in width.

byte getnibble(){
byte aa = 0;
byte bb = 0;
byte dly = 0;

// add a random delay to create timimg jitter
dly = (PIND & 0b00111100) >> 2; // 0 to 15
// delayMicroseconds(dly + 1);
for (int i = 0;i<=dly;i++) { }
  
delayMicroseconds(10);
    aa = PIND & 0b00111100;
delayMicroseconds(10);
    bb = PIND & 0b00111100; 
return (aa ^ bb);
}

void self_test(){
int aa;
int bb;
unsigned long starttime;
unsigned long endtime;
unsigned long counter;  
float psu_avr;
int psu_max;
int psu_min;
int psu_delta;

// read the psu voltage for 1 second and check stats 

psu_delta = 1000;
psu_avr = 1000;
while (psu_delta > 25 || psu_avr < 498 || psu_avr > 512){
starttime = millis();
endtime = starttime;
counter = 0;
psu_min = 1000;
psu_max = 0;
psu_avr = 0;
while ((endtime - starttime) <=1000) {
  int val = analogRead(vin);
  if (val > psu_max) { psu_max = val; }
  if (val < psu_min) { psu_min = val; }
  psu_avr = psu_avr + val;
  counter +=1;
  endtime = millis();
}
psu_avr = psu_avr / (counter);
psu_delta = psu_max - psu_min;
//delay(3000);
//Serial.println(psu_avr);
if (psu_avr > 512) { flash(5); }
if (psu_avr < 498) { flash(6); }
if (psu_delta > 25) { flash(7); }
}
 
// check entropy channel frequencies (ideal >8000)
// 8000 is not the real count, only what the Arduino "sees" in the loop. Real value is much higher

unsigned long lowest = 7100;

// check channel 1 (d2)
counter =0;
while (counter < lowest) {
  starttime = millis();
  endtime = starttime;
  counter = 0;
  while ((endtime - starttime) <=1000) {
    aa = (PIND & 0b00000100);
    bb = (PIND & 0b00000100);
    if (bb > aa) { counter+=1; }
    endtime = millis();
  }
//Serial.println(counter);
if (counter < lowest) { flash(1); }
}

// check channel 2 (d3)
counter =0;
while (counter < lowest) {
  starttime = millis();
  endtime = starttime;
  counter = 0;
  while ((endtime - starttime) <=1000) {
    aa = (PIND & 0b00001000);
    bb = (PIND & 0b00001000);
    if (bb > aa) { counter+=1; }
    endtime = millis();
  }
//Serial.println(counter);
if (counter < lowest) { flash(2); }
}

// check channel 3 (d4)
counter =0;
while (counter < lowest) {
  starttime = millis();
  endtime = starttime;
  counter = 0;
  while ((endtime - starttime) <=1000) {
    aa = (PIND & 0b00010000);
    bb = (PIND & 0b00010000);
    if (bb > aa) { counter+=1; }
    endtime = millis();
  }
//Serial.println(counter);
if (counter < lowest) { flash(3); }
}

// check channel 4 (d5)
counter =0;
while (counter < lowest) {
  starttime = millis();
  endtime = starttime;
  counter = 0;
  while ((endtime - starttime) <=1000) {
    aa = (PIND & 0b00100000);
    bb = (PIND & 0b00100000);
    if (bb > aa) { counter+=1; }
    endtime = millis();
  }
//Serial.println(counter);
if (counter < lowest) { flash(4); }
}

}

void flash( int nblinks ){ 
// this version repeats the error code and never exits
  while (1){
  digitalWrite(6,0);
  delay(600);
  for (int i = 0;i<=(nblinks - 1);i++) { 
    digitalWrite(6,1);
    delay(200);
    digitalWrite(6,0);
     delay(200);
 }
 delay(400);
  }
}

// ********************************************************
// calibration mode. plug in usb without 12v to enter
void calibrate(){
int aa;
int bb;
unsigned long starttime;
unsigned long endtime;
unsigned long counter;  
float psu_avr;
int psu_max;
int psu_min;
int psu_delta;

while(1){

psu_delta = 1000;
psu_avr = 1000;
starttime = millis();
endtime = starttime;
counter = 0;
psu_min = 1000;
psu_max = 0;
psu_avr = 0;
while ((endtime - starttime) <=1000) {
  int val = analogRead(vin);
  if (val > psu_max) { psu_max = val; }
  if (val < psu_min) { psu_min = val; }
  psu_avr = psu_avr + val;
  counter +=1;
  endtime = millis();
}
psu_avr = psu_avr / (counter);
psu_delta = psu_max - psu_min;

Serial.print("PSU samples: ");
Serial.println(counter);
Serial.print("PSU Average: ");
Serial.println(psu_avr);
Serial.print("PSU min: ");
Serial.println(psu_min);
Serial.print("PSU max: ");
Serial.println(psu_max);
Serial.print("PSU delta: ");
Serial.println(psu_delta);
Serial.println();

// check channel 1 (d2)
counter =0;
starttime = millis();
endtime = starttime;
counter = 0;
while ((endtime - starttime) <=1000) {
  aa = (PIND & 0b00000100);
  bb = (PIND & 0b00000100);
  if (bb > aa) { counter+=1; }
  endtime = millis();
}
Serial.print("Channel 1 (d2) count: ");
Serial.println(counter);

// check channel 2 (d3)
counter =0;
starttime = millis();
endtime = starttime;
counter = 0;
while ((endtime - starttime) <=1000) {
  aa = (PIND & 0b00001000);
  bb = (PIND & 0b00001000);
  if (bb > aa) { counter+=1; }
  endtime = millis();
}
Serial.print("Channel 2 (d3) count: ");
Serial.println(counter);

// check channel 3 (d4)
counter =0;
starttime = millis();
endtime = starttime;
counter = 0;
while ((endtime - starttime) <=1000) {
  aa = (PIND & 0b00010000);
  bb = (PIND & 0b00010000);
  if (bb > aa) { counter+=1; }
  endtime = millis();
}
Serial.print("Channel 3 (d4) count: ");
Serial.println(counter);

// check channel 4 (d5)
counter =0;
starttime = millis();
endtime = starttime;
counter = 0;
while ((endtime - starttime) <=1000) {
  aa = (PIND & 0b00100000);
  bb = (PIND & 0b00100000);
  if (bb > aa) { counter+=1; }
  endtime = millis();
}
Serial.print("Channel 4 (d5) count: ");
Serial.println(counter);
Serial.println();
}
}

