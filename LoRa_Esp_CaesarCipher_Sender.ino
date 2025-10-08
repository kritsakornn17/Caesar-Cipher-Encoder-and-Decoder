/*
  ESP32 + SX1278 (433 MHz)
  Sender Side
 */

#include <WiFi.h>
#include <WebServer.h>
#include <LoRa.h>
#include <SPI.h>

//Wi-Fi
const char* ssid     = "111016_KMITL_2.4G"; //ชื่อSSID ของWiFi    
const char* password = "77264776"; //passwordของWiFi ใส่ให้ถูกต้องด้วย

//LoRa pins (ESP32)
#define LORA_SCK      18 //พวก SCK , MISO , MOSI ต้องดูดีๆว่า Default SCK, MISO, MOSI อยู่Pinไหน ของMicro controller หรือถ้าไม่รู้ก็ใช้ SPI.h ในการกำหนดPinต่างๆ
#define LORA_MISO     19
#define LORA_MOSI     23
#define LORA_SS_PIN    5   // บางmoduleของพวกLoRA บางครั้งก็เจอ NSS กับ CS แต่มันคืออันเดียวกัน CS (Chip Select)
#define LORA_RST_PIN  14 // RS หมายถึง Reset Pin
#define LORA_DIO0_PIN  2

//LoRa RF
#define LORA_FREQUENCY 433E6 // 433kHz ความถี่มาตรฐานที่อนุญาตให้ใช้กันได้

WebServer server(80);
String statusMessage = "";

//Caesar
String caesarCipherEncrypt(String text, int shift) {
  String result = "";
  shift = ((shift % 26) + 26) % 26;
  for (int i = 0; i < text.length(); i++) {
    char ch = text[i];
    if (ch >= 'a' && ch <= 'z') ch = (ch - 'a' + shift) % 26 + 'a';
    else if (ch >= 'A' && ch <= 'Z') ch = (ch - 'A' + shift) % 26 + 'A';
    result += ch;
  }
  return result;
}

//Web UI
void handleRoot() {
  // ถ้ามีการส่ง (GET /?key=..&message=..)
  if (server.hasArg("key") && server.hasArg("message")) {
    int key = server.arg("key").toInt();
    String messageStr = server.arg("message");

    if (key >= 1 && key <= 25 && messageStr.length() > 0) {
      LoRa.beginPacket();
      LoRa.print(messageStr);
      LoRa.endPacket();

      Serial.println("Sent LoRa Packet: " + messageStr);
      statusMessage = "SENT OK";
    } else {
      statusMessage = "ERROR: key must be 1..25 and message not empty";
    }
  }

  // HTML/CSS/JS
  String html = R"rawliteral(
<!DOCTYPE html><html lang="th"><head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Encrypt Protocol</title>
<link href="https://fonts.googleapis.com/css2?family=Share+Tech+Mono&display=swap" rel="stylesheet">
<style>
:root{--primary:#00ff41;--bg:#0d1117;--b:#00ff414d}
*{box-sizing:border-box}html,body{height:100%}
body{margin:0;font-family:'Share Tech Mono',monospace;background:#0d1117;color:var(--primary);display:flex;align-items:center;justify-content:center;padding:24px;position:relative;overflow:hidden}
body:after{content:"";position:absolute;inset:0;background:repeating-linear-gradient(0deg,rgba(0,0,0,.3)0,rgba(0,0,0,.3)1px,transparent 1px,transparent 2px);animation:sl 20s linear infinite;pointer-events:none}
@keyframes sl{from{background-position:0 0}to{background-position:0 100%}}
.container{width:100%;max-width:820px;background:#0d1117cc;border:1px solid var(--b);box-shadow:0 0 24px #00ff4133;padding:24px}
h1{text-align:center;text-transform:uppercase;text-shadow:0 0 6px var(--primary),0 0 12px var(--primary);margin:0 0 24px}
label{display:block;margin:18px 0 6px;text-transform:uppercase;letter-spacing:1px}
label:after{content:"_";animation:b 1s step-end infinite;margin-left:6px}@keyframes b{50%{opacity:0}}
textarea,input[type=text]{width:100%;background:rgba(0,0,0,.3);border:1px solid var(--b);color:var(--primary);padding:.8rem;font:inherit}
textarea:focus,input[type=text]:focus{outline:none;box-shadow:0 0 14px #00ff4180;border-color:var(--primary)}
.slider-container{display:flex;align-items:center;gap:12px}
input[type=range]{appearance:none;height:3px;background:var(--b);flex:1}
input[type=range]::-webkit-slider-thumb{appearance:none;width:16px;height:16px;background:var(--primary);box-shadow:0 0 10px var(--primary)}
.status-sent{display:flex;align-items:center;gap:8px;margin-top:18px;font-size:28px;font-weight:bold;text-shadow:0 0 10px var(--primary)}
.chev{opacity:0}.c1{animation:f .5s .2s forwards}.c2{animation:f .5s .4s forwards}.c3{animation:f .5s .6s forwards}.c4{animation:f .5s .8s forwards}.st{opacity:0;animation:f .5s 1s forwards}
@keyframes f{to{opacity:1}}
button.send{background:none;border:none;color:inherit;cursor:pointer;font:inherit}
.note{margin-top:8px;font-size:.9rem;color:#7cffb4}
</style></head><body><div class="container">
<h1>[ ENCRYPT PROTOCOL ]</h1>
<form class="protocol-box" action="/" method="GET" onsubmit="beforeSend()">
  <label for="plaintext">INPUT PLAINTEXT</label>
  <textarea id="plaintext" rows="4" placeholder="type here..."></textarea>

  <label for="key">SET CIPHER CAESAR KEY</label>
  <div class="slider-container">
    <input type="range" id="key" name="key" min="1" max="25" value="10" oninput="syncKey()">
    <span id="key-value">10</span>
  </div>

  <label for="ciphertext">GENERATED CIPHERTEXT</label>
  <textarea id="ciphertext" rows="4" readonly></textarea>
  <input type="hidden" id="message" name="message">

  <div class="status-sent">
    <span class="chev c1">&gt;</span><span class="chev c2">&gt;</span>
    <span class="chev c3">&gt;</span><span class="chev c4">&gt;</span>
    <button type="submit" class="send st">SENT</button>
  </div>
  <div class="note">Key range: 1-25 | Status: )rawliteral";
  html += statusMessage;
  html += R"rawliteral(</div>
</form></div>
<script>
const keyEl=document.getElementById('key');
const keyVal=document.getElementById('key-value');
const plain=document.getElementById('plaintext');
const cipher=document.getElementById('ciphertext');
const hiddenMsg=document.getElementById('message');

function caesar(s,k){
  k=((k%26)+26)%26;
  return s.replace(/[a-z]/gi,c=>{
    const A=(c>='a'?'a':'A').charCodeAt(0);
    return String.fromCharCode((c.charCodeAt(0)-A+k)%26 + A);
  });
}
function syncKey(){ keyVal.textContent=keyEl.value; renderCipher(); }
function renderCipher(){ cipher.value = caesar(plain.value, parseInt(keyEl.value||'0')); }
function beforeSend(){ hiddenMsg.value = cipher.value; }

plain.addEventListener('input', renderCipher);
window.addEventListener('DOMContentLoaded', renderCipher);
</script>
</body></html>)rawliteral";

  server.send(200, "text/html", html);
  statusMessage = ""; //เคลียร์ทุกครั้ง
}

void setup() {
  Serial.begin(9600);
  while (!Serial) {}

  //Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) { delay(400); Serial.print("."); }
  Serial.println("\nWiFi OK. IP: " + WiFi.localIP().toString());

  //SPI + LoRa
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS_PIN);
  LoRa.setPins(LORA_SS_PIN, LORA_RST_PIN, LORA_DIO0_PIN);
  if (!LoRa.begin(LORA_FREQUENCY)) { Serial.println("LoRa begin FAILED"); while (1) delay(1000); }

  // RF params (ต้องตรงกับ Receiver ฝั่งผู้รับ)
  LoRa.setTxPower(17); //กำลังส่งของคลื่นวิทยุ ยิ่งมากยิ่งส่งได้ไกล แต่ต้องระยะความร้อนที่อาจจะทำให้module เสียหาย
  LoRa.setSpreadingFactor(7); //ค่า SFมากยิ่ง ส่งได้ไกล powerในการส่งมากขึ้น แต่จะใช้เวลาในการส่งนานขึ้ร
  LoRa.setSignalBandwidth(125E3); //ความกว้างแถบสัญญาณของLoRa ยิ่งมาก ยิ่งเร็วขึ้น
  LoRa.setCodingRate4(5); // เป็นการกำหนด Foward Error Correction แก้บิตที่ผิดพลาด กำหนดได้หลายแบบ 4/5. 4/6, 4/7, 4/8
  LoRa.setPreambleLength(8); //ยิ่งมาก ระยะการส่งไกลขึ้นแต่ เวลาส่งออกไปจะนานขึ้น
  LoRa.setSyncWord(0xF3); //เป็นเหมือนkey ที่เอาไว้อ่านทั้งฝั่งผู้รับ กับ ผู้ส่ง ถ้าตรงกันก็จะอ่านกันได้
  LoRa.enableCrc(); //ตรวจสอบ CRC 

  Serial.println("LoRa TX ready.");

  server.on("/", handleRoot);
  server.begin();
  Serial.println("HTTP server started.");
}

void loop() { server.handleClient(); }
