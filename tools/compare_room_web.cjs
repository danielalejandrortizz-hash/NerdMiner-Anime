'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const vm=require('node:vm');
const source=fs.readFileSync(path.join(__dirname,'../web/animation.js'),'utf8');
function color565(hex){const n=parseInt(hex.slice(1),16);return (((n>>16)&248)<<8)|(((n>>8)&252)<<3)|((n&255)>>3)}
function reference(theme,time){
  let tick,style,alpha=1,rotation=0,position=[0,0],state=[];
  const trace={glows:[],rects:[],petals:[],water:[],body:0};
  function reset(){for(const key of ['glows','rects','petals','water'])trace[key]=[];trace.body=0}
  const ctx={
    scale(){},clearRect(){},beginPath(){},moveTo(){},lineTo(){},closePath(){},clip(){},stroke(){},fill(){},
    save(){state.push({style,alpha,rotation,position:position.slice()})},
    restore(){const s=state.pop();({style,alpha,rotation,position}=s)},
    translate(x,y){position=[x,y]},rotate(a){rotation=a},
    ellipse(x,y,rx,ry){trace.petals.push([position[0],position[1],rx,ry,rotation,color565(style)])},
    createRadialGradient(x,y,_,__,___,r){const g={x,y,r,color:null,addColorStop(at,c){if(at===0)this.color=c}};return g},
    fillRect(x,y,w,h){
      if(typeof style==='object')trace.glows.push([style.x,style.y,style.r,alpha,color565(style.color)]);
      else trace.rects.push([x,y,w,h,alpha,color565(style)]);
    },
    drawImage(...args){
      if(args[7]===320)trace.water.push(args[5]);
      else if(args[5]===210&&args[6]===176)trace.body=176-args[2];
    },
    set fillStyle(s){style=s},get fillStyle(){return style},
    set globalAlpha(a){alpha=a},get globalAlpha(){return alpha}
  };
  const screen={dataset:{theme:String(theme)},classList:{contains(){return false},toggle(){}}};
  const art={complete:true,naturalWidth:320,naturalHeight:240,after(){}};
  const context=vm.createContext({
    document:{hidden:false,getElementById:id=>id==='lcd'?screen:id==='scene-art'?art:{setAttribute(){}},createElement:()=>({setAttribute(){},getContext(){return ctx}})},
    matchMedia:()=>({matches:false}),MutationObserver:class{observe(){}},requestAnimationFrame:fn=>{tick=fn}
  });
  vm.runInContext(source,context);tick(1000);
  let at=0;while(at<time*1000){at=Math.min(at+100,time*1000);reset();tick(1000+at)}
  return trace;
}
const cases=JSON.parse(fs.readFileSync(process.argv[2],'utf8'));
let fields=0;
for(const native of cases){
  const web=reference(native.theme,native.t);
  for(const name of ['glows','rects','petals','water']){
    assert.equal(native[name].length,web[name].length,`${name} count, theme ${native.theme}, t=${native.t}`);
    for(let i=0;i<native[name].length;i++){
      const a=Array.isArray(native[name][i])?native[name][i]:[native[name][i]];
      const b=Array.isArray(web[name][i])?web[name][i]:[web[name][i]];
      for(let j=0;j<a.length;j++){
        assert.ok(Math.abs(a[j]-b[j])<.002,`${name}[${i}][${j}]: ${a[j]} vs ${b[j]}, theme ${native.theme}, t=${native.t}`);fields++;
      }
    }
  }
  assert.ok(Math.abs(native.body-web.body)<.002,'Goku body/arm deformation');
}
console.log(`OK: ${cases.length} web/firmware samples, ${fields} matching coordinates, colors and timing values; web renderer unchanged`);
// Typography remains anchored to the reference CSS rather than an unrelated
// TFT default font. A web size change must be reflected in the firmware.
const css=fs.readFileSync(path.join(__dirname,'../web/room.css'),'utf8');
const firmware=fs.readFileSync(path.join(__dirname,'../src/drivers/displays/roomTheme.inc'),'utf8');
for(const [webRule,nativeRule] of [
  [/\.mini-time\{[^}]*font-size:(\d+)px/,/m\.hdr\.time,roomFont(\d+)/],
  [/\.clock-digits\{[^}]*font-size:(\d+)px/,/c\.hh,roomFont(\d+)/],
  [/\.market-price\{[^}]*font-size:(\d+)px/,/price,roomFont(\d+)/],
  [/\.stat-label\{[^}]*font-size:(\d+)px/,/label,roomFont(\d+)/]
])assert.equal(Number(firmware.match(nativeRule)?.[1]),Number(css.match(webRule)?.[1]),'Firmware font size must match web CSS');
console.log('OK: landscape clock, central clock, price and statistic label sizes match the reference CSS');
