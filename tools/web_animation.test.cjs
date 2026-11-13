const { test } = require('node:test');
const assert = require('node:assert/strict');
const vm = require('node:vm');
const fs = require('node:fs');
const path = require('node:path');

function animation(theme, paused = false) {
  const calls = [];
  const classes = new Set(paused ? ['paused'] : []);
  const screen = {dataset:{theme:String(theme)},classList:{contains:name=>classes.has(name),toggle(name){if(classes.has(name)){classes.delete(name);return false;}classes.add(name);return true;}}};
  const art = {complete:true,naturalWidth:1448,naturalHeight:1086,after(){}};
  const compare = {setAttribute(){}};
  let tick;
  const ctx = new Proxy({}, {get(target,key){
    if(key==='createRadialGradient')return ()=>({addColorStop(){}});
    return (...args)=>calls.push(key);
  },set(){return true;}});
  const context = vm.createContext({
    document:{hidden:false,getElementById:id=>id==='lcd'?screen:id==='scene-art'?art:compare,
      createElement:()=>({setAttribute(){},getContext:()=>ctx})},
    matchMedia:()=>({matches:false}),
    MutationObserver:class{observe(){}},
    requestAnimationFrame:fn=>{tick=fn;},
  });
  vm.runInContext(fs.readFileSync(path.join(__dirname,'../web/animation.js'),'utf8'),context);
  return {calls,frame:()=>tick(1000)};
}

test('all four scenes draw foreground petals after the character image',()=>{
  for(let theme=0;theme<4;theme++){
    const app=animation(theme);app.frame();
    assert.equal(app.calls.filter(name=>name==='ellipse').length,20);
    assert.ok(app.calls.indexOf('ellipse')>app.calls.lastIndexOf('drawImage'));
  }
});
test('paused scene does not repaint',()=>{
  const app=animation(0,true);const count=app.calls.length;app.frame();
  assert.equal(app.calls.length,count);
});
