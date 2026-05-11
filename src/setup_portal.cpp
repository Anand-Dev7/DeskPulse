#include "setup_portal.h"
#include "colors.h"

#include <Arduino.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

#include "config.h"

namespace {

constexpr const char* kPhoto1Path = "/photo1.b64";
constexpr const char* kPhoto2Path = "/photo2.b64";

bool writeTextFile(const char* path, const String& data) {
  // Guarantee latest upload replaces older content, even when new file is shorter.
  if (SPIFFS.exists(path)) SPIFFS.remove(path);
  File f = SPIFFS.open(path, FILE_WRITE);
  if (!f) {
    Serial.printf("Failed to open %s for write\n", path);
    return false;
  }
  size_t n = f.print(data);
  f.close();
  return n == (size_t)data.length();
}

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
input[type="text"], input[type="password"], select, input[type="number"] { width:100%; margin-top:6px; padding:10px; border-radius:8px; border:1px solid #355a8f; background:#0c162a; color:#e8f0ff; outline:none; }
button { margin-top:16px; width:100%; padding:12px; border:0; border-radius:10px; background:#33d17a; color:#001b10; font-weight:700; font-size:15px; cursor:pointer; }
button.secondary { background:#1f2f4f; color:#e8f0ff; border:1px solid #355a8f; }
.grid { display:grid; grid-template-columns:1fr; gap:10px; }
.preview { margin-top:8px; background:#0c162a; border:1px dashed #355a8f; border-radius:10px; padding:8px; text-align:center; min-height:44px; }
.preview img { max-width:200px; max-height:200px; border-radius:10px; display:block; margin:0 auto 8px; }
.photo-actions { display:flex; gap:8px; margin-top:8px; }
.photo-actions button { margin-top:0; padding:9px; font-size:12px; }
button.remove-photo { background:#7f1d1d; color:#ffecec; border:1px solid #ef4444; }
.removed-note { color:#fca5a5; font-size:12px; margin-top:6px; }
.status { margin-top:16px; color:#98b5db; font-size:12px; text-align:center; }
.tip { background:#0c162a; border:1px solid #26416d; border-radius:10px; padding:10px; font-size:12px; color:#98b5db; }
</style>
</head>
<body>
<div class="card">
<h1>DeskBuddy Setup</h1>

<form id="setupForm" method="POST" action="/save">
<h2>WiFi & Location</h2>

<label>WiFi SSID</label>
<div class="grid">
  <input type="text" id="ssidInput" name="ssid" placeholder="WiFi name" />
  <select id="ssidSelect" onchange="useSelectedSsid()">
    <option value="">-- Scan nearby WiFi --</option>
  </select>
</div>

<label>WiFi Password</label>
<input type="password" name="pass" placeholder="WiFi password" />

<label>City</label>
<input type="text" name="city" placeholder="City" />

<label>Country Code</label>
<input type="text" name="country" placeholder="IN / US / GB etc" />

<label>Device Name</label>
<input type="text" name="name" placeholder="DeskBuddy" />

<h2>Photo Booth</h2>
<div class="tip">
  Upload up to 2 photos for the slideshow (JPEG/PNG only)<br/>
  Image Requirements:<br/>
  ✓ Format: JPG or PNG<br/>
  ✓ Original size: Any resolution, up to ~6MB<br/>
  ✓ Browser compresses before sending to the device<br/>
  ✓ Output: Auto-cropped to crisp 240x240px for the screen<br/>
  <em style="color:#98b5db;">Tip: Use JPG format and smaller original images for best results</em>
</div>

<div class="photo-section">
<label>Photo 1</label>
<input type="file" id="file1" accept="image/*" />
<div id="preview1" class="preview"></div>
<div class="photo-actions"><button type="button" class="remove-photo" onclick="removePhoto(1)">Remove Photo 1</button></div>
<input type="hidden" id="photo1Data" name="photo1Data" />
<input type="hidden" id="photo1Remove" name="photo1Remove" value="0" />
</div>

<div class="photo-section">
<label>Photo 2</label>
<input type="file" id="file2" accept="image/*" />
<div id="preview2" class="preview"></div>
<div class="photo-actions"><button type="button" class="remove-photo" onclick="removePhoto(2)">Remove Photo 2</button></div>
<input type="hidden" id="photo2Data" name="photo2Data" />
<input type="hidden" id="photo2Remove" name="photo2Remove" value="0" />
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

  const slot = fileInput.id === 'file1' ? 1 : 2;
  document.getElementById('photo' + slot + 'Remove').value = '0';
  document.getElementById('photo' + slot + 'Data').value = '';

  const sizeKB = (file.size / 1024).toFixed(1);

  // Browser-side compression handles large originals; keep a safety cap for phone photos.
  if (file.size > 6000000) {
    alert('⚠️ File too large! (' + sizeKB + 'KB)\n\nMax ~6MB original image. Please choose a smaller JPG/PNG.');
    fileInput.value = '';
    return;
  }

  const reader = new FileReader();
  reader.onload = function(e) {
    const preview = document.getElementById(previewId);
    preview.innerHTML = '<img src="' + e.target.result + '" /><div class="size-info">Selected: ' + sizeKB + 'KB → will save latest image</div>';
  };
  reader.readAsDataURL(file);
}

function removePhoto(slot) {
  document.getElementById('file' + slot).value = '';
  document.getElementById('photo' + slot + 'Data').value = '';
  document.getElementById('photo' + slot + 'Remove').value = '1';
  document.getElementById('preview' + slot).innerHTML = '<div class="removed-note">Photo ' + slot + ' will be removed when you save.</div>';
}

function compressAndEncode(file, callback) {
  const reader = new FileReader();
  reader.onload = function(e) {
    const img = new Image();
    img.onload = function() {
      // The display is 240x240. Generate an exact 240x240 JPEG so the ESP32
      // can render at 1:1 without JPEGDEC down-scaling blur.
      const TARGET = 240;
      const MAX_B64 = 50000; // must match ESP32 /save MAX_PHOTO_B64

      const canvas = document.createElement('canvas');
      const width = TARGET;
      const height = TARGET;

      canvas.width = TARGET;
      canvas.height = TARGET;
      const ctx = canvas.getContext('2d');

      // High-quality center-crop / cover. This avoids letterboxed tiny images
      // and preserves more detail than squeezing the full image into 200px.
      ctx.imageSmoothingEnabled = true;
      ctx.imageSmoothingQuality = 'high';
      ctx.fillStyle = '#000';
      ctx.fillRect(0, 0, TARGET, TARGET);

      const srcAspect = img.width / img.height;
      const dstAspect = 1;
      let sx = 0, sy = 0, sw = img.width, sh = img.height;
      if (srcAspect > dstAspect) {
        sw = Math.round(img.height * dstAspect);
        sx = Math.round((img.width - sw) / 2);
      } else if (srcAspect < dstAspect) {
        sh = Math.round(img.width / dstAspect);
        sy = Math.round((img.height - sh) / 2);
      }

      ctx.drawImage(img, sx, sy, sw, sh, 0, 0, TARGET, TARGET);

      // Try lower JPEG qualities until it fits the ESP32 limit.
      let quality = 0.90;
      let base64 = '';
      while (quality >= 0.55) {
        const compressed = canvas.toDataURL('image/jpeg', quality);
        base64 = (compressed.split(',')[1] || '').trim();
        if (base64.length <= MAX_B64) break;
        quality -= 0.05;
      }

      const sizeKB = ((base64.length * 0.75) / 1024).toFixed(1);

      if (!base64 || base64.length > MAX_B64) {
        alert('⚠️ Compressed image is still too large (' + sizeKB + 'KB).\n\nPlease try:\n• JPG format\n• Lower resolution\n• Less detailed image');
        callback(null);
      } else {
        console.log('Image compressed to 240x240 cover, q=' + quality.toFixed(2) + ', ' + sizeKB + 'KB');
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

function encodeFieldValue(value) {
  return encodeURIComponent(value == null ? '' : String(value));
}

async function uploadPhotoChunks(slot, base64, submitBtn) {
  if (!base64) return;
  const CHUNK = 900; // small enough for ESP32 WebServer stability
  const total = Math.ceil(base64.length / CHUNK);
  for (let i = 0; i < total; i++) {
    const part = base64.slice(i * CHUNK, (i + 1) * CHUNK);
    submitBtn.textContent = 'Uploading photo ' + slot + ' ' + (i + 1) + '/' + total;
    const body = [
      'slot=' + slot,
      'seq=' + i,
      'total=' + total,
      'data=' + encodeFieldValue(part)
    ].join('\n');
    const res = await fetch('/photoChunk', {
      method: 'POST',
      headers: { 'Content-Type': 'text/plain; charset=utf-8', 'Cache-Control': 'no-store' },
      body: body,
      cache: 'no-store'
    });
    const txt = await res.text();
    if (!res.ok) throw new Error(txt || ('Photo chunk upload failed: HTTP ' + res.status));
  }
}

async function sendConfigPayload(form, submitBtn, originalText) {
  // Upload photos separately in small chunks first. Do NOT include base64 in /save.
  const fd = new FormData(form);

  const photo1 = fd.get('photo1Data') || '';
  const photo2 = fd.get('photo2Data') || '';
  const remove1 = fd.get('photo1Remove') === '1';
  const remove2 = fd.get('photo2Remove') === '1';

  try {
    if (photo1 && !remove1) await uploadPhotoChunks(1, photo1, submitBtn);
    if (photo2 && !remove2) await uploadPhotoChunks(2, photo2, submitBtn);

    submitBtn.textContent = 'Saving config...';
    const fields = [
      'ssid', 'pass', 'city', 'country', 'name', 'interval',
      'photo1Remove', 'photo2Remove'
    ];
    const body = fields.map(k => k + '=' + encodeFieldValue(fd.get(k) || '')).join('\n');

    const res = await fetch('/save', {
      method: 'POST',
      headers: { 'Content-Type': 'text/plain; charset=utf-8', 'Cache-Control': 'no-store' },
      body: body,
      cache: 'no-store'
    });
    const html = await res.text();
    if (!res.ok) throw new Error(html || ('HTTP ' + res.status));
    document.open();
    document.write(html);
    document.close();
  } catch (err) {
    alert('Save failed: ' + err.message + '\nPlease reconnect to DeskBuddy-Setup and try again.');
    submitBtn.textContent = originalText;
    submitBtn.disabled = false;
  }
}

document.getElementById('setupForm').addEventListener('submit', async function(e) {
  e.preventDefault();

  const submitBtn = document.querySelector('button[type="submit"]');
  const originalText = submitBtn.textContent;
  submitBtn.textContent = 'Saving...';
  submitBtn.disabled = true;

  const file1 = document.getElementById('file1').files[0];
  const file2 = document.getElementById('file2').files[0];

  let completed = 0;
  const checkSubmit = async () => {
    completed++;
    if (completed >= 2) {
      await sendConfigPayload(e.target, submitBtn, originalText);
    }
  };

  const failSubmit = () => {
    submitBtn.textContent = originalText;
    submitBtn.disabled = false;
  };

  if (file1) {
    compressAndEncode(file1, (b64) => {
      if (!b64) { failSubmit(); return; }
      document.getElementById('photo1Data').value = b64;
      checkSubmit();
    });
  } else {
    checkSubmit();
  }

  if (file2) {
    compressAndEncode(file2, (b64) => {
      if (!b64) { failSubmit(); return; }
      document.getElementById('photo2Data').value = b64;
      checkSubmit();
    });
  } else {
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

} // namespace

SetupPortal::SetupPortal(Adafruit_ST7789& display) : tft(display) {
  gInstance = this;
}

void SetupPortal::begin() {
  completed = false;

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(kApSsid, kApPass);

  gDns.start(kDnsPort, "*", WiFi.softAPIP());

  gServer.on("/", HTTP_GET, []() {
    gServer.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    gServer.sendHeader("Pragma", "no-cache");
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

  gServer.on("/photoChunk", HTTP_POST, []() {
    String rawBody = gServer.arg("plain");

    auto urlDecode = [](String s) {
      s.replace("+", " ");
      String out;
      out.reserve(s.length());
      for (int i = 0; i < (int)s.length(); ++i) {
        if (s[i] == '%' && i + 2 < (int)s.length()) {
          char hex[3] = { s[i + 1], s[i + 2], 0 };
          out += (char) strtol(hex, nullptr, 16);
          i += 2;
        } else {
          out += s[i];
        }
      }
      return out;
    };

    auto field = [&](const char* key) {
      String prefix = String(key) + "=";
      int start = rawBody.indexOf(prefix);
      if (start < 0) return String("");
      start += prefix.length();
      int end = rawBody.indexOf('\n', start);
      if (end < 0) end = rawBody.length();
      return urlDecode(rawBody.substring(start, end));
    };

    int slot = field("slot").toInt();
    int seq = field("seq").toInt();
    int total = field("total").toInt();
    String data = field("data");

    if ((slot != 1 && slot != 2) || total <= 0 || seq < 0 || seq >= total || data.isEmpty()) {
      Serial.printf("/photoChunk bad request: slot=%d seq=%d total=%d data=%d raw=%d\n", slot, seq, total, data.length(), rawBody.length());
      gServer.send(400, "text/plain", "Bad photo chunk");
      return;
    }

    const char* path = (slot == 1) ? kPhoto1Path : kPhoto2Path;
    if (!SPIFFS.begin(true)) {
      gServer.send(500, "text/plain", "SPIFFS mount failed");
      return;
    }
    if (seq == 0 && SPIFFS.exists(path)) SPIFFS.remove(path);

    File f = SPIFFS.open(path, seq == 0 ? FILE_WRITE : FILE_APPEND);
    if (!f) {
      Serial.printf("/photoChunk failed open %s\n", path);
      gServer.send(500, "text/plain", "Photo open failed");
      return;
    }
    size_t written = f.print(data);
    f.close();
    if (written != (size_t)data.length()) {
      Serial.printf("/photoChunk short write slot=%d seq=%d written=%u expected=%d\n", slot, seq, (unsigned)written, data.length());
      gServer.send(500, "text/plain", "Photo write failed");
      return;
    }

    if (seq == 0 || seq == total - 1) {
      Serial.printf("Photo %d chunk %d/%d saved (%d chars)\n", slot, seq + 1, total, data.length());
    }
    gServer.send(200, "text/plain", "OK");
  });

  gServer.on("/save", HTTP_POST, []() {
    String rawBody = gServer.arg("plain");
    Serial.printf("/save POST received: args=%d, raw=%d chars\n", gServer.args(), rawBody.length());
    if (!gInstance) {
      Serial.println("/save ignored: no SetupPortal instance");
      gServer.send(500, "text/plain", "Setup portal not ready");
      return;
    }

    auto urlDecode = [](String s) {
      s.replace("+", " ");
      String out;
      out.reserve(s.length());
      for (int i = 0; i < (int)s.length(); ++i) {
        if (s[i] == '%' && i + 2 < (int)s.length()) {
          char hex[3] = { s[i + 1], s[i + 2], 0 };
          out += (char) strtol(hex, nullptr, 16);
          i += 2;
        } else {
          out += s[i];
        }
      }
      return out;
    };

    auto field = [&](const char* key) {
      String v = gServer.arg(key);
      if (!v.isEmpty() || rawBody.isEmpty()) return v;
      String prefix = String(key) + "=";
      int start = rawBody.indexOf(prefix);
      if (start < 0) return String("");
      start += prefix.length();
      int end = rawBody.indexOf('\n', start);
      if (end < 0) end = rawBody.length();
      return urlDecode(rawBody.substring(start, end));
    };

    String ssidArg = field("ssid");
    String passArg = field("pass");
    ssidArg.trim();
    passArg.trim();

    // Preserve existing WiFi credentials when reconfiguring only photos/name/etc.
    // Blank fields in the portal should not erase saved credentials.
    if (!ssidArg.isEmpty()) Config::wifiSsid = ssidArg;
    if (!passArg.isEmpty()) Config::wifiPass = passArg;

    String city = field("city");
    String country = field("country");
    String name = field("name");
    String intervalStr = field("interval");

    // Photos are uploaded separately via /photoChunk to avoid WebServer crashes on large POST bodies.
    String photo1Base64 = "";
    String photo2Base64 = "";
    bool removePhoto1 = field("photo1Remove") == "1";
    bool removePhoto2 = field("photo2Remove") == "1";

    if (gServer.args() == 0 && rawBody.isEmpty()) {
      Serial.println("/save rejected: empty POST body");
      gServer.send(400, "text/plain", "Empty save request rejected. Please reload setup page and press Save again.");
      return;
    }

    Serial.printf("Parsed save: ssid=%d city=%d photo1=%d photo2=%d rm1=%d rm2=%d\n",
                  ssidArg.length(), city.length(), photo1Base64.length(), photo2Base64.length(),
                  removePhoto1, removePhoto2);

    // Limit photo data size (base64 chars) to keep RAM + decode time safe
    const int MAX_PHOTO_B64 = 50000;
    if (photo1Base64.length() > MAX_PHOTO_B64) {
      Serial.println("Photo 1 too large (" + String(photo1Base64.length()) + " chars), rejected");
      photo1Base64 = "";
    }
    if (photo2Base64.length() > MAX_PHOTO_B64) {
      Serial.println("Photo 2 too large (" + String(photo2Base64.length()) + " chars), rejected");
      photo2Base64 = "";
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

    // Store photo data in SPIFFS (NOT NVS!)
    if (!SPIFFS.begin(true)) {
      Serial.println("SPIFFS mount failed! Photos will not be saved.");
    } else {
      if (removePhoto1) {
        bool ok = !SPIFFS.exists(kPhoto1Path) || SPIFFS.remove(kPhoto1Path);
        Serial.println(String("Photo 1 remove: ") + (ok ? "OK" : "FAIL"));
      } else if (!photo1Base64.isEmpty()) {
        bool ok = writeTextFile(kPhoto1Path, photo1Base64);
        Serial.println(String("Photo 1 file save: ") + (ok ? "OK" : "FAIL") + " (" + photo1Base64.length() + " chars)");
      }

      if (removePhoto2) {
        bool ok = !SPIFFS.exists(kPhoto2Path) || SPIFFS.remove(kPhoto2Path);
        Serial.println(String("Photo 2 remove: ") + (ok ? "OK" : "FAIL"));
      } else if (!photo2Base64.isEmpty()) {
        bool ok = writeTextFile(kPhoto2Path, photo2Base64);
        Serial.println(String("Photo 2 file save: ") + (ok ? "OK" : "FAIL") + " (" + photo2Base64.length() + " chars)");
      }
    }

    // Save config (WiFi/location/etc) to Preferences
    Serial.println("Saving configuration to flash...");
    Config::save();
    Serial.println("Configuration saved successfully");

    // Mark completed before sending response; main loop waits briefly before stopping AP.
    gInstance->completed = true;
    Serial.println("Setup portal save completed; exit requested");

    String successHtml =
      "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'/>"
      "<title>Saved</title></head><body style='font-family:Segoe UI;background:#0a111f;color:#e8f0ff;padding:24px;'>"
      "<h2>✅ Configuration Saved!</h2>"
      "<p>Your settings have been saved successfully.</p>"
      "<p>DeskBuddy is closing setup mode and returning to the clock.</p>"
      "<p>If this page stops responding, reconnect to your normal WiFi.</p>"
      "</body></html>";

    gServer.send(200, "text/html", successHtml);
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
