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

Informe o caminho de um arquivo de instância `.tsp`:

```powershell
.\main.exe caminho\para\instancia.tsp
```

No Linux ou macOS:

```bash
./main caminho/para/instancia.tsp
```

Por exemplo, se a instância estiver disponível localmente:

```powershell
.\main.exe data\descompressed\a280\a280.tsp
```

Para remover os arquivos gerados pela compilação, execute `make clean`.


# Atas de reunião

### Reunião — 17/09/2026

**Pontos combinados:**

- Compartilhar acesso ao Git do projeto, junto da apresentação da solução inicial criada
- Estudo e implementação de métodos resolutivos usando programação dinâmica
