from pathlib import Path
p=Path('.slide-build/build.mjs');s=p.read_text(encoding='utf8');a=s.index(' line(sl,[48,658]');b=s.index('\n}',a)
s=s[:a]+''' line(sl,[48,658],[99,658],blue,3,'solid',0);
 const tree=d.phase==='Construção da árvore',dfs=d.phase==='Ordem inicial';
 txt(sl,tree?'Arestas da MST':dfs?'Rota em pré-ordem':'Caminhão',110,645,220,30,20);
 if(dfs){line(sl,[351,658],[402,658],'#D4DCE6',2,'solid',0);txt(sl,'MST de referência',414,645,260,30,20);}
 else if(!tree){line(sl,[269,658],[320,658],orange,4,'dotted',0);txt(sl,'Drone em voo',330,645,180,30,20);}
 txt(sl,d.state?'Nós: branco Simple, verde Combined, azul Truck, laranja Drone. Dourado destaca a alteração.':'Contorno dourado: nó alterado. Posições esquemáticas fixas.',48,687,1100,22,15,'#64748B');'''+s[b:]
s=s.replace("d.title.replace(' • ', ' · ')","d.title.replace(' • ', ': ')");p.write_text(s,encoding='utf8')
p=Path('.slide-build/finalize.mjs');s=p.read_text(encoding='utf8').replace('Greedy_Ulysses16_passo_a_passo.pptx','Greedy_Ulysses16.pptx');p.write_text(s,encoding='utf8')
