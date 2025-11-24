#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

Preferences prefs;
WebServer server(80);

String ssid_saved = "";
String pass_saved = "";

/* -------------------------------------------------------
    Utility
------------------------------------------------------- */
String generateWiFiList() {
  int n = WiFi.scanNetworks();
  if (n == 0) return "<option>No WiFi found</option>";

  String list = "";
  for (int i = 0; i < n; i++) {
    String ssid = WiFi.SSID(i);
    int rssi = WiFi.RSSI(i);
    String enc  = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Secured";

    list += "<option value='" + ssid + "'>" + ssid +
            " (" + String(rssi) + " dBm, " + enc + ")</option>";
  }
  return list;
}

bool checkInternet() {
  WiFiClient client;
  if (client.connect("google.com", 80)) {
    client.stop();
    return true;
  }
  return false;
}

bool connectWiFi(const String &ssid, const String &pass, uint32_t timeout_ms = 5000) {
  WiFi.begin(ssid.c_str(), pass.c_str());
  uint32_t start = millis();

  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > timeout_ms) return false;
    delay(200);
    Serial.print(".");
  }
  return true;
}

/* -------------------------------------------------------
    HTML Templates
------------------------------------------------------- */
String htmlHeader(const String &title) {
  return
    "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>" + title +
    "</title><style>"
    "body{font-family:Arial;background:#f0f2f5;margin:0;padding:20px;text-align:center;}"
    ".card{background:#fff;padding:25px;margin:auto;width:330px;border-radius:12px;"
    "box-shadow:0 4px 15px rgba(0,0,0,0.15);} "
    "input,button{width:95%;padding:10px;margin:10px 0;border-radius:8px;border:1px solid #ccc;font-size:15px;}"
    "button{background:#0078ff;color:white;border:none;cursor:pointer;}button:hover{background:#005fe0;}"
    ".red{background:#e63946 !important;}"
    "</style></head><body>";
}

String htmlFooter() {
  return "</body></html>";
}

/* -------------------------------------------------------
    Web Handlers
------------------------------------------------------- */
void handleRoot() {
  String wifiList = generateWiFiList();

  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta charset="UTF-8">
    <title>WiFi Config</title>
    <style>
      body {
        font-family: Arial, sans-serif;
        background: #f0f2f5;
        display: flex;
        justify-content: center;
        align-items: center;
        height: 100vh;
        margin: 0;
      }
      .card {
        background: #fff;
        padding: 25px 30px;
        width: 320px;
        border-radius: 12px;
        box-shadow: 0 4px 15px rgba(0,0,0,0.15);
        text-align: center;
      }
      select, input {
        width: 95%;
        padding: 10px;
        margin: 8px 0 15px 0;
        border: 1px solid #ccc;
        border-radius: 8px;
        font-size: 15px;
      }
      button {
        width: 100%;
        padding: 12px;
        background: #0078ff;
        border: none;
        color: white;
        font-size: 16px;
        border-radius: 8px;
        cursor: pointer;
      }
      button:hover { background: #005fe0; }
    </style>
  </head>
  <body>
    <div class="card">
      <h2>WiFi Config</h2>
      <form action="/save" method="POST">
        <select name="ssid">
          )rawliteral"
          + wifiList +
          R"rawliteral(
        </select>
        <input name="pass" type="password" placeholder="Password" required>
        <button type="submit">Save</button>
      </form>

      <form action="/" method="GET">
        <button style="background:#28a745;margin-top:10px;">Rescan WiFi</button>
      </form>
    </div>
  </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", html);
}


void handleStatus() {
  bool online = checkInternet();
  String html = htmlHeader("WiFi Status");

  html += "<div class='card'><h2>WiFi Status</h2>";
  html += "<p><b>SSID:</b> " + WiFi.SSID() + "</p>";
  html += "<p><b>IP:</b> " + WiFi.localIP().toString() + "</p>";
  html += "<p><b>MAC:</b> " + WiFi.macAddress() + "</p>";
  html += "<p><b>RSSI:</b> " + String(WiFi.RSSI()) + " dBm</p>";

html += String("<p><b>Internet:</b> ") +
        String(online ? "<span style='color:green'>Online</span>"
                     : "<span style='color:red'>Offline</span>") +
        String("</p>");

  html +=
    "<form action='/reconnect' method='POST'><button>Reconnect</button></form>"
    "<form action='/disconnect' method='POST'><button class='red'>Disconnect</button></form>"
    "<form action='/' method='GET'><button class='red'>Back</button></form>"
    "</div>";

  html += htmlFooter();

  server.send(200, "text/html", html);
}

void handleFail() {
  String html =
    htmlHeader("WiFi Failed") +
    "<div class='card'><h2 style='color:red'>WiFi Connect Failed</h2>"
    "<p>Could not connect to network.</p>"
    "<form action='/' method='GET'><button>Back</button></form></div>" +
    htmlFooter();

  server.send(200, "text/html", html);
}

void handleSave() {
  prefs.begin("wifi", false);
  prefs.putString("ssid", server.arg("ssid"));
  prefs.putString("pass", server.arg("pass"));
  prefs.putBool("need_redirect", true);
  prefs.end();

  String html =
    htmlHeader("Saved") +
    "<h2>Saved!</h2><p>Rebooting...</p>"
    "<script>setTimeout(()=>location.href='/boot',4000);</script>" +
    htmlFooter();

  server.send(200, "text/html", html);
  delay(800);
  ESP.restart();
}

void handleReconnectFail() {
  String html = R"rawliteral(
  <html>
    <body style="font-family:Arial; text-align:center; padding-top:40px;">
      <h2 style="color:red;">Reconnect Failed!</h2>
      <p>Cannot reconnect to saved WiFi.</p>
      <form action="/status" method="GET">
        <button style="padding:12px; font-size:16px;">Back to Status</button>
      </form>
    </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

void handleDisconnectedPage() {
  String html = R"rawliteral(
  <html>
    <body style="font-family:Arial; text-align:center; padding-top:40px;">
      <h2 style="color:red;">WiFi Disconnected!</h2>
      <p>Your ESP32 is now disconnected from WiFi.</p>
      <form action="/" method="GET">
        <button style="padding:12px; font-size:16px;">Back to WiFi Config</button>
      </form>
    </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

void handleReconnectingPage() {
  String html = R"rawliteral(
  <html>
    <body style="font-family:Arial; text-align:center; padding-top:40px;">
      <h2>Reconnecting...</h2>
      <p>Please wait...</p>

      <script>
        // tự kiểm tra lại sau 2 giây
        setTimeout(() => { window.location = "/check_reconnect"; }, 2000);
      </script>
    </body>
  </html>
  )rawliteral";

  server.send(200, "text/html", html);
}

void handleReconnect() {
  // redirect browser sang trang reconnecting
  server.sendHeader("Location", "/reconnecting");
  server.send(302, "text/plain", "");

  // reconnect WiFi trong nền
  WiFi.disconnect();
  WiFi.begin(ssid_saved.c_str(), pass_saved.c_str());
}

void handleDisconnect() {
  WiFi.disconnect();  // Ngắt kết nối WiFi
  // Chuyển trình duyệt sang trang disconnected
  server.sendHeader("Location", "/disconnected");
  server.send(302, "text/plain", "");
}

void handleCheckReconnect() {
  if (WiFi.status() == WL_CONNECTED) {
    // nếu kết nối thành công → chuyển về status
    server.sendHeader("Location", "/status");
    server.send(302, "text/plain", "");
  } else {
    // nếu thất bại → chuyển sang trang fail
    server.sendHeader("Location", "/reconnect_fail");
    server.send(302, "text/plain", "");
  }
}

void handleBootRedirect() {
  if (WiFi.status() == WL_CONNECTED)
    server.sendHeader("Location", "/status");
  else
    server.sendHeader("Location", "/fail");

  server.send(302, "text/plain", "");
}

/* -------------------------------------------------------
    AP + Web server
------------------------------------------------------- */
void startAP() {
  Serial.println("Starting AP...");
  WiFi.softAP("ESP32_Config", "12345678");

  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.on("/status", handleStatus);
  server.on("/fail", handleFail);
  server.on("/boot", handleBootRedirect);
  server.on("/reconnect", HTTP_POST, handleReconnect);
  server.on("/disconnect", HTTP_POST, handleDisconnect);
  server.on("/reconnect", HTTP_POST, handleReconnect);
  server.on("/reconnecting", handleReconnectingPage);
  server.on("/check_reconnect", handleCheckReconnect);
  server.on("/reconnect_fail", handleReconnectFail);
  server.on("/disconnect", HTTP_POST, handleDisconnect);
  server.on("/disconnected", handleDisconnectedPage);

  server.begin();
  Serial.println("Web server started!");
}

/* -------------------------------------------------------
    Setup
------------------------------------------------------- */
void setup() {
  Serial.begin(115200);
  delay(300);

  WiFi.mode(WIFI_AP_STA);
  startAP();

  prefs.begin("wifi", true);
  ssid_saved = prefs.getString("ssid", "");
  pass_saved = prefs.getString("pass", "");
  bool need_redirect = prefs.getBool("need_redirect", false);
  prefs.end();

  if (need_redirect) {
    bool ok = connectWiFi(ssid_saved, pass_saved);

    prefs.begin("wifi", false);
    prefs.putBool("need_redirect", false);
    prefs.end();

    Serial.println(ok ? "WiFi Connected!" : "WiFi Connect Failed!");
  }
}

/* -------------------------------------------------------
    Loop
------------------------------------------------------- */
void loop() {
  if (WiFi.status() != WL_CONNECTED && ssid_saved.length()) {
    connectWiFi(ssid_saved, pass_saved);
  }

  server.handleClient();
}
