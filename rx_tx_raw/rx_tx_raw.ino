#include <Wire.h>
#include <SSD1306.h>

//EEPROM library
#include <EEPROM.h>

SSD1306  display(0x3c, D3, D5);

#define tx_pin D6    //Pin to which IR LED is connected
#define rx_pin D2
#define trigger_pin D1
#define hit_pin LED_BUILTIN
#define disp_update_interval 1000
#define hp_delay 250
#define ammo_delay 100

//Player Variables
boolean team_id = 1;
uint8_t player_id = 0;
uint8_t max_hp = 100;
uint8_t max_ammo = 20 ;
uint8_t hp = max_hp;        //hp indicates life of player at current time
uint8_t ammo = max_ammo;    //ammo indicates player bullets at current time
uint8_t hp_damage = 5;
uint8_t last_hp = hp, last_ammo = ammo;
//IR Communication Variables
uint16_t data_duration[2] = {4000, 8000};

//session variables
int time_minutes = 5; //Game wil get reset after 5 minutes...i.e., session over
unsigned long time_limit = time_minutes * 60 * 1000; //converting session time into milliseconds
unsigned long last_ms = 0;
uint16_t seconds = time_minutes * 60;
boolean time_up = false;

void setup() {
  init_pins();
  init_display();
  init_EEPROM();
  Serial.begin(9600);
  last_ms = millis();
}

void loop() {
  // put your main code here, to run repeatedly:
  if (time_up)
  {
    display_game_over();
    delay(100);
  }
  rx_data();
  if(!time_up)
  {
     update_display(); 
  }
  if (Serial.available() > 0)
  {
    if (Serial.read() == 'R')
    {
      //new_game = true;
      hp = max_hp;
      ammo = max_ammo;
      seconds = time_minutes * 60;
      time_up = false;
      update_EE();
      //new_game = false;
    }
  }
}

void start_new_game()
{
  hp = max_hp;
  ammo = max_ammo;
  seconds = time_minutes * 60;
  time_up = false;
  update_EE();
}

void update_display()
{
  if ((millis() - last_ms) > disp_update_interval)
  {
    last_ms = millis();
    if (seconds <= 0)
    {
      time_up = true;
    }
    else
    {
      seconds--;
    }
    update_EE();  //Update EEPROM
    display_hp_ammo();
    last_hp = hp;
    last_ammo = ammo;
  }
  else if (last_hp != hp || last_ammo != ammo && hp > 0 && ammo > 0)
  {
    display_hp_ammo();
    last_hp = hp;
    last_ammo = ammo;
  }
  if (hp == 0)
  {
    display_player_dead();
  }
  else if (ammo == 0)
  {
    display_ammo_over();
  }
}

void update_EE()
{
  EEPROM.write(0, hp);
  EEPROM.commit();
  EEPROM.write(1, ammo);
  EEPROM.commit();
  EEPROM.write(2, (seconds & 0xFF));
  EEPROM.commit();
  EEPROM.write(3, ((seconds >> 8) & 0xFF));
  EEPROM.commit();
}

void tx_data()
{
  cli();
  if (ammo > 0 && hp > 0)
  {
    ammo--;
    oscillationWrite(tx_pin, data_duration[team_id]);
    digitalWrite(tx_pin, LOW);
    delay(ammo_delay);
  }
  sei();
}

void player_hit()
{
  if (hp <= 0)
  {
    hp = 0;
  }
  else
  {
    hp = hp - hp_damage;
  }
}

void rx_data()
{
  int pulse = pulseIn(rx_pin, LOW);
  if ((pulse > (data_duration[!team_id] - 2000)) && (pulse < (data_duration[!team_id] + 2000)))
  {
    digitalWrite(hit_pin, LOW);
    //    Serial.println(pulse);
    player_hit();
    delay(hp_delay);
  }
  digitalWrite(hit_pin, HIGH);
}

void oscillationWrite(int pin, int time)
{
  for (int i = 0; i <= time / 26; i++) {
    digitalWrite(pin, HIGH);
    delayMicroseconds(13);
    digitalWrite(pin, LOW);
    delayMicroseconds(13);
  }
}

void display_player_dead()
{
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 10, "health: " + String(hp));
  display.setFont(ArialMT_Plain_10);
  display.drawString(40, 40, "PLAYER DEAD!!!");
  display.display();
}

void display_game_over()
{
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 10, "Time Up!" );
  display.setFont(ArialMT_Plain_10);
  display.drawString(40, 40, "Game Over!!");
  display.display();
}

void display_ammo_over()
{
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 10, "AMMO: " + String(ammo));
  display.setFont(ArialMT_Plain_10);
  display.drawString(40, 40, "AMMO OVER!!!");
  display.display();
}

void display_hp_ammo()
{

  int disp_hp = map(hp, 0, max_hp, 0, 100);
  int disp_ammo = map(ammo, 0, max_ammo, 0, 100);
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "TECHNITES    Timer:" +  String(seconds));
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 22, "hp:" + String(hp));
  display.drawProgressBar(0, 34, 100, 8, disp_hp);
  display.drawString(0, 44, "Ammo:" + String(ammo));
  display.drawProgressBar(0, 55 , 100, 8, disp_ammo);
  display.display();
}

void init_EEPROM()
{
  //EEPROM - 4 bytes
  EEPROM.begin(4);
  hp = EEPROM.read(0);
  ammo = EEPROM.read(1);
  uint16_t msb_secs = EEPROM.read(3);
  seconds = ((msb_secs << 8) | (EEPROM.read(2)));
}

void init_display()
{
  display.init();
  display.flipScreenVertically();
  display_hp_ammo();
}

void init_pins()
{
  pinMode(tx_pin, OUTPUT);
  pinMode(rx_pin, INPUT);
  pinMode(trigger_pin, INPUT_PULLUP);
  pinMode(hit_pin, OUTPUT);

  digitalWrite(hit_pin, HIGH);
  attachInterrupt(digitalPinToInterrupt(trigger_pin), tx_data, FALLING);
}

