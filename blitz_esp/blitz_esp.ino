#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include <Wire.h>
#include "SSD1306.h"

const char* ssid = "kamal";       //my WiFi or Hotspot name
const char* password = "kamalkamal";//my WiFi Password
SSD1306  display(0x3c, 0, 2);

uint8_t hp, ammo;
uint8_t max_hp = 100; //maximum hp this player can have
uint8_t max_ammo = 50;  //maximum ammo this player can have
unsigned long update_db_interval = 1000;

uint8_t player_ID = 1;//total of 12 players
//session variables
int time_minutes = 5; //Game wil get reset after 5 minutes...i.e., session over
unsigned long time_limit = time_minutes * 60 * 1000; //converting session time into milliseconds
unsigned long present_ms = 0,last_ms =0,update_db,time_ms = 0;

boolean new_game = false;

String player_name = "Test";
int reset = 0, prev_reset = 0;
String server = "http://192.168.137.1/";
int port = 80; //default port
String response;

int time_up = 0;

String ard_data;

HTTPClient http;//http client object to communicate with website

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  present_ms = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //Serial.print(".");
    if(present_ms - time_ms > 10000)
    {
      time_ms = present_ms;
      break;
    }
  }
  req_server();  //fetch player detail
  display.init();
  display.flipScreenVertically();
  display_hp_ammo();
}

void loop() {
  // put your main code here, to run repeatedly:
  present_ms = millis();
  if(Serial.available() > 0)
  {
    ard_data = Serial.readStringUntil('\n');
    parse_data();
    Serial.println((String)hp + " " + (String)ammo);
  }
  if (reset != 0 && prev_reset == 0)
  {
    Serial.println("#"+(String)reset + "#" + (String)time_up + '#');
    new_game = true;
    time_ms = present_ms;
  }
  else
  {
    new_game = false;
  }
  prev_reset = reset;
  if(present_ms - last_ms > 1000)
  {
    last_ms = present_ms;
    if(hp > 0 && ammo > 0)
    {
      display_hp_ammo();
    }
    else if(ammo <= 0)
    {
      display_ammo_over();
    }
    else
    {
      display_player_dead();
    }
  }
  if (present_ms - time_ms > time_limit)
  {
    Serial.println("#"+(String)reset + "#" + (String)time_up + '#');
    display_game_over();
    req_server();
    delay(100);
  }
  else
  {
    if (present_ms - update_db > update_db_interval)
    {
      display_hp_ammo();
      //Serial.priintln("requestin server");
      req_server();
      update_db = present_ms;
    }
  }
}

void parse_data()
{
  int l = ard_data.length(), k = 0;
  int limits[100];
  for (int i = 0; i < l - 1 ; i++)
  {
    if (ard_data[i] == '#')
    {
      limits[k] = i + 1;
      k++;
    }
  }
  Serial.println(ard_data);
  String temp = ard_data.substring(limits[0], limits[1] - 1);
  hp = temp.toInt();
  temp = ard_data.substring(limits[1], limits[2] - 1);
  ammo = temp.toInt();
}
void req_server()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    String url = "LaserTag/get_player_data.php/?player=";
    url += String(player_ID);
    url += "&hp=";
    url += String(hp);
    url += "&ammo=";
    url += String(ammo);
    url += "&time=";
    url += String(present_ms - time_ms);

    String request = server + url;
    //Serial.priintln(request);
    http.begin(request);
    int response_code = http.GET();
    if (response_code == HTTP_CODE_OK)
    {
      response = http.getString();
      parse_response();
    }
    
    else {
      //Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(response_code).c_str());
    }
    //Serial.println(response);
    http.end();
    delay(10);
  }
}



void parse_response()
{
  int l = response.length(), k = 0;
  int limits[100];
  for (int i = 0; i < l - 1 ; i++)
  {
    if (response[i] == '#' && response[i + 1] == '_')
    {
      limits[k] = i + 2;
      k++;
    }
  }
  player_name = response.substring(limits[0], limits[1] - 2);
  String temp = response.substring(limits[1], limits[2] - 2);
  reset = temp.toInt();
  //Serial.priintln(player_name);
}

void display_hp_ammo()
{
  int disp_hp = map(hp, 0, max_hp, 0, 100);
  int disp_ammo = map(ammo, 0, max_ammo, 0, 100);
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, "Name: " + String(player_name));
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 22, "hp:" + String(hp));
  display.drawProgressBar(0, 34, 100, 8, disp_hp);
  display.drawString(0, 44, "Ammo:" + String(ammo));
  display.drawProgressBar(0, 55 , 100, 8, disp_ammo);
  display.display();
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

