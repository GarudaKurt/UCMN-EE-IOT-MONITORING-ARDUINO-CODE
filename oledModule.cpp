#include "oledModule.h"
#include <Wire.h>
#include <math.h>
#include <string.h>

Display::Display() : oled(U8G2_R0, U8X8_PIN_NONE, OLED_SCL_PIN, OLED_SDA_PIN) {}

void Display::initOled() {
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  Wire.setTimeOut(25);
  oled.setI2CAddress(OLED_I2C_ADDRESS << 1);
  oled.begin();
  oled.setContrast(180);
  oled.clearBuffer();
  oled.sendBuffer();
  // The selected controller is 128x64; guard the fixed shadow-buffer capacity.
  if (oled.getDisplayWidth() != 128 || oled.getDisplayHeight() != 64) {
    Serial.println("OLED dimensions unsupported");
    return;
  }
  if (xTaskCreate(taskEntry, "oled", 4096, this, 1, &task) != pdPASS)
    Serial.println("OLED task allocation failed");
}

void Display::screenDisplay(const SensorReadings& r, bool wifi, bool mqtt, bool ssrOn) {
  portENTER_CRITICAL(&lock);
  snapshot = r;
  wifiState = wifi;
  mqttState = mqtt;
  ssrState = ssrOn;
  if (powerStatusActive && powerStatusOn != ssrOn) powerStatusActive = false;
  snapshotAt = millis();
  portEXIT_CRITICAL(&lock);
}

void Display::showPowerStatus(bool ssrOn) {
  portENTER_CRITICAL(&lock);
  powerStatusOn = ssrOn;
  powerStatusAt = millis();
  powerStatusActive = true;
  portEXIT_CRITICAL(&lock);
}

void Display::taskEntry(void* context) {
  auto& self = *static_cast<Display*>(context);
  for (;;) {
    SensorReadings r;
    bool wifi, mqtt, ssrOn, statusActive, statusOn;
    uint32_t updated, statusAt;
    portENTER_CRITICAL(&self.lock);
    r = self.snapshot;
    wifi = self.wifiState;
    mqtt = self.mqttState;
    ssrOn = self.ssrState;
    statusActive = self.powerStatusActive;
    statusOn = self.powerStatusOn;
    statusAt = self.powerStatusAt;
    updated = self.snapshotAt;
    portEXIT_CRITICAL(&self.lock);
    const uint32_t now = millis();
    if (now - updated > 3500) { r.error = ErrorType::disconnected; wifi = mqtt = false; }
    self.render(r, wifi, mqtt, ssrOn,
                statusActive && statusOn == ssrOn && now - statusAt < 3000UL, now);
    // Yield only this task; sensor/MQTT work is never delayed by rendering.
    vTaskDelay(pdMS_TO_TICKS(80));
  }
}

void Display::cell(int x,int y,int width,float value,const char* unit,int decimals,bool ssrOn) {
  oled.setClipWindow(x+2,y+1,x+width-1,y+15);
  char text[64];
  oled.setFont(u8g2_font_5x8_tf);
  bool fits = false;
  for (int precision=decimals; precision>=0; --precision) {
    if (!ssrOn) snprintf(text,sizeof(text),"NaN %s",unit);
    else if (!isfinite(value)) snprintf(text,sizeof(text),"-- %s",unit);
    else snprintf(text,sizeof(text),"%.*f %s",precision,value,unit);
    if (oled.getStrWidth(text) <= width-4) { fits=true; break; }
  }
  if (!fits) {
    snprintf(text,sizeof(text),"%.0e %s",value,unit);
    if (oled.getStrWidth(text)>width-4) snprintf(text,sizeof(text),"OVF %s",unit);
  }
  oled.drawStr(x+width-2-oled.getStrWidth(text),y+11,text);
  oled.setMaxClipWindow();
}

void Display::render(const SensorReadings& r,bool wifi,bool mqtt,bool ssrOn,
                     bool showStatus,uint32_t now) {
  const int w=oled.getDisplayWidth(), h=oled.getDisplayHeight();
  oled.clearBuffer();
  if (showStatus) {
    oled.setFont(u8g2_font_10x20_tf);
    const char* message = ssrOn ? "POWER ON" : "POWER OFF";
    oled.drawStr((w-oled.getStrWidth(message))/2, 39, message);
  } else {
    oled.setFont(u8g2_font_5x8_tf);
    const char* title="RENTAL HOUSE LIVE MONITORING";
    const int period=oled.getStrWidth(title)+12;
    const int offset=(now/80)%period;
    oled.setClipWindow(0,0,w-16,11);
    for (int x=-offset; x<w-16; x+=period) {
      oled.drawStr(x,9,title);
    }
    oled.setMaxClipWindow();
    oled.drawFrame(w-14,2,5,6); if(wifi) oled.drawBox(w-13,3,3,4);
    oled.drawFrame(w-6,2,5,6); if(mqtt) oled.drawBox(w-5,3,3,4);
    oled.drawHLine(0,13,w);
    const float values[]={r.voltage,r.power,r.frequency,r.current,r.energy,r.powerFactor};
    const char* units[]={"V","W","Hz","A","kWh","PF"};
    const int decimals[]={1,1,1,3,3,2};
    const int rowHeight=(h-16)/3;
    for(int i=0;i<6;++i) cell((i%2)*(w/2),16+(i/2)*rowHeight,w/2,
        r.valid()?values[i]:NAN,units[i],decimals[i],ssrOn);
  }
  // Compare 8x8 tiles and transmit only changed contiguous runs.
  uint8_t* buffer=oled.getBufferPtr();
  for(int row=0;row<h/8;++row) {
    int start=-1;
    for(int col=0;col<=w/8;++col) {
      bool changed=col<w/8 && memcmp(buffer+row*w+col*8,previous+row*w+col*8,8)!=0;
      if(changed && start<0) start=col;
      if(!changed && start>=0) {
        oled.updateDisplayArea(start,row,col-start,1);
        memcpy(previous+row*w+start*8,buffer+row*w+start*8,(col-start)*8);
        start=-1;
      }
    }
  }
}
