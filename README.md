# Como executar o projeto

## Pré-requisitos

- Compilador C++ com suporte ao padrão C++17 (como `g++`)
- `make`
- Bibliotecas de desenvolvimento do SFML (`graphics`, `window` e `system`)
- `bash`, `tar` e `gzip` para preparar as instâncias

## Preparar as instâncias

Antes da primeira execução, descompacte as instâncias usadas pelo projeto. Com o arquivo `data/ALL_tsp.tar.gz` disponível, execute na raiz do repositório:

```bash
bash scripts/descompress_data.sh
```

O script cria e organiza os arquivos em `data/descompressed/`. No Windows, execute-o pelo Git Bash ou pelo WSL.

## Compilar

Na raiz do projeto, execute:

```bash
make
```

O comando gera o executável `main.exe` no Windows e `main` em outros sistemas.

## Executar

Informe o caminho de um arquivo de instancia `.txt` no formato TSP-D ou de um diretorio:

```powershell
.\main.exe caminho\para\instancia.txt
```

No Linux ou macOS:

```bash
./main caminho/para/instancia.txt
```

Por exemplo, se a instância estiver disponível localmente:

```powershell
.\main.exe data\singlecenter\singlecenter-1-n5.txt
```

### Rodadas de teste

Um arquivo seleciona somente aquela instancia. Um diretorio seleciona recursivamente
todas as instancias `.txt` de suas subpastas, em ordem de caminho. Pastas `solutions/`
e arquivos auxiliares sem a assinatura do formato TSP-D sao ignorados. Os arquivos
precisam estar descompactados; arquivos ZIP e instancias TSPLIB `.tsp` nao sao lidos
pelo leitor atual.

```powershell
# Todos os algoritmos em todas as instancias, sem graficos
.\main.exe data/

# Somente a rota TSP por Kruskal e DFS, com custo e tempo proprios
.\main.exe data/ --algorithms tsp

# HGVNS, com semente reproduzivel (padrao: 0)
.\main.exe data/ --algorithms hgvns --seed 42

# Somente instancias de singlecenter, com Greedy
.\main.exe data/singlecenter/ --algorithms greedy

# Um arquivo, todos os algoritmos, sem abrir janelas
.\main.exe data/singlecenter/singlecenter-1-n5.txt --no-graphics

# Um arquivo, somente ExactPartitioning, sem graficos
.\main.exe --no-graphics --algorithms exact-partitioning data/singlecenter/singlecenter-1-n5.txt

# Selecao explicita de varios algoritmos (no PowerShell, coloque a lista entre aspas)
.\main.exe data/ --algorithms "greedy,exact-partitioning"

# Ajuda e identificadores disponiveis
.\main.exe --help
```

Sem `--algorithms`, todos os algoritmos cadastrados sao executados. A lista usa
virgulas, sem espacos; selecoes repetidas sao executadas uma unica vez, na ordem
do cadastro. Opcoes podem aparecer antes ou depois do caminho. Identificadores
desconhecidos e listas vazias geram erro antes de iniciar a rodada.

Um arquivo individual abre graficos por padrao, a menos que seja informado
`--no-graphics`. Diretorios nunca abrem graficos. Cada algoritmo recebe uma copia
da instancia original e um novo objeto do algoritmo; as execucoes sao sequenciais.
O cadastro central fica em `src/experiment_runner.cpp` e usa `SolutionStrategy::Solve`.

O `TSPSolver`, em `src/TSP/`, implementa `SolutionStrategy` e constroi a rota TSP
com Kruskal e DFS. O executor o chama uma unica vez por instancia e compartilha
a mesma `Solution` inicial com Greedy e ExactPartitioning. A selecao `tsp` registra
essa solucao no CSV sem recalcula-la; sem `--algorithms`, ela tambem e registrada.
Mesmo ao selecionar apenas uma heuristica, o TSP necessario e calculado uma vez.

Para chamar os metodos diretamente:

```cpp
tspd::tsp::TSPSolver tspSolver;
const auto tspSolution = tspSolver.Solve(instance);

tspd::greedyHeuristic::GreedyHeuristic greedy;
const auto greedySolution = greedy.Solve(instance, tspSolution);

tspd::exactPartitioning::ExactPartitioning exact;
const auto exactSolution = exact.Solve(instance, tspSolution);
```

Inclua `TSP/TSPSolver.hpp` e os headers das heuristicas utilizadas. Greedy e Exact
lancam erro ao serem chamados sem TSP previo, com rota invalida ou custo
inconsistente. Uma nova estrategia que nao implemente
`Optimize(instance, tspSolution)` tambem gera erro explicito ao receber o TSP.
As buscas locais continuam podendo modificar suas proprias copias da rota inicial.

O algoritmo `hgvns` implementa construcao por economias, sete vizinhancas e
GVNS/RVND para o TSP-D do modelo atual. Tambem recebe a solucao TSP compartilhada.
Use `--seed` com um inteiro de 0 a 4294967295 para controlar suas escolhas
aleatorias. A semente padrao e 0 e e impressa no inicio da rodada; os demais
algoritmos deterministicos nao utilizam essa opcao. As adaptacoes ao artigo, a politica de ajuste
das viagens e a convencao de `tsp_cost` estao em [src/hgvns/README.md](src/hgvns/README.md).

Foi mantida para analise apenas `hgvns-pd-waiting-rvnd`, que aplica a PD apos
cada RVND, antes da aceitacao do candidato, priorizando a espera dos veiculos.
As outras sete variantes experimentais nao estao mais no executor.
Por padrao sao ate cinco blocos por chamada, com ate 20
nos distintos. Cada bloco avalia ate 32 ordens (incluindo a original), com
insercao mais distante, inversoes e realocacoes, e somente melhorias sao aceitas.
Os criterios de parada do HGVNS permanecem; as chamadas adicionais custam tempo.
Detalhes e parametros pela API estao em `src/hgvns/README.md`.

```powershell
.\main.exe caminho\instancias --algorithms "hgvns,hgvns-pd-waiting-rvnd" --seed 42
```

Os algoritmos `hga-tac` e `hga-tac-plus` implementam cromossomos que distinguem
caminhao e drone, programacao dinamica Join, cruzamentos, mutacoes, busca local
e gestao de populacoes viaveis/inviaveis. A variante plus acrescenta o escape de
otimos locais. Ambos usam o TSP compartilhado e a opcao `--seed` (padrao 0).

```powershell
.\main.exe caminho\instancia.txt --no-graphics --algorithms "hga-tac,hga-tac-plus" --seed 42
```

Esta e uma integracao ao modelo TSPD atual, com autonomia ilimitada pela CLI,
e TSP inicial por Kruskal/DFS em lugar de LKH. Parametros, suporte opcional a
autonomia pela API, diferencas em relacao ao artigo e validacao estao em
[src/hga_tac/README.md](src/hga_tac/README.md).

Cada rodada gera dois CSVs relativos ao diretorio de execucao, sem sobrescrever
rodadas anteriores. `results/analise-<data-hora>.csv` e o arquivo para comparar
todos os metodos lado a lado, com UMA linha por instancia:

| Coluna | Conteudo |
|---|---|
| `name`, `instance_path`, `nodes`, `seed` | Identificacao, caminho, dimensao e semente |
| `optimal_tspd_cost`, `optimal_source` | Otimo global TSP-D explicitamente informado e arquivo de origem; vazios se desconhecido |
| `literature_tspd_cost`, `literature_tsp_cost` | Referencias disponiveis; nao presumem optimalidade global |
| `tsp_result`, `tsp_seconds`, `tsp_status`, `tsp_error` | Rota inicial compartilhada: custo, segundos, situacao e erro |
| `<metodo>_result`, `<metodo>_seconds` | Custo TSP-D e tempo exclusivo do metodo |
| `<metodo>_total_seconds` | Tempo do metodo mais tempo do TSP compartilhado |
| `<metodo>_status`, `<metodo>_error` | `success`, `error` ou `not_selected`, e motivo de erro |

Os seis prefixos de metodos sao `greedy`, `exact-partitioning`, `hgvns`,
`hgvns-pd-waiting-rvnd`, `hga-tac` e `hga-tac-plus`. O arquivo sempre possui
as colunas de todos eles; `--algorithms` limita a execucao e marca os demais
como `not_selected`, com resultados e tempos vazios. O TSP inicial e registrado
mesmo quando `tsp` nao foi explicitamente selecionado.

Para fornecer um otimo global TSP-D conhecido, use um arquivo opcional
`solutions/<nome-da-instancia-sem-extensao>-optimal.txt`, contendo somente
um numero finito nao negativo, com ponto decimal. Esse valor e metadado
informado pelo usuario/conjunto de dados; o programa nao prova sua optimalidade.
Arquivo malformado gera erro explicito. Arquivos `-DP.txt` nunca preenchem
automaticamente o campo de otimo, pois sao referencias de particionamento.

O CSV detalhado continua em `results/rodada-<data-hora>.csv`, relativo ao
diretorio de execucao, sem sobrescrever rodadas anteriores. Cada linha representa
um par instancia/algoritmo, com as colunas:

| Coluna | Conteudo |
|---|---|
| `name` | Somente o nome do arquivo, incluindo `.txt`, sem o caminho |
| `algorithm` | Identificador do algoritmo |
| `tspd_cost` | Custo TSP-D retornado pelo algoritmo |
| `tsp_cost` | Custo da rota TSP completa retornada pelo algoritmo |
| `literature_tspd_cost` | Custo da referencia TSP-D disponivel |
| `literature_tsp_cost` | Custo da referencia TSP disponivel |
| `solve_seconds` | Tempo do algoritmo da linha, sem leitura ou espera nas janelas; nas heuristicas, exclui o TSP inicial compartilhado |
| `status` | `success` ou `error` |
| `error` | Mensagem da falha, quando houver |

Custos indisponiveis ficam vazios, sem substituir por zero. Uma falha do algoritmo
mantem vazios seus custos, mas preserva as referencias disponiveis da instancia.
Uma referencia TSP usada como fallback pelo leitor nao preenche `literature_tspd_cost`.
Na linha `tsp`, `tspd_cost` e `tsp_cost` sao iguais: representam a operacao apenas
com caminhao. Seu `solve_seconds` registra a construcao da rota inicial. Para comparar
o tempo completo de uma heuristica, some esse tempo ao `solve_seconds` dela.

O CSV e atualizado apos cada execucao. Falhas de leitura sao registradas para
todos os algoritmos selecionados; falhas de um algoritmo nao impedem os demais.
O resumo informa instancias concluidas e com falhas, execucoes bem-sucedidas e
erros de descoberta. O codigo de saida e `0` quando tudo termina com sucesso e `1`
quando ha falhas, caminho invalido ou nenhuma instancia encontrada.

O limite atual do ExactPartitioning permanece: instancias com mais de 20 nos
produzem uma falha registrada, e a rodada continua.

Depois de compilar, execute os testes de integracao com Python 3:

```powershell
python tests/test_experiment_runner.py
```

Os testes usam instancias pequenas em uma pasta temporaria dentro de `build/`
e verificam recursao, selecao dos algoritmos, CSV e continuidade apos falhas.

Para remover os arquivos gerados pela compilação, execute `make clean`.


# Atas de reunião

### Reunião — 17/09/2026

**Pontos combinados:**

- Compartilhar acesso ao Git do projeto, junto da apresentação da solução inicial criada
- Estudo e implementação de métodos resolutivos usando programação dinâmica
