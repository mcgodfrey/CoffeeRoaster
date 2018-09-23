#include "webServer.h"
#include <Arduino.h>
#include <FS.h>
#include <ArduinoJson.h>
#include "controller.h"


typedef struct {
  String name;
  String dataType;
  double doubleValue;
  String strValue;
  int intValue;
} Param;

typedef struct {
  Param data[12];
  int numParams;
} Status;

ESP8266WebServer server = ESP8266WebServer(80);       // create a web server on port 80
WebSocketsServer webSocket = WebSocketsServer(81);    // and listen for websocket on port 81
File fsUploadFile;

// Handlers
//admin
static void handleUpload(void);
static void handleFileUpload(void);
//void handleFileDelete(void);
static void handleFileList(void);
static bool handleFileRead(String path);
static void handleNotFound(void);
//websocket
static void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);


//helpers
static bool lookup_param_value(String param, Param *response_data);
static bool set_param_value(String param, JsonVariant value);
static bool run_command(String command);
static void getStatus(Status *status);
static void sendStatus(uint8_t num);
static String status2JSON(Status status);
static bool handleUpdateMessage(uint8_t *message);

// Helpers
static String getContentType(String filename);
static String formatBytes(size_t bytes);
static void serve_html(String path);
static void send_404(void);

///////////////////////////////////
// Setup functions
///////////////////////////////////
void webserverSetup(){
  server.on("/", HTTP_GET, [](){serve_html("/index.html");});
  server.on("/list", HTTP_GET, handleFileList);

  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  //server.on("/upload", HTTP_GET, [](){serve_html("/upload.html");});
  server.on("/upload", HTTP_GET, handleUpload);
  server.on("/upload", HTTP_POST, [](){ server.send(200, "text/plain", ""); }, handleFileUpload);

  server.onNotFound(handleNotFound);

  server.begin();

  Serial.println("HTTP server started");

  webSocket.onEvent(webSocketEvent);
  webSocket.begin();
  Serial.println("Websocket server started on port 81");
}

/*
 * push a datapoint to any connected websocket clients
 */
void webserverPushDatapoint(uint32_t timestamp, double setpoint, double output, double temperature){
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["type"] = "data";
  root["data"] = String(timestamp) + String(",") + String(setpoint) + String(",") + String(output) + String(",") + String(temperature);
  root.printTo(Serial);Serial.println("");
  String dataString;
  root.printTo(dataString);
  webSocket.broadcastTXT(dataString);
}

void webserverLog(String log){
  webSocket.broadcastTXT(String("{\"type\":\"log\",\"data\":\"") + log + String("\"}"));
}

void webserverPushData(String name, double data){
  Status status;
  status.numParams = 1;
  status.data[0].name = name;
  status.data[0].doubleValue = data;
  status.data[0].dataType = "double";
  String statusString = status2JSON(status);
  webSocket.broadcastTXT(statusString);
}


/*
 * Callback whenever a websocket message is received
 */
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  Serial.printf("Websocket Event started\n");
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED:
      // Send the status immediately on connection
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
      }
      sendStatus(num);
      break;
    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);
      handleUpdateMessage(payload);
      sendStatus(num);
      break;
    case WStype_BIN:
      Serial.printf("[%u] get binary length: %u\n", num, length);
      webSocket.sendTXT(num, "Binary data not supported");
      break;
    case WStype_ERROR:
      Serial.printf("[%u] error\n", num);
      break;
    default:
      Serial.printf("Unhandled websocket type: <%d>\n", type);
      break;
  }
}


/*
 * Send the status to client number "num"
 */
void sendStatus(uint8_t num){
  Status status;
  getStatus(&status);
  String statusString = status2JSON(status);
  webSocket.sendTXT(num, statusString);
}

/*
 * Called whenever we receive a message from the client.
 * It should handle updating parameters and running commands.
 */
bool handleUpdateMessage(uint8_t *message){
  bool error = false;
  
  //parse the JSON input
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(message);
  if(root.success()){  
    //Check if we need to set any parameters
    if(root.containsKey("parameters")){
      JsonObject& parameters = root["parameters"];
      for (const auto& element: parameters){
        set_param_value(element.key, element.value);
      }
    }
     //Check if there are any commands to run
    if(root.containsKey("commands")){
      JsonArray& commands = root["commands"];
      for (const auto& element: commands){
        run_command(element.as<char*>());
      }
    }
  }else{
    error = true;
  }

  return(error);
}


/*
 * populate a status cluster with data
 */
void getStatus(Status *status){
  Serial.printf("Getting status\n");
  status->numParams = 0;
  
  int num_params = 8;
  String params[] = {"p", "i", "d", "setpoint", "temperature", "programMode", "state", "duty_cycle"};
  for(int i=0; i<num_params; i++){
    lookup_param_value(params[i], &status->data[i]);
    status->numParams++;
  }
}


bool run_command(String command){
  bool success = false;
  if(command == "start"){
    Serial.println("  Starting controller");
    controller.start();
    success = true;
  }else if(command == "stop"){
    Serial.println("  Stopping controller");
    controller.stop();
    success = true;
  }else if(command == "restart"){
    Serial.println("  Restarting controller");
    controller.restart();
    success = true;
  }else if(command == "saveConfig"){
    Serial.println("  Saving controller config");
    controller.saveConfig();
    success = true;
  }else if(command == "simple_mode"){
    Serial.println("  Switch to simple mode - Not implemented yet");
    success = true;
  }else if(command == "program_mode"){
    Serial.println("  Switch to program mode - Not implemented yet");
    success = true;
  }else{
    Serial.print("  Unknown command: "); Serial.println(command);
    success = false;
  }
  return(success);
}

/*
 * Look up the value of parameter "param"
 * Populate the response_data struct
 * return true/false for found/not found
 */
bool lookup_param_value(String param, Param *response_data){
  response_data->name = param;
  if(param == "p"){
    response_data->doubleValue = controller.getP();
    response_data->dataType = "double";
  }else if(param == "i"){
    response_data->doubleValue = controller.getI();
    response_data->dataType = "double";
  }else if(param == "d"){
    response_data->doubleValue = controller.getD();
    response_data->dataType = "double";
  }else if(param == "setpoint"){
    response_data->doubleValue = controller.getSetpoint();
    response_data->dataType = "double";
  }else if(param == "ramp_rate"){
    response_data->doubleValue = controller.ramp_rate;
    response_data->dataType = "double";
  }else if(param == "temperature"){
    response_data->doubleValue = controller.getTemperature();
    response_data->dataType = "double";
  }else if(param == "programMode"){
    response_data->doubleValue = controller.programMode;
    response_data->dataType = "double";
  }else if(param == "state"){
    response_data->doubleValue = controller.state;
    response_data->dataType = "double";
  }else if(param == "duty_cycle"){
    response_data->doubleValue = controller.triac.duty_cycle;
    response_data->dataType = "double";           
  }else{
    return false;
  }
  return true;
}


bool set_param_value(String param, JsonVariant value){
  bool status = false;
  if(param == "p" && value.is<float>()){
    controller.setP(value.as<float>());
    status = true;
  }else if(param == "i" && value.is<float>()){
    controller.setI(value.as<float>());
    status = true;
  }else if(param == "d" && value.is<float>()){
    controller.setD(value.as<float>());
    status = true;
  }else if(param == "setpoint" && value.is<float>()){
    controller.setSetpoint(value.as<float>());
    status = true;
  }else if(param == "ramp_rate" && value.is<float>()){
    controller.ramp_rate = value.as<float>();
    status = true;
  }
  return(status);
}


/*
 * Convert a status struct into a json status string
 */
String status2JSON(Status status){
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["type"] = "status";
  JsonObject& data = root.createNestedObject("data");
  for(int i=0; i<status.numParams; i++){
    if(status.data[i].dataType == "double"){
      data[status.data[i].name] = status.data[i].doubleValue;
    }else if(status.data[i].dataType == "int"){
      data[status.data[i].name] = status.data[i].intValue;
    }else if(status.data[i].dataType == "string"){
      data[status.data[i].name] = status.data[i].strValue;
    }else{
      Serial.print("Unknown data type: ");Serial.println(status.data[i].dataType);
    }
  }
  root.printTo(Serial);Serial.println("");
  String statusString;
  root.printTo(statusString);
  return(statusString);
}


void handleUpload(){
  //Should first check if there is an upload.html and try to use that. If not then fall back onto this hard coded one.
  String message = String("<!DOCTYPE html><html><head><title>ESP8266 SPIFFS File Upload</title></head><body><h1>ESP8266 SPIFFS File Upload</h1><p>Select a new file to upload to the ESP8266. Existing files will be replaced.</p><form method=\"POST\" enctype=\"multipart/form-data\"><input type=\"file\" name=\"data\"> <input class=\"button\" type=\"submit\" value=\"Upload\"></form></body></html>");
  server.send(200, "text/html", message);
}

// Upload a new file to the SPIFFS
void handleFileUpload(){
  HTTPUpload& upload = server.upload();
  Serial.print("Uploading file: "); Serial.println(upload.filename);
  if(upload.status == UPLOAD_FILE_START){
    Serial.println("Starting file upload...");
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    Serial.print("handleFileUpload Name: "); Serial.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    Serial.println("Performing file upload...");
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if(upload.status == UPLOAD_FILE_END){
    Serial.println("Finished file upload");
    if(fsUploadFile) {                                    // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
      server.sendHeader("Location","/upload");      // Redirect the client to the success page
      server.send(303);
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}


/*
void handleFileDelete(){
  if(server.args() == 0) return server.send(500, "text/plain", "BAD ARGS");
  String path = server.arg(0);
  Serial.println("handleFileDelete: " + path);
  if(path == "/")
    return server.send(500, "text/plain", "BAD PATH");
  if(!SPIFFS.exists(path))
    return server.send(404, "text/plain", "FileNotFound");
  SPIFFS.remove(path);
  server.send(200, "text/plain", "");
  path = String();
}
*/


void handleFileList() {
  String path;
  if(server.hasArg("dir")){
    path = server.arg("dir");
  }else{
    path = "/";
  }
  
  Serial.println("handleFileList: " + path);
  Dir dir = SPIFFS.openDir(path);

  String output = "[";
  while(dir.next()){
    File entry = dir.openFile("r");
    if (output != "[") output += ',';
    bool isDir = false;
    output += "{\"type\":\"";
    output += (isDir)?"dir":"file";
    output += "\",\"name\":\"";
    output += String(entry.name()).substring(1);

    size_t fileSize = dir.fileSize();
    output += "\",\"size\": \"";
    output += formatBytes(fileSize);
    
    output += "\"}";
    entry.close();
  }
  
  output += "]";
  server.send(200, "text/json", output);
}


// send the right file to the client (if it exists)
bool handleFileRead(String path) {
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";         // If a folder is requested, send the index file
  String contentType = getContentType(path);            // Get the MIME type
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
    if(SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    server.streamFile(file, contentType);
    file.close();
    return true;
  }
  Serial.println("handleFileRead: " + path);
  Serial.println("\tFile Not Found");
  return false;                                         // If the file doesn't exist, return false
}

void handleNotFound(){
  if (!handleFileRead(server.uri())){
    send_404();
  }
}

///////////////////////////////////
// Helpers
///////////////////////////////////

// convert the file extension to the MIME type
String getContentType(String filename) {
  if(server.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

String formatBytes(size_t bytes){
  if (bytes < 1024){
    return String(bytes)+"B";
  } else if(bytes < (1024 * 1024)){
    return String(bytes/1024.0)+"KB";
  } else if(bytes < (1024 * 1024 * 1024)){
    return String(bytes/1024.0/1024.0)+"MB";
  } else {
    return String(bytes/1024.0/1024.0/1024.0)+"GB";
  }
}


void serve_html(String path){
  Serial.print("Attempting to serve html file: "); Serial.println(path);
  if (!handleFileRead(path)){
    server.send(404, "text/plain", String("404: Not Found: ") + path); // otherwise, respond with a 404 (Not Found) error
  }
}

void send_404(){
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET)?"GET":"POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i=0; i<server.args(); i++){
      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
    Serial.println(message);
}

