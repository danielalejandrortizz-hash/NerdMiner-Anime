'use strict';
const themes = [
  {name:'Goku', place:'Lago al atardecer', image:'assets/goku-sunset.png', alt:'Goku minando junto a un lago al atardecer, rodeado de montañas y flores'},
  {name:'Super Saiyan', place:'La última luz del día', image:'assets/saiyan-sunset.png', alt:'Super Saiyan Goku junto al lago al atardecer'},
  {name:'Pikachu', place:'Una chispa de tranquilidad', image:'assets/pikachu-sunset.png', alt:'Pikachu junto a unos cristales en un paisaje tranquilo'},
  {name:'Charmander', place:'Al calor de una pequeña llama', image:'assets/charmander-sunset.png', alt:'Charmander entre flores junto al lago al atardecer'}
];
const $ = id => document.getElementById(id);
const lcd = $('lcd'), displayWindow = $('display-window');
let selected=0, currentPage=0, offline=false, paused=false, night=false, shares=24, celebrationTimer;
function save(key,value){try{localStorage.setItem(key,String(value))}catch(_){}}
function read(key){try{return localStorage.getItem(key)}catch(_){return null}}
function selectTheme(index){
  if(!Number.isInteger(index)||!themes[index])return;
  selected=index;
  const theme=themes[index];
  lcd.dataset.theme=String(index);
  $('scene-art').src=theme.image+'?v=3';
  $('scene-art').alt=theme.alt;
  $('scene-name').textContent=theme.name+' · '+theme.place;
  document.querySelectorAll('[data-character]').forEach(button=>button.setAttribute('aria-pressed',String(Number(button.dataset.character)===index)));
  save('ki-animation-demo-theme',index);
}
function selectPage(index){
  if(!Number.isInteger(index)||index<0||index>4)return;
  currentPage=index;lcd.dataset.view=String(index);
  document.querySelectorAll('[data-page]').forEach(page=>{page.hidden=Number(page.dataset.page)!==index});
  document.querySelectorAll('[data-tab]').forEach(button=>button.setAttribute('aria-pressed',String(Number(button.dataset.tab)===index)));
  save('ki-animation-demo-view',index);
}
themes.forEach((theme,index)=>{
  const card=document.createElement('button');
  card.className='theme-card';card.dataset.character=String(index);
  card.setAttribute('aria-pressed',String(index===selected));
  card.innerHTML=`<img class="theme-thumb" src="${theme.image}?v=3" alt=""><span><strong>${theme.name}</strong><small>${theme.place}</small></span><span class="theme-check" aria-hidden="true">✓</span>`;
  card.onclick=()=>selectTheme(index);$('theme-picker').append(card);
  const small=document.createElement('button');
  small.className='lcd-character';small.dataset.character=String(index);
  small.setAttribute('aria-pressed',String(index===selected));
  small.innerHTML=`<img src="${theme.image}?v=3" alt=""><span>${theme.name}</span>`;
  small.onclick=()=>{selectTheme(index);selectPage(0)};$('lcd-theme-picker').append(small);
});
document.querySelectorAll('[data-tab]').forEach(button=>{button.onclick=()=>selectPage(Number(button.dataset.tab))});
function updateClock(now=new Date()){
  const hour=String(now.getHours()).padStart(2,'0'),minute=String(now.getMinutes()).padStart(2,'0');
  $('mini-time').textContent=hour+':'+minute;
  $('clock-hours').textContent=hour;$('clock-minutes').textContent=minute;
  $('clock-day').textContent=new Intl.DateTimeFormat('es-ES',{weekday:'long'}).format(now);
  $('clock-date').textContent=new Intl.DateTimeFormat('es-ES',{day:'numeric',month:'long'}).format(now);
  $('mini-date').textContent=new Intl.DateTimeFormat('es-ES',{weekday:'long',day:'numeric',month:'long'}).format(now);
  $('mini-time').dateTime=hour+':'+minute;$('main-time').dateTime=hour+':'+minute;
}
function toggleNight(value){night=value;lcd.classList.toggle('night-mode',night);$('night').setAttribute('aria-checked',String(night));save('ki-animation-demo-night',night)}
$('night').onclick=()=>toggleNight(!night);
$('pause').onclick=()=>{paused=!paused;lcd.classList.toggle('paused',paused);$('pause').setAttribute('aria-checked',String(paused))};
$('share').onclick=()=>{
  if(offline)return;
  shares++;$('detail-shares').textContent=String(shares);$('shares').textContent=String(shares);$('market-shares').textContent=String(shares);
  lcd.classList.add('celebrating');clearTimeout(celebrationTimer);celebrationTimer=setTimeout(()=>lcd.classList.remove('celebrating'),3000);
};
$('network').onclick=()=>{
  offline=!offline;$('detail-hash').textContent=offline?'—':'990 KH/s';lcd.classList.toggle('offline',offline);
  $('network').setAttribute('aria-pressed',String(offline));$('network').textContent=offline?'Volver a conectar':'Simular desconexión';
  $('mining-status').textContent=offline?'Sin conexión':'Minando';
  $('connection-text').textContent=offline?'Sin conexión':'Conectado · cifras de ejemplo';
  document.querySelector('.rate').innerHTML=offline?'— <small>KH/s</small>':'990 <small>KH/s</small>';
  $('workers').textContent=offline?'—':'3';
  $('workers-note').textContent=offline?'sin datos':'activos';
  $('share').disabled=offline;
  if(offline){clearTimeout(celebrationTimer);lcd.classList.remove('celebrating')}
};
function resizeDisplay(){lcd.style.transform=`scale(${displayWindow.clientWidth/320})`}
if(typeof ResizeObserver==='function')new ResizeObserver(resizeDisplay).observe(displayWindow);
window.addEventListener('resize',resizeDisplay);
$('fullscreen').onclick=async()=>{
  try{if(document.fullscreenElement)await document.exitFullscreen();else if(displayWindow.requestFullscreen)await displayWindow.requestFullscreen();else $('fullscreen').textContent='Amplía con el zoom del navegador'}
  catch(_){$('fullscreen').textContent='Amplía con el zoom del navegador'}
};
document.addEventListener('fullscreenchange',()=>{$('fullscreen').textContent=document.fullscreenElement?'Salir de pantalla completa ↙':'Ver en grande ↗';resizeDisplay()});
$('scene-art').onerror=()=>{
  let note=$('asset-error');
  if(!note){note=document.createElement('div');note.id='asset-error';note.className='asset-error';lcd.append(note)}
  note.textContent='No se pudo cargar esta escena. Prueba a recargar la página.';
};
$('scene-art').onload=()=>{const note=$('asset-error');if(note)note.remove()};
const storedTheme=read('ki-animation-demo-theme'),storedView=read('ki-animation-demo-view');
selectTheme(storedTheme===null?0:Number(storedTheme));selectPage(storedView===null?0:Number(storedView));toggleNight(read('ki-animation-demo-night')==='true');
updateClock();setInterval(updateClock,1000);resizeDisplay();
