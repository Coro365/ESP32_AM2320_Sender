#include <WiFi.h>
#include <HTTPClient.h>
#include <Ticker.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_AM2320.h>

// Network
const char* ssid = "<YOUR_SSID>"; // 2.4GHz ONLY
const char* password = "<YOUR_PW>";
const char* influxdb_addr = "http://192.168.0.210:8086";
const char* domain = "my_room";

// Location id (InfluxDB will use it)
int location = 1;         // Location Number

// Initialize global variable
bool ticker_flag = false;     // Every minute
String version_n = "20210801";// Version of NES01.ino

// Initialize global instance
Ticker ticker;
Adafruit_AM2320 am2320 = Adafruit_AM2320();


/////////////////////////////////////////
//             Ticker

void ticker_flag_up() {
  ticker_flag = true;
}

void ticker_flag_down() {
  ticker_flag = false;
}


/////////////////////////////////////////
//             Setup

void setup() {

  // Initialize Serial
  Serial.begin(115200);
  while (!Serial) {
    delay(10); // hang out until serial port opens
  }
  Serial.print("\n\n\n");
  Serial.println("Serial connect.          [ O K ]");

  // Initialize AM2320
  am2320.begin();
  Serial.println("AM2320 start.            [ O K ]");

  // Initialize ticker
  ticker.attach(60, ticker_flag_up);
  Serial.println("Ticker start.            [ O K ]");

  // Initialize WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  wifi_connect_wait();

  Serial.println("                         [ O K ]");

  // Send boot log
  boot_log();
  Serial.println("Boot log send.           [ O K ]");
  
  
  Serial.print("\n=========== v" + version_n + " ===========\n");
  Serial.print("======== Setup complete! ========\n\n\n");
}


/////////////////////////////////////////
//             Network

void wifi_connect_wait() {
  Serial.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("  SSID: "); Serial.println(ssid);
  Serial.print("  IP:   "); Serial.println(WiFi.localIP());
}


void influx_post(String payload, bool mes_flg) {
  if (payload == "") {
    Serial.println("[ERROR] InfluxDB payload empty.");
    return;
  } else {
    if (mes_flg == true){ Serial.print(payload); }
  }

  HTTPClient http;
  http.begin(String(influxdb_addr) + "/write?db=home-sensor");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int http_code;
  for (int i = 0; i < 5; i++) {
    http_code = http.POST(payload);
    if (http_code == 204) {
      if (mes_flg == true){ Serial.printf("[ O K ] InfluxDB send!\n\n"); }
      break;
    } else {
      Serial.printf("[ERROR] Sending failure! %02dtimes.\n[ERROR] HTTP Status Code: %d\n", i, http_code); delay(1000);
    }
  }

  http.end();
}

void boot_log() {
  String payload = "iotboot,domain=" + String(domain) + ".local,ip=" + WiFi.localIP().toString() + ",location=" + String(location) + " value=1"+ "\n";
  influx_post(payload, false);
}


/////////////////////////////////////////
//              Loop

void loop() {
  if (ticker_flag) {
    senser();
    ticker_flag_down();
  }
}

void senser() {
  String payload = "";
  payload += "temperature,location=" + String(location) + " value=" + String(am2320.readTemperature()) + "\n";
  payload += "humidity,location=" + String(location) + " value=" + String(am2320.readHumidity()) + "\n";
  influx_post(payload, true);
}
