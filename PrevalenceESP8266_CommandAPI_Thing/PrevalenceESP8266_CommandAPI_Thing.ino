#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <CommandHandler.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>

#define WIFI_AP ""
#define WIFI_PASSWORD ""

String TOKEN = "5C:CF:7F:34:39:BD";  //9jPma5EpXY0blSVNot3P
char thingsboardServer[] = "mqtt.thingcontrol.io";

static const char *fingerprint PROGMEM = "69 E5 FE 17 2A 13 9C 7C 98 94 CA E0 B0 A6 CB 68 66 6C CB 77";
unsigned long startMillis;  //some global variables available anywhere in the program
unsigned long currentMillis;
const unsigned long period = 500;  //the value is a number of milliseconds
CommandHandler cmdHdl("=", '\n');

WiFiClientSecure wifiClient;
PubSubClient client(wifiClient);

int status = WL_IDLE_STATUS;
String downlink = "";

struct device
{
  byte power;
  byte temp;
  byte mod;
  byte fan;
  byte louv;
};
typedef struct device Device;
Device aDevice;

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void setup() {
  
  Serial.begin(9600);
  Serial.println(F("Cloud on Ship Thingcontrol.io"));
  WiFiManager wifiManager;
  //wifiManager.resetSettings();
  wifiManager.setAPCallback(configModeCallback);

  if (!wifiManager.autoConnect("@Thingcontrol.io")) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  wifiClient.setFingerprint(fingerprint);
  client.setServer( thingsboardServer, 8883 );
  client.setCallback(callback);
  reconnectMqtt();

  cmdHdl.addCommand("AT", ATcheck);
  cmdHdl.addCommand("AT+TEMP", setTemp);
  cmdHdl.addCommand("AT+MODE", setMod);
  cmdHdl.addCommand("AT+FAN", setFan);
  cmdHdl.addCommand("AT+POWER", setPower);
  cmdHdl.addCommand("AT+LOUV", setLouv);

  cmdHdl.addCommand("AT&MAC",   getMac);
  cmdHdl.addCommand("AT+MAC", processToken);
  cmdHdl.addCommand("AT&V",     viewActive);
  cmdHdl.addCommand("AT+SETWiFi", setWiFi);
  //  cmdHdl.addCommand("AT+ATTSEND", processAtt);
  cmdHdl.addCommand("AT&STATUS", processStatus);
  //  cmdHdl.addCommand("AT+TELSEND", processTele);
  cmdHdl.addCommand("AT&MEM", getMem);
  cmdHdl.addCommand("AT&RESET", reboot);
  cmdHdl.addCommand("AT?", commandHelp);
  cmdHdl.setDefaultHandler(unrecognized);

  cmdHdl.setCmdHeader("+");
  cmdHdl.initCmd();
  cmdHdl.addCmdString("Boot..");
  cmdHdl.addCmdString("OK..");
  cmdHdl.addCmdString("RDY!!");
  cmdHdl.addCmdTerm();

  Serial.println(cmdHdl.getOutCmd());
  cmdHdl.sendCmdSerial();
  Serial.println();
  startMillis = millis();  //initial start time

}

void loop()
{
  status = WiFi.status();
  if ( status == WL_CONNECTED)
  {
    if ( !client.connected() )
    {
      reconnectMqtt();
    }
    client.loop();
  }

  cmdHdl.processSerial(Serial);
  currentMillis = millis();  //get the current "time" (actually the number of milliseconds since the program started)
  if (currentMillis - startMillis >= period)  //test whether the period has elapsed
  {
    //    processCall();
    startMillis = currentMillis;  //IMPORTANT to save the start time of the current LED state.


  }

}

void getMem() {
  Serial.print("ESP:");
  Serial.println(ESP.getFreeHeap());
}
void ATcheck()
{
  Serial.println("OK");
}

void getMac()
{
  Serial.println("OK");
  Serial.print("+TOKEN: ");
  Serial.println(WiFi.macAddress());
}

void setTemp()
{
  char *aString;

  aString = cmdHdl.readStringArg();
  Serial.print("aString:");
  Serial.println(aString);

  if (cmdHdl.argOk) {
    String json = "{\"temp\":";
    json.concat(aString);
    json.concat("}");
    Serial.println(json);
    client.publish( "v1/devices/me/telemetry", json.c_str());
    Serial.println("OK");
    Serial.print("+:topic v1/devices/me/telemetry , ");

    //    client.publish("v1/devices/me/rpc/response/" + 44, "{\"getTemp\":99");

  } else {
    Serial.println("ERROR");
  }

}
void setMod()
{
  char *aString;

  aString = cmdHdl.readStringArg();
  Serial.print("aString:");
  Serial.println(aString);

  if (cmdHdl.argOk) {
    String json = "{\"mode\":";
    json.concat(aString);
    json.concat("}");
    Serial.println(json);
    client.publish( "v1/devices/me/telemetry", json.c_str());
    Serial.println("OK");
    Serial.print(F("+:topic v1/devices/me/telemetry , "));
    aDevice.mod=String(aString).toInt();

  }
  else {
    Serial.println("ERROR");
  }
}
void setFan()
{
  char *aString;

  aString = cmdHdl.readStringArg();
  Serial.print("aString:");
  Serial.println(aString);

  if (cmdHdl.argOk) {
    String json = "{\"fan\":";
    json.concat(aString);
    json.concat("}");
    Serial.println(json);
    client.publish( "v1/devices/me/telemetry", json.c_str());
    Serial.println("OK");
    Serial.print(F("+:topic v1/devices/me/telemetry , "));
    aDevice.fan=String(aString).toInt();
  }
  else {
    Serial.println("ERROR");
  }
}
void setPower()
{
  char *aString;

  aString = cmdHdl.readStringArg();
  Serial.print("aString:");
  Serial.println(aString);

  if (cmdHdl.argOk) {
    Serial.println("OK");
    String json = "{\"pow\":";
    json.concat(aString);
    json.concat("}");
    Serial.println(json);
    client.publish( "v1/devices/me/telemetry", json.c_str());
    Serial.println("OK");
    Serial.print(F("+:topic v1/devices/me/telemetry , "));
    aDevice.power=String(aString).toInt();
  }
  else {
    Serial.println("ERROR");
  }
}
void setLouv()
{
  char *aString;

  aString = cmdHdl.readStringArg();
  Serial.print("aString:");
  Serial.println(aString);

  if (cmdHdl.argOk) {
    Serial.println("OK");
    String json = "{\"louv\":";
    json.concat(aString);
    json.concat("}");
    Serial.println(json);
    client.publish( "v1/devices/me/telemetry", json.c_str());
    Serial.println("OK");
    Serial.print(F("+:topic v1/devices/me/telemetry , "));
    aDevice.louv=String(aString).toInt();

  }
  else {
    Serial.println("ERROR");
  }
}
void viewActive()
{
  Serial.println("OK");
  Serial.print("+:WiFi, ");
  Serial.println(WiFi.status());
  if (client.state() == 0)
  {
    Serial.print(F("+:MQTT, [CONNECTED] [status="));
    Serial.print( client.state() );
  }
  else
  {
    Serial.print(F("+:MQTT, [FAILED] [rc="));
    Serial.print( client.state() );
    Serial.println( " : retrying]" );
  }
}

void setWiFi()
{
  Serial.println("OK");
  Serial.println(F("+:Reconfig WiFi  Restart..."));
  WiFiManager wifiManager;
  wifiManager.resetSettings();
  if (!wifiManager.startConfigPortal("@ThingControl.io"))
  {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  Serial.println("OK");
  Serial.println("+:WiFi Connect");

  wifiClient.setFingerprint(fingerprint);
  client.setServer( thingsboardServer, 8883 );
  client.setCallback(callback);
  reconnectMqtt();
}

void processStatus()
{
  //  Serial.println("OK");
  //  Serial.print("+:");
  Serial.print("pow:");
  Serial.println(aDevice.power);
  Serial.print("temp:");
  Serial.println(aDevice.temp);
  Serial.print("mode:");
  Serial.println(aDevice.mod);
  Serial.print("fan:");
  Serial.println(aDevice.fan);
  Serial.print("louv:");
  Serial.println(aDevice.louv);

}


void reboot() {
  Serial.println("...shutting down..");
  delay(2000);
  ESP.reset();
}


void processAtt()
{
  char *aString;

  aString = cmdHdl.readStringArg();
  if (cmdHdl.argOk) {
    Serial.println("OK");
    Serial.print(F("+:topic v1/devices/me/attributes , "));
    Serial.println(aString);
    client.publish( "v1/devices/me/attributes", aString);

  }
  else {
    Serial.println("ERROR");
  }
}

void processTele()
{
  char *aString;

  aString = cmdHdl.readStringArg();
  if (cmdHdl.argOk) {
    Serial.println("OK");
    Serial.print(F("+:topic v1/devices/me/telemetry , "));
    Serial.println(aString);
    client.publish( "v1/devices/me/telemetry", aString);

  }
  else {
    Serial.println("ERROR");
  }
}

void processToken()
{
  char *aString;

  aString = cmdHdl.readStringArg();
  if (cmdHdl.argOk) {
    Serial.println("OK");
    Serial.print("+:TOKEN , ");
    Serial.println(aString);
    TOKEN = aString;
  }
  else {
    Serial.println("ERROR");
  }
  reconnectMqtt();
}

void unrecognized(const char *command)
{
  Serial.println("ERROR");
}

void callback(char* topic, byte* payload, unsigned int length)
{
  String id = "";
//  Serial.print("Message arrived [");
//  Serial.print(topic);

  String topicStr = topic;
  int start_ = topicStr.lastIndexOf("/");
  int stop_ = topicStr.length();
//  Serial.print("id:");
//  Serial.println(topicStr.substring(start_ + 1, stop_));
  id =  String(topicStr.substring(start_ + 1, stop_));
  String topicResp = "v1/devices/me/rpc/response/";
  topicResp.concat(id);
  Serial.print("response:");  Serial.println(topicResp);
//  char *res = "{\"method\":\"setPow\",\"params\":false}";
//  client.publish(topicResp.c_str(),res  , 0);
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  char json[length + 1];
  strncpy (json, (char*)payload, length);
  json[length] = '\0';

  Serial.print("  Topic: ");
  Serial.println(topic);
  Serial.print("  Message: ");
  Serial.println(json);

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& data = jsonBuffer.parseObject((char*)json);

  String methodName = String((const char*)data["method"]);
  if (!methodName.equals("")) {
    String param = String((const char*)data["params"]);

    commandParser(methodName, param);
  }

  if (methodName.equals("setValue"))
  {
    char json[length + 1];
    strncpy (json, (char*)payload, length);
    json[length] = '\0';

    downlink = json;
  }
  else
  {
    downlink = "";
  }
  //      Serial.println(topicResp.c_str());
  //      client.publish(topicResp.c_str() , "{\"temp\":\"30\"}");


}

void commandParser(String cmd, String params) {
  Serial.print("cmd:");  Serial.print(cmd);  Serial.print("=");  Serial.println(params);
  if (cmd.equals("setPow")) {
    if (params.equals("true")) {
    aDevice.power = 1;
  } else {
    aDevice.power = 0;
  }

} else if (cmd.equals("setTemp")) {
    aDevice.temp = params.toInt();
  } else if (cmd.equals("setMode")) {
    aDevice.mod = params.toInt();
  } else if (cmd.equals("setFan")) {
    aDevice.fan = params.toInt();
  } else if (cmd.equals("setLouv"))  {
    aDevice.louv = params.toInt();

  } else {
    Serial.println("command not found");
  }



}
void reconnectMqtt()
{
  if ( client.connect("Thingcontrol_AT", TOKEN.c_str(), TOKEN.c_str()) )
  {
    Serial.println( "Connect MQTT Success." );
    client.subscribe("v1/devices/me/rpc/request/+");
  } else {
    Serial.print( "[FAILED] [ rc = " );
    Serial.print( client.state() );
    Serial.println( " : retrying]" );
  }
}

void commandHelp()
{

  Serial.println("OK");
  Serial.println("\"AT+TEMP\" :Set Temp");
  Serial.println("\"AT+FAN\" :Set Fan");
  Serial.println("\"AT+MODE\" :Set Mode");
  Serial.println("\"AT+LOUV\" :Set LOUV");
  Serial.println("\"AT&MAC\" :Get Token");
  Serial.println("\"AT+MAC\" :Set Token Ex. AT+MAC=9jPma5EpXY0blSVNot3P");
  Serial.println("\"AT&V\"   :View active configuration");
  Serial.println("\"AT&MEM\"   :Get Available Memory");
  Serial.println("\"AT+SETWiFi\"  :Redirect to AP WiFi Seting mode");
  //  Serial.println("\"AT+ATTSEND\"  :Send JSON to server topic v1/devices/me/attributes");
  Serial.println("\"AT&STATUS\"   :Get JSON Call back");
  Serial.println("\"AT&RESET\" :Reset MCU");
  //  Serial.println("\"AT+TELSEND\"  :Send JSON to server topic v1/devices/me/telemetry");
}
