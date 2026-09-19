import fs from 'node:fs/promises';
import {Presentation} from '@oai/artifact-tool';
const root='C:/Users/dalmo/ufes/10_periodo/tcc/.slide-build';
const proto=JSON.parse(await fs.readFile(root+'/deck-final.json','utf8'));
const start=Number(process.argv[2]),end=Math.min(start+5,proto.slides.length);
proto.slides=proto.slides.slice(start,end);
const p=Presentation.load(proto);
for(let i=0;i<p.slides.items.length;i++){const slide=p.slides.items[i];const b=await p.export({slide,format:'png',scale:1});await fs.writeFile(`${root}/slide-${String(start+i+1).padStart(2,'0')}.png`,new Uint8Array(await b.arrayBuffer()));}
console.log(start+1,end);

