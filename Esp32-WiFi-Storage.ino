// created by @krishna_upx61 
// give me credit for using this code
// give me support for creating something rare
// github https://github/@esp32king
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <SPI.h>
#include <SPIFFS.h>


const byte DNS_PORT = 53;
const char* ssid = "WiFi Storage";
const char* password = "";

DNSServer dnsServer;
WebServer server(80);

unsigned long startTime;

// ---------- ADMIN LOGIN ----------
const char* adminUser = "pkmkb";
const char* adminPass = "pkmkb1234";
bool isLoggedIn = false;


// ---------- SD CS auto-detect (will be set in setup) ----------
int sdCs = -1; // detected CS pin, -1 = not detected

// candidate CS pins to try (add more if your board uses other pins)
const int candidateCsPins[] = {5, 4, 13, 2, 15};
const int candidateCount = sizeof(candidateCsPins) / sizeof(candidateCsPins[0]);

// ---------- STORAGE INFO STRUCT ----------
struct StorageInfo {
  uint64_t total;
  uint64_t used;
  uint8_t percent;
};

// ---------- HTML HEADER ----------
String htmlHeader = R"rawliteral(
<!DOCTYPE html><html><head><meta charset="UTF-8" name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 Media Portal</title>
<style>
body {
  margin: 0;
  background: #111;
  color: #fff;
  font-family: Arial, sans-serif;
  text-align: center;
}
h2 { color: #ff9800; margin-top: 10px; }

.section{padding:10px;}
.thumb{display:inline-block;margin:6px;background:#222;border-radius:10px;width:120px;height:150px;overflow:hidden;vertical-align:top;}
.thumb video,.thumb img{width:100%;height:100px;object-fit:cover;border-bottom:1px solid #333;}
.thumb p{margin:4px;font-size:12px;color:#ccc;word-wrap:break-word;}
#typing{font-size:18px;color:#ffcc00;font-weight:bold;margin-top:10px;height:24px;}
.progress{width:90%;margin:10px auto;height:14px;background:#333;border-radius:6px;overflow:hidden;}
.progress-bar{height:100%;background:#ff9800;width:0%;}
.btn{padding:8px 12px;border-radius:6px;border:none;cursor:pointer;margin:6px;}
.btn-green{background:#4CAF50;color:#fff;}
.btn-blue{background:#2196F3;color:#fff;}
.input{padding:8px;border-radius:6px;border:1px solid #444;background:#222;color:#fff;width:80%;}
.file-row{display:flex;justify-content:space-between;align-items:center;padding:6px 10px;background:#181818;margin:6px;border-radius:6px;}
.file-actions button{margin-left:6px;}
.small{font-size:13px;color:#bbb;}
.container{padding:10px;}
.carousel {
  position: relative;
  width: 100%;
  height: 230px;
  display: flex;
  justify-content: center;
  align-items: center;
  overflow: hidden;
  margin-top: 16px;
}
.carousel img {
  position: absolute;
  width: 60%;
  height: 230px;
  object-fit: cover;
  border-radius: 15px;
  transition: all 1s ease;
  opacity: 0;
  transform: scale(0.75);
}
.carousel img.active {
  z-index: 3;
  opacity: 1;
  transform: scale(1);
  filter: brightness(1);
}
.carousel img.left {
  transform: translateX(-95%) scale(0.75);
  opacity: 0.6;
  filter: brightness(0.5);
  z-index: 2;
}
.carousel img.right {
  transform: translateX(80%) scale(0.7);
  opacity: 0.6;
  filter: brightness(0.5);
  z-index: 1;
}
.caption {
  margin-top: 8px;
  font-size: 16px;
  color: #fff;
  font-weight: bold;
}
.section { padding:10px; }
.thumb { display:inline-block; margin:6px; background:#222; border-radius:10px; width:120px; height:150px; overflow:hidden; vertical-align:top; }
.thumb img, .thumb video, .thumb audio { width:100%; height:100px; object-fit:cover; border-bottom:1px solid #333; }
.thumb p { margin:4px; font-size:12px; color:#ccc; word-wrap:break-word; }
video { background:#000; }
a { text-decoration:none; color:white; }
button, input[type=submit] {
  background: #ff9800;
  border: none;
  padding: 8px 15px;
  color: white;
  border-radius: 8px;
  cursor: pointer;
  margin: 4px;
}
input[type=file], input[type=text] {
  background: #222;
  color: #fff;
  border: 1px solid #444;
  border-radius: 6px;
  padding: 6px;
}
#typing{font-size:18px;color:#fff;font-weight:bold;
  margin-top:10px;height:24px;}
</style>
</head><body>
)rawliteral";

// ---------- FOOTER ----------
String htmlFooter = R"rawliteral(
</body></html>
)rawliteral";

// ---------- URL ENCODE / DECODE ----------
String urlEncode(const String &str){
  String encoded = "";
  char buf[5];
  for (size_t i = 0; i < str.length(); ++i) {
    char c = str.charAt(i);
    if ( (c >= '0' && c <= '9') ||
         (c >= 'A' && c <= 'Z') ||
         (c >= 'a' && c <= 'z') ||
         c=='-'||c=='_'||c=='.'||c=='~' ) {
      encoded += c;
    } else if (c == ' ') {
      encoded += "%20";
    } else {
      sprintf(buf, "%%%02X", (uint8_t)c);
      encoded += String(buf);
    }
  }
  return encoded;
}

String urlDecode(const String &str) {
  String ret;
  char ch;
  for (size_t i = 0; i < str.length(); ++i) {
    if (str[i] == '%') {
      if (i + 2 < str.length()) {
        String hex = str.substring(i+1, i+3);
        ch = (char) strtol(hex.c_str(), NULL, 16);
        ret += ch;
        i += 2;
      }
    } else if (str[i] == '+') ret += ' ';
    else ret += str[i];
  }
  return ret;
}

// ---------- SD helper: recursive folder size ----------
uint64_t sdFolderSize(const String &path) {
  uint64_t total = 0;
  File dir = SD.open(path);
  if (!dir) return 0;
  dir.rewindDirectory();
  File f;
  while ((f = dir.openNextFile())) {
    if (f.isDirectory()) {
      String sub = String(f.name());
      // ensure it starts with /
      if (!sub.startsWith("/")) sub = String("/") + sub;
      total += sdFolderSize(sub);
    } else {
      total += f.size();
    }
    f.close();
  }
  dir.close();
  return total;
}

// ---------- STORAGE HELPERS ----------
StorageInfo getStorageInfoFlash() {
  StorageInfo info;
  uint64_t total = SPIFFS.totalBytes();
  uint64_t used = SPIFFS.usedBytes();
  info.total = total;
  info.used = used;
  info.percent = (total>0) ? (used * 100) / total : 0;
  return info;
}

StorageInfo getStorageInfoSD() {
  StorageInfo info;
  uint64_t total = SD.totalBytes();
  uint64_t used = SD.usedBytes();
  info.total = total;
  info.used = used;
  if (total > 0) info.percent = (used * 100) / total;
  else info.percent = 0;
  return info;
}

// ---------- TYPEWRITER ----------
String typewriterJS() {
  return R"rawliteral(
<div id='typing'></div>
<script>
const words=['Enjoy the Show 🍿','Now Playing 🎬','Movie Time 🎞️','Sit Back & Relax 😎'];
let i=0;let el=document.getElementById('typing');
function typeWord(){
  let w=words[i];let c=0;el.textContent='';
  let t=setInterval(()=>{ el.textContent=w.substring(0,c++); if(c>w.length){clearInterval(t);setTimeout(()=>eraseWord();},1000);} ,80);
}
function eraseWord(){ let w=words[i];let c=w.length;
  let t=setInterval(()=>{ el.textContent=w.substring(0,c--); if(c<0){clearInterval(t);i=(i+1)%words.length;setTimeout(()=>typeWord();},500);} ,50);
}
typeWord();
</script>
)rawliteral";
}

// ---------- CAROUSEL (home) ----------
String makeCarousel() {
   String html;

  // ----- 1️⃣ SPIFFS root first -----
  if (SPIFFS.begin(true)) {
    File root = SPIFFS.open("/"); // root scan
    if (root && root.isDirectory()) {
      File entry = root.openNextFile();
      while (entry) {
        if (!entry.isDirectory()) {
          String path = entry.name(); // e.g., "/image.jpg"
          String lower = path;
          lower.toLowerCase();

          if (lower.endsWith(".jpg") || lower.endsWith(".png") || lower.endsWith(".jpeg") || lower.endsWith(".gif")) {
            String displayName = path;
            if (displayName.startsWith("/")) displayName = displayName.substring(1);

            int dotIndex = displayName.lastIndexOf('.');
            if(dotIndex > 0) displayName = displayName.substring(0, dotIndex);

            String src = String("/file?storage=flash&path=") + urlEncode(path);
            html += "<img class='slideImg' src='" + src + "' data-name='" + displayName + "'>";
          }
        }
        entry.close();
        entry = root.openNextFile();
      }
      root.close();
    }
  }

 // ----- 2️⃣ SD-CARD /media (fixed) -----  
if (sdCs >= 0 && SD.begin(sdCs)) {
  File sdDir = SD.open("/media");
  if (sdDir && sdDir.isDirectory()) {
    File entry = sdDir.openNextFile();
    while (entry) {
      if (!entry.isDirectory()) {
        String name = entry.name(); // filename ya path
        String path = name;

        // Ensure it starts with /media/
        if (!path.startsWith("/media")) {
          if (path.startsWith("/")) path = "/media" + path;
          else path = "/media/" + path;
        }
        path.replace("//", "/"); // double slash avoid

        String lower = path;
        lower.toLowerCase();

        if (lower.endsWith(".jpg") || lower.endsWith(".png") || lower.endsWith(".jpeg") || lower.endsWith(".gif")) {
          String displayName = path;
          displayName.replace("/media/", ""); // just filename
          int dotIndex = displayName.lastIndexOf('.');
          if(dotIndex > 0) displayName = displayName.substring(0, dotIndex);

          String src = String("/file?storage=sd&path=") + urlEncode(path); // full path
          html += "<img class='slideImg' src='" + src + "' data-name='" + displayName + "'>";
        }
      }
      entry.close();
      entry = sdDir.openNextFile();
    }
    sdDir.close();
  }
}


  if (html == "") html = "<p>❌ No images found on SPIFFS or SD</p>";

  return html;
}

// ---------- LOGIN ----------
void handleLogin(){
  if(server.method()==HTTP_POST){
    String user = server.arg("username");
    String pass = server.arg("password");
    if(user==adminUser && pass==adminPass){
      isLoggedIn = true;
      server.sendHeader("Location","/admin");
      server.send(303);
      return;
    } else {
      server.send(200,"text/html",htmlHeader + "<div class='container'><h2>❌ Invalid login</h2><a href='/login'>Try again</a></div>" + htmlFooter);
      return;
    }
  }
  String html = htmlHeader;
  html += "<div class='container'><h2>🔐 Admin Login</h2>";
  html += "<form method='POST' action='/login'><input class='input' name='username' placeholder='Username'><br><br>";
  html += "<input class='input' type='password' name='password' placeholder='Password'><br><br>";
  html += "<input class='btn btn-green' type='submit' value='Login'></form></div>";
  html += htmlFooter;
  server.send(200,"text/html",html);
}


// ---------- ADMIN PAGE ----------
void handleAdmin(){
  if(!isLoggedIn){
    server.sendHeader("Location","/login");
    server.send(303);
    return;
  }

  StorageInfo flash = getStorageInfoFlash();
  StorageInfo sd = getStorageInfoSD();

  String html = htmlHeader;
  html += "<div class='container'>";
  html += "<a href='/logout' class='small'>Logout</a>";
  html += "<h2>Admin Panel</h2>";

  // Storage bars at top (Flash + SD)
  html += "<div style='width:95%;margin:8px auto;text-align:left;'><h4>ESP32 Flash Storage</h4>";
  html += "<div class='progress'><div class='progress-bar' style='width:"+String(flash.percent)+"%'></div></div>";
  html += "<p class='small'>"+String(flash.used/1024)+" KB / "+String(flash.total/1024)+" KB ("+String(flash.percent)+"%)</p></div>";

  html += "<div style='width:95%;margin:8px auto;text-align:left;'><h4>SD Card Storage</h4>";
  if(sd.total>0){
    html += "<div class='progress'><div class='progress-bar' style='width:"+String(sd.percent)+"%'></div></div>";
    html += "<p class='small'>"+String(sd.used/1024/1024)+" MB / "+String(sd.total/1024/1024)+" MB ("+String(sd.percent)+"%)</p></div>";
  } else {
    html += "<p class='small'>Used: "+String(sd.used/1024/1024)+" MB / Total: Unknown (SD size API not available)</p>";
  }

  // Storage selection UI
  html += "<div style='margin-top:18px;'><h3>Select Storage</h3>";
  html += "<button class='btn btn-blue' onclick=\"selectStorage('flash')\">Flash (SPIFFS)</button>";
  html += "<button class='btn btn-green' onclick=\"selectStorage('sd')\">SD Card</button></div>";

  // area where file list will appear
  html += "<div id='fileArea' style='margin-top:18px; text-align:left; width:95%; margin-left:auto; margin-right:auto;'></div>";

  // upload form (dynamic, will include storage param)
  html += "<div id='uploadArea' style='margin-top:12px; display:none; text-align:left; width:95%; margin-left:auto; margin-right:auto;'>";
  html += "<h4>Upload File</h4>";
  html += "<input id='fileInput' type='file'><br><br>";
  // NOTE: using query param for storage so server can read it during multipart upload
  html += "<button class='btn btn-green' onclick='uploadFile()'>Upload</button>";
  html += "</div>";

  // JS: selectStorage, fetch list, delete, upload, folder navigation
  html += R"rawliteral(
<script>
let currentStorage = '';
function selectStorage(s){
  currentStorage = s;
  document.getElementById('fileArea').innerHTML = '<p class="small">Loading...</p>';
  document.getElementById('uploadArea').style.display = 'block';
  fetch('/list?storage='+s+'&path=/')
    .then(r=>r.text())
    .then(t=> document.getElementById('fileArea').innerHTML = t)
    .catch(e=> document.getElementById('fileArea').innerHTML = '<p>Failed to load</p>');
}

function viewFile(path){
  let p = path;
  if(!p.startsWith("/")) p = "/" + p;
  window.open('/file?storage='+currentStorage+'&path='+encodeURIComponent(p), '_blank');
}

function deleteFile(path){
  if(!confirm('Delete '+path+'?')) return;
  fetch('/delete?storage='+currentStorage+'&path='+encodeURIComponent(path))
    .then(r=>r.text()).then(t=>{ alert(t); selectStorage(currentStorage); });
}

function uploadFile(){
  let f=document.getElementById('fileInput').files[0];
  if(!f){ alert('Select file first'); return; }
  let form=new FormData();
  form.append('file', f);
  // send storage as query param so server can read it reliably during upload
  let xhr = new XMLHttpRequest();
  xhr.open('POST', '/upload?storage='+encodeURIComponent(currentStorage), true);
  xhr.onload = function () {
    if (xhr.status == 200) {
      alert(xhr.responseText);
      selectStorage(currentStorage);
    } else {
      alert('Upload failed: ' + xhr.statusText);
    }
  };
  xhr.send(form);
}

// folder navigation for SD
function openFolder(path){
  currentStorage = 'sd';
  fetch('/list?storage=sd&path='+encodeURIComponent(path))
    .then(r=>r.text()).then(t=> document.getElementById('fileArea').innerHTML = t);
}
function goBack(current){
  let p = current;
  if(p === '/' || p.length === 0){ selectStorage('sd'); return; }
  if(p.endsWith('/')) p = p.slice(0, -1);
  let idx = p.lastIndexOf('/');
  let parent = '/';
  if(idx > 0) parent = p.substring(0, idx);
  fetch('/list?storage=sd&path='+encodeURIComponent(parent))
    .then(r=>r.text()).then(t=> document.getElementById('fileArea').innerHTML = t);
}
</script>
)rawliteral";


  html += "</div>" + htmlFooter;
  server.send(200,"text/html",html);
}

// ---------- LOGOUT ----------
void handleLogout(){
  isLoggedIn = false;
  server.sendHeader("Location","/");
  server.send(303);
}

// ---------- LIST endpoint (AJAX) ----------
void handleList(){
  if(!isLoggedIn){ server.send(403,"text/plain","Forbidden"); return; }
  if(!server.hasArg("storage")){ server.send(400,"text/plain","Missing storage"); return; }
  String storage = server.arg("storage");

  // optional path (for SD folder navigation)
  String path = "/";
  if(server.hasArg("path")) {
    path = urlDecode(server.arg("path"));
    if(!path.startsWith("/")) path = "/" + path;
  }

  String out;
  out += "<h3>Browsing: " + storage + " <span class='small'>("+path+")</span></h3>";
  out += "<div>";

  if(storage == "flash"){
    File root = SPIFFS.open(path);
    if(!root){ out += "<p>SPIFFS not available or path not found</p>"; server.send(200,"text/html",out); return; }
    root.rewindDirectory();
    File file = root.openNextFile();
    bool any=false;
    while(file){
      String name = file.name();
      any=true;
      String base = name;
      int idx = base.lastIndexOf('/');
      if(idx >= 0) base = base.substring(idx+1);
      out += "<div class='file-row'><div><b>"+base+"</b><div class='small'>SPIFFS</div></div>";
      out += "<div class='file-actions'><button class='btn btn-blue' onclick=\"viewFile('"+name+"')\">Open</button>";
      out += "<button class='btn' style='background:#d9534f;color:white' onclick=\"deleteFile('"+name+"')\">Delete</button></div></div>";
      file.close();
      file = root.openNextFile();
    }
    if(!any) out += "<p class='small'>No files in SPIFFS</p>"; 

  }
  else if(storage == "sd"){
    if (sdCs < 0) { out += "<p>SD not detected</p>"; server.send(200,"text/html",out); return; }
    if(!SD.begin(sdCs)){ out += "<p>SD not mounted</p>"; server.send(200,"text/html",out); return; }
    if(path.endsWith("/") && path.length() > 1) path = path.substring(0, path.length()-1);
    File root = SD.open(path);
    if(!root){ out += "<p>SD open failed</p>"; server.send(200,"text/html",out); return; }

    if(path != "/"){
      out += "<button class='btn btn-blue' onclick=\"goBack('"+path+"')\">⬅ Back</button><br><br>";
    }

    root.rewindDirectory();
    File file = root.openNextFile();
    bool any=false;
    while(file){
      String name = file.name();
      any=true;
      String base = name;
      int idx = base.lastIndexOf('/');
      if(idx >= 0) base = base.substring(idx+1);

      String fullPath;

// Root folder → use name exactly as SD returns
if (path == "/") {
    fullPath = String(file.name());
} else {
    fullPath = path + "/" + base;
    fullPath.replace("//","/");
}

      if(file.isDirectory()){
        out += "<div class='file-row'><div><b>"+base+"</b><div class='small'>Folder</div></div>";
        out += "<div class='file-actions'><button class='btn btn-blue' onclick=\"openFolder('"+fullPath+"')\">Open</button></div></div>";
      } else {
        out += "<div class='file-row'><div><b>"+base+"</b><div class='small'>SD Card</div></div>";
        out += "<div class='file-actions'><button class='btn btn-blue' onclick=\"viewFile('"+fullPath+"')\">Open</button>";
        out += "<button class='btn' style='background:#d9534f;color:white' onclick=\"deleteFile('"+fullPath+"')\">Delete</button></div></div>";
      }
      file.close();
      file = root.openNextFile();
    }
    if(!any) out += "<p class='small'>No files on SD at this path</p>";
    root.close();
  }
  else {
    out += "<p>Unknown storage</p>";
  }

  out += "</div>";
  server.send(200,"text/html",out);
}

// ---------- FILE SERVE (/file?storage=...&path=...) ----------
void handleFile(){
  if(!server.hasArg("storage")||!server.hasArg("path")){ server.send(400,"text/plain","Missing"); return; }
  String storage = server.arg("storage");
  String path = urlDecode(server.arg("path"));
  if(!path.startsWith("/")) path = "/" + path;

  String contentType = "application/octet-stream";
  String lower = path;
  lower.toLowerCase();
  if(lower.endsWith(".mp4")||lower.endsWith(".m4v")) contentType = "video/mp4";
  else if(lower.endsWith(".mp3")) contentType = "audio/mpeg";
  else if(lower.endsWith(".wav")) contentType = "audio/wav";
  else if(lower.endsWith(".jpg")||lower.endsWith(".jpeg")) contentType = "image/jpeg";
  else if(lower.endsWith(".png")) contentType = "image/png";
  else if(lower.endsWith(".gif")) contentType = "image/gif";
  // TEXT & CODE FILES
else if(lower.endsWith(".txt")) contentType = "text/plain";
else if(lower.endsWith(".py")) contentType = "text/plain";
else if(lower.endsWith(".json")) contentType = "application/json";
else if(lower.endsWith(".html")||lower.endsWith(".htm")) contentType = "text/html";
else if(lower.endsWith(".css")) contentType = "text/css";
else if(lower.endsWith(".js")) contentType = "application/javascript";

// DATA / OTHER
else if(lower.endsWith(".csv")) contentType = "text/csv";
else if(lower.endsWith(".pdf")) contentType = "application/pdf";
else if(lower.endsWith(".zip")) contentType = "application/zip";
else if(lower.endsWith(".bin")) contentType = "application/octet-stream";

  if(storage == "sd"){
    if (sdCs < 0) { server.send(500,"text/plain","SD not available"); return; }
    if(!SD.exists(path)){ server.send(404,"text/plain","Not Found: " + path); return; }
    File f = SD.open(path, FILE_READ);
    if(!f){ server.send(500,"text/plain","Failed to open file"); return; }
    server.sendHeader("Accept-Ranges","bytes");
    server.streamFile(f, contentType);
    f.close();
    return;
  } else if(storage == "flash"){
    if(!SPIFFS.exists(path)){ server.send(404,"text/plain","Not Found"); return; }
    File f = SPIFFS.open(path, FILE_READ);
    if(!f){ server.send(500,"text/plain","Failed to open file"); return; }
    server.sendHeader("Accept-Ranges","bytes");
    server.streamFile(f, contentType);
    f.close();
    return;
  } else {
    server.send(400,"text/plain","Unknown storage");
    return;
  }
}

// ---------- DELETE endpoint ----------
void handleDelete(){
  if(!isLoggedIn){ server.send(403,"text/plain","Forbidden"); return; }
  if(!server.hasArg("storage")||!server.hasArg("path")){ server.send(400,"text/plain","Missing"); return; }

  String storage = server.arg("storage");
  String path = urlDecode(server.arg("path"));
  if(!path.startsWith("/")) path = "/" + path;
  bool ok=false;
  if(storage=="sd"){
    if (sdCs < 0) { server.send(500,"text/plain","SD not available"); return; }
    if(SD.exists(path)) ok = SD.remove(path);
  } else if(storage=="flash"){
    if(SPIFFS.exists(path)) ok = SPIFFS.remove(path);
  }
  if(ok) server.send(200,"text/plain","Deleted");
  else server.send(500,"text/plain","Delete failed or file not found");
}

// ---------- UPLOAD endpoint ----------
void handleUpload() {
  // Expect storage as query param: /upload?storage=sd or /upload?storage=flash
  String targetStorage = "flash";
  if (server.hasArg("storage")) targetStorage = server.arg("storage");

  HTTPUpload& upload = server.upload();
  static File uploadFile;
  static String uploadPath;

  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) filename = String("/") + filename;
    uploadPath = filename;

    if (targetStorage == "sd") {
      if (sdCs < 0) { server.send(500,"text/plain","SD not available"); return; }
      if (!SD.begin(sdCs)) { server.send(500,"text/plain","SD init failed"); return; }
      uploadFile = SD.open(uploadPath, FILE_WRITE);
    } else {
      uploadFile = SPIFFS.open(uploadPath, FILE_WRITE);
    }

    if (!uploadFile) {
      server.send(500,"text/plain","Failed to create file");
      return;
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) uploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
      server.send(200,"text/plain", String("Uploaded to ") + targetStorage + String(" : ") + uploadPath);
    } else {
      server.send(500,"text/plain","Upload error");
    }
  }
}

// ---------- HOME PAGE ----------
void handleRoot() {
  String html = htmlHeader;
  html += "<a2>Scenic <span style='color: red;'>v/s</span> Sinner</a2><div class='carousel' id='carousel'>";
  html += makeCarousel();
  html += R"rawliteral(
</div><div class="caption" id="caption"></div>
<script>
let index = 0;
let imgs = document.querySelectorAll('#carousel img');
let caption = document.getElementById('caption');
let names = Array.from(imgs).map(img => img.getAttribute('data-name'));
function showSlides(){
  imgs.forEach((img,i)=>{
    img.classList.remove('left','right','active');
    if(i===index) img.classList.add('active');
    else if(i===(index+imgs.length-1)%imgs.length) img.classList.add('left');
    else if(i===(index+1)%imgs.length) img.classList.add('right');
  });
  caption.textContent = names[index];                          
  index=(index+1)%imgs.length;
}
if(imgs.length>0){ showSlides(); setInterval(showSlides,3000); }
</script>
        <style>
        body { font-family: 'Roboto Mono', monospace; background-color: #0a0a1a; color: #a0a0b0; overflow: hidden; display: flex; flex-direction: column; justify-content: center; align-items: center; min-height: 100vh; margin: 0; position: relative; background-image: radial-gradient(circle at center, #1a1a3a 0%, #0a0a1a 70%); }
.container { text-align: center; padding: 2.5rem; background-color: rgba(0,0,0,0.02); border-radius: 1.5rem; box-shadow: none; max-width: 90%; width: 800px; position: relative; z-index: 10; border: none; animation: fadeIn 2s ease-out forwards; }
@keyframes fadeIn { from { opacity:0; transform:translateY(20px); } to { opacity:1; transform:translateY(0); } }
h1 { font-family: 'Inter', sans-serif; font-size:3.5rem; font-weight:900; color:#00e6e6; text-shadow:none; margin-bottom:1.5rem; animation:sparkGlitchBlue 0.2s infinite alternate; letter-spacing:0.1em; }
@keyframes sparkGlitchBlue { 0% { text-shadow:none; transform:translate(0,0); } 10% { text-shadow:1px 0 0 #FF00FF,-1px 0 0 #FFFF00; transform:translate(1px,0); } 20% { text-shadow:none; transform:translate(0,0); } 30% { text-shadow:-1px 0 0 #00FFFF,1px 0 0 #FF00FF; transform:translate(-1px,0); } 40% { text-shadow:none; transform:translate(0,0); } 50% { text-shadow:0 1px 0 #FFFF00,0 -1px 0 #00FFFF; transform:translate(0,1px); } 60% { text-shadow:none; transform:translate(0,0); } 70% { text-shadow:0 -1px 0 #FF00FF,0 1px 0 #FFFF00; transform:translate(0,-1px); } 80% { text-shadow:none; transform:translate(0,0); } 90% { text-shadow:1px 1px 0 #00FFFF,-1px -1px 0 #FF00FF; transform:translate(1px,1px); } 100% { text-shadow:none; transform:translate(0,0); } }
.betrayal-text { font-family:'Inter',sans-serif; font-size:1.2rem; font-weight:700; color:#FF0000; margin-top:1rem; margin-bottom:2rem; }
.description-message { margin-top:2rem; font-size:1rem; color:#FFFFFF; line-height:1.8; max-width:600px; margin-left:auto; margin-right:auto; text-shadow:none; animation:none; }
.icf_tg { margin:20px 0; }
.tg-button { display:inline-flex; align-items:center; justify-content:center; background-color:transparent; border:2px solid #FFFFFF; border-radius:5px; padding:8px 15px; text-decoration:none; color:#00e6e6; font-size:1.1em; transition: transform 0.2s, background-color 0.3s, box-shadow 0.3s; cursor:pointer; position:relative; overflow:hidden; z-index:10; box-shadow:0 0 10px rgba(0,230,230,0.4),0 0 20px rgba(0,230,230,0.2); }
.tg-button:hover { background-color:rgba(0,230,230,0.1); box-shadow:0 0 15px rgba(0,230,230,0.6),0 0 30px rgba(0,230,230,0.4); transform:translateY(-2px); }
.tg-button i { margin-right:0.75rem; font-size:1.3rem; }
canvas { position:absolute; top:0; left:0; width:100%; height:100%; z-index:1; opacity:0.3; }
@media (max-width:768px) { .container { padding:1.5rem; border-radius:1rem; } h1 { font-size:2.5rem; } .betrayal-text { font-size:1rem; } .description-message { font-size:0.9rem; } .tg-button { padding:6px 12px; font-size:1em; } .tg-button i { margin-right:0.5rem; font-size:1.1rem; } }
@media (max-width:480px) { h1 { font-size:2rem; } .betrayal-text { font-size:0.9rem; } .description-message { font-size:0.85rem; } .tg-button { padding:5px 10px; font-size:0.9em; } .tg-button i { margin-right:0.5rem; font-size:1.1rem; } }
.lay { animation:paths 5s step-end infinite; }
@keyframes paths { 0% { clip-path:polygon(0% 43%,83% 43%,83% 22%,23% 22%,23% 24%,91% 24%,91% 26%,18% 26%,18% 83%,29% 83%,29% 17%,41% 17%,41% 39%,18% 39%,18% 82%,54% 82%,54% 88%,19% 88%,19% 4%,39% 4%,39% 14%,76% 14%,76% 52%,23% 52%,23% 35%,19% 35%,19% 8%,36% 8%,36% 31%,73% 31%,73% 16%,1% 16%,1% 56%,50% 56%,50% 8%); } 5% { clip-path:polygon(0% 29%,44% 29%,44% 83%,94% 83%,94% 56%,11% 56%,11% 64%,94% 64%,94% 70%,88% 70%,88% 32%,18% 32%,18% 96%,10% 96%,10% 62%,9% 62%,9% 84%,68% 84%,68% 50%,52% 50%,52% 55%,35% 55%,35% 87%,25% 87%,25% 39%,15% 39%,15% 88%,52% 88%); } 30% { clip-path:polygon(0% 53%,93% 53%,93% 62%,68% 62%,68% 37%,97% 37%,97% 89%,13% 89%,13% 45%,51% 45%,51% 88%,17% 88%,17% 54%,81% 54%,81% 75%,79% 75%,79% 76%,38% 76%,38% 28%,61% 28%,61% 12%,55% 12%,55% 62%,68% 62%,68% 51%,0% 51%,0% 92%,63% 92%,63% 4%,65% 4%); } 45% { clip-path:polygon(0% 33%,2% 33%,2% 69%,58% 69%,58% 94%,55% 94%,55% 25%,33% 25%,33% 85%,16% 85%,16% 19%,5% 19%,5% 20%,79% 20%,79% 96%,93% 96%,93% 50%,5% 50%,5% 74%,55% 74%,55% 57%,96% 57%,96% 59%,87% 59%,87% 65%,82% 65%,82% 39%,63% 39%,63% 92%,4% 92%,4% 36%,24% 36%,24% 70%,1% 70%,1% 43%,15% 43%,15% 28%,23% 28%,23% 71%,90% 71%,90% 86%,97% 86%,97% 1%,60% 1%,60% 67%,71% 67%,71% 91%,17% 91%,17% 14%,39% 14%,39% 30%,58% 30%,58% 11%,52% 11%,52% 83%,68% 83%); } 76% { clip-path:polygon(0% 26%,15% 26%,15% 73%,72% 73%,72% 70%,77% 70%,77% 75%,8% 75%,8% 42%,4% 42%,4% 61%,17% 61%,17% 12%,26% 12%,26% 63%,73% 63%,73% 43%,90% 43%,90% 67%,50% 67%,50% 41%,42% 41%,42% 46%,50% 46%,50% 84%,96% 84%,96% 78%,49% 78%,49% 25%,63% 25%,63% 14%); } 90% { clip-path:polygon(0% 41%,13% 41%,13% 6%,87% 6%,87% 93%,10% 93%,10% 13%,89% 13%,89% 6%,3% 6%,3% 8%,16% 8%,16% 79%,0% 79%,0% 99%,92% 99%,92% 90%,5% 90%,5% 60%,0% 60%,0% 48%,89% 48%,89% 13%,80% 13%,80% 43%,95% 43%,95% 19%,80% 19%,80% 85%,38% 85%,38% 62%); } 1%,7%,33%,47%,78%,93% { clip-path:none; } }
    </style>
</head>
<body>
    <div class="container rounded-xl shadow-lg p-8 md:p-12">
        <center>
            
            <br>
            <center>
              <h2 class="lay">
              <font size="20" font="" face="New Rocker" color=""style="color: White; text-shadow: 0px 4px 14px red;">⚓ SLAYER
                <font color=""style="color: White; text-shadow: 0px 3px 12px red;">⚔️</font></font></h2><br>
            </center>

        <p class="betrayal-text">A WARRIOR’S PROMIS IS SIMPLE:</br>
FIGHT TILL THE FINAL BREATH.</p>
        <p class="description-message">

        </p>
        
    </div>

    <div class="icf_tg">
        <a id="icf-button" class="tg-button" href="https://instagram.com/krishna_upx61" target="_blank">
            <i class="fab fa-telegram-plane"></i> @Krishna_upx61
        </a>
    </div>


    <canvas id="matrix-canvas"></canvas>

    <script>
        const canvas = document.getElementById('matrix-canvas');
        const ctx = canvas.getContext('2d');
        const icfButton = document.getElementById('icf-button');

        let columns;
        let drops;

        function initializeMatrix() {
            canvas.width = window.innerWidth;
            canvas.height = window.innerHeight;

            columns = Math.floor(canvas.width / 15);
            drops = [];
            for (let x = 0; x < columns; x++) {
                drops[x] = 1;
            }
        }

        const characters = '01';

        function drawMatrix() {
            ctx.fillStyle = 'rgba(10, 10, 26, 0.15)';
            ctx.fillRect(0, 0, canvas.width, canvas.height);

            ctx.fillStyle = '#00e6e6';
            ctx.font = '15px Roboto Mono';

            for (let i = 0; i < drops.length; i++) {
                const text = characters.charAt(Math.floor(Math.random() * characters.length));
                ctx.fillText(text, i * 15, drops[i] * 15);

                if (drops[i] * 15 > canvas.height && Math.random() > 0.98) {
                    drops[i] = 0;
                }

                drops[i]++;
            }
        }


        window.onload = function() {
            initializeMatrix();
            setInterval(drawMatrix, 30);
            window.addEventListener('resize', initializeMatrix);
        };
    </script>
    <style> 
    .memeck { position: absolute; margin: auto; height: 50%; top: 0; bottom: 0; left: 0; right: 0; }
</style> 

<body onclick="playAudio(); goFull(); doVibrate(); speakNow();">

<audio id="mp3">
  <source src="http://192.168.4.1/file?storage=sd&path=%2Freels2%2FOthlali.mp3" type="audio/mpeg">
</audio>

<div class="memeck">

<script>
    var icf = document.getElementById("mp3");

    function playAudio() {
        icf.pause();              // reset fix
        icf.currentTime = 0;      // restart from beginning
        icf.play().catch(err => {
            console.log("Play blocked:", err);
        });
    }

    function goFull() {
        let el = document.documentElement;
        (el.requestFullscreen || el.webkitRequestFullscreen || el.msRequestFullscreen).call(el);
    }

    function doVibrate() {
        if ("vibrate" in navigator) {
            navigator.vibrate([
                200, 600, 
                400, 400,
                200, 600,
                3000, 1000,

                100, 400,
                400, 400,
                200, 400,
                400, 2000,

                100, 400,
                100, 400,
                400, 400,
                100, 400,
                400, 400,
                6000, 2000,
                100
            ]);
        }
    }

    function speakNow() {
        let msg = new SpeechSynthesisUtterance("Hell, Hello. Hello Bro, Don't Touch Any Where Otherwize You Gonna will FINISH.... SO Please stay from me! Jai Hind Jai Bharat. Vande Matram");
        speechSynthesis.speak(msg);
    }
</script>
</body>

</html>

)rawliteral";

  html += typewriterJS();

  html += "<div style='position: fixed; bottom: 10px; left: 0; width: 100%; text-align: center; font-size: 14px;'>created by <span style='color: red;'>@krishna_upx61</span></div>";
  server.send(200,"text/html",html);
}

// ---------- SD CS AUTO-DETECT ----------
int detectSdCs() {
  Serial.println("Trying to auto-detect SD CS pin...");
  // ensure SPI is started (some boards require)
  SPI.begin(); // use default SCK/MOSI/MISO
  for (int i = 0; i < candidateCount; ++i) {
    int pin = candidateCsPins[i];
    Serial.printf("Trying CS pin %d ...\n", pin);
    // try begin with this CS
    if (SD.begin(pin, SPI)) {
      Serial.printf("SD detected on CS pin %d\n", pin);
      return pin;
    }
    // if begin failed ensure end is called
    SD.end();
    delay(50);
  }
  Serial.println("SD CS auto-detect failed.");
  return -1;
}

// ---------- SETUP ----------
void setup(){
  Serial.begin(115200);
  SPIFFS.begin();
  startTime = millis();

  WiFi.mode(WIFI_AP);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);  // Max power
  // Start Access Point
   WiFi.softAP(ssid);
  Serial.println("AP IP: " + WiFi.softAPIP().toString());

  dnsServer.start(53, "*", WiFi.softAPIP());

   // detect SD CS
  sdCs = detectSdCs();
  if (sdCs >= 0) {
    // try mount once, with lower SPI speed first (some cards need lower)
    Serial.printf("Attempting SD.begin on CS=%d\n", sdCs);
    if (SD.begin(sdCs, SPI, 20000000UL)) {
      // sdMounted = true;
      Serial.println("SD initialized (20MHz)");
    } else if (SD.begin(sdCs, SPI, 4000000UL)) {
     // sdMounted = true;
      Serial.println("SD initialized (4MHz) fallback");
    } else {
     // sdMounted = false;
      Serial.println("SD begin failed after detect");
    }
  } else {
    Serial.println("No SD detected automatically.");
  }

  // Routes
  server.on("/", handleRoot);
  server.on("/login", HTTP_ANY, handleLogin);
  server.on("/admin", handleAdmin);
  server.on("/logout", handleLogout);
  server.on("/list", HTTP_GET, handleList);
  server.on("/file", HTTP_GET, handleFile);
  server.on("/delete", HTTP_GET, handleDelete);
  server.on("/upload", HTTP_POST, []() {
    server.send(200,"text/plain","Upload endpoint");
  }, handleUpload);

   server.onNotFound([]() {
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
});


server.on("/generate_204", []() { server.sendHeader("Location","/"); server.send(302); });
server.on("/fwlink", []() { server.sendHeader("Location","/"); server.send(302); });
server.on("/hotspot-detect.html", []() { server.sendHeader("Location","/"); server.send(302); });
server.on("/connecttest.txt", []() { server.sendHeader("Location","/"); server.send(302); });

  server.begin();
  Serial.println("Server started. Connect to http://192.168.4.1");
}

// ---------- LOOP ----------
void loop(){
  dnsServer.processNextRequest();
  server.handleClient();
}
