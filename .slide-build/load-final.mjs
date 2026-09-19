import fs from 'node:fs/promises';import {FileBlob,PresentationFile} from '@oai/artifact-tool';
const p=await PresentationFile.importPptx(await FileBlob.load('C:/Users/dalmo/ufes/10_periodo/tcc/outputs/Greedy_Ulysses16_Final.pptx'));
await fs.writeFile('.slide-build/deck-final.json',JSON.stringify(p.toProto()));
