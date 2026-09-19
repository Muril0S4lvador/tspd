import fs from 'node:fs/promises';
import path from 'node:path';
import {Presentation,PresentationFile} from '@oai/artifact-tool';
const root='C:/Users/dalmo/ufes/10_periodo/tcc';
const data=JSON.parse(await fs.readFile(root+'/.slide-build/data.json','utf8'));
const proto=JSON.parse(await fs.readFile(root+'/.slide-build/template.json','utf8'));
const source=structuredClone(proto.slides[14]); const cover=structuredClone(proto.slides[0]);
proto.slides=data.slides.map((d,i)=>{
 const s=structuredClone(i===0?cover:source);s.id='greedy'+i;s.index=i;delete s.creationId;
 if(i>0)s.elements=s.elements.filter(e=>e.id==='533'||e.placeholderType==='sldNum'||e.placeholderType==='ftr');
 return s;
});
const p=Presentation.load(proto);
const font='Helvetica Neue';const blue='#175CD4',navy='#08284F',orange='#C45D0A',teal='#407879';
function txt(sl,text,x,y,w,h,size=24,color=navy,bold=false){const t=sl.shapes.add({geometry:'textbox',position:{left:x,top:y,width:w,height:h},fill:'none',line:{fill:'none',width:0}});t.text=text;t.text.style={typeface:font,fontSize:size,color,bold,autoFit:'none',insets:{top:0,left:0,bottom:0,right:0}};return t;}
function line(sl,a,b,color,width=3,style='solid',trim=22){let[x,y]=a,[u,v]=b;let len=Math.hypot(u-x,v-y);if(!len)return;let dx=(u-x)/len,dy=(v-y)/len;x+=dx*trim;y+=dy*trim;u-=dx*trim;v-=dy*trim;
 if(trim>0){
  const blocked=Object.values(coords).some(([cx,cy])=>{
   if(Math.hypot(cx-a[0],cy-a[1])<1||Math.hypot(cx-b[0],cy-b[1])<1)return false;
   const t=Math.max(0,Math.min(1,((cx-x)*(u-x)+(cy-y)*(v-y))/((u-x)**2+(v-y)**2)));
   return Math.hypot(cx-x-t*(u-x),cy-y-t*(v-y))<29;
  });
  if(blocked){const mx=(x+u)/2,my=(y+v)/2,k=Math.hypot(398-mx,388-my);const bend=[mx+(398-mx)/k*42,my+(388-my)/k*42];line(sl,[x,y],bend,color,width,style,0);line(sl,bend,[u,v],color,width,style,0);return;}
 }
 return sl.shapes.add({geometry:'custom',position:{left:Math.min(x,u),top:Math.min(y,v),width:Math.max(0.1,Math.abs(u-x)),height:Math.max(0.1,Math.abs(v-y))},fill:'none',line:{fill:color,width,style},customPaths:[{width:Math.max(.1,Math.abs(u-x)),height:Math.max(.1,Math.abs(v-y)),commands:[{moveTo:{x:x-Math.min(x,u),y:y-Math.min(y,v)}},{lineTo:{x:u-Math.min(x,u),y:v-Math.min(y,v)}}]}]});}
// Fixed schematic positions follow the final order, avoiding crossings in the final truck route.
const order=data.final[0].route.slice(0,-1);const coords={};order.forEach((id,i)=>{let t=-Math.PI/2+i*Math.PI/8;coords[id]=[398+300*Math.cos(t),388+224*Math.sin(t)]});
function graph(sl,d){
 for(const e of d.backgroundEdges??[])line(sl,coords[e[0]],coords[e[1]],'#D4DCE6',1.6);
 for(const e of d.edges??d.truckEdges??[])line(sl,coords[e[0]],coords[e[1]],blue,3);
 for(const e of d.droneEdges??[])line(sl,coords[e[0]],coords[e[1]],orange,4,'dotted');
 const labels={};if(d.state)d.state.route.slice(0,-1).forEach((id,i)=>labels[id]=d.state.labels[i]);
 for(let id=1;id<=16;id++){
  const [x,y]=coords[id];let label=labels[id]??0;let color=['#FFFFFF',teal,blue,orange][label];if(id===1)color=navy;
  if((d.highlight??[]).includes(id))sl.shapes.add({geometry:'ellipse',position:{left:x-27,top:y-27,width:54,height:54},fill:'none',line:{fill:'#D7A13B',width:3}});
  const n=sl.shapes.add({geometry:id===1?'rect':'ellipse',name:'Vértice '+id,position:{left:x-21,top:y-21,width:42,height:42},fill:color,line:{fill:label===0?'#7E91A8':color,width:2}});
  n.text=String(id);n.text.style={typeface:font,fontSize:23,bold:true,color:color==='#FFFFFF'?navy:'#FFFFFF',alignment:'center',verticalAlignment:'middle',insets:{top:0,left:0,right:0,bottom:0}};
 }
 line(sl,[48,658],[99,658],blue,3,'solid',0);
 const tree=d.phase==='Construção da árvore',dfs=d.phase==='Ordem inicial';
 txt(sl,tree?'Arestas da MST':dfs?'Rota em pré-ordem':'Caminhão',110,645,220,30,20);
 if(dfs){line(sl,[351,658],[402,658],'#D4DCE6',2,'solid',0);txt(sl,'MST de referência',414,645,260,30,20);}
 else if(!tree){line(sl,[269,658],[320,658],orange,4,'dotted',0);txt(sl,'Drone em voo',330,645,180,30,20);}
 txt(sl,d.state?'Nós: branco Simple, verde Combined, azul Truck, laranja Drone. Dourado destaca a alteração.':'Contorno dourado: nó alterado. Posições esquemáticas fixas.',48,687,1100,22,15,'#64748B');
}
for(let i=0;i<data.slides.length;i++){
 const sl=p.slides.items[i],d=data.slides[i];
 if(i===0){
  const repl={'Title 3':'Greedy heuristic\nUlysses16'};
  const els=sl.shapes.items;
  for(const e of els){if(e.name==='Title 3'){e.text=repl[e.name];e.text.style={typeface:font,fontSize:83,color:'#EDF5FE'};} }
  // Update existing cover placeholders while preserving the template geometry.
  const cp=sl.toProto?null:null;
  const texts=els.filter(e=>e.text?.toString?.());
  for(const e of els){const s=e.text?.toString?.()??'';if(s.includes('Week ending')){e.text='Construção passo a passo';e.text.style={typeface:font,fontSize:24,color:'#EDF5FE'};}else if(s.includes('Prepared by')){e.text='Caminhão e drone';e.text.style={typeface:font,fontSize:24,color:'#EDF5FE'};}else if(s==='Q2 2026'){e.text='TSPD';}else if(s==='Company Name'){e.text='Algoritmos de otimização';}}
 }else{
  const title=sl.shapes.items.find(e=>e.name==='Google Shape;533;p58');title.text=d.title.replace(' • ', ': ');title.text.style={typeface:font,fontSize:38,color:navy,insets:{top:0,left:0,right:0,bottom:0}};title.position={left:41.33,top:36.1,width:1197.33,height:58};
  for(const e of sl.shapes.items){if(e.name?.startsWith('Slide Number'))e.text=String(i+1);}
  txt(sl,d.phase.toUpperCase(),43,111,1170,30,19,blue,true);
  graph(sl,d);
  let y=177;
  for(const text of d.body){const approxLines=text.split('\n').reduce((n,l)=>n+Math.max(1,Math.ceil(l.length/36)),0);let h=approxLines*29+10;txt(sl,text,812,y,420,h,23);y+=h+18;}
  if(d.state)txt(sl,`Custo total: ${d.state.cost.toLocaleString('pt-BR')}`,812,627,420,38,29,blue,true);
  else if(d.metric)txt(sl,d.metric,812,627,420,38,27,blue,true);
 }
 const notes=(d.note??'Execução do código local na instância data/descompressed/ulysses16/ulysses16.tsp.')+'\n'+(d.state?JSON.stringify(d.state):'')+'\nOs diagramas usam disposição esquemática fixa, sem escala. Traçado sólido: caminhão, incluindo drone embarcado. Pontilhado: apenas voos. Rótulos: Simple branco, Combined verde, Truck azul, Drone laranja. Depósito 1 em quadrado.';
 sl.speakerNotes.textFrame.setText(notes);
}
await fs.writeFile(root+'/.slide-build/deck.json',JSON.stringify(p.toProto()));
await(await PresentationFile.exportPptx(p)).save(root+'/.slide-build/candidate.pptx');
console.log('EXPORTED',p.slides.items.length);
