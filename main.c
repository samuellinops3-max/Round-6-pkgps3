#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <math.h>
#include <ppu-types.h>
#include <sysutil/sysutil.h>
#include <sysutil/video.h>
#include <rsx/rsx.h>
#include <io/pad.h>
#define CB_SIZE (0x100000)
#define HOST_SIZE (32*1024*1024)
static gcmContextData *ctx; static u32 W,H,*fb[2],fboff[2],*back; static int cur=0,first_flip=1; static volatile int running=1;
static void sysutil_cb(u64 s,u64 p,void*u){(void)p;(void)u; if(s==SYSUTIL_EXIT_GAME) running=0;}
static void init_screen(void){ void *host=memalign(1024*1024,HOST_SIZE); rsxInit(&ctx,CB_SIZE,HOST_SIZE,host); videoState st; videoGetState(VIDEO_PRIMARY,0,&st); u32 resid=st.displayMode.resolution; if(videoGetResolutionAvailability(VIDEO_PRIMARY,VIDEO_RESOLUTION_720,VIDEO_ASPECT_AUTO,0)) resid=VIDEO_RESOLUTION_720; videoResolution res; videoGetResolution(resid,&res); W=res.width; H=res.height; videoConfiguration vc; memset(&vc,0,sizeof(vc)); vc.resolution=resid; vc.format=VIDEO_BUFFER_FORMAT_XRGB; vc.aspect=VIDEO_ASPECT_AUTO; vc.pitch=W*4; videoConfigure(VIDEO_PRIMARY,&vc,NULL,0); videoGetState(VIDEO_PRIMARY,0,&st); gcmSetFlipMode(GCM_FLIP_VSYNC); for(int i=0;i<2;i++){fb[i]=(u32*)rsxMemalign(64,W*H*4); gcmAddressToOffset(fb[i],&fboff[i]); gcmSetDisplayBuffer(i,fboff[i],W*4,W,H);} back=(u32*)memalign(64,W*H*4); memset(back,0,W*H*4); gcmResetFlipStatus(); }
static void wait_flip(void){ while(gcmGetFlipStatus()!=0) usleep(200); gcmResetFlipStatus(); }
static void present(void){ if(!first_flip) wait_flip(); else gcmResetFlipStatus(); memcpy(fb[cur],back,W*H*4); gcmSetFlip(ctx,cur); rsxFlushBuffer(ctx); gcmSetWaitFlip(ctx); cur^=1; first_flip=0; }
static void fill_rect(int x,int y,int w,int h,u32 c){ int x0=x<0?0:x,y0=y<0?0:y,x1=x+w,y1=y+h; if(x1>(int)W)x1=W; if(y1>(int)H)y1=H; for(int yy=y0;yy<y1;yy++){u32*p=back+yy*W; for(int xx=x0;xx<x1;xx++)p[xx]=c;} }
static void fill_circle(int cx,int cy,int r,u32 c){ for(int y=-r;y<=r;y++) for(int x=-r;x<=r;x++) if(x*x+y*y<=r*r) fill_rect(cx+x,cy+y,1,1,c); }
typedef struct { char c; const char *p; } Glyph;
static const Glyph glyphs[]={ {'0',"###" "#.#" "#.#" "#.#" "###"},{'1',".#." "##." ".#." ".#." "###"},{'2',"###" "..#" "###" "#.." "###"},{'3',"###" "..#" "###" "..#" "###"},{'4',"#.#" "#.#" "###" "..#" "..#"},{'5',"###" "#.." "###" "..#" "###"},{'6',"###" "#.." "###" "#.#" "###"},{'7',"###" "..#" "..#" "..#" "..#"},{'8',"###" "#.#" "###" "#.#" "###"},{'9',"###" "#.#" "###" "..#" "###"},{'A',".#." "#.#" "###" "#.#" "#.#"},{'B',"##." "#.#" "##." "#.#" "##."},{'C',"###" "#.." "#.." "#.." "###"},{'D',"##." "#.#" "#.#" "#.#" "##."},{'E',"###" "#.." "###" "#.." "###"},{'F',"###" "#.." "###" "#.." "#.."},{'G',"###" "#.." "#.#" "#.#" "###"},{'H',"#.#" "#.#" "###" "#.#" "#.#"},{'I',"###" ".#." ".#." ".#." "###"},{'L',"#.." "#.." "#.." "#.." "###"},{'M',"#.#" "###" "###" "#.#" "#.#"},{'N',"#.#" "###" "###" "#.#" "#.#"},{'O',"###" "#.#" "#.#" "#.#" "###"},{'P',"###" "#.#" "###" "#.." "#.."},{'R',"##." "#.#" "##." "#.#" "#.#"},{'S',"###" "#.." "###" "..#" "###"},{'T',"###" ".#." ".#." ".#." ".#."},{'U',"#.#" "#.#" "#.#" "#.#" "###"},{'V',"#.#" "#.#" "#.#" "#.#" ".#."},{'X',"#.#" "#.#" ".#." "#.#" "#.#"},{'Y',"#.#" "#.#" ".#." ".#." ".#."},{'!',"###" ".#." ".#." "..." ".#."},{':',"..." ".#." "..." ".#." "..."},{'.',"..." "..." "..." "..." ".#."},{'-',"..." "..." "###" "..." "..."},{' ',"..." "..." "..." "..." "..."} };
static const char *find_glyph(char c){ for(unsigned i=0;i<sizeof(glyphs)/sizeof(glyphs[0]);i++) if(glyphs[i].c==c) return glyphs[i].p; return NULL; }
static void draw_text(const char *s,int x,int y,int sc,u32 c){ for(;*s;s++){ const char *g=find_glyph(*s); if(g) for(int r=0;r<5;r++) for(int k=0;k<3;k++) if(g[r*3+k]=='#') fill_rect(x+k*sc,y+r*sc,sc,sc,c); x+=4*sc; } }
static void draw_text_center(const char *s,int y,int sc,u32 c){ int w=strlen(s)*4*sc; draw_text(s,(W-w)/2,y,sc,c); }
static void audio_init(void){ }
static void beep(float f,int m,int v){ (void)f;(void)m;(void)v; }
static void play_batatinha(void){ }
static void play_shot(void){ }
static void play_win(void){ }
static void draw_player(int x,int y,int color,int run,int dir){ int skin=0x00FFD0A0; int suit=color; int legOff = run ? (rand()%6-3) : 0; fill_circle(x+15,y+8,14,skin); fill_rect(x+8,y+2,14,4,0x00000000); fill_circle(x+11,y+8,2,0x00000000); fill_circle(x+19,y+8,2,0x00000000); fill_rect(x+5,y+20,20,28,suit); fill_rect(x+10,y+22,10,6,0x00FFFFFF); fill_rect(x+2,y+22,6,16,suit); fill_rect(x+22,y+22,6,16,suit); fill_rect(x+6,y+48,7,14,0x00202020); fill_rect(x+17,y+48,7,14,0x00202020); if(run){ fill_rect(x+6+legOff,y+62,7,4,0x00FFFFFF); fill_rect(x+17-legOff,y+62,7,4,0x00FFFFFF); } else { fill_rect(x+6,y+62,7,4,0x00FFFFFF); fill_rect(x+17,y+62,7,4,0x00FFFFFF); } if(dir<0) fill_rect(x+2,y+8,4,4,skin); }
static void draw_guard(int x,int y){ fill_circle(x+15,y+8,14,0x00000000); fill_rect(x+2,y+0,26,22,0x00FF1493); fill_circle(x+10,y+8,4,0x00000000); fill_circle(x+20,y+8,4,0x00000000); fill_rect(x+5,y+22,20,30,0x00FF1493); fill_rect(x+5,y+22,20,6,0x00000000); fill_rect(x+6,y+52,7,12,0x00000000); fill_rect(x+17,y+52,7,12,0x00000000); fill_rect(x+22,y+28,4,20,0x00202020); }
static void draw_doll(int x,int y,int facing){ int skin=0x00FFD0A0; fill_circle(x+40,y+20,38,skin); fill_rect(x+5,y+0,70,18,0x00000000); fill_circle(x+28,y+18,6,0x00000000); fill_circle(x+52,y+18,6,0x00000000); if(facing==0){ fill_rect(x+5,y+0,70,40,0x00000000); fill_rect(x+30,y+45,20,6,0x00FF0000); } else { fill_circle(x+28,y+18,8,0x00FF0000); fill_circle(x+52,y+18,8,0x00FF0000); fill_rect(x+30,y+32,20,6,0x00FF0000); } fill_rect(x+10,y+60,60,70,0x00FFA500); fill_rect(x+10,y+60,60,10,0x00FFFFFF); fill_rect(x+15,y+130,12,20,skin); fill_rect(x+55,y+130,12,20,skin); fill_circle(x+40,y+65,10,0x00FFFFFF); }
int main(int argc,char**argv){ (void)argc;(void)argv; init_screen(); ioPadInit(7); sysUtilRegisterCallback(0,sysutil_cb,NULL); audio_init();
float px=640, py=620; int state=0; int light=0; int lightTimer=0, lightPhase=0, frame=0; int dollFacing=0;
int npcX[6]={500,600,700,750,550,680}; int npcY[6]={650,680,620,660,600,640}; int npcAlive[6]={1,1,1,1,1,1};
int shake=0, songPlayed=0; int timeLeft=60*60;
while(running){ sysUtilCheckCallback(); frame++;
padInfo info; padData d; memset(&d,0,sizeof(d)); ioPadGetInfo(&info); if(info.status[0]) ioPadGetData(0,&d);
int ax = d.ANA_L_H - 128; int ay = d.ANA_L_V - 128;
int run = d.BTN_R2; int start = d.BTN_START; int cross = d.BTN_CROSS;
if(state==0 && start){ state=1; px=640; py=620; light=0; lightTimer=0; songPlayed=0; for(int i=0;i<6;i++){ npcAlive[i]=1; npcY[i]=600+rand()%80; } timeLeft=60*60; play_batatinha(); }
if(state==2 && cross){ state=0; }
if(state==3 && cross){ state=0; }
if(state==1){
  timeLeft--; if(timeLeft<=0){ state=2; }
  lightTimer++;
  if(lightPhase==0){ light=0; dollFacing=0; if(lightTimer>220){ lightPhase=1; lightTimer=0; } }
  else if(lightPhase==1){ if(lightTimer>40){ lightPhase=2; lightTimer=0; light=1; dollFacing=1; beep(880,200,70); } }
  else { light=1; dollFacing=1; if(lightTimer>170){ lightPhase=0; lightTimer=0; songPlayed=0; play_batatinha(); } }
  float speed = run ? 4.2f : 2.2f;
  float mx=0, my=0;
  if(ax>20) mx=1; if(ax<-20) mx=-1; if(ay>20) my=1; if(ay<-20) my=-1;
  if(my<0) py+=my*speed;
  if(mx!=0) px+=mx*speed*0.7f;
  if(px<100) px=100; if(px>1180) px=1180; if(py<80) py=80; if(py>640) py=640;
  if(light==1 && (mx!=0 || my!=0)){ state=2; shake=20; play_shot(); }
  for(int i=0;i<6;i++) if(npcAlive[i]){ if(light==0){ npcY[i]-= (rand()%3); if(npcY[i]<90) npcY[i]=90; } else { if(rand()%100<8){ npcAlive[i]=0; } } }
  if(py<=100){ state=3; play_win(); }
}
if(shake>0) shake--;
int shx=0, shy=0; if(shake>0){ shx=rand()%8-4; shy=rand()%8-4; }
for(int y=0;y<(int)H;y++){ u32 c = (y<550) ? 0x00FFC0CB : 0x0030A030; fill_rect(0,y+shy,W,1,c); }
fill_rect(0,120+shy,W,4,0x00FFFFFF);
fill_rect(0,640+shy,W,4,0x00FFFFFF);
fill_rect(80+shx,180+shy,20,120,0x00804000); fill_circle(90+shx,160+shy,50,0x00008000);
fill_rect(1150+shx,180+shy,20,120,0x00804000); fill_circle(1160+shx,160+shy,50,0x00008000);
draw_guard(200+shx,250+shy); draw_guard(1020+shx,250+shy); draw_guard(150+shx,500+shy);
draw_doll(600+shx,60+shy,dollFacing);
for(int i=0;i<6;i++) if(npcAlive[i]) draw_player(npcX[i]+shx,npcY[i]+shy,0x00108040, light==0, 1);
else { fill_rect(npcX[i]+shx,npcY[i]+shy+50,30,10,0x00FF0000); }
draw_player((int)px+shx,(int)py+shy,0x00108040, run, 1);
fill_rect((int)px+shx,(int)py+shy-18,30,12,0x00FFFF00); draw_text("VOCE",(int)px+shx,(int)py+shy-18,2,0x00000000);
if(state==1){
  if(light==0){ fill_circle(640+shx,40+shy,22,0x0000FF00); draw_text_center("LUZ VERDE - CORRE!",90,6,0x0000FF00); }
  else { fill_circle(640+shx,40+shy,22,0x00FF0000); draw_text_center("LUZ VERMELHA - PARA!",90,6,0x00FF0000); }
  char buf[64]; snprintf(buf,sizeof(buf),"TEMPO %d",timeLeft/60); draw_text(buf,20,20,5,0x00FFFFFF);
  draw_text("L3 ANDAR  R2 CORRER",20,680,4,0x00FFFFFF);
}
if(state==0){ fill_rect(200,180,880,360,0x00000000); fill_rect(210,190,860,340,0x00FF1493); draw_text_center("ROUND 6",230,12,0x00FFFFFF); draw_text_center("BATATINHA FRITA 1 2 3",320,8,0x00FFFF00); draw_text_center("L3 = ANDAR  R2 = CORRER",400,6,0x00FFFFFF); draw_text_center("VERDE CORRE  VERMELHO PARA",440,6,0x00FF0000); draw_text_center("APERTE START",500,7,0x00000000); }
if(state==2){ fill_rect(250,200,780,300,0x00000000); draw_text_center("ELIMINADO!",250,12,0x00FF0000); draw_text_center("VOCE SE MEXEU NO VERMELHO!",350,6,0x00FFFFFF); draw_text_center("APERTE X P/ TENTAR DE NOVO",420,6,0x00FFFF00); }
if(state==3){ fill_rect(250,200,780,300,0x00000000); draw_text_center("VOCE VENCEU!",250,12,0x0000FF00); draw_text_center("45 BILHOES DE WON!",350,7,0x00FFFF00); draw_text_center("APERTE X P/ JOGAR DE NOVO",430,6,0x00FFFFFF); }
present(); usleep(16000); }
return 0; }