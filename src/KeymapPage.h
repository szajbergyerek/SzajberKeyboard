#pragma once

#include <Arduino.h>

// Generated from web/keymap.html, do not edit by hand.
static const char KEYMAP_PAGE[] PROGMEM = R"HTML(<!doctype html>
<html lang="hu">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Szájber Keyboard</title>
<style>
:root{--bg:#23272e;--fg:#e7eaf0;--mut:#8b93a1;--acc:hsl(205,90%,62%);
--sh:6px 6px 12px #1b1e23,-6px -6px 12px #2b3038;
--in:inset 4px 4px 8px #1b1e23,inset -4px -4px 8px #2b3038}
*{box-sizing:border-box}
body{margin:0;padding:14px;background:var(--bg);color:var(--fg);font:15px/1.45 system-ui,sans-serif}
h1{font-size:19px;margin:0 0 12px;text-align:center}
.c{max-width:560px;margin:0 auto}
.p{border-radius:14px;box-shadow:var(--sh);padding:12px;margin-bottom:12px}
button{font:inherit;color:var(--fg);background:var(--bg);border:0;border-radius:10px;box-shadow:var(--sh);padding:10px 14px;cursor:pointer}
button:active{box-shadow:var(--in)}
.pri{color:var(--acc);font-weight:600}
.cap{box-shadow:var(--in);border-radius:10px;padding:14px 12px;margin:8px 0;text-align:center;cursor:pointer;outline:0;color:var(--mut)}
.cap.on{color:var(--acc)}
.cap b{color:var(--fg);font-size:17px;display:block;margin-top:3px}
.lbl{font-size:12px;color:var(--mut);text-transform:uppercase;letter-spacing:.07em;margin-bottom:8px}
.row{display:flex;align-items:center;gap:10px;box-shadow:var(--in);border-radius:10px;padding:8px 12px;margin-bottom:8px}
.row span{flex:1;word-break:break-word}
.del{padding:7px 11px;color:#e8697d}
.st{font-size:13px;color:var(--mut);min-height:18px}
.fl{display:flex;gap:12px;align-items:center;flex-wrap:wrap}
.ar{text-align:center;color:var(--mut);font-size:13px}
label{display:flex;gap:8px;align-items:center;font-size:14px;color:var(--mut);margin:10px 0}
#ab,#sb{width:100%}
.inp,select,textarea{font:inherit;color:var(--fg);background:var(--bg);border:0;border-radius:10px;box-shadow:var(--in);padding:10px 12px;width:100%;margin-bottom:8px;outline:0;-webkit-appearance:none;appearance:none}
textarea{resize:vertical;min-height:54px}
:disabled{opacity:.4}
details{box-shadow:var(--in);border-radius:10px;padding:10px 12px;margin:10px 0}
summary{cursor:pointer;color:var(--acc);font-size:14px}
details[open] summary{margin-bottom:8px}
.note{font-size:12px;color:var(--mut);margin:0 0 10px}
.tag{display:inline-block;font-size:11px;font-weight:600;color:var(--bg);background:var(--acc);border-radius:6px;padding:1px 6px;margin-right:7px}
.sub{display:block;font-size:12px;color:var(--mut);margin-top:4px;word-break:break-all}
</style>
</head>
<body>
<div class="c">
<h1>Szájber Keyboard</h1>
<div class="p">
<div class="fl"><button id="lb">Tanuló mód</button><span class="st" id="ls">Állapot...</span></div>
<p class="note" style="margin:8px 0 0">A lap bezárása után legkésőbb 2 perccel magától kikapcsol.</p>
</div>
<div class="p">
<div class="lbl">Szabályok</div>
<div id="list"></div>
</div>
<div class="p">
<div class="lbl">Új szabály</div>
<div class="cap" id="cf" tabindex="0">Miből<b>kattints, majd nyomd le</b></div>
<div class="ar">lesz belőle</div>
<div class="cap" id="ct" tabindex="0">Mire<b>kattints, majd nyomd le</b></div>
<label><input type="checkbox" id="dis"> Letiltás: a billentyű ne csináljon semmit</label>
<details>
<summary>HTTP hívás (opcionális)</summary>
<p class="note">Ha megadsz URL-t, a billentyű parancsgomb lesz: lenyomásra a billentyűzet meghívja az URL-t, és a billentyű alapértelmezésben <b>nem megy tovább a géphez</b>. Ha az eredeti funkcióját is meg akarod tartani, a Mire mezőbe vedd fel ugyanazt a billentyűt.</p>
<input class="inp" id="u" type="url" inputmode="url" placeholder="URL, pl. http://10.20.30.45:8123/api/webhook/lampa">
<select id="me"><option value="GET">GET</option><option value="POST">POST</option></select>
<input class="inp" id="he" type="text" placeholder="Fejléc, pl. Authorization: Bearer abc123">
<textarea id="bo" placeholder="Törzs, csak POST esetén, pl. {}"></textarea>
</details>
<button class="pri" id="ab">Hozzáadás</button>
</div>
<div class="p">
<button class="pri" id="sb">Mentés a billentyűzetre</button>
<div class="st" id="msg" style="margin-top:10px">Betöltés...</div>
</div>
</div>
<script>
var M={},R={},i,L;
for(i=0;i<26;i++)M["Key"+String.fromCharCode(65+i)]=4+i;
for(i=1;i<10;i++){M["Digit"+i]=29+i;M["Numpad"+i]=88+i;}
M.Digit0=39;M.Numpad0=98;
for(i=1;i<13;i++)M["F"+i]=57+i;
for(i=13;i<25;i++)M["F"+i]=91+i;
L=("Enter 40 Escape 41 Backspace 42 Tab 43 Space 44 Minus 45 Equal 46 BracketLeft 47 BracketRight 48"
+" Backslash 49 Semicolon 51 Quote 52 Backquote 53 Comma 54 Period 55 Slash 56 CapsLock 57 PrintScreen 70"
+" ScrollLock 71 Pause 72 Insert 73 Home 74 PageUp 75 Delete 76 End 77 PageDown 78 ArrowRight 79 ArrowLeft 80"
+" ArrowDown 81 ArrowUp 82 NumLock 83 NumpadDivide 84 NumpadMultiply 85 NumpadSubtract 86 NumpadAdd 87"
+" NumpadEnter 88 NumpadDecimal 99 ContextMenu 101").split(" ");
for(i=0;i<L.length;i+=2)M[L[i]]=+L[i+1];
var MI="ControlLeft ShiftLeft AltLeft MetaLeft ControlRight ShiftRight AltRight MetaRight".split(" ");
for(i=0;i<8;i++)M[MI[i]]=224+i;
M.OSLeft=227;M.OSRight=231;
for(var k in M)R[M[k]]=k;
var MN="Ctrl Shift Alt Win J-Ctrl J-Shift J-Alt J-Win".split(" ");
function E(id){return document.getElementById(id);}
function nm(c){if(c>=224&&c<=231)return MN[c-224];
var n=R[c];
if(!n)return "#"+c;
if(n.indexOf("Key")===0)return n.slice(3);
if(n.indexOf("Digit")===0)return n.slice(5);
if(n.indexOf("Numpad")===0)return "Num "+n.slice(6);
return n;}
function txt(m,c){var a=[],j;
for(j=0;j<8;j++)if(m&(1<<j))a.push(MN[j]);
if(c)a.push(nm(c));
return a.length?a.join(" + "):"Letiltva";}
function esc(s){return String(s).replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;");}
function shrt(u){var s=String(u).replace("http://","").replace("https://","");
return s.length>40?s.slice(0,39)+"…":s;}
function say(s){E("msg").textContent=s;}
var rules=[];
function draw(){var h="",j,r,o;
for(j=0;j<rules.length;j++){r=rules[j];
o=(r.outMods||r.outKey)?txt(r.outMods,r.outKey):(r.url?"csak parancs":"Letiltva");
h+="<div class=row><span>"+(r.url?"<span class=tag>HTTP</span>":"")
+txt(r.mods,r.key)+"  &rarr;  "+o
+(r.url?"<span class=sub>"+esc((r.method||"GET")+" "+shrt(r.url))+"</span>":"")
+"</span><button class=del data-i="+j+">Törlés</button></div>";}
E("list").innerHTML=h||"<div class=st>Még nincs szabály.</div>";}
E("list").onclick=function(e){var d=e.target.getAttribute("data-i");
if(d===null)return;
rules.splice(+d,1);draw();say("Szabály törölve. A mentés még hátravan.");};
var cap={cf:{m:0,k:0},ct:{m:0,k:0}},down={};
window.addEventListener("keydown",function(e){if(M[e.code]>=224)down[M[e.code]]=1;},true);
window.addEventListener("keyup",function(e){delete down[M[e.code]];},true);
window.addEventListener("blur",function(){down={};});
function md(e,self){var m=0,j;
for(j=0;j<8;j++)if(down[224+j])m|=1<<j;
if(e.ctrlKey&&!(m&17))m|=1;
if(e.shiftKey&&!(m&34))m|=2;
if(e.altKey&&!(m&68))m|=4;
if(e.metaKey&&!(m&136))m|=8;
if(self>=224&&self<=231)m&=~(1<<(self-224));
return m;}
function fill(id,pre){var o=cap[id];
E(id).innerHTML=pre+"<b>"+(o.k||o.m?txt(o.m,o.k):"kattints, majd nyomd le")+"</b>";}
function bind(id,pre){var el=E(id);
el.onclick=function(){el.focus();};
el.onfocus=function(){el.className="cap on";};
el.onblur=function(){el.className="cap";};
el.onkeydown=function(e){e.preventDefault();
var o=cap[id],c=M[e.code]||0;
o.k=c;o.m=md(e,c);fill(id,pre);};
fill(id,pre);}
bind("cf","Miből");bind("ct","Mire");
function hup(){var u=E("u").value.trim();
E("me").disabled=!u;E("he").disabled=!u;
E("bo").disabled=!u;
E("bo").style.display=(u&&E("me").value==="POST")?"":"none";}
E("u").oninput=hup;E("me").onchange=hup;hup();
E("ab").onclick=function(){var f=cap.cf,t=cap.ct,d=E("dis").checked,
u=E("u").value.trim(),r,h,b;
if(!f.k){say("Nyomd le a Miből mező billentyűjét.");return;}
if(!d&&!u&&!t.k){say("Nyomd le a Mire mező billentyűjét, vagy adj meg URL-t, vagy jelöld a letiltást.");return;}
r={mods:f.m,key:f.k,outMods:d?0:t.m,outKey:d?0:t.k};
if(u){r.url=u;r.method=E("me").value;
h=E("he").value.trim();if(h)r.header=h;
b=E("bo").value;if(r.method==="POST"&&b)r.body=b;}
rules.push(r);
cap.cf={m:0,k:0};cap.ct={m:0,k:0};E("dis").checked=false;
E("u").value="";E("he").value="";E("bo").value="";E("me").value="GET";hup();
fill("cf","Miből");fill("ct","Mire");draw();
say("Szabály hozzáadva. A mentés még hátravan.");};
E("sb").onclick=function(){say("Mentés...");
fetch("/keymap.json",{method:"POST",headers:{"Content-Type":"application/json"},
body:JSON.stringify({rules:rules})}).then(function(r){return r.json();})
.then(function(j){say(j.ok?"Mentve, "+j.count+" szabály.":"Mentés sikertelen: "+(j.error||"ismeretlen hiba"));})
.catch(function(){say("Mentés sikertelen: nincs kapcsolat a billentyűzettel.");});};
var learn=false,hb=0;
function sl(v){fetch("/learn?on="+(v?1:0)).then(function(r){return r.json();})
.then(function(j){learn=!!j.learn;
E("ls").textContent=learn?"BE: az átírás most nem él":"KI: az átírás aktív";
E("lb").className=learn?"pri":"";
if(learn&&!hb)hb=setInterval(function(){fetch("/learn?on=1");},30000);
if(!learn&&hb){clearInterval(hb);hb=0;}})
.catch(function(){E("ls").textContent="A tanuló mód állapota nem ismert.";});}
E("lb").onclick=function(){sl(!learn);};
function norm(a){var o=[],j,r,x;
for(j=0;j<(a||[]).length;j++){r=a[j]||{};
x={mods:+r.mods||0,key:+r.key||0,outMods:+r.outMods||0,outKey:+r.outKey||0};
if(r.url){x.url=String(r.url);
x.method=String(r.method||"GET").toUpperCase()==="POST"?"POST":"GET";
if(r.header)x.header=String(r.header);
if(r.body&&x.method==="POST")x.body=String(r.body);}
o.push(x);}
return o;}
fetch("/keymap.json").then(function(r){return r.json();})
.then(function(j){rules=norm(j.rules);draw();say(rules.length+" szabály betöltve.");})
.catch(function(){draw();say("A szabályok betöltése nem sikerült.");});
sl(true);
window.addEventListener("beforeunload",function(){fetch("/learn?on=0",{keepalive:true});});
</script>
</body>
</html>)HTML";
