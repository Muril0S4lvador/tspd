# HGA-TAC e HGA-TAC+ para o TSP-D

Referencia principal: Mahmoudinazlou e Kwon, *A Hybrid Genetic Algorithm with
Type-Aware Chromosomes for Traveling Salesman Problems with Drone*, versao de
30/04/2024. Secoes 4.1--4.7, Algoritmos 1--4 e Tabela 1.
Implementacao de referencia consultada para esclarecer as vizinhancas herdadas:
https://github.com/Sasanm88/TSPDroneHGATAC.jl (src/neighborhood.jl).
O codigo C++ foi escrito para as interfaces e o modelo deste repositorio.

## Execucao

```powershell
.\main.exe caminho\instancia.txt --no-graphics --algorithms hga-tac --seed 42
.\main.exe caminho\instancia.txt --no-graphics --algorithms hga-tac-plus --seed 42
.\main.exe caminho\instancias --algorithms "tsp,hga-tac,hga-tac-plus" --seed 42
```

Ambos recebem o mesmo TSP inicial compartilhado com as demais heuristicas.
O executor registra custos, tempo e falhas no CSV existente. `solve_seconds`
exclui o calculo do TSP compartilhado. A semente padrao e 0; o gerador e reiniciado
em cada chamada. Cada linha do CSV corresponde a UMA execucao; as dez repeticoes
por instancia usadas no artigo nao sao feitas automaticamente.

```cpp
#include "hga_tac/HGATAC.hpp"

tspd::hga_tac::HGATAC basic(42);
auto solution = basic.Solve(instance, tspSolution);

tspd::hga_tac::Parameters parameters;
parameters.endurance = 100.0; // opcional, em unidades de tempo da instancia
tspd::hga_tac::HGATAC plus(42, true, parameters);
auto restrictedSolution = plus.Solve(instance, tspSolution);
auto statistics = plus.statistics();
```

## Correspondencia com o artigo

| Artigo | Implementacao |
| --- | --- |
| Algoritmo 1 | `HGATAC.cpp`: geracao, selecao, cruzamento, mutacao, reparo, busca local e gestao das subpopulacoes |
| Secao 4.1, TAC | Permutacao de IDs 1..N-1; positivo = caminhao, negativo = drone; deposito 0 implicito |
| Algoritmo 2 | Particionamento exato da ordem TSP inicial e perturbacoes por elemento ou subsequencia |
| Algoritmo 3, Join | `join.cpp`: DP quadratica, com reconstrucao de lancamentos e reencontros |
| Secao 4.3 | Makespan penalizado para voos com multiplos clientes e excesso de autonomia; diversidade por Hamming |
| Secao 4.4 | Torneio, OX1, OX2, TOX1, TOX2; mutacoes de sinais e de ordem |
| Secao 4.5 | `operators.cpp`: N1..N15 e L1..L7; candidatos limitados aos vizinhos proximos |
| Secao 4.6 | Sobrevivencia por custo, penalidades adaptativas e diversificacao |
| Algoritmo 4 | Buffer de candidatos, incluindo pioras relativas abaixo de 5%; apenas na variante plus |

N1/N2 realocam um/dois clientes livres do caminhao; N3 troca dois clientes do
caminhao; N4/N5 trocam um bloco de dois por um cliente, com/sem restricao de
clientes livres; N6--N9 reconectam dois pares do caminhao; N10 troca um cliente
aereo com um cliente do trecho de seu voo; N11/N12 trocam o cliente aereo com
lancamento/reencontro; N13 troca as extremidades; N14 transfere cliente livre do
caminhao para o drone; N15 transfere um cliente aereo para o caminhao.

L1 converte o meio de tres clientes consecutivos do caminhao em cliente aereo;
L2 realoca cliente aereo; L3 troca IDs entre veiculos mantendo tipos das posicoes;
L4 inverte um trecho entre arcos do caminhao, incluindo genes aereos intermediarios;
L5/L6 trocam clientes aereos e convertem ambos/um para caminhao; L7 insere um
cliente aereo junto ao cliente aereo de outra operacao, convertido em caminhao.
A Join recalcula as extremidades depois de cada movimento.

Cada vizinhanca sorteia um movimento admissivel, nao enumera a melhor melhoria
global. A busca local embaralha a ordem das 22 vizinhancas e reinicia apos uma
melhoria. Apenas candidatos viaveis com melhoria maior que a tolerancia relativa
de 1e-9 sao aceitos. O escape usa movimentos sem o filtro de melhoria local,
permitindo a passagem por solucoes um pouco piores.

Parametros padrao da Tabela 1: mu=15, lambda=25, torneio=5, probabilidade de
mutacao=0.1, reparo=0.5, parada apos 2500 iteracoes sem melhoria, diversificacao
a cada 100 e tentativa de escape a cada 1000 iteracoes sem melhoria. O escape
usa 10000 tentativas, buffer=40, limite relativo=0.05. A fracao de proximidade
e 0.3; arredondamentos de quantidades usam teto, minimo 1. Os parametros sao
configuraveis pela API C++ para permitir experimentos e testes.

## Modelo e decisoes de integracao

- O escopo executavel e TSPD: um caminhao, um drone, um cliente por voo viavel,
  todos os clientes elegiveis, servicos instantaneos e espera em solo permitida.
  O leitor atual nao fornece os parametros necessarios ao FSTSP. Esta integracao
  nao implementa as restricoes especificas do FSTSP nem afirma reproduzir seus
  resultados publicados.
- O padrao e autonomia ilimitada; `Parameters::endurance` permite TSPD com limite
  de tempo aereo pela API. Nao se infere autonomia a partir do nome do arquivo.
  A CLI permanece com o modelo de entrada existente.
- Os fatores de deslocamento multiplicam as distancias, como no restante do
  projeto; eles nao sao velocidades fisicas pelas quais dividir as distancias.
- O TSP inicial vem de Kruskal/DFS do projeto, em vez de LKH do artigo. Um
  particionamento de ordem fixa O(N^3), sem a VND e sem limite de 20 nos, forma
  a primeira solucao. O metodo nao chama o algoritmo ExactPartitioning existente.
- Join tem estados (grupo de clientes aereos consumido, posicao do caminhao).
  Esse refinamento evita autorreferencia ao recuperar o drone no mesmo ponto do
  lancamento. So ha transicoes entre intervalos consecutivos da representacao;
  a soma dos produtos dos tamanhos desses intervalos e O(N^2). Prefixos calculam
  os tempos do caminhao em O(1). A memoria da DP e O(N^2).
- Genes negativos consecutivos formam um voo multi-cliente somente para avaliar
  inviabilidade Tipo 1. O peso das arestas internas cresce como w1^q. Tipo 2
  identifica excesso de autonomia, quando nao existe decodificacao viavel.
  A Join estrita e usada para TODOS os individuos classificados como viaveis.
- O reparo de Tipo 1 alterna conversoes para caminhao ate separar os negativos.
  Com autonomia limitada, converte progressivamente clientes aereos para caminhao
  ate obter uma Join viavel. A rota toda de caminhao garante terminacao.
- A inicializacao usa no maximo 100*mu tentativas para preencher as classes:
  classes impossiveis em casos degenerados podem ficar vazias; classes nao vazias
  sao completadas com copias. Isso evita travar, por exemplo, com um unico cliente
  ou com autonomia tao alta que nao existe Tipo 2. A parada evolutiva nao recebe
  um limite adicional de tentativas, clientes ou tempo.
- O fitness usa o custo vezes 0.8 elevado a media das duas menores distancias
  de Hamming aos outros individuos. Torneios selecionam sem reposicao. Sobreviventes
  sao ordenados pelo custo (penalizado nas classes inviaveis). Ao mudar penalidades,
  os custos inviaveis armazenados sao atualizados antes da selecao seguinte.
- A janela das proporcoes de viabilidade tem os ultimos 100 filhos, classificados
  antes do reparo; a proporcao-alvo e 0.2. A penalidade de multiplos clientes inicia
  em 2, conforme o texto, e usa os limites 3..8 nas reducoes/aumentos; a de alcance
  inicia em 2, com limites 1.5..5. Mantem-se a incumbente viavel separadamente.
- Diversificacao e escape sao disparados nos multiplos de seus periodos de
  estagnacao; melhorias reiniciam esse contador. A diversificacao conserva os
  ceil(0.3*mu) melhores por fitness de cada classe e preenche a partir da semente.
- Escolhas de candidatos nas vizinhancas sao normalizadas a operacoes sobre
  permutacoes; empates nos vizinhos proximos usam IDs. Nao e um porte literal do
  codigo Julia e nao promete sequencias aleatorias ou resultados identicos.

## Saida e validacao

As extremidades dos voos sao POSICOES na rota fechada do caminhao; clientes sao
IDs. Isso distingue a saida e o retorno ao deposito. O makespan inclui espera;
`truck_cost` e `drone_cost` incluem somente deslocamento. A rota do drone inclui
trechos transportados no caminhao para compatibilidade com a visualizacao existente.
`tsp_cost` refere-se a permutacao final em valores absolutos, fechada no deposito;
a linha `tsp` do CSV continua representando o TSP inicial compartilhado.

`tests/hga_tac_test.cpp` compara a Join com enumeracao exaustiva independente em
640 casos pequenos e tres autonomias, simula eventos por coordenadas, cobre
espera/voos estacionarios, valida permutacoes dos quatro cruzamentos e das 22
vizinhancas, custos penalizados, entradas invalidas, semente, reparo, diversificacao
e escape. `tests/test_experiment_runner.py` cobre as duas variantes no executor,
TSP compartilhado, repetibilidade, selecao e CSV. Os testes nao comprovam a
reproducao dos benchmarks nem a optimalidade global da metaheuristica.
