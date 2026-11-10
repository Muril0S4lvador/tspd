# HGVNS adaptado ao TSP-D do projeto

Referencia: de Freitas e Penna, *A Variable Neighborhood Search for Flying Sidekick
Traveling Salesman Problem*, PDF de 2018, algoritmos 1--4 e secoes 4.1--4.8.

Esta implementacao utiliza o TSP compartilhado de `TSPSolver`, por Kruskal e DFS,
em lugar do TSP exato usado no artigo. Nao chama Greedy nem ExactPartitioning.
E uma implementacao adaptada e documentada, nao uma reproducao dos resultados
publicados. As decisoes abaixo preenchem detalhes nao especificados pelo PDF.

## Modelo e custo

- Um caminhao e um drone, com todos os clientes elegiveis, autonomia ilimitada e
  tempos de preparacao/recolhimento nulos, usando o modelo atual do repositorio.
- Os fatores de caminhao e drone multiplicam a distancia, conforme os demais
  algoritmos do projeto. O modulo nao altera a interpretacao existente dos dados.
- As distancias sao simetricas e compartilhadas pelos dois veiculos.
- Clientes usam IDs zero-based, e o deposito e 0. A rota do caminhao e fechada.
- As extremidades de cada voo sao POSICOES na rota do caminhao, e o cliente
  atendido e um ID. Isso distingue partida e retorno ao deposito.
- Conforme a variante TSP-D, um voo pode retornar ao mesmo ponto de lancamento,
  inclusive enquanto o caminhao permanece parado. Voos nesse ponto ocorrem em
  sequencia. Nao e permitido relancar depois da ultima ocorrencia do deposito.
- Cada cliente recebe exatamente uma entrega. Lancamentos e reencontros sao
  pontos do caminhao; viagens nao podem se sobrepor nem ficar aninhadas.

Para cada voo, calcula-se seu tempo aereo e o tempo do caminhao entre seus
extremos. O custo total e a soma do percurso completo do caminhao com as esperas
`max(0, tempoDrone - tempoCaminhaoDoTrecho)`. A validade dos intervalos garante
que essas esperas possam ser somadas. `truck_cost` e `drone_cost` representam
apenas deslocamentos; `total_cost` inclui a sincronizacao.

O leitor atual nao fornece autonomia, elegibilidade ou tempos de servico para
este modulo. Esta implementacao nao deve ser apresentada como suporte a todas
as restricoes do FSTSP ou de conjuntos de instancias restritas.

## Construcao, vizinhancas e controle

A construcao inicial procura a maior economia positiva entre criar um voo em
uma subrota livre e reinserir um cliente nao misto em uma subrota com voo.
Termina quando nao existe melhoria positiva; essa explicita o `until Stop`
nao detalhado no Algoritmo 2. Empates sao resolvidos de forma deterministica.

As sete vizinhancas seguem a ordem de apresentacao no PDF: Reinsertion, Or-opt2,
Exchange, Exchange(2,1), Exchange(2,2), 2-opt e Relocate Customer. Os blocos das
trocas preservam a ordem interna. A 2-opt exclui duas arestas adjacentes, inclusive
a simples inversao do ciclo inteiro. Relocate Customer transfere um cliente
nao misto de uma subrota livre, com lancamento e reencontro nessa mesma subrota.
O cliente retirado nao precisa estar entre os extremos na ordem anterior.

O GVNS sorteia um movimento viavel, mesmo que piore o custo, e aplica RVND. Aceita
o resultado se melhorar a incumbente e reinicia na primeira vizinhanca; caso
contrario, avanca ate esgotar as sete. A RVND explora a melhor melhoria da
vizinhanca, embaralhando e reiniciando a lista depois de melhorar. Melhorias
devem superar `1e-9 * max(1, abs(custoAtual))` para evitar ciclos numericos.

Nao ha corte por numero de movimentos, clientes, iteracoes ou tempo. Limites
inferiores de custo descartam apenas candidatos que nao podem melhorar a melhor
solucao da varredura. Na criacao de voos, atingir espera zero prova o menor
custo para aquele cliente e permite encerrar seus outros pares de extremos.
As perturbacoes sorteiam movimentos gerados viaveis; a selecao nao filtra por
melhoria. Para o caminhao, sorteiam primeiro um movimento cru uniforme e tentam
seu ajuste. Se ele falhar, usam reservatorio entre todos os movimentos viaveis.
Essa mistura preserva o sorteio uniforme dos movimentos viaveis. Novas viagens
usam contagem dos pares admissiveis, sem guardar toda a vizinhanca.

## Politica de ajuste de viagens: decisao de implementacao

O artigo mostra inversoes e mudancas de extremos nas figuras 3--8, mas nao
fornece uma regra completa para todos os movimentos. A politica aqui e:

1. Remapear os extremos por ID, preservando as duas ocorrencias do deposito.
2. Se os extremos trocaram de ordem, inverter a viagem. A simetria das distancias
   permite essa inversao. Se nao houver conflitos, preservar esses extremos.
3. Identificar as viagens cujos novos intervalos se sobrepoem. Para cada uma,
   considerar os extremos remapeados e suas POSICOES anteriores na rota.
4. Para as viagens em conflito, avaliar os pares ordenados desse conjunto finito
   de posicoes. Manter os clientes atribuidos ao drone e os extremos das demais
   viagens, na ordem determinada pelos intervalos remapeados.
5. Escolher a combinacao viavel de menor espera por uma recorrencia sobre essas
   alternativas. Empates seguem a ordem deterministica das opcoes. Se nao houver
   combinacao viavel, descartar o movimento.

Essa recorrencia ajusta os extremos das viagens existentes; nao e um
particionamento exato global, nao reatribui os clientes entre veiculos e nao
representa uma regra integral especificada pelos autores. Ela tambem nao
enumera todos os extremos possiveis de toda a rota. A vizinhanca desta adaptacao
e definida pela politica acima, e os testes verificam a melhor melhoria dentro
dela. A semente afeta a perturbacao e a ordem das vizinhancas, nao esse reparo.

## Uso e resultados

```powershell
.\main.exe caminho\instancia.txt --no-graphics --algorithms hgvns --seed 42
.\main.exe caminho\instancias --algorithms "tsp,greedy,exact-partitioning,hgvns" --seed 42
```

```cpp
tspd::hgvns::HGVNS hgvns(42);
const auto resultado = hgvns.Solve(instance, tspSolution);
```

O padrao da semente e 0. O gerador e reiniciado a cada chamada, permitindo repetir
rotas com a mesma semente e o mesmo executavel. O executor imprime a semente;
o cabecalho do CSV permanece inalterado. O tempo `solve_seconds` do HGVNS exclui
o TSP compartilhado, como nas outras heuristicas.

O resultado e uma `Solution` com rota do caminhao, percurso do drone e custos.
Para `tsp_cost`, cada cliente aereo e inserido imediatamente apos seu lancamento
na sequencia do caminhao, em ordem de voo. Trata-se de uma linearizacao completa
e fechada para comparacao; o TSP de partida continua na linha separada `tsp`.
Essa convencao nao afirma que exista um unico TSP associado a uma solucao HGVNS.

Chamadas sem TSP, com rota/custo inconsistentes ou fatores invalidos lancam
excecoes. O executor registra a falha no CSV e continua a rodada.

## PD por blocos e multiplas ordens

O executor mantem somente `hgvns-pd-waiting-rvnd`: selecao pela espera e
reotimizacao depois de cada RVND, antes de aceitar o candidato no GVNS.
As outras sete combinacoes foram retiradas da CLI e do CSV de analise.
Os tipos e funcoes internos das experiencias permanecem disponiveis na API
para permitir os testes e reproducao das experiencias anteriores.

As operacoes atomicas sao voos completos (incluindo voos estacionarios) e arestas
do caminhao sem voo. Em ordem de percurso, agrupam-se operacoes completas em
blocos disjuntos, adicionando a proxima enquanto a uniao tem ate 20 IDs distintos.
Uma operacao individual acima desse limite e preservada e nao e candidata.
O deposito repetido conta como um ID. Extremos compartilhados representam um
mesmo evento de sincronizacao e podem pertencer a dois blocos consecutivos.

As duas politicas usam esse mesmo conjunto de candidatos e selecionam exatamente
min(5, quantidade de candidatos) blocos. Waiting ordena pela soma de
abs(tempoDrone - tempoCaminhao) de cada voo: inclui espera de ambos os veiculos,
nao apenas a espera acrescentada ao custo do caminhao. Empates seguem a ordem
dos blocos. Random embaralha os candidatos com um gerador separado, reiniciado
com a mesma semente; a selecao nao altera o percurso aleatorio do HGVNS.
Sem voos, todos os escores sao zero, e Waiting segue a ordem de percurso.

Cada bloco gera uma sequencia completa: extremo inicial, clientes internos do
caminhao na ordem de visita com clientes aereos inseridos apos o lancamento,
e extremo final. Os extremos sao reservados e cada cliente interno aparece uma
unica vez; voos estacionarios no extremo final sao inseridos antes desse extremo.
Essa conversao pode modificar a ordem implicita da solucao e nao garante melhoria.

A PD considera arestas exclusivamente terrestres e operacoes com um cliente
aereo interno, minimizando a soma de max(tempoCaminhao, tempoDrone). Cada chamada
de particionamento resolve uma ordem fixa e usa O(m^3) tempo e O(m) memoria
auxiliar para um bloco linearizado com m posicoes; com 20 IDs distintos, a
sequencia tem no maximo 21 posicoes quando os extremos coincidem. Reconstrucao
preserva os extremos e converte voos com extremos iguais para uma unica posicao
do caminhao. O bloco original e mantido quando nao ha melhoria estrita.
Se os extremos coincidem, uma reconstrucao que retorna a esse ponto por caminhao
e descartada, pois criaria revisita; a excecao e um bloco que cobre toda a rota
fechada no deposito. Voos estacionarios continuam permitidos.
Voos fora do bloco sao preservados e suas posicoes sao remapeadas na montagem.

Cada bloco agora busca ate 32 ordens distintas, mantendo os extremos fixos.
A primeira e a linearizacao original. Em seguida avaliam-se a insercao mais
distante com extremos fixos e as inversoes internas dessas duas sequencias.
Empates na insercao sao resolvidos por ID e primeira posicao de insercao.
Depois geram-se inversoes de subsequencias (2-opt na ordem) e realocacoes de
um cliente a partir da melhor ordem avaliada por custo TSP-D. Os movimentos sao
embaralhados em um gerador separado, derivado da semente e inicio do bloco.
Se a varredura produzir uma nova melhor ordem e ainda houver orcamento, a proxima
varredura parte dela. Encerra ao esgotar as 32 avaliacoes ou a vizinhanca dessa
ordem sem avancar. Ordens repetidas nao consomem novas chamadas a PD.
A ordem original e o bloco original sao sempre considerados, garantindo que
usar mais ordens nao perca o resultado obtido com maxOrders=1 na mesma chamada.
Os geradores de ordens, escolha de blocos e perturbacao GVNS sao separados.
O limite e por bloco e por chamada, nao por execucao inteira do HGVNS.
Materializar os candidatos de inversao/realocacao usa O(m^3) memoria temporaria
(O(m^2) sequencias de tamanho O(m)), alem da memoria do particionamento.

### Pontos de integracao experimentados (historico da API)

| Sufixo do identificador | Ponto de aplicacao | Frequencia |
|---|---|---|
| sem sufixo | Apos terminar o HGVNS | Uma vez |
| `-initial` | Apos a construcao por economias, antes do GVNS | Uma vez |
| `-shake` | Apos uma perturbacao viavel, antes da RVND | Cada perturbacao viavel |
| `-rvnd` | Apos a RVND, antes de comparar candidato com incumbente | Cada candidato refinado |

Cada identificador escolhe apenas um ponto; nao acumula uma PD final extra.
Initial permite que o GVNS explore uma construcao reotimizada. Shake permite
que a RVND explore a reconstrucao do estado perturbado. RVND avalia o candidato
reotimizado antes de aceita-lo, sem intervir nas sete vizinhancas da RVND.
Nao ha reinicios, aumento do numero de tentativas ou novo limite de tempo.
Aplicacoes internas podem mudar a trajetoria e a duracao natural da busca;
nao ha garantia de superar o HGVNS puro em cada semente. A garantia de nao piora
em relacao ao HGVNS puro vale para o ponto final; nas demais posicoes vale para
cada chamada local da PD. Nenhuma variante foi identificada a priori como melhor.

```powershell
.\main.exe caminho\instancia.txt --no-graphics --algorithms "hgvns,hgvns-pd-waiting-rvnd" --seed 42
```

As combinacoes com random nao estao mais disponiveis no executor.
Somente `hgvns-pd-waiting-rvnd` permanece em --help e na selecao padrao.
Os demais identificadores dessa tabela sao historicos e nao sao aceitos na CLI.
O console informa chamadas da PD, ordens avaliadas e quantidade
de blocos melhorados. A API oferece os mesmos contadores por statistics().

```cpp
tspd::hgvns::BlockParameters parameters;
parameters.maxNodes = 20; // minimo 3
parameters.maxBlocks = 5; // zero desativa o pos-processamento
parameters.maxOrders = 32; // 1 recupera a versao de ordem unica; minimo 1
tspd::hgvns::HGVNS waiting(42, tspd::hgvns::BlockSelection::Waiting, parameters);
tspd::hgvns::HGVNS random(42, tspd::hgvns::BlockSelection::Random, parameters);
tspd::hgvns::HGVNS afterRVND(42, tspd::hgvns::BlockSelection::Waiting, parameters,
                           tspd::hgvns::PDPlacement::AfterRVND);
```

Os limites sao configuraveis pela API; a CLI usa os padroes. CSV registra cada
variante em sua propria linha, e solve_seconds inclui HGVNS e pos-processamento.
Nao e uma implementacao de ICP: nao ha chainlets sobrepostos, cache, selecao pelo
ganho calculado ou TSP-EP-ALL. O limite de 20 controla o trabalho adicional, e nao
restringe o tamanho total da instancia. Em instancias pequenas ou quando todos
os blocos cabem no orcamento, as duas selecoes podem produzir o mesmo resultado.
Ganhos de qualidade e custo em benchmarks ainda precisam ser medidos.

## Validacao

`tests/hgvns_test.cpp` confronta os custos com uma simulacao por eventos a partir
das coordenadas, testa esperas e voos com caminhao parado, rejeita estados
invalidos, exercita as figuras com clientes mistos e compara a melhor melhoria
das sete vizinhancas com enumeracoes pequenas independentes. Tambem verifica
repetibilidade e a linearizacao TSP. Os testes de dependencia do TSP e de
integracao do executor incluem o novo algoritmo.

Os testes de blocos comparam a PD com enumeracao exaustiva de operacoes em 30
sequencias pequenas, verificam selecao por espera, variacao por semente,
orcamentos iguais, melhorias aceitas e preservacao do custo. Exercitam as duas
politicas com cinco tamanhos de bloco e voos estacionarios, confrontando o custo
com a simulacao independente. A integracao cobre as duas novas linhas do CSV.
Tambem verifica melhoria adicional frente a ordem unica em casos pequenos,
limite de ordens, as quatro posicoes de chamada, desativacao sem alterar o
percurso aleatorio do GVNS e repetibilidade das oito variantes com PD.
