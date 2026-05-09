#include "setup_portal.h"
#include "colors.h"

#include <Arduino.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFi.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

#include "config.h"

namespace {

const char* kApSsid = "DeskBuddy-Setup";
const char* kApPass = "12345678";
const byte kDnsPort = 53;

DNSServer gDns;
WebServer gServer(80);
SetupPortal* gInstance = nullptr;

const char kPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1" />
<title>DeskBuddy Setup</title>
<style>
body { font-family: 'Segoe UI', sans-serif; background:#0a111f; color:#e8f0ff; margin:0; padding:24px; }
.card { max-width:600px; margin:auto; background:#121f36; border:1px solid #26416d; border-radius:14px; padding:20px; }
h1 { margin:0 0 12px; color:#63d6ff; font-size:22px; }
h2 { margin:20px 0 12px; color:#63d6ff; font-size:16px; border-top:1px solid #26416d; padding-top:16px; }
small { color:#98b5db; }
label { display:block; margin-top:12px; font-size:14px; color:#a9c3e8; }
input[type="text"], input[type="password"], select, input[type="number"] { width:100%; margin-top:6px; padding:10px; border-radius:8px; border:1px solid #355a8f; background:#0f1b31; color:#e8f0ff; box-sizing:border-box; }
input[type="file"] { margin-top:6px; color:#e8f0ff; }
.preview { width:100px; height:100px; margin-top:8px; border:1px solid #26416d; border-radius:8px; background:#0f1b31; overflow:hidden; }
.preview img { width:100%; height:100%; object-fit:cover; }
button { width:100%; margin-top:16px; padding:12px; border:none; border-radius:8px; background:#38bdf8; color:#041525; font-weight:700; cursor:pointer; }
button.secondary { margin-top:10px; background:#2c4d78; color:#d6e7ff; }
.row { display:flex; gap:8px; }
.row > * { flex:1; }
.status { margin-top:14px; font-size:12px; color:#98b5db; }
.photo-section { margin:20px 0; padding:16px; background:#0f1b31; border-radius:8px; border:1px solid #355a8f; }
</style>
</head>
<body>
<div class="card">
<h1>DeskBuddy Configuration</h1>
<small>Configure WiFi, weather, and photo booth settings.</small>
<form id="setupForm" action="/save" method="POST">

<h2>WiFi & Location</h2>
<label>WiFi SSID</label>
<select id="ssidSelect" onchange="useSelectedSsid()"><option value="">-- Scan nearby WiFi --</option></select>
<input id="ssidInput" name="ssid" required placeholder="WiFi SSID" />

<label>WiFi Password</label>
<input name="pass" type="password" required placeholder="WiFi password" />

<div class="row">
<div>
<label>City</label>
<input name="city" required placeholder="Enter city" />
</div>
<div>
<label>Country Code</label>
<input name="country" required placeholder="IN" maxlength="3" />
</div>
</div>

<label>Device Name</label>
<input name="name" placeholder="DeskBuddy" />

<h2>Photo Booth</h2>
<small>Upload up to 2 photos for the slideshow (JPEG/PNG, max 200x200px)</small>

<div class="photo-section">
<label>Photo 1</label>
<input type="file" id="file1" accept="image/*" />
<div id="preview1" class="preview"></div>
<input type="hidden" id="photo1Data" name="photo1Data" />
</div>

<div class="photo-section">
<label>Photo 2</label>
<input type="file" id="file2" accept="image/*" />
<div id="preview2" class="preview"></div>
<input type="hidden" id="photo2Data" name="photo2Data" />
</div>

<label>Slideshow Interval (seconds)</label>
<input type="number" name="interval" min="1" max="60" value="3" placeholder="3" />
<small>Time between photo transitions (1-60 seconds)</small>

<button type="submit">Save Configuration</button>
</form>
<button class="secondary" onclick="scanWifi()">Refresh WiFi List</button>
<div class="status">Connect to AP: DeskBuddy-Setup | Password: 12345678</div>
</div>

<script>
function useSelectedSsid(){
  const sel=document.getElementById('ssidSelect');
  const inp=document.getElementById('ssidInput');
  if(sel && inp && sel.value){ inp.value = sel.value; }
}

function previewImage(fileInput, previewId) {
  const file = fileInput.files[0];
  if (!file) return;
  const reader = new FileReader();
  reader.onload = function(e) {
    const preview = document.getElementById(previewId);
    preview.innerHTML = '<img src="' + e.target.result + '" />';
  };
  reader.readAsDataURL(file);
}

function fileToBase64(file, callback) {
  const reader = new FileReader();
  reader.onload = function(e) {
    const base64 = e.target.result.split(',')[1];
    callback(base64);
  };
  if (file) reader.readAsDataURL(file);
}

document.getElementById('file1').addEventListener('change', function() {
  previewImage(this, 'preview1');
});

document.getElementById('file2').addEventListener('change', function() {
  previewImage(this, 'preview2');
});

document.getElementById('setupForm').addEventListener('submit', async function(e) {
  e.preventDefault();
  
  // Convert files to base64 before submission
  const file1 = document.getElementById('file1').files[0];
  const file2 = document.getElementById('file2').files[0];
  
  let completed = 0;
  
  if (file1) {
    fileToBase64(file1, function(base64) {
      document.getElementById('photo1Data').value = base64;
      completed++;
      if (completed === (file1 ? 1 : 0) + (file2 ? 1 : 0)) {
        document.getElementById('setupForm').submit();
      }
    });
  } else {
    completed++;
  }
  
  if (file2) {
    fileToBase64(file2, function(base64) {
      document.getElementById('photo2Data').value = base64;
      completed++;
      if (completed === (file1 ? 1 : 0) + (file2 ? 1 : 0)) {
        document.getElementById('setupForm').submit();
      }
    });
  } else {
    completed++;
  }
  
  if (!file1 && !file2) {
    document.getElementById('setupForm').submit();
  }
});

async function scanWifi(){
  const sel=document.getElementById('ssidSelect');
  sel.innerHTML='<option value="">-- Scanning... --</option>';
  try {
    const res=await fetch('/scan',{cache:'no-store'});
    const list=await res.json();
    sel.innerHTML='<option value="">-- Select WiFi --</option>';
    if(Array.isArray(list)&&list.length){
      list.forEach(n=>{ const o=document.createElement('option'); o.value=n.ssid; o.textContent=n.ssid+' ('+n.rssi+' dBm)'; sel.appendChild(o); });
    } else {
      sel.innerHTML='<option value="">No networks found</option>';
    }
  } catch(e) {
    sel.innerHTML='<option value="">Scan failed</option>';
  }
}
window.addEventListener('load', scanWifi);
</script>
</body>
</html>
)rawliteral";

String escapeJson(const String& in) {
    String out;
    out.reserve(in.length() + 8);
    for (size_t i = 0; i < in.length(); ++i) {
        char c = in[i];
        if (c == '"' || c == '\\') out += '\\';
        out += c;
    }
    return out;
}

void drawPortalHint(Adafruit_ST7789& tft) {
    tft.fillScreen(COLOR_BLACK);
    tft.setTextColor(COLOR_CYAN);
    tft.setFont(&FreeSansBold9pt7b);
    tft.setTextSize(1);
    tft.setCursor(40, 34);
    tft.print("DeskBuddy Setup");

    tft.setTextColor(COLOR_WHITE);
    tft.setFont(&FreeSans9pt7b);
    tft.setCursor(10, 76);
    tft.print("DeskBuddy-Setup");
    tft.setCursor(10, 104);
    tft.print("Pass: 12345678");
    tft.setCursor(10, 132);
    tft.print("Open: 192.168.4.1");
    tft.setFont(NULL);
}

}  // namespace

SetupPortal::SetupPortal(Adafruit_ST7789& display) : tft(display) {
    gInstance = this;
}

void SetupPortal::begin() {
    completed = false;
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(kApSsid, kApPass);
    gDns.start(kDnsPort, "*", WiFi.softAPIP());

    gServer.on("/", HTTP_GET, []() {
        gServer.send_P(200, "text/html", kPage);
    });

    gServer.on("/scan", HTTP_GET, []() {
        int n = WiFi.scanNetworks(false, true);
        String json = "[";
        bool first = true;
        for (int i = 0; i < n; i++) {
            String ssid = WiFi.SSID(i);
            if (ssid.isEmpty()) continue;
            if (!first) json += ",";
            json += "{\"ssid\":\"" + escapeJson(ssid) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
            first = false;
        }
        json += "]";
        WiFi.scanDelete();
        gServer.send(200, "application/json", json);
    });

    gServer.on("/save", HTTP_POST, []() {
        if (!gInstance) return;

        Config::wifiSsid = gServer.arg("ssid");
        Config::wifiPass = gServer.arg("pass");

        String city = gServer.arg("city");
        String country = gServer.arg("country");
        String name = gServer.arg("name");
        String intervalStr = gServer.arg("interval");
        String photo1Base64 = gServer.arg("photo1Data");
        String photo2Base64 = gServer.arg("photo2Data");

        if (!city.isEmpty()) Config::city = city;
        if (!country.isEmpty()) Config::countryCode = country;
        Config::refreshTimezoneFromLocation();
        if (!name.isEmpty()) Config::deviceName = name;
        
        // Parse slideshow interval (convert seconds to milliseconds)
        if (!intervalStr.isEmpty()) {
            int seconds = intervalStr.toInt();
            if (seconds >= 1 && seconds <= 60) {
                Config::photoBoothInterval = seconds * 1000;
            }
        }

        // Store photo data (base64 strings sent from client)
        if (!photo1Base64.isEmpty()) {
            Config::photoData1 = photo1Base64;
            Serial.println("Photo 1 saved: " + String(Config::photoData1.length()) + " bytes");
        }
        if (!photo2Base64.isEmpty()) {
            Config::photoData2 = photo2Base64;
            Serial.println("Photo 2 saved: " + String(Config::photoData2.length()) + " bytes");
        }
        
        Config::save();

        gServer.send(200, "text/html",
            "<html><body style='font-family:sans-serif;background:#0a111f;color:#e8f0ff;text-align:center;padding-top:60px;'>"
            "<h2>Configuration Saved!</h2><p>Device is connecting...</p></body></html>");

        gInstance->completed = true;
    });

    gServer.onNotFound([]() {
        gServer.sendHeader("Location", "http://192.168.4.1/", true);
        gServer.send(302, "text/plain", "");
    });

    gServer.begin();
    drawPortalHint(tft);
}

void SetupPortal::loop() {
    gDns.processNextRequest();
    gServer.handleClient();
}

void SetupPortal::stop() {
    gDns.stop();
    gServer.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
}

bool SetupPortal::isCompleted() const {
    return completed;
}
