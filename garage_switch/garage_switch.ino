#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <WIFIcredentials.h>

const int output = 4;
const int LED = 2;
#define DHTPIN 25     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22     // DHT 22 (AM2302)

// Variables for reconnecting WIFI
unsigned long previousMillis = 0;
unsigned long interval = 30000;

DHT dht(DHTPIN, DHTTYPE);
float t_bak = 999;
float t = 999;

String readDHTTemperature() {
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  // Read temperature as Celsius (the default)
  float t_bak = t;
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  //float t = dht.readTemperature(true);
  // Check if any reads failed and exit early (to try again).
  Serial.println("-----------------------------------------------------------");
  if (isnan(t)) {
    Serial.println("Failed to read from DHT sensor!\nReturning old value for Temperature: " + String(t_bak));
    String asw = String(t_bak) + "!";
    return asw;
  }
  else {
    Serial.println(t);
    return String(t);
  }
}

float h_bak = 999;
float h = 999;

String readDHTHumidity() {
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  h_bak = h;
  float h = dht.readHumidity();
  if (isnan(h)) {
    Serial.println("Failed to read from DHT sensor!\nReturning old value for Humidity: " + String(h_bak));
    String asw = String(h_bak) + "!";
    return asw;
  }
  else {
    Serial.println(h);
    return String(h);
  }
}

// HTML web page
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>Garage</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="icon" type="image/png">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
    <style>
    html {
    font-family: Arial;
    display: inline-block;
    margin: 0px auto;
    text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
        font-size: 1.5rem;
        vertical-align:middle;
        padding-bottom: 15px;
    }
    body { font-family: Arial; text-align: center; margin:0px auto; padding-top: 30px;}

    .button {
        display: block;
        height: 200px;
        width: 200px;
        background-color: #008000;
        border: none;
        color: white;
        padding: 0px 0px;
        text-decoration: none;
        font-size: 40px;
        margin: 0px auto 35px;
        cursor: pointer;
        border-radius: 100px;
        line-height: 190px;
        box-shadow: 0px 4px 10px #002100;
    }
    .button:active{
        box-shadow: 0px 0px 0px #002100;
    }
    </style>
  </head>
  <body>
    <h1>Klick to open or close!</h1>
    <p>
      <a class="button" href="/switch" title="Home">Klick</a>
    </p>
    <p>
      <i class="fas fa-thermometer-half" style="color:#059e8a;"></i>
      <span class="dht-labels">Temperature</span>
      <span id="temperature">__.__</span>
      <sup class="units">&deg;C</sup>
    </p>
    <p>
      <i class="fas fa-tint" style="color:#00add6;"></i>
      <span class="dht-labels">Humidity</span>
      <span id="humidity">__.__</span>
      <sup class="units">&percnt;</sup>
    </p>

</script>
</body>
<script>
    /* Skript gleich beim Laden ausfuehren, dann alle x Sekunden! */
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
        document.getElementById("temperature").innerHTML = this.responseText;
        }
    };
    xhttp.open("GET", "/temperature", true);
    xhttp.send();

    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
        document.getElementById("humidity").innerHTML = this.responseText;
        }
    };
    xhttp.open("GET", "/humidity", true);
    xhttp.send();

    setInterval(function ( ) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
        document.getElementById("temperature").innerHTML = this.responseText;
        }
    };
    xhttp.open("GET", "/temperature", true);
    xhttp.send();
    }, 30000 ) ;

    setInterval(function ( ) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
        document.getElementById("humidity").innerHTML = this.responseText;
        }
    };
    xhttp.open("GET", "/humidity", true);
    xhttp.send();
    }, 30000 ) ;

</script>
</html>)rawliteral";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

AsyncWebServer server(80);

void success()
{
  for(int i=0; i<3; i++)
  {
    digitalWrite(LED, HIGH);
    delay(500);
    digitalWrite(LED, LOW);
    delay(500);
  }
}

void setup() {
  Serial.begin(115200);

  dht.begin();

  pinMode(output, OUTPUT);
  digitalWrite(output, LOW);

  pinMode(LED, OUTPUT);
  digitalWrite(output, LOW);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println();
  Serial.print("ESP IP Address: http://");
  Serial.println(WiFi.localIP());
  success();

  if (!SPIFFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  pinMode(output, OUTPUT);
  digitalWrite(output, LOW);

  // Send web page to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html);
  });

  // Receive an HTTP GET request
  server.on("/switch", HTTP_GET, [] (AsyncWebServerRequest * request) {
    digitalWrite(output, HIGH);
    delay(500);
    digitalWrite(output, LOW);
    request->send(200, "text/html", index_html);
  });

  server.on("/icon", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/G.png", "image/png");
  });

  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", readDHTTemperature().c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/plain", readDHTHumidity().c_str());
  });

  server.onNotFound(notFound);
  server.begin();
}

void loop() {
  unsigned long currentMillis = millis();
  // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >=interval)) {
    Serial.print(millis());
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    previousMillis = currentMillis;
  } 
}
