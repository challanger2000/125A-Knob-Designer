import fs from 'node:fs';
import path from 'node:path';
import { chromium } from 'playwright';

const webRoot=process.cwd();
const repoRoot=path.resolve(webRoot,'../..');
const examples=path.resolve(repoRoot,'designer-v2/examples');
const outDir=path.resolve(repoRoot,'generated-assets/mechamorph-v0.1.0');
fs.mkdirSync(outDir,{recursive:true});

const cases=[
  ['main','mechamorph-main.125agui.json'],
  ['scale','mechamorph-scale.125agui.json'],
  ['utility','mechamorph-utility.125agui.json'],
  ['machine','mechamorph-machine.125agui.json']
];

const browser=await chromium.launch({
  headless:true,
  args:['--use-angle=swiftshader','--enable-webgl','--ignore-gpu-blocklist']
});
const page=await browser.newPage({viewport:{width:1200,height:1000}});
await page.goto('http://127.0.0.1:5173',{waitUntil:'networkidle'});

async function prepare(project){
  await page.evaluate(async p=>{
    const {KnobPreviewRenderer}=await import('/src/renderer.js');
    window.__headlessRenderer?.dispose?.();
    document.body.innerHTML='';
    document.body.style.margin='0';
    document.body.style.background='transparent';
    const canvas=document.createElement('canvas');
    canvas.id='headless-canvas';
    canvas.style.setProperty('width',p.output.frameWidth+'px','important');
    canvas.style.setProperty('height',p.output.frameHeight+'px','important');
    canvas.style.setProperty('min-width','0','important');
    canvas.style.setProperty('min-height','0','important');
    canvas.style.setProperty('max-width','none','important');
    canvas.style.setProperty('max-height','none','important');
    canvas.style.display='block';
    document.body.appendChild(canvas);
    const renderer=new KnobPreviewRenderer(canvas);
    window.__headlessRenderer=renderer;
    window.__headlessProject=p;
    renderer.update(p);
    renderer.setPreviewAngle((p.output.startAngle+p.output.endAngle)*0.5);
    await new Promise(resolve=>requestAnimationFrame(()=>requestAnimationFrame(resolve)));
  },project);
}

async function renderStrip(project,fileName){
  const downloadPromise=page.waitForEvent('download',{timeout:180000});
  await page.evaluate(async ({p,fileName})=>{
    const renderer=window.__headlessRenderer;
    renderer.update(p);
    const blob=await renderer.exportFilmstrip(p);
    const a=document.createElement('a');
    const url=URL.createObjectURL(blob);
    a.href=url;
    a.download=fileName;
    document.body.appendChild(a);
    a.click();
    a.remove();
    setTimeout(()=>URL.revokeObjectURL(url),1000);
  },{p:project,fileName});
  const download=await downloadPromise;
  await download.saveAs(path.join(outDir,fileName));
}

const manifest={generatedAt:new Date().toISOString(),renderer:'125A GUI Designer v0.2.0 / Three.js',controls:{}};

for(const [id,file] of cases){
  const project=JSON.parse(fs.readFileSync(path.join(examples,file),'utf8'));
  await prepare(project);
  await page.locator('#headless-canvas').screenshot({
    path:path.join(outDir,`${id}-preview.png`),
    omitBackground:true
  });

  const outputs=[];
  for(const scale of project.output.scaleExports){
    const p=structuredClone(project);
    p.output.frameWidth=Math.round(project.output.frameWidth*scale);
    p.output.frameHeight=Math.round(project.output.frameHeight*scale);
    await prepare(p);
    const scaleName=String(scale).replace('.','p');
    const fileName=`${id}_${p.output.frameWidth}px_${p.output.frameCount}f_scale-${scaleName}x.png`;
    await renderStrip(p,fileName);
    const stat=fs.statSync(path.join(outDir,fileName));
    outputs.push({
      scale,
      frameWidth:p.output.frameWidth,
      frameHeight:p.output.frameHeight,
      frameCount:p.output.frameCount,
      layout:p.output.layout,
      bytes:stat.size,
      file:fileName
    });
  }
  manifest.controls[id]={
    source:file,
    baseFrame:[project.output.frameWidth,project.output.frameHeight],
    frameCount:project.output.frameCount,
    startAngle:project.output.startAngle,
    endAngle:project.output.endAngle,
    outputs
  };
}

fs.writeFileSync(path.join(outDir,'manifest.json'),JSON.stringify(manifest,null,2)+'\n');
await browser.close();
console.log(JSON.stringify(manifest,null,2));
