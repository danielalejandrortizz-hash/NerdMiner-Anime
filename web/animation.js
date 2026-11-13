
'use strict';
(() => {
  const screen=document.getElementById('lcd');
  const art=document.getElementById('scene-art');
  const canvas=document.createElement('canvas');
  canvas.width=640;canvas.height=480;canvas.className='scene-motion';
  canvas.setAttribute('aria-hidden','true');art.after(canvas);
  const ctx=canvas.getContext('2d');
  if(!ctx)return;
  ctx.scale(2,2);
  const reduce=matchMedia('(prefers-reduced-motion: reduce)');
  const compare=document.getElementById('compare');
  compare.onclick=()=>{
    const original=screen.classList.toggle('original');
    compare.setAttribute('aria-checked',String(original));
  };
  let elapsed=0,previous=0;
  function glow(x,y,r,color,alpha){
    const g=ctx.createRadialGradient(x,y,0,x,y,r);
    g.addColorStop(0,color);g.addColorStop(1,'transparent');
    ctx.globalAlpha=alpha;ctx.fillStyle=g;ctx.fillRect(x-r,y-r,2*r,2*r);
    ctx.globalAlpha=1;
  }
  function draw(t){
    ctx.clearRect(0,0,320,240);
    if(!art.complete||!art.naturalWidth)return;
    const theme=Number(screen.dataset.theme);
    // Restrict the water distortion to the lake, away from the character and text.
    ctx.save();ctx.beginPath();ctx.moveTo(0,178);ctx.lineTo(90,165);
    ctx.lineTo(224,165);ctx.lineTo(186,191);ctx.lineTo(118,205);
    ctx.lineTo(0,214);ctx.closePath();ctx.clip();
    const iw=art.naturalWidth,ih=art.naturalHeight;
    const scale=Math.max(320/iw,240/ih),sw=320/scale,sh=240/scale;
    const sx=(iw-sw)/2,sy=(ih-sh)/2;
    for(let y=164;y<216;y++){
      const shift=Math.sin(y*.65+t*2.6)*(1.2+(y-164)*.045);
      ctx.drawImage(art,sx,sy+y/scale,sw,1/scale,shift,y,320,1.15);
    }
    ctx.restore();
    // Warm reflected light follows the lake ripple.
    for(let i=0;i<20;i++){
      const y=168+i*2.4;
      ctx.globalAlpha=.19+.13*Math.sin(t*2.1+i);
      ctx.fillStyle='#ffda91';ctx.fillRect(106+Math.sin(t+i)*3-i*.6,y,6+i*1.7,.65);
    }
    ctx.globalAlpha=1;
    if(theme===0||theme===1){
      // Local deformation tapers to zero at the edges: the body breathes and
      // the forearms/tool follow a repeating mining rhythm without cutout seams.
      const cycle=(t*.65)%1;
      const strike=Math.pow(Math.max(0,Math.sin(cycle*Math.PI*2)),8);
      ctx.save();
      ctx.beginPath();
      ctx.moveTo(174,145);ctx.lineTo(187,145);ctx.lineTo(196,155);
      ctx.lineTo(217,156);ctx.lineTo(236,158);ctx.lineTo(247,160);
      ctx.lineTo(254,177);ctx.lineTo(251,191);ctx.lineTo(233,194);
      ctx.lineTo(223,207);ctx.lineTo(174,213);ctx.closePath();ctx.clip();
      for(let y=145;y<214;y++){
        const breath=Math.sin(t*2.3)*.65;
        const arm=Math.exp(-Math.pow((y-176)/12,2));
        const shift=(breath+strike*1.7*arm)*Math.sin((y-145)/69*Math.PI);
        // Warp each row across its width, keeping the surrounding scene still.
        for(let x=174;x<255;x+=3){
          const taper=Math.sin((x-174)/81*Math.PI);
          const sourceY=y-shift*taper;
          ctx.drawImage(art,sx+x/scale,sy+sourceY/scale,3/scale,1.1/scale,x,y,3.1,1.1);
        }
      }
      ctx.restore();
      const gold=theme===1;
      const color=gold?'#ffdb69':'#9edfff';
      glow(210,166,35,color,(gold?.16:.07)+.07*Math.sin(t*2.3));
      // Sparks leave the crystal on each strike, then arc downwards.
      const age=(cycle-.25+1)%1;
      if(age<.45){
        const life=age/.45;
        glow(251,184,12,'#e6b5ff',(1-life)*.55);
        for(let i=0;i<10;i++){
          const angle=-2.9+i*.27;
          const speed=13+(i%4)*5;
          const x=251+Math.cos(angle)*speed*life;
          const y=184+Math.sin(angle)*speed*life+12*life*life;
          ctx.globalAlpha=(1-life)*.9;ctx.fillStyle=i%2?color:'#f8e0ff';
          ctx.fillRect(x,y,1.1,1.1);ctx.globalAlpha=1;
        }
      }
      if(gold){
        // Golden streams rise around the silhouette and hair.
        for(let i=0;i<14;i++){
          const phase=(t*.48+i/14)%1;
          const side=i%2?1:-1;
          const x=210+side*(18+Math.sin(phase*5+i)*5);
          const y=211-phase*87;
          glow(x,y,2.3,'#ffdc6b',Math.sin(phase*Math.PI)*.8);
        }
        glow(213,138,20,'#ffce50',.23+.12*Math.sin(t*3));
      }else{
        // Blue ki gathers at the hands, with orbiting motes.
        glow(222,179,9,'#9ee9ff',.25+.14*Math.sin(t*3));
        for(let i=0;i<5;i++){
          const a=t*1.8+i*Math.PI*2/5;
          glow(218+Math.cos(a)*14,177+Math.sin(a)*7,1.7,'#b6ecff',.7);
        }
      }
    }else if(theme===2){
      glow(260,184,34,'#ffe862',.25+.13*Math.sin(t*3));
      if(Math.sin(t*2.2)>.65){
        ctx.strokeStyle='#fff5a4';ctx.lineWidth=1.2;ctx.beginPath();
        ctx.moveTo(279,174);ctx.lineTo(284,170);ctx.lineTo(281,178);ctx.lineTo(287,175);ctx.stroke();
      }
    }else if(theme===3){
      glow(286,174,21,'#ff9e32',.5+.2*Math.sin(t*9));
      for(let i=0;i<12;i++){
        const phase=(t*.75+i/12)%1;
        glow(286+Math.sin(phase*5+i)*3,174-phase*35,1.3,'#ffca59',(1-phase)*.95);
      }
    }else{
      glow(206,195,17,'#d5adff',.3+.16*Math.sin(t*2.8));
    }
    // Petals drift through the foreground.
    for(let i=0;i<20;i++){
      const x=(((i*37+t*(8+i%3))%370)+370)%370-25;
      const y=30+((i*29+t*(5+i%2))%205);
      ctx.save();ctx.translate(x,y);ctx.rotate(t*.6+i);
      ctx.fillStyle=i%2?'#f5bad8':'#ffe5aa';ctx.globalAlpha=.95;
      ctx.beginPath();ctx.ellipse(0,0,2.4,1.1,0,0,Math.PI*2);ctx.fill();ctx.restore();
    }

  }
  function tick(now){
    const delta=previous?Math.min((now-previous)/1000,.1):0;previous=now;
    if(!document.hidden&&!screen.classList.contains('paused')&&!screen.classList.contains('original')){
      elapsed+=delta;draw(elapsed);
    }
    requestAnimationFrame(tick);
  }
  new MutationObserver(()=>{ctx.clearRect(0,0,320,240);if(!screen.classList.contains('paused'))draw(elapsed)}).observe(screen,{attributes:true,attributeFilter:['data-theme']});
  requestAnimationFrame(tick);
})();
