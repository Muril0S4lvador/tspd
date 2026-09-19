from pathlib import Path
p=Path('.slide-build/build.mjs');s=p.read_text(encoding='utf8');anchor=' return sl.shapes.add({geometry:'
idx=s.index(anchor,s.index('function line('))
s=s[:idx]+''' if(trim>0){
  const blocked=Object.values(coords).some(([cx,cy])=>{
   if(Math.hypot(cx-a[0],cy-a[1])<1||Math.hypot(cx-b[0],cy-b[1])<1)return false;
   const t=Math.max(0,Math.min(1,((cx-x)*(u-x)+(cy-y)*(v-y))/((u-x)**2+(v-y)**2)));
   return Math.hypot(cx-x-t*(u-x),cy-y-t*(v-y))<29;
  });
  if(blocked){const mx=(x+u)/2,my=(y+v)/2,k=Math.hypot(398-mx,388-my);const bend=[mx+(398-mx)/k*42,my+(388-my)/k*42];line(sl,[x,y],bend,color,width,style,0);line(sl,bend,[u,v],color,width,style,0);return;}
 }
'''+s[idx:]
p.write_text(s,encoding='utf8')
p=Path('.slide-build/finalize.mjs');s=p.read_text(encoding='utf8').replace('Greedy_Ulysses16.pptx','Greedy_Ulysses16_Final.pptx').replace('validation-v2.json','validation-v3.json');p.write_text(s,encoding='utf8')
p=Path('.slide-build/load-final.mjs');s=p.read_text(encoding='utf8').replace('Greedy_Ulysses16.pptx','Greedy_Ulysses16_Final.pptx');p.write_text(s,encoding='utf8')
