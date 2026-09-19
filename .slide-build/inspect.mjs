import fs from 'node:fs/promises';
import {PresentationFile,FileBlob} from '@oai/artifact-tool';
const p=await PresentationFile.importPptx(await FileBlob.load('C:/Users/dalmo/.codex/plugins/cache/openai-curated-remote/openai-templates/0.1.1/skills/artifact-template-operating-review/assets/reference.pptx'));
await fs.writeFile('.slide-build/template.json',JSON.stringify(p.toProto()));
console.log((await p.inspect({kind:'slide,textbox,shape',maxChars:16000})).ndjson);
for(let i=0;i<p.slides.items.length;i++){const b=await p.export({slide:p.slides.items[i],format:'png',scale:0.7});await fs.writeFile(`.slide-build/template-${i}.png`,new Uint8Array(await b.arrayBuffer()));}
