#include <ArduinoBLE.h>
#include <U8g2lib.h>
#include <Wire.h>

// OLED Display (SH1106 128x64)
// If display stays black, try: U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

// BLE Service and Characteristic UUIDs
BLEService oledService("12345678-1234-1234-1234-1234567890ab");
BLECharacteristic oledData("87654321-4321-4321-4321-ba0987654321", BLEWrite, 512);
BLECharacteristic controlChar("87654321-4321-4321-4321-ba0987654322", BLEWrite, 1);
BLECharacteristic speedChar("87654321-4321-4321-4321-ba0987654323", BLEWrite, 2);

// Frame storage (RAM - cleared on restart)
#define MAX_FRAMES 60
#define FRAME_SIZE 1024
#define CHUNK_SIZE 512
uint8_t* frames[MAX_FRAMES];
int frameCount = 0;
bool animationRunning = false;
uint16_t frameDelay = 100; // Default: 100ms per frame

// Temporary buffer
uint8_t tempFrameBuffer[FRAME_SIZE];
int receivedBytes = 0;
bool receivingFrame = false;

// Connection stability
unsigned long lastBLEActivity = 0;
unsigned long connectionStartTime = 0;
bool wasConnected = false;
bool isReceiving = false; // True while actively receiving frames - blocks OLED I2C
const unsigned long BLE_TIMEOUT = 30000; // 30 seconds timeout

// Forward declarations
void clearFrames();
void onFrameReceived(BLEDevice central, BLECharacteristic characteristic);
void onControlReceived(BLEDevice central, BLECharacteristic characteristic);
void onSpeedReceived(BLEDevice central, BLECharacteristic characteristic);
void onBLEConnected(BLEDevice central);
void onBLEDisconnected(BLEDevice central);

void setup() {
  Serial.begin(115200);
  while(!Serial && millis() < 3000); // Wait max 3 seconds for Serial
  
  Serial.println("=== OLED Display Studio - Professional Edition ===");
  Serial.println("Hardware: Arduino Nano 33 BLE + SH1106 OLED");
  Serial.println("ℹ Frame storage: RAM only (cleared on restart)");
  
  // Initialize OLED Display
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB10_tr);
  u8g2.drawStr(5, 20, "OLED");
  u8g2.drawStr(5, 40, "Display");
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(5, 55, "Studio Pro");
  u8g2.sendBuffer();
  delay(1000);
  
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(5, 30, "Starting");
  u8g2.drawStr(5, 45, "Bluetooth...");
  u8g2.sendBuffer();

  // Initialize BLE
  if(!BLE.begin()) {
    Serial.println("✗ BLE initialization failed!");
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB10_tr);
    u8g2.drawStr(0, 30, "BLE Error!");
    u8g2.sendBuffer();
    while(1);
  }

  // Configure BLE connection parameters for stability
  BLE.setLocalName("Nano33_OLED_Pro");
  BLE.setDeviceName("OLED Display Pro");
  BLE.setAdvertisedService(oledService);
  
  // Add characteristics
  oledService.addCharacteristic(oledData);
  oledService.addCharacteristic(controlChar);
  oledService.addCharacteristic(speedChar);
  BLE.addService(oledService);
  
  // Register event handlers
  oledData.setEventHandler(BLEWritten, onFrameReceived);
  controlChar.setEventHandler(BLEWritten, onControlReceived);
  speedChar.setEventHandler(BLEWritten, onSpeedReceived);
  
  // Connection event handlers
  BLE.setEventHandler(BLEConnected, onBLEConnected);
  BLE.setEventHandler(BLEDisconnected, onBLEDisconnected);
  
  // Start BLE advertising
  BLE.advertise();
  
  Serial.println("✓ BLE ready! Name: Nano33_OLED_Pro");
  Serial.println("✓ Features:");
  Serial.println("  • Variable speed (50-1000ms)");
  Serial.println("  • Up to 60 frames");
  Serial.println("  • Auto-reconnect support");
  
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB10_tr);
  u8g2.drawStr(20, 25, "Ready!");
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(5, 45, "Waiting for");
  u8g2.drawStr(5, 58, "connection...");
  u8g2.sendBuffer();
  
  lastBLEActivity = millis();
}

void loop() {
  // Poll BLE events - REQUIRED for event handlers to fire
  BLE.poll();
  BLEDevice central = BLE.central();
  
  // Check connection status
  if(central) {
    if(!wasConnected) {
      wasConnected = true;
      connectionStartTime = millis();
    }
    lastBLEActivity = millis();
  } else if(wasConnected) {
    // Connection lost - prepare for reconnect
    wasConnected = false;
    Serial.println("⚠ Connection lost - ready for reconnect");
  }
  
  // Play animation if frames available and NOT receiving new frames
  if(isReceiving) {
    // During frame transfer: do nothing heavy, just let BLE.poll() run
    // No OLED I2C operations - they block for ~20ms and starve BLE
  } else if(frameCount > 0 && animationRunning) {
    for(int i = 0; i < frameCount; i++) {
      u8g2.clearBuffer();
      u8g2.drawXBMP(0, 0, 128, 64, frames[i]);
      u8g2.sendBuffer();
      
      unsigned long frameStart = millis();
      unsigned long frameEnd = frameStart + frameDelay;
      
      // Process BLE events during frame delay
      while(millis() < frameEnd) {
        BLE.poll();
        if(!animationRunning || isReceiving) break;
      }
      
      // If animation stopped or new data incoming, break
      if(!animationRunning || isReceiving) break;
    }
  } else if(frameCount > 0 && !animationRunning) {
    // Show first frame once, then just poll
    static bool firstFrameShown = false;
    if(!firstFrameShown) {
      u8g2.clearBuffer();
      u8g2.drawXBMP(0, 0, 128, 64, frames[0]);
      u8g2.sendBuffer();
      firstFrameShown = true;
    }
  } else {
    // Reset flag when no frames
    static bool firstFrameShown;
    firstFrameShown = false;
  }
  
  // Small delay to prevent tight loop (shorter during receive for faster BLE)
  delay(isReceiving ? 1 : 10);
}

// BLE Connected callback
void onBLEConnected(BLEDevice central) {
  Serial.print("✓ Connected: ");
  Serial.println(central.address());
  
  if(frameCount == 0) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB10_tr);
    u8g2.drawStr(5, 30, "Connected!");
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(5, 50, "Ready to");
    u8g2.drawStr(5, 62, "receive...");
    u8g2.sendBuffer();
  }
}

// BLE Disconnected callback
void onBLEDisconnected(BLEDevice central) {
  Serial.print("✗ Disconnected: ");
  Serial.println(central.address());
  
  if(frameCount == 0) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB10_tr);
    u8g2.drawStr(5, 30, "Waiting...");
    u8g2.sendBuffer();
  }
}

// Frame chunk received
void onFrameReceived(BLEDevice central, BLECharacteristic characteristic) {
  int len = characteristic.valueLength();
  const uint8_t* data = characteristic.value();
  
  lastBLEActivity = millis();  isReceiving = true; // Block OLED operations in main loop
  Serial.print("▼ Chunk: ");
  Serial.print(len);
  Serial.print(" bytes (");
  Serial.print(receivedBytes);
  Serial.print("/");
  Serial.print(FRAME_SIZE);
  Serial.println(")");

  // Check if max frames reached
  if(frameCount >= MAX_FRAMES && receivedBytes == 0) {
    Serial.println("⚠ Max frames reached!");
    return;
  }

  // Validate chunk size
  if(len != CHUNK_SIZE && len != (FRAME_SIZE - receivedBytes)) {
    Serial.print("⚠ Unexpected chunk size: ");
    Serial.print(len);
    Serial.println(" bytes");
  }

  // Copy chunk to temporary buffer
  if(receivedBytes + len <= FRAME_SIZE) {
    memcpy(tempFrameBuffer + receivedBytes, data, len);
    receivedBytes += len;
    receivingFrame = true;

    Serial.print("  Progress: ");
    Serial.print((receivedBytes * 100) / FRAME_SIZE);
    Serial.println("%");
  }

  // Complete frame received (1024 bytes)
  if(receivedBytes >= FRAME_SIZE) {
    // Allocate memory for new frame
    frames[frameCount] = new uint8_t[FRAME_SIZE];
    if(frames[frameCount] == nullptr) {
      Serial.println("✗ Memory allocation failed!");
      receivedBytes = 0;
      receivingFrame = false;
      return;
    }

    // Copy frame data
    memcpy(frames[frameCount], tempFrameBuffer, FRAME_SIZE);
    frameCount++;
    // Don't auto-start animation - wait for explicit start command (255)

    Serial.print("✓ Frame ");
    Serial.print(frameCount);
    Serial.print("/");
    Serial.print(MAX_FRAMES);
    Serial.println(" saved");
    
    // Reset for next frame
    receivedBytes = 0;
    receivingFrame = false;
    
    // NO OLED update here - I2C takes ~20ms and blocks BLE processing
    // The display will update when animation starts (cmd 255)
  }
}

// Control commands received
// Protocol: 1-60 = frame count (clear & prepare), 254 = clear display, 255 = start animation
void onControlReceived(BLEDevice central, BLECharacteristic characteristic) {
  uint8_t cmd = characteristic.value()[0];
  
  lastBLEActivity = millis();
  
  Serial.print("⚙ Control: ");
  Serial.println(cmd);
  
  if(cmd >= 1 && cmd <= MAX_FRAMES) {
    // Frame count received - clear old frames and prepare to receive new ones
    clearFrames();
    isReceiving = true; // Block OLED in main loop during entire transfer
    Serial.print(">> Preparing to receive ");
    Serial.print(cmd);
    Serial.println(" frame(s)");
    
    // One quick OLED update, then no more during transfer
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(5, 30, "Receiving");
    u8g2.drawStr(5, 45, "frames...");
    u8g2.sendBuffer();
  }
  else if(cmd == 254) {
    // Clear display
    clearFrames();
    Serial.println("🗑 Display cleared");
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB10_tr);
    u8g2.drawStr(15, 35, "Cleared!");
    u8g2.sendBuffer();
    delay(500);
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(5, 35, "Ready...");
    u8g2.sendBuffer();
  }
  else if(cmd == 255) {
    // Start animation
    isReceiving = false; // Transfer complete, allow OLED operations
    if(frameCount > 0) {
      animationRunning = true;
      Serial.print("▶ Animation started (");
      Serial.print(frameCount);
      Serial.println(" frames)");
    } else {
      Serial.println("⚠ No frames to animate");
    }
  }
  else if(cmd == 0) {
    // Stop animation
    animationRunning = false;
    Serial.println("⏸ Animation stopped");
  }
  else {
    Serial.print("? Unknown command: ");
    Serial.println(cmd);
  }
}

// Speed change received
void onSpeedReceived(BLEDevice central, BLECharacteristic characteristic) {
  if(characteristic.valueLength() >= 2) {
    const uint8_t* data = characteristic.value();
    uint16_t newSpeed = data[0] | (data[1] << 8);
    
    lastBLEActivity = millis();
    
    // Limit speed to reasonable range (10ms to 60 seconds)
    if(newSpeed >= 10 && newSpeed <= 60000) {
      frameDelay = newSpeed;
      
      Serial.print("Speed changed: ");
      Serial.print(frameDelay);
      Serial.println("ms per frame");
      
      // Show confirmation on display
      if(frameCount == 0) {
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_ncenB08_tr);
        u8g2.drawStr(5, 25, "Speed:");
        
        char speedStr[24];
        if(frameDelay < 1000) {
          sprintf(speedStr, "%dms/frame", frameDelay);
        } else {
          sprintf(speedStr, "%d.%ds/frame", frameDelay / 1000, (frameDelay % 1000) / 100);
        }
        u8g2.drawStr(15, 45, speedStr);
        u8g2.sendBuffer();
        delay(1500);
        
        u8g2.clearBuffer();
        u8g2.drawStr(5, 35, "Ready...");
        u8g2.sendBuffer();
      }
    } else {
      Serial.print("⚠ Invalid speed: ");
      Serial.println(newSpeed);
    }
  }
}

// Clear all frames and free memory
void clearFrames() {
  for(int i = 0; i < frameCount; i++) {
    if(frames[i] != nullptr) {
      delete[] frames[i];
      frames[i] = nullptr;
    }
  }
  frameCount = 0;
  animationRunning = false;
  isReceiving = false;
  receivedBytes = 0;
  receivingFrame = false;
}
