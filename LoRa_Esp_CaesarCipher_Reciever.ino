/*
 ESP32 + SX1278 (433 MHz)
  Receiver Side
 */

#include <WiFi.h>
#include <WebServer.h>
#include <LoRa.h>
#include <SPI.h>
#include <vector>

// ---------- Wi-Fi ----------
const char* ssid     = "111016_KMITL_2.4G";   //ชื่อSSID ของWiFi
const char* password = "77264776"; //passwordของWiFi ใส่ให้ถูกต้องด้วย

// ---------- LoRa pins (ESP32) ----------
#define LORA_SCK      18 //พวก SCK , MISO , MOSI ต้องดูดีๆว่า Default SCK, MISO, MOSI อยู่Pinไหน ของMicro controller หรือถ้าไม่รู้ก็ใช้ SPI.h ในการกำหนดPinต่างๆ
#define LORA_MISO     19
#define LORA_MOSI     23
#define LORA_SS_PIN    5 // บางmoduleของพวกLoRA บางครั้งก็เจอ NSS กับ CS แต่มันคืออันเดียวกัน CS (Chip Select)
#define LORA_RST_PIN  14 // RS หมายถึง Reset Pin
#define LORA_DIO0_PIN  2

#define LORA_FREQUENCY 433E6 // 433kHz ความถี่มาตรฐานที่อนุญาตให้ใช้กันได้

WebServer server(80);
std::vector<String> receivedMessages;
const int MAX_MESSAGES = 10;

//Caesar (decrypt)
String caesarCipherDecrypt(String text, int shift) {
  shift = ((shift % 26) + 26) % 26;
  int d_shift = (26 - shift) % 26;
  String result = "";
  for (int i = 0; i < text.length(); i++) {
    char ch = text[i];
    if (ch >= 'a' && ch <= 'z') ch = (ch - 'a' + d_shift) % 26 + 'a';
    else if (ch >= 'A' && ch <= 'Z') ch = (ch - 'A' + d_shift) % 26 + 'A';
    result += ch;
  }
  return result;
}

//ป้องกัน quote แตกตอนยัดลง JS
String jsEscape(String s){ s.replace("\\","\\\\"); s.replace("'","\\'"); s.replace("\r"," "); s.replace("\n"," "); return s; }

//LoRa polling
void readLoRa() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String incoming = "";
    while (LoRa.available()) incoming += (char)LoRa.read();

    Serial.print("LoRa RX: ");
    Serial.print(incoming);
    Serial.print(" | RSSI=");
    Serial.print(LoRa.packetRssi());
    Serial.print(" SNR=");
    Serial.println(LoRa.packetSnr());

    if ((int)receivedMessages.size() >= MAX_MESSAGES)
      receivedMessages.erase(receivedMessages.begin());
    receivedMessages.push_back(incoming);
  }
}

//DELETE endpoint
void handleDelete(){
  if(!server.hasArg("i")) { server.send(400,"text/plain","missing index"); return; }
  int idx = server.arg("i").toInt();
  if(idx<0 || idx >= (int)receivedMessages.size()){
    server.send(400,"text/plain","bad index"); return;
  }
  receivedMessages.erase(receivedMessages.begin()+idx);
  server.sendHeader("Location","/"); //กลับหน้าแรก
  server.send(303);
}

//Main page
void handleRoot() {
  String key = "";
  String secretMessage = "";
  String decodedMessage = "";

  if (server.hasArg("secret")) secretMessage = server.arg("secret");
  if (server.hasArg("key")) {
    key = server.arg("key");
    if (key.toInt() >= 1 && key.toInt() <= 25 && secretMessage.length() > 0)
      decodedMessage = caesarCipherDecrypt(secretMessage, key.toInt());
  }

  String html = R"rawliteral(
<!DOCTYPE html><html lang="th"><head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Decrypt Protocol</title>
<link href="https://fonts.googleapis.com/css2?family=Share+Tech+Mono&display=swap" rel="stylesheet">
<style>
:root{--primary:#00ff41;--bg:#0d1117;--b:#00ff414d;--del:#ff0041}
*{box-sizing:border-box}html,body{height:100%}
body{margin:0;font-family:'Share Tech Mono',monospace;background:#0d1117;color:var(--primary);display:flex;align-items:center;justify-content:center;padding:24px;position:relative;overflow:hidden}
body:after{content:"";position:absolute;inset:0;background:repeating-linear-gradient(0deg,rgba(0,0,0,.3)0,rgba(0,0,0,.3)1px,transparent 1px,transparent 2px);animation:sl 20s linear infinite;pointer-events:none}
@keyframes sl{from{background-position:0 0}to{background-position:0 100%}}
.container{width:100%;max-width:980px;background:#0d1117cc;border:1px solid var(--b);box-shadow:0 0 24px #00ff4133;padding:24px}
h1{text-align:center;text-transform:uppercase;text-shadow:0 0 6px var(--primary),0 0 12px var(--primary);margin:0 0 24px}
label{display:block;margin:18px 0 6px;text-transform:uppercase;letter-spacing:1px}
label:after{content:"_";animation:b 1s step-end infinite;margin-left:6px}@keyframes b{50%{opacity:0}}
input[type=text],textarea{width:100%;background:rgba(0,0,0,.3);border:1px solid var(--b);color:var(--primary);padding:.8rem;font:inherit}
input[type=text]:focus,textarea:focus{outline:none;box-shadow:0 0 14px #00ff4180;border-color:var(--primary)}
.protocol-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(320px,1fr));gap:24px}
.right-panel h2{text-align:center;margin:0 0 12px;text-transform:uppercase;text-shadow:0 0 6px var(--primary)}
.message-list{list-style:none;display:flex;flex-direction:column;gap:8px;margin:0;padding:0}
.message-list li{display:flex;justify-content:space-between;align-items:center;padding:.75rem;border:1px solid var(--b);background:rgba(0,0,0,.2)}
.buttons{display:flex;gap:8px}
.btn{background:none;border:1px solid;padding:.45em 1em;font:inherit;text-transform:uppercase;cursor:pointer;transition:.2s}
.btn-read{color:var(--primary);border-color:var(--primary)}
.btn-read:hover{background:var(--primary);color:#0d1117;box-shadow:0 0 15px var(--primary)}
.btn-delete{color:var(--del);border-color:var(--del);text-decoration:none;display:inline-block}
.btn-delete:hover{background:var(--del);color:#0d1117;box-shadow:0 0 15px var(--del)}
.slider-container{display:flex;align-items:center;gap:12px}
input[type=range]{appearance:none;height:3px;background:var(--b);flex:1}
input[type=range]::-webkit-slider-thumb{appearance:none;width:16px;height:16px;background:var(--primary);box-shadow:0 0 10px var(--primary)}
.small{margin-top:6px;color:#9cf}
</style></head><body><div class="container">
<h1>[ DECRYPT PROTOCOL ]</h1>
<div class="protocol-grid">
  <div class="left-panel">
    <form action="/" method="GET">
      <!-- ===== Ciphertext ด้านบน ===== -->
      <label for="ciphertext">ENCRYPTED CIPHERTEXT</label>
      <textarea id="ciphertext" name="secret" rows="4" placeholder="ciphertext here...">)rawliteral";
  html += secretMessage;
  html += R"rawliteral(</textarea>

      <!-- ===== Slider Key 1..25 ===== -->
      <label for="key">SET CIPHER CAESAR KEY</label>
      <div class="slider-container">
        <input type="range" id="key" name="key" min="1" max="25" value=")rawliteral";
  html += (key.length()? key : "10");
  html += R"rawliteral(" oninput="syncKey()">
        <span id="key-value">)rawliteral";
  html += (key.length()? key : "10");
  html += R"rawliteral(</span>
      </div>

      <!-- ===== Plaintext ด้านล่าง (ผลถอดรหัส) ===== -->
      <label for="undecrypt-text">DECRYPTED PLAINTEXT</label>
      <input type="text" id="undecrypt-text" value=")rawliteral";
  html += decodedMessage;
  html += R"rawliteral(">

      <div class="small">Tip: เลือกข้อความจากกล่องขวา แล้วเลื่อนคีย์เพื่อถอดแบบทันที</div>
      <div style="margin-top:10px">
        <button class="btn btn-read" type="submit">DECODE (REFRESH)</button>
      </div>
    </form>
  </div>

  <div class="right-panel">
    <h2>:: SECRET MESSAGE ::</h2>
    <ul class="message-list">)rawliteral";

  // เติมรายการข้อความที่รับได้
  for (int i = 0; i < (int)receivedMessages.size(); i++) {
    String safe = jsEscape(receivedMessages[i]);
    html += "<li><span>" + String(i+1) + "|MSG" + String(i+1) + "</span>";
    html += "<div class='buttons'>";
    html += "<button class='btn btn-read' onclick=\"readMsg('"+ safe +"')\">READ</button>";
    html += "<a class='btn btn-delete' href='/delete?i=" + String(i) + "'>DELETE</a>";
    html += "</div></li>";
  }

  html += R"rawliteral(</ul>
  </div>
</div>
</div>
<script>
const keyEl=document.getElementById('key');
const keyVal=document.getElementById('key-value');
const cipher=document.getElementById('ciphertext');
const plain=document.getElementById('undecrypt-text');

function uncaesar(s,k){
  k=((k%26)+26)%26;
  return s.replace(/[a-z]/gi,c=>{
    const A=(c>='a'?'a':'A').charCodeAt(0);
    return String.fromCharCode((c.charCodeAt(0)-A-(k%26)+26)%26 + A);
  });
}
function syncKey(){ keyVal.textContent=keyEl.value; renderPlain(); }
function readMsg(text){ cipher.value=text; renderPlain(); }
function renderPlain(){ plain.value = uncaesar(cipher.value, parseInt(keyEl.value||'0')); }

cipher.addEventListener('input', renderPlain);
window.addEventListener('DOMContentLoaded', renderPlain);
</script>
</body></html>)rawliteral";

  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(9600);
  while (!Serial) {}

  // Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(400); Serial.print("."); }
  Serial.println("\nWiFi OK. IP: " + WiFi.localIP().toString());

  // SPI + LoRa
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS_PIN);
  LoRa.setPins(LORA_SS_PIN, LORA_RST_PIN, LORA_DIO0_PIN);
  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("LoRa begin FAILED"); while (1) delay(1000);
  }

  // RF parameters (ต้องตรงกับ Sender)
  LoRa.setSpreadingFactor(7); //ค่า SFมากยิ่ง ส่งได้ไกล powerในการส่งมากขึ้น แต่จะใช้เวลาในการส่งนานขึ้ร
  LoRa.setSignalBandwidth(125E3); //ความกว้างแถบสัญญาณของLoRa ยิ่งมาก ยิ่งเร็วขึ้น
  LoRa.setCodingRate4(5); // เป็นการกำหนด Foward Error Correction แก้บิตที่ผิดพลาด กำหนดได้หลายแบบ 4/5. 4/6, 4/7, 4/8
  LoRa.setPreambleLength(8); //ยิ่งมาก ระยะการส่งไกลขึ้นแต่ เวลาส่งออกไปจะนานขึ้น
  LoRa.setSyncWord(0xF3); //เป็นเหมือนkey ที่เอาไว้อ่านทั้งฝั่งผู้รับ กับ ผู้ส่ง ถ้าตรงกันก็จะอ่านกันได้
  LoRa.enableCrc(); //ตรวจสอบ CRC 

  Serial.println("LoRa RX ready.");

  server.on("/", handleRoot);
  server.on("/delete", handleDelete);
  server.begin();
  Serial.println("HTTP server started.");
}

void loop() {
  server.handleClient();
  readLoRa(); // polling RX
}
