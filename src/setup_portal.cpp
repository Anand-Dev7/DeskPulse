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
<small>Upload up to 2 photos for the slideshow (JPEG/PNG only)</small>
<div style="margin:12px 0; padding:12px; background:#0f1b31; border:1px solid #355a8f; border-radius:8px; font-size:12px; line-height:1.6;">
  <strong>Image Requirements:</strong><br/>
  ✓ Format: JPG or PNG<br/>
  ✓ Original size: Any resolution, recommended 300x300px or smaller<br/>
  ✓ File size: Max 200KB (will be compressed)<br/>
  ✓ Output: Auto-resized to 200x200px, compressed for device storage<br/>
  <em style="color:#98b5db;">Tip: Use JPG format and smaller original images for best results</em>
</div>

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
  
  const sizeKB = (file.size / 1024).toFixed(1);
  
  // Check file size - warn if too large
  if (file.size > 200000) {
    alert('⚠️ File too large! (' + sizeKB + 'KB)\n\nMax 200KB. Please use:\n• Smaller image\n• JPG format (better compression)\n• Lower resolution');
    fileInput.value = '';
    return;
  }
  
  const reader = new FileReader();
  reader.onload = function(e) {
    const preview = document.getElementById(previewId);
    
    // Show image preview
    let html = '<img src="' + e.target.result + '" />';
    
    // Add size info below preview
    const previewParent = preview.parentElement;
    let sizeInfo = previewParent.querySelector('.size-info');
    if (!sizeInfo) {
      sizeInfo = document.createElement('div');
      sizeInfo.className = 'size-info';
      sizeInfo.style.cssText = 'font-size:11px;color:#98b5db;margin-top:6px;';
      preview.parentElement.appendChild(sizeInfo);
    }
    
    let sizeHtml = 'Original: ' + sizeKB + 'KB → ';
    if (file.size < 50000) {
      sizeHtml += '✓ Will compress well';
    } else if (file.size < 150000) {
      sizeHtml += '✓ OK (will be compressed)';
    } else {
      sizeHtml += '⚠️ Large (test compression)';
    }
    sizeInfo.innerHTML = sizeHtml;
    
    preview.innerHTML = html;
  };
  reader.readAsDataURL(file);
}

function compressAndEncode(file, callback) {
  // Create image element to resize
  const reader = new FileReader();
  reader.onload = function(e) {
    const img = new Image();
    img.onload = function() {
      // Resize to max 200x200
      const canvas = document.createElement('canvas');
      let width = img.width;
      let height = img.height;
      
      const maxSize = 200;
      if (width > height) {
        if (width > maxSize) {
          height *= maxSize / width;
          width = maxSize;
        }
      } else {
        if (height > maxSize) {
          width *= maxSize / height;
          height = maxSize;
        }
      }
      
      canvas.width = width;
      canvas.height = height;
      const ctx = canvas.getContext('2d');
      ctx.drawImage(img, 0, 0, width, height);
      
      // Convert to JPEG with compression
      const compressed = canvas.toDataURL('image/jpeg', 0.7);
      const base64 = compressed.split(',')[1];
      
      // Check compressed size
      const sizeInBytes = base64.length * 0.75; // Rough estimate
      const sizeKB = (sizeInBytes / 1024).toFixed(1);
      
      if (sizeInBytes > 60000) {
        alert('⚠️ Compressed size: ' + sizeKB + 'KB (still too large)\n\nPlease try:\n• Use JPG format instead of PNG\n• Lower resolution original image\n• Smaller dimensions (under 300x300px)\n• Compress before upload (use online tool)');
        callback(null);
      } else {
        console.log('Image compressed to ' + sizeKB + 'KB');
        callback(base64);
      }
    };
    img.onerror = function() {
      alert('Failed to load image. Please use a valid JPG or PNG file.');
      callback(null);
    };
    img.src = e.target.result;
  };
  reader.readAsDataURL(file);
}

document.getElementById('file1').addEventListener('change', function() {
  previewImage(this, 'preview1');
});

document.getElementById('file2').addEventListener('change', function() {
  previewImage(this, 'preview2');
});

document.getElementById('setupForm').addEventListener('submit', async function(e) {
  e.preventDefault();
  
  // Show loading state
  const submitBtn = document.querySelector('button[type="submit"]');
  const originalText = submitBtn.textContent;
  submitBtn.textContent = 'Processing...';
  submitBtn.disabled = true;
  
  // Convert files to compressed base64 before submission
  const file1 = document.getElementById('file1').files[0];
  const file2 = document.getElementById('file2').files[0];
  
  let completed = 0;
  let failed = false;
  
  const checkSubmit = function() {
    if (completed === expectedCount) {
      if (!failed) {
        document.getElementById('setupForm').submit();
      } else {
        submitBtn.textContent = originalText;
        submitBtn.disabled = false;
      }
    }
  };
  
  const expectedCount = (file1 ? 1 : 0) + (file2 ? 1 : 0);
  
  if (file1) {
    compressAndEncode(file1, function(base64) {
      if (base64) {
        document.getElementById('photo1Data').value = base64;
      } else {
        failed = true;
      }
      completed++;
      checkSubmit();
    });
  } else {
    completed++;
    checkSubmit();
  }
  
  if (file2) {
    compressAndEncode(file2, function(base64) {
      if (base64) {
        document.getElementById('photo2Data').value = base64;
      } else {
        failed = true;
      }
      completed++;
      checkSubmit();
    });
  } else {
    completed++;
    checkSubmit();
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

        // Limit photo data size to prevent flash overflow (max 30KB each after compression)
        const int MAX_PHOTO_SIZE = 30000;
        if (photo1Base64.length() > MAX_PHOTO_SIZE) {
            Serial.println("Photo 1 too large (" + String(photo1Base64.length()) + " bytes), rejected");
            photo1Base64 = "";  // Discard too-large image
        }
        if (photo2Base64.length() > MAX_PHOTO_SIZE) {
            Serial.println("Photo 2 too large (" + String(photo2Base64.length()) + " bytes), rejected");
            photo2Base64 = "";  // Discard too-large image
        }

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
        
        // Save config with watchdog feeding to prevent timeout
        Serial.println("Saving configuration to flash...");
        Config::save();
        Serial.println("Configuration saved successfully");

        // Send response with success page and countdown
        String successHtml = 
            "<html><head><title>DeskBuddy Setup</title>"
            "<meta http-equiv='cache-control' content='no-cache'/>"
            "<meta http-equiv='pragma' content='no-cache'/>"
            "<meta http-equiv='expires' content='0'/>"
            "<style>"
            "body{font-family:'Segoe UI',sans-serif;background:#0a111f;color:#e8f0ff;margin:0;padding:0;"
            "display:flex;align-items:center;justify-content:center;min-height:100vh;}"
            ".success-card{background:#121f36;border:2px solid #38bdf8;border-radius:14px;padding:40px;"
            "text-align:center;max-width:450px;box-shadow:0 8px 32px rgba(0,0,0,0.3);}"
            ".checkmark{font-size:60px;color:#10b981;margin:20px 0;display:inline-block;}"
            "h1{color:#63d6ff;margin:0 0 16px;font-size:28px;}"
            "p{color:#98b5db;margin:12px 0;line-height:1.6;}"
            ".info-text{font-size:13px;margin-top:25px;color:#98b5db;}"
            ".countdown{font-size:16px;margin-top:16px;color:#38bdf8;font-weight:bold;padding:12px;"
            "background:rgba(56,189,248,0.1);border-radius:8px;border-left:4px solid #38bdf8;}"
            "small{font-size:11px;color:#6b7b95;display:block;margin-top:12px;}"
            "</style></head><body>"
            "<div class='success-card'>"
            "<div class='checkmark'>✓</div>"
            "<h1>Configuration Saved!</h1>"
            "<p>Your settings have been saved successfully.</p>"
            "<p class='info-text'>DeskBuddy is now initializing and connecting to your WiFi network.</p>"
            "<div class='countdown'>Closing connection in <span id='timer'>10</span> seconds...</div>"
            "<small>You can close this page anytime. Device will complete setup automatically.</small>"
            "</div>"
            "<script>"
            "var countdown = 10;"
            "var timerEl = document.getElementById('timer');"
            "setInterval(function(){"
            "countdown--;"
            "timerEl.textContent = countdown;"
            "if(countdown <= 0){"
            "document.body.innerHTML='<div style=\"text-align:center;padding-top:120px;color:#e8f0ff;font-family:sans-serif;\">"
            "<h2>Setup Complete!</h2><p>DeskBuddy is starting up...</p><p style=\"font-size:12px;color:#6b7b95;margin-top:20px;\">"
            "Your device will be ready in a few moments.</p></div>';"
            "}"
            "}, 1000);"
            "</script>"
            "</body></html>";
        
        gServer.send(200, "text/html", successHtml);

        // Mark as completed, but add a small delay before actually stopping
        // This ensures the browser receives the response before connection closes
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
