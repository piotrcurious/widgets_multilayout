/*
  ESP32 Dual Channel Oscilloscope (using widget framework)
  - Two analog inputs: GPIO36 (CH1), GPIO39 (CH2)
  - Trigger input: GPIO34 (rising edge)
  - Widgets: two rolling graphs (CH1, CH2), trigger status tile
*/

#include <Arduino.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// --- Display pins ---
#define TFT_CS   5
#define TFT_DC   2
#define TFT_RST  4
#define TFT_MOSI 23
#define TFT_MISO 19
#define TFT_CLK  18

Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

// --- Trigger input ---
#define TRIGGER_PIN 34

// === Framework boilerplate ===
struct ColorScheme {
  uint16_t screenBG, panelBG, border, text, accent, muted;
};
static ColorScheme COLORS;

struct WidgetState;
struct WidgetDef;
typedef void (*WidgetProcessFn)(WidgetState*, uint32_t);
typedef void (*WidgetDisplayFn)(WidgetState*, Adafruit_GFX&);

struct WidgetState {
  int16_t x,y,w,h;
  uint32_t processHz,lastProcessUs;
  uint32_t decorFlags;
  uint8_t textSize;
  bool visible;
  void* user;
  volatile bool dirty;
};

struct WidgetDef {
  const char* id;
  const char* title;
  WidgetProcessFn process;
  WidgetDisplayFn display;
  uint16_t userStateSizeBytes;
};

struct WidgetPlacement {
  const WidgetDef* def;
  int16_t x,y,w,h;
  uint32_t processHz;
  uint32_t decorFlags;
  uint8_t textSize;
  bool visible;
};

struct LayoutDef {
  const char* name;
  const WidgetPlacement* items;
  uint8_t count;
};

// Utility
template <typename T>
static T clampT(T v,T lo,T hi){return v<lo?lo:(v>hi?hi:v);}
static uint32_t hzToPeriodUs(uint32_t hz){ return hz?1000000UL/hz:0; }
enum : uint32_t { DECOR_BG=1, DECOR_BORDER=2, DECOR_TITLE=4 };

static const uint8_t MAX_WIDGETS=8;
static WidgetState g_states[MAX_WIDGETS];
static const WidgetDef* g_defs[MAX_WIDGETS];
static uint8_t g_count=0;
static const LayoutDef* g_layout=nullptr;
static uint32_t lastFrameUs=0;
static const uint32_t FRAME_HZ=20;
static bool g_forceFullRedraw=true;

// --- Colors ---
static void initColorScheme(){
  COLORS.screenBG=tft.color565(0,0,0);
  COLORS.panelBG =tft.color565(16,16,16);
  COLORS.border  =tft.color565(90,110,140);
  COLORS.text    =tft.color565(255,255,255);
  COLORS.accent  =tft.color565(0,200,0);
  COLORS.muted   =tft.color565(160,160,160);
}

// --- Decorations ---
static void drawDecor(const WidgetDef* def,WidgetState* ws,Adafruit_GFX& gfx){
  if(ws->decorFlags&DECOR_BG)
    gfx.fillRect(ws->x,ws->y,ws->w,ws->h,COLORS.panelBG);
  if(ws->decorFlags&DECOR_TITLE){
    gfx.setTextSize(1);
    gfx.setTextColor(COLORS.muted);
    gfx.setCursor(ws->x+4,ws->y+2);
    gfx.print(def->title?def->title:def->id);
  }
  if(ws->decorFlags&DECOR_BORDER)
    gfx.drawRect(ws->x,ws->y,ws->w,ws->h,COLORS.border);
}

// === Oscilloscope Data Structures ===
struct ScopeChannel {
  static const int N=240;   // width of screen buffer
  uint16_t buf[N];
  int head;
};
struct TriggerState { bool armed; bool triggered; };

// --- Widgets ---
static void scopeChProcess(WidgetState* ws,uint32_t nowUs){
  ScopeChannel* sc=(ScopeChannel*)ws->user;
  int adcPin=(ws->id[2]=='1')?36:39; // CH1=GPIO36, CH2=GPIO39
  int raw=analogRead(adcPin); // 0..4095
  sc->buf[sc->head]=raw;
  sc->head=(sc->head+1)%ScopeChannel::N;
}

static void scopeChDisplay(WidgetState* ws,Adafruit_GFX& gfx){
  ScopeChannel* sc=(ScopeChannel*)ws->user;
  drawDecor(g_defs[ws-g_states],ws,gfx);
  int left=ws->x+2,top=ws->y+2,width=ws->w-4,height=ws->h-4;
  gfx.fillRect(left,top,width,height,COLORS.panelBG);
  int idx=sc->head;
  for(int i=0;i<width;i++){
    idx=(idx-1+ScopeChannel::N)%ScopeChannel::N;
    int val=sc->buf[idx];
    int y=top+height-(val*height/4096);
    gfx.drawPixel(left+width-1-i,y,COLORS.accent);
  }
}

static void triggerProcess(WidgetState* ws,uint32_t nowUs){
  TriggerState* ts=(TriggerState*)ws->user;
  bool trig=digitalRead(TRIGGER_PIN);
  if(trig && ts->armed){ ts->triggered=true; ts->armed=false; }
  if(!trig) ts->armed=true;
}

static void triggerDisplay(WidgetState* ws,Adafruit_GFX& gfx){
  TriggerState* ts=(TriggerState*)ws->user;
  drawDecor(g_defs[ws-g_states],ws,gfx);
  gfx.setTextSize(ws->textSize);
  gfx.setTextColor(ts->triggered?COLORS.accent:COLORS.text);
  gfx.setCursor(ws->x+4,ws->y+ws->h/2-8);
  gfx.print(ts->triggered?"TRIGGERED":"armed...");
}

// --- WidgetDefs ---
static const WidgetDef W_CH1={"CH1","Channel 1",&scopeChProcess,&scopeChDisplay,sizeof(ScopeChannel)};
static const WidgetDef W_CH2={"CH2","Channel 2",&scopeChProcess,&scopeChDisplay,sizeof(ScopeChannel)};
static const WidgetDef W_TRIG={"TRIG","Trigger",&triggerProcess,&triggerDisplay,sizeof(TriggerState)};

// --- Layout ---
static const WidgetPlacement LAYOUT_SCOPE_ITEMS[]={
  {&W_CH1,10,10,300,100,2000,DECOR_BG|DECOR_BORDER|DECOR_TITLE,1,true},
  {&W_CH2,10,120,300,100,2000,DECOR_BG|DECOR_BORDER|DECOR_TITLE,1,true},
  {&W_TRIG,220,230,100,30,50,DECOR_BG|DECOR_BORDER,1,true}
};
static const LayoutDef LAYOUT_SCOPE={"Scope",LAYOUT_SCOPE_ITEMS,(uint8_t)(sizeof(LAYOUT_SCOPE_ITEMS)/sizeof(LAYOUT_SCOPE_ITEMS[0]))};

// --- Layout switching ---
static void switchToLayout(const LayoutDef* L){
  g_layout=L;
  tft.fillScreen(COLORS.screenBG);
  g_count=L->count;
  for(uint8_t i=0;i<g_count;i++){
    const WidgetPlacement& p=L->items[i];
    g_defs[i]=p.def;
    WidgetState& ws=g_states[i];
    ws.x=p.x;ws.y=p.y;ws.w=p.w;ws.h=p.h;
    ws.processHz=p.processHz;
    ws.lastProcessUs=0;
    ws.decorFlags=p.decorFlags;
    ws.textSize=p.textSize;
    ws.visible=p.visible;
    ws.user=malloc(p.def->userStateSizeBytes);
    memset(ws.user,0,p.def->userStateSizeBytes);
  }
  g_forceFullRedraw=true;
}

// --- Scheduler ---
static void processPass(uint32_t nowUs){
  for(uint8_t i=0;i<g_count;i++){
    WidgetState& ws=g_states[i];
    uint32_t per=hzToPeriodUs(ws.processHz);
    if(per && (nowUs-ws.lastProcessUs)>=per){
      ws.lastProcessUs=nowUs;
      g_defs[i]->process(&ws,nowUs);
    }
  }
}
static void displayPass(uint32_t nowUs){
  uint32_t framePer=hzToPeriodUs(FRAME_HZ);
  if((nowUs-lastFrameUs)<framePer && !g_forceFullRedraw) return;
  lastFrameUs=nowUs;
  for(uint8_t i=0;i<g_count;i++)
    g_defs[i]->display(&g_states[i],tft);
  g_forceFullRedraw=false;
}

// --- Setup/Loop ---
void setup(){
  Serial.begin(115200);
  pinMode(TRIGGER_PIN,INPUT);
  analogReadResolution(12);
  SPI.begin(TFT_CLK,TFT_MISO,TFT_MOSI);
  tft.begin();
  tft.setRotation(1);
  initColorScheme();
  switchToLayout(&LAYOUT_SCOPE);
}
void loop(){
  uint32_t now=micros();
  processPass(now);
  displayPass(now);
}
