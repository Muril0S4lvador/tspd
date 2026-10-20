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

Cada rodada gera um CSV proprio em `results/rodada-<data-hora>.csv`, relativo ao
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
| `solve_seconds` | Tempo de otimizacao, sem incluir leitura e espera nas janelas |
| `status` | `success` ou `error` |
| `error` | Mensagem da falha, quando houver |

Custos indisponiveis ficam vazios, sem substituir por zero. Uma falha do algoritmo
mantem vazios seus custos, mas preserva as referencias disponiveis da instancia.
Uma referencia TSP usada como fallback pelo leitor nao preenche `literature_tspd_cost`.

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
