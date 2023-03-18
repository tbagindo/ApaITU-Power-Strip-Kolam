void otaSetup(){
    ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    DEBUG_SERIAL.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    DEBUG_SERIAL.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    DEBUG_SERIAL.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    DEBUG_SERIAL.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      DEBUG_SERIAL.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      DEBUG_SERIAL.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      DEBUG_SERIAL.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      DEBUG_SERIAL.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      DEBUG_SERIAL.println("End Failed");
    }
  });
  ArduinoOTA.begin();
}
