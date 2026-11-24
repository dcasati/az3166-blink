#pragma once

// This browser checksum detects transfer mistakes; it does not authenticate firmware.
static const char OTA_PAGE[] = R"HTML(<!doctype html>
<html><meta name="viewport" content="width=device-width"><title>AZ3166 OTA</title>
<body><h1>Firmware update</h1>
<p>Trusted LAN HTTP only. Images are NOT signed. Keep power connected.
No automatic rollback; ST-Link recovery may be required.</p>
<input id="image" type="file" accept=".bin">
<button id="upload">Upload BIN</button><button id="apply" disabled>Apply and reboot</button>
<button id="abort">Discard</button><pre id="status"></pre><a href="/">Back</a>
<script>
function sha256Fallback(data) {
  const k = [
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2];
  const h = [0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19];
  const bytes = new Uint8Array(Math.ceil((data.byteLength + 9) / 64) * 64);
  bytes.set(new Uint8Array(data)); bytes[data.byteLength] = 128;
  const view = new DataView(bytes.buffer);
  view.setUint32(bytes.length - 4, data.byteLength * 8);
  const w = new Uint32Array(64), rr = (x,n) => (x >>> n) | (x << (32-n));
  for (let offset=0; offset<bytes.length; offset+=64) {
    for (let i=0;i<16;i++) w[i]=view.getUint32(offset+i*4);
    for (let i=16;i<64;i++) {
      const a=w[i-15],b=w[i-2];
      w[i]=(w[i-16]+(rr(a,7)^rr(a,18)^(a>>>3))+w[i-7]+(rr(b,17)^rr(b,19)^(b>>>10)))>>>0;
    }
    let [a,b,c,d,e,f,g,j]=h;
    for (let i=0;i<64;i++) {
      const t1=(j+(rr(e,6)^rr(e,11)^rr(e,25))+((e&f)^(~e&g))+k[i]+w[i])>>>0;
      const t2=((rr(a,2)^rr(a,13)^rr(a,22))+((a&b)^(a&c)^(b&c)))>>>0;
      j=g;g=f;f=e;e=(d+t1)>>>0;d=c;c=b;b=a;a=(t1+t2)>>>0;
    }
    [a,b,c,d,e,f,g,j].forEach((value,i)=>h[i]=(h[i]+value)>>>0);
  }
  const output=new Uint8Array(32), result=new DataView(output.buffer);
  h.forEach((value,i)=>result.setUint32(i*4,value));
  return output;
}
const token='__CSRF__';
let hash='';
const status=document.getElementById('status'), apply=document.getElementById('apply');
function report(text){status.textContent=text}
async function action(name) {
  if(name==='apply'&&!confirm('Install verified firmware and reboot?')) return;
  try {
    const r=await fetch('/ota/'+name,{method:'POST',headers:{
      'X-CSRF-Token':token,'X-Firmware-SHA256':hash}});
    report(await r.text()); apply.disabled=true;
    if(!r.ok) throw Error('Request rejected: '+r.status);
  } catch(e) { report(String(e)); }
}
document.getElementById('upload').onclick=async()=>{
  try {
    const file=document.getElementById('image').files[0];
    if(!file||file.size<8||file.size>999424) throw Error('Choose a raw application BIN, 8-999424 bytes');
    apply.disabled=true;report('Hashing...');
    const bytes=await file.arrayBuffer();
    const digest=globalThis.crypto&&crypto.subtle?
      new Uint8Array(await crypto.subtle.digest('SHA-256',bytes)):sha256Fallback(bytes);
    hash=Array.from(digest,x=>x.toString(16).padStart(2,'0')).join('');
    report('Uploading and verifying...');
    const response=await fetch('/ota/upload',{method:'POST',headers:{
      'X-CSRF-Token':token,'X-Firmware-SHA256':hash,'Content-Type':'application/octet-stream'},body:bytes});
    report(await response.text());apply.disabled=!response.ok;
  } catch(e){report(String(e));}
};
apply.onclick=()=>action('apply');
document.getElementById('abort').onclick=()=>action('abort');
</script></body></html>)HTML";
