// Port of web/animation.js. Browser coordinates, timing, colors and layer order
// remain the reference; only rasterization and refresh cadence are LCD-specific.
struct RoomWebGlow { float x,y,r,alpha; uint16_t color; };
struct RoomWebRect { float x,y,w,h,alpha; uint16_t color; };
struct RoomWebPetal { float x,y,rx,ry,angle,cs,sn; uint16_t color; };
struct RoomWebCommand { uint8_t kind,index; };
struct RoomWebFrame {
  float t, water[52], arm[69], taper[27];
  RoomWebGlow glows[24]; RoomWebRect rects[32]; RoomWebPetal petals[28];
  RoomWebCommand commands[56];
  uint8_t ng,nr,nc; bool goku;
};
static float roomWebClamp(float n) { return n<0 ? 0 : n>1 ? 1 : n; }
static void roomWebGlow(RoomWebFrame &f,float x,float y,float r,uint16_t color,float alpha)
{
  f.glows[f.ng]={x,y,r,alpha,color};f.commands[f.nc++]={0,f.ng++};
}
static void roomWebRect(RoomWebFrame &f,float x,float y,float w,float h,uint16_t color,float alpha)
{
  f.rects[f.nr]={x,y,w,h,alpha,color};f.commands[f.nc++]={1,f.nr++};
}
static float roomPetalNoise(int i,uint32_t salt)
{
  uint32_t n=uint32_t(i+1)*1103515245u+salt;
  n=(n^(n>>16))*2246822519u;
  n=(n^(n>>13))*3266489917u;
  return (n^(n>>16))/4294967296.0f;
}
static void roomBuildWebFrame(RoomWebFrame &f,float t,int theme)
{
  const float pi=3.14159265358979323846f;
  f.t=t;f.ng=f.nr=f.nc=0;f.goku=theme<2;
  for(int y=164;y<216;++y) f.water[y-164]=sinf(y*.65f+t*2.6f)*(1.2f+(y-164)*.045f);
  for(int i=0;i<20;++i)
    roomWebRect(f,106+sinf(t+i)*3-i*.6f,168+i*2.4f,6+i*1.7f,.65f,RGB565(255,218,145),.19f+.13f*sinf(t*2.1f+i));
  if(f.goku) {
    const float cycle=fmodf(t*.65f,1);
    float strike=sinf(cycle*pi*2);if(strike<0)strike=0;strike*=strike;strike*=strike;strike*=strike;
    for(int y=145;y<214;++y) {
      const float d=(y-176)/12.0f;
      f.arm[y-145]=(sinf(t*2.3f)*.65f+strike*1.7f*expf(-d*d))*sinf((y-145)/69.0f*pi);
    }
    for(int i=0;i<27;++i)f.taper[i]=sinf(i*3/81.0f*pi);
    const bool gold=theme==1;
    const uint16_t color=gold?RGB565(255,219,105):RGB565(158,223,255);
    roomWebGlow(f,210,166,35,color,(gold?.16f:.07f)+.07f*sinf(t*2.3f));
    const float age=fmodf(cycle-.25f+1,1);
    if(age<.45f) {
      const float life=age/.45f;
      roomWebGlow(f,251,184,12,RGB565(230,181,255),(1-life)*.55f);
      for(int i=0;i<10;++i) {
        const float angle=-2.9f+i*.27f, speed=13+(i%4)*5;
        roomWebRect(f,251+cosf(angle)*speed*life,184+sinf(angle)*speed*life+12*life*life,1.1f,1.1f,
                    i%2?color:RGB565(248,224,255),(1-life)*.9f);
      }
    }
    if(gold) {
      for(int i=0;i<14;++i) {
        const float phase=fmodf(t*.48f+i/14.0f,1),side=i%2?1:-1;
        roomWebGlow(f,210+side*(18+sinf(phase*5+i)*5),211-phase*87,2.3f,RGB565(255,220,107),sinf(phase*pi)*.8f);
      }
      roomWebGlow(f,213,138,20,RGB565(255,206,80),.23f+.12f*sinf(t*3));
    } else {
      roomWebGlow(f,222,179,9,RGB565(158,233,255),.25f+.14f*sinf(t*3));
      for(int i=0;i<5;++i) {
        const float a=t*1.8f+i*pi*2/5;
        roomWebGlow(f,218+cosf(a)*14,177+sinf(a)*7,1.7f,RGB565(182,236,255),.7f);
      }
    }
  } else if(theme==2) {
    roomWebGlow(f,260,184,34,RGB565(255,232,98),.25f+.13f*sinf(t*3));
    // The four lightning segments are rasterized with the same blink timing.
  } else if(theme==3) {
    roomWebGlow(f,286,174,21,RGB565(255,158,50),.5f+.2f*sinf(t*9));
    for(int i=0;i<12;++i) {
      const float phase=fmodf(t*.75f+i/12.0f,1);
      roomWebGlow(f,286+sinf(phase*5+i)*3,174-phase*35,1.3f,RGB565(255,202,89),(1-phase)*.95f);
    }
  }
  for(int i=0;i<28;++i) {
    RoomWebPetal &p=f.petals[i];
    const float a=roomPetalNoise(i,0x68bc21ebu),b=roomPetalNoise(i,0x02e5be93u);
    const float c=roomPetalNoise(i,0x967a889bu),d=roomPetalNoise(i,0x368cc8b7u);
    const float phase=roomPetalNoise(i,0x51ed270bu)*pi*2;
    const float baseX=8+((i*3)%7+a)*304/7,baseY=8+(i/7+b)*56;
    p.x=fmodf(baseX+25+t*(7+c*7),370)-25+sinf(t*(.45f+c*.45f)+phase)*(2+a*3);
    p.y=fmodf(baseY+12+t*(3+d*4),264)-12+sinf(t*(.35f+d*.3f)+phase)*2;
    p.rx=2.6f+(i%3)*.45f;p.ry=1.25f+(i%2)*.3f;p.angle=t*(.3f+d*.4f)+phase;
    p.cs=cosf(p.angle);p.sn=sinf(p.angle);
    p.color=i%2?RGB565(255,208,233):RGB565(255,240,188);
  }
}

static void roomWebMark(uint8_t *dirty,int y,float x,float top,float width,float height)
{
  if(y+1<=top || y>=top+height)return;
  int left=int(floorf(x)),right=int(ceilf(x+width));
  if(left<0)left=0;
  if(right>SCR_W)right=SCR_W;
  for(int i=left;i<right;++i)dirty[i]=1;
}
static void roomWebDirty(uint8_t *dirty,int y,const RoomWebFrame &f)
{
  roomWebMark(dirty,y,0,164,224,52);
  if(f.goku)roomWebMark(dirty,y,174,145,81,69);
  for(int i=0;i<f.ng;++i) {
    const RoomWebGlow &g=f.glows[i];roomWebMark(dirty,y,g.x-g.r,g.y-g.r,g.r*2,g.r*2);
  }
  for(int i=0;i<f.nr;++i) {
    const RoomWebRect &r=f.rects[i];roomWebMark(dirty,y,r.x,r.y,r.w,r.h);
  }
  for(int i=0;i<28;++i) {
    const RoomWebPetal &p=f.petals[i];const float r=p.rx+2;
    roomWebMark(dirty,y,p.x-r,p.y-r,r*2,r*2+1);
  }
  // CSS fireflies, plus Pikachu's lightning.
  roomWebMark(dirty,y,90,151,192,56);
}
static uint16_t roomWebSample(float x,float y)
{
  const int ix=int(floorf(x)),iy=int(floorf(y));
  const uint8_t fx=uint8_t((x-ix)*255),fy=uint8_t((y-iy)*255);
  return mix565(mix565(roomRawPixel(ix,iy),roomRawPixel(ix+1,iy),fx),
                mix565(roomRawPixel(ix,iy+1),roomRawPixel(ix+1,iy+1),fx),fy);
}
static bool roomWebBody(int x,int y)
{
  static const int16_t points[13][2]={{174,145},{187,145},{196,155},{217,156},{236,158},{247,160},
                                    {254,177},{251,191},{233,194},{223,207},{174,213},{174,145},{174,145}};
  bool inside=false;
  for(int i=0,j=10;i<11;j=i++) {
    const int ax=points[i][0],ay=points[i][1],bx=points[j][0],by=points[j][1];
    if((ay>y)!=(by>y) && x < (bx-ax)*float(y-ay)/(by-ay)+ax)inside=!inside;
  }
  return inside;
}
static uint16_t roomWebBase(int x,int y,const RoomWebFrame &f)
{
  const bool water=y>=165 && y<214 && y>=178-x*13/90 &&
    (y<191 ? x<224-(y-165)*38/26 : y<205 ? x<186-(y-191)*68/14 : x<118-(y-205)*118/9);
  if(water)return roomWebSample(x-f.water[y-164],float(y));
  if(f.goku && y>=145 && y<214 && x>=174 && x<255 && roomWebBody(x,y))
    return roomWebSample(float(x),y-f.arm[y-145]*f.taper[(x-174)/3]);
  return roomPixel(x,y);
}
static void roomWebCompositeGlow(uint16_t *row,const uint8_t *dirty,int y,const RoomWebGlow &g)
{
  const float dy=y+.5f-g.y;if(fabsf(dy)>=g.r || g.alpha<=0)return;
  int left=int(floorf(g.x-g.r)),right=int(ceilf(g.x+g.r));if(left<0)left=0;
  if(right>SCR_W)right=SCR_W;
  for(int x=left;x<right;++x)if(dirty[x]) {
    const float dx=x+.5f-g.x,d=dx*dx+dy*dy;
    if(d<g.r*g.r)row[x]=mix565(row[x],g.color,uint8_t(255*roomWebClamp(g.alpha*(1-sqrtf(d)/g.r))));
  }
}
static void roomWebCompositeRect(uint16_t *row,const uint8_t *dirty,int y,const RoomWebRect &r)
{
  const float bottom=r.y+r.h,yy=float(y);
  const float coverY=(bottom<yy+1?bottom:yy+1)-(r.y>yy?r.y:yy);
  if(coverY<=0)return;
  int left=int(floorf(r.x)),right=int(ceilf(r.x+r.w));if(left<0)left=0;
  if(right>SCR_W)right=SCR_W;
  for(int x=left;x<right;++x)if(dirty[x]) {
    const float xx=float(x),edge=r.x+r.w;
    const float coverX=(edge<xx+1?edge:xx+1)-(r.x>xx?r.x:xx);
    row[x]=mix565(row[x],r.color,uint8_t(255*roomWebClamp(coverX*coverY*r.alpha)));
  }
}
static void roomWebCompositePetals(uint16_t *row,const uint8_t *dirty,int y,const RoomWebFrame &f)
{
  for(int i=0;i<28;++i) {
    const RoomWebPetal &p=f.petals[i];const float radius=p.rx+2;
    if(fabsf(y+.5f-p.y)>radius+1)continue;
    int left=int(floorf(p.x-radius)),right=int(ceilf(p.x+radius));if(left<0)left=0;
  if(right>SCR_W)right=SCR_W;
    for(int x=left;x<right;++x)if(dirty[x]) {
      const float dx=x+.5f-p.x,dy=y+.5f-p.y;
      const float u=dx*p.cs+dy*p.sn,v=-dx*p.sn+dy*p.cs;
      const float shadowV=v-.6f;
      const float shadow=u*u/((p.rx+.75f)*(p.rx+.75f))+shadowV*shadowV/((p.ry+.75f)*(p.ry+.75f));
      const float edge=u*u/(p.rx*p.rx)+v*v/(p.ry*p.ry);
      if(shadow<1.5f)row[x]=mix565(row[x],RGB565(99,51,70),uint8_t(70*roomWebClamp(1.5f-shadow)));
      if(edge<1.5f)row[x]=mix565(row[x],p.color,uint8_t(245*roomWebClamp((1-edge)*p.ry+.5f)));
    }
  }
}
static void roomWebCompositeLights(uint16_t *row,const uint8_t *dirty,int y,const RoomWebFrame &f)
{
  static const float x[4]={195.2f,246.4f,96,272},yy[4]={187.2f,163.2f,201.6f,196.8f};
  static const float delay[4]={0,3,5,1};
  for(int i=0;i<4;++i) {
    const float phase=fmodf((f.t+delay[i])/8,1);
    const float alpha=phase<.35f ? .65f*phase/.35f : phase<.65f ? .65f-.55f*(phase-.35f)/.3f : .1f*(1-phase)/.35f;
    const float drift=roomWebClamp(phase/.65f);
    const RoomWebGlow g={x[i]+drift*3,yy[i]-drift*7,3,alpha,RGB565(249,229,172)};
    roomWebCompositeGlow(row,dirty,y,g);
  }
  if(roomTheme==2 && sinf(f.t*2.2f)>.65f) {
    static const float points[4][2]={{279,174},{284,170},{281,178},{287,175}};
    for(int i=0;i<3;++i) {
      const float ax=points[i][0],ay=points[i][1],dx=points[i+1][0]-ax,dy=points[i+1][1]-ay;
      for(int x=278;x<289;++x)if(dirty[x]) {
        const float px=x+.5f-ax,py=y+.5f-ay;
        const float t=roomWebClamp((px*dx+py*dy)/(dx*dx+dy*dy));
        const float ex=px-t*dx,ey=py-t*dy,dist=sqrtf(ex*ex+ey*ey);
        if(dist<1.1f)row[x]=mix565(row[x],RGB565(255,245,164),uint8_t(255*roomWebClamp(1.1f-dist)));
      }
    }
  }
}
