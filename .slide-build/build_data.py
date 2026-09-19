import json,math
from pathlib import Path
lines=Path('.slide-build/trace.txt').read_text().splitlines();groups={'INITIAL':[],'FINAL':[]};mst=[];accepted=[];g=None;i=0
while i<len(lines):
 a=lines[i].split();i+=1
 if not a:continue
 if a[0]=='MST':mst.append(list(map(int,a[1:])))
 elif a[0] in ['INITIAL','FINAL']:g=groups[a[0]]
 elif a[0]=='ACCEPT':
  accepted.append({'before':int(a[1]),'after':int(a[2]),'route':list(map(int,lines[i].split())),'states':[]});i+=1
 elif a[0]=='ACCEPT_PARTITION':g=accepted[-1]['states']
 elif a[0]=='STATE':
  v=list(map(int,a[1:]));r,l,b=[list(map(int,lines[i+k].split())) for k in range(3)];i+=3
  g.append(dict(zip(['op','pos','saving','cost','truck','drone','simple'],v),route=r,labels=l,bounds=b))
coords=[]
for line in Path('data/descompressed/ulysses16/ulysses16.tsp').read_text().splitlines()[7:23]:coords.append(list(map(float,line.split()))[1:])
def dist(a,b):
 if a==b:return 0
 c=lambda z:math.pi*(math.trunc(z)+5*(z-math.trunc(z))/3)/180
 x,y=map(c,coords[a-1]);u,v=map(c,coords[b-1]);return math.ceil(6378.388*math.acos(max(-1,min(1,math.sin(x)*math.sin(u)+math.cos(x)*math.cos(u)*math.cos(y-v)))))
def edges(s):
 r,l,b=s['route'],s['labels'],s['bounds'];truck=[x for x,t in zip(r,l) if t!=3];te=list(map(list,zip(truck,truck[1:])));de=[]
 for start,end in zip(b,b[1:]):
  dr=[k for k in range(start+1,end) if l[k]==3]
  if dr:de.extend([[r[start],r[dr[0]]],[r[dr[0]],r[end]]])
 return te,de
for s in groups['INITIAL']+groups['FINAL']+sum([a['states'] for a in accepted],[]):
 te,de=edges(s);assert sum(dist(*e) for e in te)==s['truck'];assert sum(dist(*e) for e in de)==s['drone']
slides=[]
def add(title,phase,body,s=None,**kw):
 d=dict(title=title,phase=phase,body=body,**kw)
 if s:d.update(state=s,truckEdges=edges(s)[0],droneEdges=edges(s)[1])
 slides.append(d)
def path(r):return ' – '.join(map(str,r))
add('Greedy heuristic\nUlysses16','cover',[])
add('A instância e a representação','Entrada',['16 vértices, com o vértice 1 como depósito. O leitor cria as 120 arestas do grafo completo.','O código usa distâncias GEO arredondadas para cima. Os custos exibidos são unidades do código.','Os nós ficam em posições fixas no diagrama. O desenho é esquemático, sem escala geográfica.'],edges=[],highlight=[1])
for k,e in enumerate(mst):
 add(f'Kruskal {k+1:02d} • aresta {e[0]}–{e[1]}','Construção da árvore',[f'Aceita a aresta {e[0]}–{e[1]}, de peso {e[2]}, porque conecta componentes diferentes.',f'A árvore acumula {k+1} de 15 arestas. Arestas que fecham ciclos são descartadas.',('Com 15 arestas, Kruskal encerra a construção.' if k==14 else 'A próxima decisão considera a ordem crescente de peso.')],edges=[x[:2] for x in mst[:k+1]],highlight=e[:2],metric=f'Peso da árvore: {sum(x[2] for x in mst[:k+1])}',note='src/agatz_solution/kruskal/kruskal.cpp: Kruskal::kruskal e _findUnion. Mostram-se as inserções aceitas.')
r=groups['INITIAL'][0]['route'][:-1]
for k,n in enumerate(r):
 add(f'DFS {k+1:02d} • visita ao vértice {n}','Ordem inicial',[f'A DFS marca {n} como visitado e o acrescenta à pré-ordem.', 'A busca parte de 1 e visita os vizinhos em ordem crescente. Ao terminar um ramo, retorna pela árvore.',f'Pré-ordem até aqui:\n{path(r[:k+1])}'],edges=list(map(list,zip(r[:k+1],r[1:k+1]))),backgroundEdges=[x[:2] for x in mst],highlight=[n],metric=f'Visitados: {k+1}/16',note='src/agatz_solution/dfs/dfs.cpp: _getAdjacencyList, _recurseDFS. Linhas azuis ligam visitas consecutivas da rota, não a pilha de deslocamentos da DFS.')
add('Fechamento da rota inicial','partitionRoute',['Repete o depósito no final da pré-ordem e fecha o ciclo.', 'Os extremos recebem Combined. As 15 posições internas começam como Simple.',f'Rota fechada:\n{path(r+[1])}'],groups['INITIAL'][0])
add('Custo e escolha do candidato','Regras da partição',['Cada segmento custa max(caminhão, drone). O custo total soma os segmentos entre fronteiras.','A fila prioriza a maior economia. Em empate: MakeFly, PushLeft, PushRight e menor posição.','O laço segue enquanto houver Simple. Economia zero também é aceita.'],groups['INITIAL'][0],note='greedy_heuristic.cpp: segmentCost, isBetterCandidate, partitionRoute. A mesma matriz de distâncias vale para ambos os veículos, sem fator de velocidade ou limite de autonomia.')
def step(s,prev,k,phase):
 n=s['route'][s['pos']];op=['MakeFly','PushLeft','PushRight','Combined'][s['op']]
 if s['op']==0:
  left,right=s['route'][s['pos']-1],s['route'][s['pos']+1]
  body=[f'O cliente {n} passa de Simple para Drone. {left} e {right} tornam-se Combined.',f'O caminhão desvia de {n}. O drone voa {left} – {n} – {right}.']
 elif s['op']==1:
  old=s['route'][s['pos']-1]
  body=[f'{n} passa de Simple para Combined. O antigo encontro, em {old}, passa a Truck.',f'PushLeft estende a operação à esquerda de {n}. O drone passa a reencontrar o caminhão em {n}.']
 else:body=[f'Aplica {op} no vértice {n}.']
 body.append(f'Custo: {prev["cost"]} − {s["saving"]} = {s["cost"]}.\nRestam {s["simple"]} posições Simple.')
 if s['saving']==0:body.append('O movimento muda a partição mesmo sem reduzir o custo neste passo.')
 add(f'{k:02d} • {op} no vértice {n}',phase,body,s,highlight=[n],note='greedy_heuristic.cpp: PartitionState::apply, savingFor'+op+', refreshCandidatesAround. Após aplicar, invalida e recalcula candidatos em raio 5. isCurrentCandidate descarta versões antigas. Índice da posição: '+str(s['pos']))
for k in range(1,len(groups['INITIAL'])):step(groups['INITIAL'][k],groups['INITIAL'][k-1],k,'Primeira partição')
add('Busca local sobre a ordem das visitas','iterativeImprovement',['A primeira partição custa 7.668. O código testa trocas, inversões 2-opt e realocações.','Em Ulysses16, avalia os 420 movimentos de cada rodada e particiona cada rota candidata do zero.','Aceita a melhor redução estrita da rodada. Rotas apenas testadas não substituem a solução corrente.'],groups['INITIAL'][-1])
previous=r
for k,a in enumerate(accepted):
 new=a['route'];desc=''
 # Match the exact first neighborhood that could produce the chosen route.
 for f in range(1,15):
  for t in range(f+1,16):
   x=previous[:];x[f],x[t]=x[t],x[f]
   if x==new and not desc:desc=f'Troca os clientes {previous[f]} e {previous[t]} (2-point move).'
 for f in range(1,15):
  for t in range(f+1,16):
   x=previous[:];x[f:t+1]=reversed(x[f:t+1])
   if x==new and not desc:desc=f'Inverte a subsequência {path(previous[f:t+1])} (2-opt).'
 for f in range(1,16):
  for t in range(1,16):
   x=previous[:];v=x.pop(f);x.insert(t,v)
   if x==new and not desc:desc=f'Realoca o cliente {v} da posição {f} para a posição {t} (1-point move).'
 add(f'Melhoria {k+1} • custo {a["after"]}','Melhoria aceita',[desc,f'A partição da nova ordem reduz o custo de {a["before"]} para {a["after"]}.',f'Nova ordem, antes do fechamento:\n{path(new)}'],a['states'][-1],note='iterativeImprovement: atualização de currentRoute após avaliar todas as três vizinhanças. O grafo mostra a partição completa da nova ordem, não a rota puramente terrestre.')
 previous=new
add('Última rodada e partição final','Encerramento da busca',['A sétima rodada testa 420 movimentos e não encontra custo menor que 4.653.','A busca encerra após seis melhorias aceitas. _greedyHeuristic chama partitionRoute novamente.','A partição reinicia com a ordem melhorada. Seu ciclo somente de caminhão custa 9.227.'],groups['FINAL'][0])
for k in range(1,len(groups['FINAL'])):step(groups['FINAL'][k],groups['FINAL'][k-1],k,'Construção da solução final')
s=groups['FINAL'][-1];tr=[n for n,l in zip(s['route'],s['labels']) if l!=3]
add('Solução retornada pelo código','Resultado',[f'Caminhão:\n{path(tr)}','Drone em voo: 1 – 11 – 12.\nNo trecho 12 – 1, segue embarcado.', 'Custo total: 4.653.\nPercurso do caminhão: 4.653.\nPercurso aéreo do drone: 4.152.'],s,note='Solution(truckRoute, droneRoute, total.duration, total.truck, total.drone). Vetor caminhão omite depósito repetido. Vetor drone [1,11,12] omite repetição. Segmento 1..12 tem caminhão 4235 e drone 4152, duração 4235. Retorno 12..1 custa 418.')
add('O que explica o resultado','Leitura da solução',['O primeiro MakeFly não economiza tempo. Os PushLeft seguintes prolongam o trabalho em paralelo.','O caminhão atende 14 clientes e o drone atende o 11. O caminhão espera apenas quando seu trecho é menor que o voo.','A busca reduz o custo da primeira partição de 7.668 para 4.653 (39,3%). Esse resultado não certifica ótimo global.'],s)
Path('.slide-build/data.json').write_text(json.dumps(dict(slides=slides,mst=mst,initial=groups['INITIAL'],final=groups['FINAL'],accepted=accepted),ensure_ascii=False),encoding='utf8')
print('SLIDES',len(slides))
