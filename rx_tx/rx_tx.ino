#include <IRremote.h>
#include <EEPROM.h>

uint8_t rx_pin  = 5;   //38khz TSOP Receiver
uint8_t trigger_pin = 2;  //Pushbutton (pulled-up) acts as trigger for gun
uint8_t buzzer_pin = 12; //buzzer to indicate firing and receiving of signal

unsigned long data; //info about the player using this gun - includes player ID and damage of gun
unsigned long hit_data; //info about player who made hit
uint8_t hit_id = 0; // id of player who made the hit
uint8_t hit_team = 0; //team id of player who made hit
uint8_t hit_msg = 0;  //info about the gun from which player got shot

uint8_t player_id = 7;  //id of player using this gun
uint8_t team_id = player_id / 7; //team of player using this gun

uint8_t max_hp = 100; //maximum hp this player can have
uint8_t max_ammo = 50;  //maximum ammo this player can have
uint8_t damage = 5;  //damage that can be done using this gun
uint8_t hp = max_hp;  //health of the player
uint8_t ammo = max_ammo;  //ammunition of the player gun

boolean friendly_fire = 1;
boolean new_game = false;
IRrecv irrecv(rx_pin);
decode_results results;

IRsend irsend;

boolean lastbutton_state = HIGH;
boolean button_state = HIGH;

void setup()
{
  //pin declarations
  pinMode(trigger_pin, INPUT_PULLUP); // Button Pin Active low
  Serial.begin(115200);
  irrecv.enableIRIn(); // Start the receiver
  prepare_data();
  hp = EEPROM.read(0);
  ammo = EEPROM.read(1);
}

void loop()
{
  if(Serial.available() > 0)
  {
    if(Serial.read() == '1')
    new_game = 1;
  }
  if(new_game == true)
  {
    hp = max_hp;
    ammo = max_ammo;
    new_game = false;
    update_EE();
  }
 // Serial.println("reset:" + String(new_game) + " " + String(hp) + " " + String(ammo));
  hit_data = 0;
  tx_rx_check();
}

void player_hit()
{
  if (friendly_fire == 1)
  {
    hp -= hit_msg;
    Serial.print("hp: ");
    Serial.println(hp);
    if (hp > max_hp || hp == 0)
    {
      hp = 0;
      player_dead();
    }
  }
  else if (team_id != hit_team)
  {
    hp -= hit_msg;
    Serial.print("hp: ");
    Serial.println(hp);
    if (hp > max_hp || hp == 0)
    {
      hp = 0;
      player_dead();
    }
  }
}

void decode_rx_data()
{
  hit_id = (hit_data & 0xF000) >> 12;
  hit_team = hit_id / 7;
  hit_msg = (hit_data & 0x0F00) >> 8;
  Serial.println("team id: " + (String)hit_team + " player id: " + (String)hit_id + " message: " + (String)hit_msg);
}

void got_hit()
{
  decode_rx_data();
  if (hit_id == 0)
  {
    //god gun command
  }
  else if (hit_id > 0 || hit_id < 13)
  {
    player_hit();
  }
  else if (hit_id == 13)
  {
    //medic_kit
  }
  else if (hit_id == 14 || hit_id == 15) {
    //grande or mine
  }

}

void player_dead()
{
  Serial.println("player dead!!!");
  tone(buzzer_pin, 50);
  delay(1000);
  noTone(buzzer_pin);
  delay(1000);
}

void ammo_over()
{
  Serial.println("Ammo Over!!");
  tone(buzzer_pin, 50);
  delay(100);
  noTone(buzzer_pin);
  delay(1000);
}

void update_EE()
{
  EEPROM.write(0,hp);
  EEPROM.write(1,ammo);
}
void tx_rx_check()
{
  button_state = digitalRead(trigger_pin);
  if (button_state == LOW && lastbutton_state == HIGH)
  {
    delayMicroseconds(1000);
    button_state = digitalRead(trigger_pin);
    if (button_state == LOW && lastbutton_state == HIGH)
    {
      if(ammo > 0 && hp > 0)
      {
        ammo--;
        update_EE();
      Serial.println("Pressed, sending");
      irsend.sendNEC(data, 16);
      delay(50); // Wait a bit
      irrecv.enableIRIn();
    }
    else if(ammo <= 0)
    {
      ammo_over();
    }
    else
    {
      player_dead();
    }
    }
  }
  else if (irrecv.decode(&results)) {
    hit_data = results.value;
    if (hit_data & 0xFFFF00FF)
    {
      Serial.println("Wrong data");
    }
    else
    {
      got_hit();
      tone(buzzer_pin, 100);
      digitalWrite(13, HIGH);
      Serial.println(hit_data, HEX);
      delay(50);
      digitalWrite(13, LOW);
      noTone(buzzer_pin);
    }
    irrecv.resume(); // resume receiver

  }

  lastbutton_state = button_state;
}

void prepare_data()
{
  data = 0;
  data |= (damage << 8);
  data |= (player_id << 12);
  Serial.println(data, HEX);
}
