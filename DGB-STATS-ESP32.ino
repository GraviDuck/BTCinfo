/*
  BTC Wallet Dashboard. GraviDuck --- 2026
  Shows BTC Balance, EUR Value, and Last 5 Transactions
  Auto-refreshes every 12 hours
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebServer.h>

const char* ssid     = "WIFI SSID";
const char* password = "WIFI PASSWORD";
const char* walletAddress = "BTC WALLET ADDRESS";

WebServer server(80);

String lastBalance = "Loading...";
String lastEurValue = "Loading...";
String lastTransactions = "Loading...";
unsigned long lastUpdate = 0;
const unsigned long refreshInterval = 12UL * 60UL * 60UL * 1000UL;  // 12 hours

// === Bitcoin balance ===
double bitcoinBalanceValue = 0.0; // Guardamos el valor numérico para cálculo EUR

String getBitcoinBalance() {
  HTTPClient http;
  String url = "https://chainz.cryptoid.info/btc/api.dws?q=getbalance&a=" + String(walletAddress);
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode != 200) {
    http.end();
    return lastBalance;  // keep previous value on error
  }

  String balance = http.getString();
  http.end();
  balance.trim();
  bitcoinBalanceValue = balance.toDouble(); // Guardar para cálculo EUR
  return balance + " BTC";
}

// === Bitcoin EUR price ===
double bitcoinPriceEUR = 0.0;

String getBitcoinEUR() {
  HTTPClient http;
  String url = "https://chainz.cryptoid.info/btc/api.dws?q=ticker.eur";
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode != 200) {
    http.end();
    return lastEurValue;
  }

  String eur = http.getString();
  http.end();
  eur.trim();
  bitcoinPriceEUR = eur.toDouble(); // Guardar precio numérico
  double totalEur = bitcoinBalanceValue * bitcoinPriceEUR;
  return "€" + String(totalEur, 2); // Mostrar balance total en EUR
}

// === Last 5 transactions ===
String getLastTransactions() {
  HTTPClient http;
  String url = "https://chainz.cryptoid.info/btc/api.dws?q=lasttxs&a=" + String(walletAddress);
  http.begin(url);
  int httpCode = http.GET();
  String html = "";

  if (httpCode == 200) {
    String payload = http.getString();
    http.end();

    DynamicJsonDocument doc(8192);
    DeserializationError err = deserializeJson(doc, payload);
    if (err || !doc.is<JsonArray>()) return "<p>JSON parse error</p>";

    html += "<h3>Last 5 Transactions</h3>";
    html += "<table style='width:95%;margin:auto;font-size:0.8em;border-collapse:collapse;text-align:center;'>";
    html += "<tr><th>TxID</th><th>Amount (BTC)</th><th>Conf</th></tr>";

    int count = 0;
    for (JsonObject tx : doc.as<JsonArray>()) {
      if (count >= 5) break;
      String hash = tx["hash"].as<String>();
      double amount = tx["total"].as<double>();
      int confs = tx["confirmations"].as<int>();

      html += "<tr>";
      html += "<td style='word-break:break-all;'>" + hash + "</td>";
      html += "<td>" + String(amount, 4) + "</td>";
      html += "<td>" + String(confs) + "</td>";
      html += "</tr>";
      count++;
    }
    html += "</table>";

    html += "<p style='margin-top:15px;font-size:0.9em;'>View detailed transactions:<br>"
            "<a href='https://chainz.cryptoid.info/btc/' target='_blank' style='color:#0ff;'>"
            "https://chainz.cryptoid.info/btc/</a></p>";

  } else {
    html = "<p style='color:red;'>Error fetching transactions</p>";
    http.end();
  }

  return html;
}

// === Web Page ===
String htmlPage() {
  String page = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>";
  page += "<title>Bitcoin Wallet Dashboard</title><style>";
  page += "body{font-family:Arial;text-align:center;background:#111;color:#0f0;margin:0;padding:20px;}";
  page += ".box{border:2px solid #0f0;padding:25px;border-radius:12px;max-width:900px;margin:auto;}";
  page += "h1{color:#00ffff;margin-bottom:20px;} p{line-height:1.8;font-size:1em;} a{color:#0ff;text-decoration:none;}";
  page += "a:hover{text-decoration:underline;} table,th,td{border:1px solid #0f0;padding:6px;} th{background:#022;} td{color:#0f0;}";
  page += "</style></head><body>";
  page += "<div class='box'>";
  page += "<h1>Bitcoin Wallet Dashboard</h1>";
  page += "<p><b>Wallet:</b><br>" + String(walletAddress) + "</p>";
  page += "<p><b>Balance:</b> " + lastBalance + "</p>";
  page += "<p><b>EUR Value:</b> " + lastEurValue + "</p>";
  page += lastTransactions;
  page += "</div></body></html>";
  return page;
}

// === Handlers ===
void handleRoot() { server.send(200, "text/html", htmlPage()); }
void handleRefresh() {
  lastBalance = getBitcoinBalance();
  lastEurValue = getBitcoinEUR();
  lastTransactions = getLastTransactions();
  lastUpdate = millis();
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

// === Setup ===
void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.println(WiFi.localIP());

  // Initial fetch
  lastBalance = getBitcoinBalance();
  lastEurValue = getBitcoinEUR();
  lastTransactions = getLastTransactions();
  lastUpdate = millis();

  server.on("/", handleRoot);
  server.on("/refresh", handleRefresh);
  server.begin();
  Serial.println("Server started!");
}

// === Loop ===
void loop() {
  server.handleClient();
  if (millis() - lastUpdate > refreshInterval) {
    lastBalance = getBitcoinBalance();
    lastEurValue = getBitcoinEUR();
    lastTransactions = getLastTransactions();
    lastUpdate = millis();
  }
}
