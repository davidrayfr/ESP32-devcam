// Choose AI Thinker CAM Board :
// cam.init(esp32cam_aithinker_config);
// Port de diffusion RTSP / port 8554 ? ou Port 554 ?
// ESP-32 CAM
// Reglage sur Box : 
// 192.168.1.153  fc:f5:c4:0c:15:44 Wifi  Camera    ESP32-CAM
#include <Arduino.h>
#include "main.h"

/** Put your WiFi credentials into this file **/
#include "wifikeys.h"

/** Camera class */
OV2640 cam;

#ifdef ENABLE_RTSPSERVER

// Use this URL to connect the RTSP stream, replace the IP address with the address of your device
// rtsp://192.168.0.109:8554/mjpeg/1

/** Forward dedclaration of the task handling RTSP */
void rtspTask(void *pvParameters);

/** Task handle of the RTSP task */
TaskHandle_t rtspTaskHandler;

/** WiFi server for RTSP */
WiFiServer rtspServer(8554);

/** Stream for the camera video */
CStreamer *streamer = NULL;
/** Session to handle the RTSP communication */
CRtspSession *session = NULL;
/** Client to handle the RTSP connection */
WiFiClient rtspClient;
/** Flag from main loop to stop the RTSP server */
boolean stopRTSPtask = false;

/**
 * Starts the task that handles RTSP streaming
 */
void initRTSP(void)
{
  // Create the task for the RTSP server
  xTaskCreate(rtspTask, "RTSP", 4096, NULL, 1, &rtspTaskHandler);

  // Check the results
  if (rtspTaskHandler == NULL)
  {
    Serial.println("Create RTSP task failed");
  }
  else
  {
    Serial.println("RTSP task up and running");
  }
}

/**
 * Called to stop the RTSP task, needed for OTA
 * to avoid OTA timeout error
 */
void stopRTSP(void)
{
  stopRTSPtask = true;
}

/**
 * The task that handles RTSP connections
 * Starts the RTSP server
 * Handles requests in an endless loop
 * until a stop request is received because OTA
 * starts
 */
void rtspTask(void *pvParameters)
{
  uint32_t msecPerFrame = 50;
  static uint32_t lastimage = millis();

  // rtspServer.setNoDelay(true);
  rtspServer.setTimeout(1);
  rtspServer.begin();

  while (1)
  {
    // If we have an active client connection, just service that until gone
    if (session)
    {
      session->handleRequests(0); // we don't use a timeout here,
      // instead we send only if we have new enough frames

      uint32_t now = millis();
      if (now > lastimage + msecPerFrame || now < lastimage)
      { // handle clock rollover
        session->broadcastCurrentFrame(now);
        lastimage = now;
      }

      // Handle disconnection from RTSP client
      if (session->m_stopped)
      {
        Serial.println("RTSP client closed connection");
        delete session;
        delete streamer;
        session = NULL;
        streamer = NULL;
      }
    }
    else
    {
      rtspClient = rtspServer.accept();
      // Handle connection request from RTSP client
      if (rtspClient)
      {
        Serial.println("RTSP client started connection");
        streamer = new OV2640Streamer(&rtspClient, cam); // our streamer for UDP/TCP based RTP transport

        session = new CRtspSession(&rtspClient, streamer); // our threads RTSP session and state
        delay(100);
      }
    }
    if (stopRTSPtask)
    {
      // User requested RTSP server stop
      if (rtspClient)
      {
        Serial.println("Shut down RTSP server because OTA starts");
        delete session;
        delete streamer;
        session = NULL;
        streamer = NULL;
      }
      // Delete this task
      vTaskDelete(NULL);
    }
    delay(10);
  }
}
#endif
/** 
 * Called once after reboot/powerup
 */
void setup()
{
  // Start the serial connection
  Serial.begin(115200);

  Serial.println("\n\n##################################");
  Serial.printf("Internal Total heap %d, internal Free Heap %d\n", ESP.getHeapSize(), ESP.getFreeHeap());
  Serial.printf("SPIRam Total heap %d, SPIRam Free Heap %d\n", ESP.getPsramSize(), ESP.getFreePsram());
  Serial.printf("ChipRevision %d, Cpu Freq %d, SDK Version %s\n", ESP.getChipRevision(), ESP.getCpuFreqMHz(), ESP.getSdkVersion());
  Serial.printf("Flash Size %d, Flash Speed %d\n", ESP.getFlashChipSize(), ESP.getFlashChipSpeed());
  Serial.println("##################################\n\n");

  // Initialize the ESP32 CAM, here we use the AIthinker ESP32 CAM
  delay(100);
  cam.init(esp32cam_aithinker_config);
  delay(100);

  // Connect the WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  // Print information how to contact the camera server
  IPAddress ip = WiFi.localIP();
  Serial.print("\nWiFi connected with IP ");
  Serial.println(ip);
#ifdef ENABLE_RTSPSERVER
  Serial.print("Stream Link: rtsp://");
  Serial.print(ip);
  Serial.println(":8554/mjpeg/1\n");
#endif
#ifdef ENABLE_WEBSERVER
  Serial.print("Browser Stream Link: http://");
  Serial.print(ip);
  Serial.println("\n");
  Serial.print("Browser Single Picture Link: http//");
  Serial.print(ip);
  Serial.println("/jpg\n");
#endif
#ifdef ENABLE_WEBSERVER
  // Initialize the HTTP web stream server
  initWebStream();
#endif

#ifdef ENABLE_RTSPSERVER
  // Initialize the RTSP stream server
  initRTSP();
#endif

}

void loop()
{

  //Nothing else to do here
  delay(100);
}