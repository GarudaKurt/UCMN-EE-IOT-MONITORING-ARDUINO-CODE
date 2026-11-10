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

void Display::screenDisplay(const SensorReadings& r, bool wifi, bool mqtt) {
  portENTER_CRITICAL(&lock);
  snapshot = r;
  wifiState = wifi;
  mqttState = mqtt;
  snapshotAt = millis();
  portEXIT_CRITICAL(&lock);
}

void Display::taskEntry(void* context) {
  auto& self = *static_cast<Display*>(context);
  for (;;) {
    SensorReadings r;
    bool wifi, mqtt;
    uint32_t updated;
    portENTER_CRITICAL(&self.lock);
    r = self.snapshot;
    wifi = self.wifiState;
    mqtt = self.mqttState;
    updated = self.snapshotAt;
    portEXIT_CRITICAL(&self.lock);
    const uint32_t now = millis();
    if (now - updated > 3500) { r.error = ErrorType::disconnected; wifi = mqtt = false; }
    self.render(r, wifi, mqtt, now);
    // Yield only this task; sensor/MQTT work is never delayed by rendering.
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void Display::icon(int x, int y, int kind, bool phase) {
  switch (kind) {
    case 0: // Lightning
      oled.drawLine(x+6,y,x+2,y+4); oled.drawHLine(x+2,y+4,5);
      oled.drawLine(x+6,y+4,x+2,y+8); if (phase) oled.drawPixel(x,y+1); break;
    case 1: // Plug
      oled.drawFrame(x+1,y+2,6,5); oled.drawVLine(x+2,y,2);
      oled.drawVLine(x+5,y,2); oled.drawVLine(x+4,y+7,2);
      if (phase) oled.drawHLine(x+2,y+4,4); break;
    case 2: // Sine wave
      for (int i=0;i<8;++i) oled.drawPixel(x+i,y+4+((i+(phase?2:0))%8<4 ? -2:2)); break;
    case 3: // Current / battery
      oled.drawFrame(x,y+2,8,6); oled.drawHLine(x+2,y,4);
      oled.drawBox(x+2,y+4,phase?4:2,2); break;
    case 4: // Energy meter
      oled.drawFrame(x,y,8,9); oled.drawHLine(x+2,y+2,4);
      oled.drawLine(x+4,y+6,x+(phase?6:2),y+4); break;
    default: // Phase / PF
      oled.drawCircle(x+4,y+4,3); oled.drawLine(x+4,y+4,x+(phase?6:4),y+1); break;
  }
}

void Display::cell(int x,int y,int width,float value,const char* unit,int decimals,int kind,bool phase) {
  icon(x+1,y+3,kind,phase);
  char text[64];
  const uint8_t* fonts[] = {u8g2_font_6x10_tf,u8g2_font_5x8_tf};
  bool fits = false;
  for (int precision=decimals; precision>=0; --precision) {
    for (auto font : fonts) {
      oled.setFont(font);
      if (!isfinite(value)) snprintf(text,sizeof(text),"-- %s",unit);
      else snprintf(text,sizeof(text),"%.*f %s",precision,value,unit);
      if (oled.getStrWidth(text) <= width-12) { fits=true; break; }
    }
    if (fits) break;
  }
  if (!fits) {
    snprintf(text,sizeof(text),"%.0e %s",value,unit);
    if (oled.getStrWidth(text)>width-12) snprintf(text,sizeof(text),"OVF %s",unit);
  }
  oled.drawStr(x+width-1-oled.getStrWidth(text),y+12,text);
}

void Display::render(const SensorReadings& r,bool wifi,bool mqtt,uint32_t now) {
  const int w=oled.getDisplayWidth(), h=oled.getDisplayHeight();
  oled.clearBuffer();
  oled.setFont(u8g2_font_5x8_tf);
  const char* title="RENTAL HOUSE LIVE MONITORING";
  const int period=oled.getStrWidth(title)+22;
  const int offset=(now/60)%period;
  oled.setClipWindow(0,0,w-16,11);
  for (int x=-offset; x<w-16; x+=period) {
    icon(x,1,0,(now/500)%2);
    oled.drawStr(x+12,9,title);
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
      r.valid()?values[i]:NAN,units[i],decimals[i],i,(now/500)%2);
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
