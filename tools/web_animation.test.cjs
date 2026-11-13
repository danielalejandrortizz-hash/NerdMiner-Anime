const { test } = require('node:test');
const assert = require('node:assert/strict');
const vm = require('node:vm');
const fs = require('node:fs');
const path = require('node:path');

function animation(theme, paused = false) {
  const calls = [], positions = [], sizes = [];
  const classes = new Set(paused ? ['paused'] : []);
  const screen = {dataset:{theme:String(theme)},classList:{contains:name=>classes.has(name),toggle(name){if(classes.has(name)){classes.delete(name);return false;}classes.add(name);return true;}}};
  const art = {complete:true,naturalWidth:1448,naturalHeight:1086,after(){}};
  const compare = {setAttribute(){}};
  let tick;
  const ctx = new Proxy({}, {get(target,key){
    if(key==='createRadialGradient')return ()=>({addColorStop(){}});
    return (...args)=>{calls.push(key);if(key==='translate')positions.push(args);if(key==='ellipse')sizes.push(args);};
  },set(){return true;}});
  const context = vm.createContext({
    document:{hidden:false,getElementById:id=>id==='lcd'?screen:id==='scene-art'?art:compare,
      createElement:()=>({setAttribute(){},getContext:()=>ctx})},
    matchMedia:()=>({matches:false}),
    MutationObserver:class{observe(){}},
    requestAnimationFrame:fn=>{tick=fn;},
  });
  vm.runInContext(fs.readFileSync(path.join(__dirname,'../web/animation.js'),'utf8'),context);
  return {calls,positions,sizes,frame:(now=1000)=>tick(now)};
}

test('all four scenes draw foreground petals after the character image',()=>{
  for(let theme=0;theme<4;theme++){
    const app=animation(theme);app.frame();
    assert.equal(app.calls.filter(name=>name==='ellipse').length,28);
    assert.ok(app.calls.indexOf('ellipse')>app.calls.lastIndexOf('drawImage'));
  }
});
test('paused scene does not repaint',()=>{
  const app=animation(0,true);const count=app.calls.length;app.frame();
  assert.equal(app.calls.length,count);
});

test('petals use larger shapes and continuous fractional motion between browser frames',()=>{
  const app=animation(0);app.frame(1000);
  const before=app.positions.slice();
  assert.equal(before.length,28);
  assert.ok(app.sizes.every(shape=>shape[2]>=2.6&&shape[3]>=1.25));
  app.frame(1016);
  const after=app.positions.slice(28);
  for(let i=0;i<28;i++){
    const dx=after[i][0]-before[i][0];
    assert.ok(dx>0&&dx<1,'16 ms of drift should move by a fraction of a pixel');
  }
});

test('petals start scattered across the whole display, without aligned rows or columns',()=>{
  const app=animation(0);app.frame();
  const points=app.positions;
  const quadrants=[0,0,0,0];
  for(const [x,y] of points){
    assert.ok(x>=0&&x<320&&y>=0&&y<240);
    quadrants[(x>=160?1:0)+(y>=120?2:0)]++;
  }
  assert.ok(quadrants.every(count=>count>=4),'each quadrant needs visible petals');
  const bands=new Map();
  for(const [,y]of points){const band=Math.floor(y/6);bands.set(band,(bands.get(band)||0)+1)}
  assert.ok(Math.max(...bands.values())<=3,'no dense horizontal lines');
  const xs=points.map(p=>p[0]),ys=points.map(p=>p[1]);
  assert.ok(Math.max(...xs)-Math.min(...xs)>270);
  assert.ok(Math.max(...ys)-Math.min(...ys)>190);
});
