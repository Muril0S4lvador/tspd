#include "exactPartitioning.hpp" // Sincroniza as definicoes com o header.
#include "../dfs/dfs.hpp" // Mantem a ordem inicial por DFS.
#include "../kruskal/kruskal.hpp" // Mantem a MST inicial.
#include <algorithm> // Disponibiliza max e reverse.
#include <cmath> // Valida fatores e custos finitos.
#include <limits> // Representa estados inalcancaveis.
#include <stdexcept> // Informa entradas e predecessores invalidos.
namespace tspd::exactPartitioning { // Mantem a estrutura necessaria para a etapa corrigida.
using TSPD = ::tspd::tspd::TSPD; // Mantem a estrutura necessaria para a etapa corrigida.
using Edge = ::tspd::edge::Edge; // Mantem a estrutura necessaria para a etapa corrigida.
using Solution = ::tspd::solution::Solution; // Mantem a estrutura necessaria para a etapa corrigida.

void ExactPartitioning::initializingParameters(TSPD& instance, const std::vector<int>& route) { // Dimensiona os estados pela rota fechada.
    n_ = instance.getDimension(); // Separa a dimensao das arestas das posicoes da rota.
    const auto size = route.size(); // Inclui a repeticao final do deposito.
    const double inf = std::numeric_limits<double>::infinity(); // Define o estado desconhecido.
    P_.assign(size, -1); // Marca predecessores ainda nao encontrados.
    V_.assign(size, inf); // Inicializa a recorrencia.
    T_.assign(size, std::vector<double>(size, inf)); // Inclui o deposito final.
    M_.assign(size, std::vector<int>(size, -1)); // Marca ausencia de cliente de drone.
    V_[0] = 0.0; // Implementa o caso base do artigo.
    droneSpeed_ = instance.getDroneSpeed(); // Preserva o fator decimal.
    truckSpeed_ = instance.getTruckSpeed(); // Preserva o fator decimal.
} 

double ExactPartitioning::getDroneCost(const std::vector<int>& route, int start, int end, const std::vector<Edge>& edges) { // Evita copiar as entradas.
    return Edge::getWeight(edges, n_, route[start], route[end]) * droneSpeed_; // Converte posicoes em IDs reais.
} 

double ExactPartitioning::getTruckCost(const std::vector<int>& route, int start, int end, const std::vector<Edge>& edges) { // Define custo de ligacao direta, nao de trecho.
    return Edge::getWeight(edges, n_, route[start], route[end]) * truckSpeed_; // Calcula tambem o salto sobre o cliente do drone.
} 

void ExactPartitioning::BestImprovementByNode(const std::vector<int>& route, const std::vector<Edge>& edges) { // Preserva a ordem e evita copias.
    const int N = static_cast<int>(route.size()) - 1; // Identifica o deposito final.
    std::vector<double> S(route.size(), 0.0); // Prepara consultas de trechos em O(1).
    for (int p = 1; p <= N; ++p) { // Inclui o retorno ao deposito.
        S[p] = S[p - 1] + getTruckCost(route, p - 1, p, edges); // Acumula as arestas da rota.
    } 
    for (int i = 0; i < N; ++i) { // Testa os inicios.
        for (int j = i + 1; j <= N; ++j) { // Inclui o destino final.
            if (j == i + 1) { // Mantem a estrutura necessaria para a etapa corrigida.
                T_[i][j] = getTruckCost(route, i, j, edges); // Usa o custo real da aresta consecutiva.
                M_[i][j] = -1; // Explicita a operacao sem entrega aerea.
            } else { // Mantem a estrutura necessaria para a etapa corrigida.
                for (int k = i + 1; k < j; ++k) { // Inclui k=j-1.
                    const double truckCost = S[j] - S[i] // Comeca pelo trecho original.
                        - getTruckCost(route, k - 1, k, edges) // Remove a entrada no cliente aereo.
                        - getTruckCost(route, k, k + 1, edges) // Remove a saida do cliente aereo.
                        + getTruckCost(route, k - 1, k + 1, edges); // Acrescenta o salto necessario.
                    const double droneCost = getDroneCost(route, i, k, edges) // Soma o voo de entrega.
                        + getDroneCost(route, k, j, edges); // Soma o voo de reencontro.
                    const double duration = std::max(truckCost, droneCost); // Considera a espera pelo mais lento.
                    if (duration < T_[i][j]) { // Acessa uma celula com dois colchetes.
                        T_[i][j] = duration; // Guarda a melhor duracao.
                        M_[i][j] = k; // Guarda a posicao escolhida para o drone.
                    } 
                } 
            } 
        } 
    } 
} 

void ExactPartitioning::ChoosingBestSequence() { // Mantem a estrutura necessaria para a etapa corrigida.
    const int N = static_cast<int>(V_.size()) - 1; // Usa o limite dos estados alocados.
    for (int j = 1; j <= N; ++j) { // Calcula tambem o estado final.
        for (int i = 0; i < j; ++i) { // Inclui o predecessor consecutivo.
            const double candidate = V_[i] + T_[i][j]; // Corrige o acesso matricial.
            if (candidate < V_[j]) { // Mantem a estrutura necessaria para a etapa corrigida.
                V_[j] = candidate; // Mantem a estrutura necessaria para a etapa corrigida.
                P_[j] = i; // Mantem a estrutura necessaria para a etapa corrigida.
            } 
        } 
    } 
} 

Solution ExactPartitioning::getSolution(const std::vector<int>& route, const std::vector<Edge>& edges) { // Recupera IDs e custos a partir da rota.
    struct Operation { int start; int end; int drone; }; // Preserva os limites de cada operacao.
    std::vector<Operation> operations; // Separa os cortes da expansao dos clientes.
    const int N = static_cast<int>(route.size()) - 1; // Usa o ultimo indice valido.
    if (!std::isfinite(V_[N])) { // Nao reconstrui um estado inalcancavel.
        throw std::runtime_error("No feasible partition found"); // Informa ausencia de solucao.
    } 
    for (int j = N; j > 0;) { // Percorre os predecessores do fim para o inicio.
        const int i = P_[j]; // Recupera o inicio da ultima operacao.
        if (i < 0 || i >= j) { // Garante indices validos e progresso.
            throw std::runtime_error("Invalid partition predecessor"); // Evita acessos invalidos e ciclos.
        } 
        operations.push_back({i, j, M_[i][j]}); // Guarda a operacao completa.
        j = i; // Recua para a operacao anterior.
    } 
    std::reverse(operations.begin(), operations.end()); // Corrige a ordem da reconstrucao.
    std::vector<int> truckNodes{route.front()}; // Usa o ID real do deposito.
    std::vector<int> droneNodes{route.front()}; // Representa a rota completa, incluindo encontros.
    double truckCost = 0.0; // Acumula deslocamentos do caminhao sem espera.
    double droneCost = 0.0; // Acumula voos sem transporte no caminhao nem espera.
    for (const auto& op : operations) { // Expande os cortes em ordem.
        int previous = op.start; // Controla a ultima posicao do caminhao.
        for (int p = op.start + 1; p <= op.end; ++p) { // Recupera clientes internos e destino.
            if (p != op.drone) { // Pula somente o cliente do drone.
                truckCost += getTruckCost(route, previous, p, edges); // Soma a aresta efetiva.
                truckNodes.push_back(route[p]); // Converte posicao em ID.
                previous = p; // Atualiza a origem da proxima aresta.
            } 
        } 
        if (op.drone != -1) { // Nao insere -1 como cliente.
            droneNodes.push_back(route[op.drone]); // Recupera o ID da entrega aerea.
            droneCost += getDroneCost(route, op.start, op.drone, edges) // Soma a ida.
                + getDroneCost(route, op.drone, op.end, edges); // Soma o reencontro.
        } 
        droneNodes.push_back(route[op.end]); // Registra o encontro ou deslocamento acompanhado.
    } 
    return Solution{truckNodes, droneNodes, V_[N], truckCost, droneCost}; // Usa o estado final e preserva os custos.
} 

std::vector<int> buildInitialRoute(TSPD& instance) { // Mantem a estrutura necessaria para a etapa corrigida.
    auto nodes = instance.getNodes(); // Mantem a estrutura necessaria para a etapa corrigida.
    auto edges = instance.getEdges(); // Mantem a estrutura necessaria para a etapa corrigida.
    if (nodes.empty()) { // Evita chamar DFS sem deposito.
        throw std::invalid_argument("Instance must contain a depot"); // Rejeita a instancia vazia.
    } 
    if (nodes.size() == 1) { // Trata a ausencia de clientes.
        return {0, 0}; // Retorna a rota trivial fechada.
    } 
    auto mst = ::tspd::kruskal::Kruskal::kruskal(nodes, edges); // Mantem a estrutura necessaria para a etapa corrigida.
    auto route = ::tspd::dfs::DFS::getDFS(nodes, mst); // Mantem a estrutura necessaria para a etapa corrigida.
    if (route.size() != nodes.size()) { // Verifica a cobertura de todos os clientes.
        throw std::invalid_argument("Initial route does not cover all nodes"); // Rejeita uma rota incompleta.
    } 
    route.push_back(route.front()); // Fecha a rota sem alterar a ordem dos clientes.
    return route; // Deixa a busca 2-opt na classe que pode acessar o particionamento exato.
} 



double ExactPartitioning::evaluateRoute(TSPD& instance, const std::vector<int>& route, const std::vector<Edge>& edges) { // Reutiliza a mesma avaliacao para a rota inicial e os vizinhos.
    initializingParameters(instance, route); // Limpa custos e predecessores da rota avaliada anteriormente.
    BestImprovementByNode(route, edges); // Recalcula as operacoes para esta ordem de clientes.
    ChoosingBestSequence(); // Encontra a melhor particao desta rota.
    return V_.back(); // Compara movimentos pelo tempo total do TSP-D.
} // Encerra a avaliacao de uma ordem fixa.

std::vector<int> ExactPartitioning::iterativeImprovement(TSPD& instance, const std::vector<int>& initialRoute, const std::vector<Edge>& edges) { // Implementa VND na ordem 2-opt, relocate e swap.
    auto route = initialRoute; // Mantem a melhor ordem aceita separada dos candidatos.
    double currentCost = evaluateRoute(instance, route, edges); // Estabelece o custo inicial do TSP-D.
    const int lastCustomer = static_cast<int>(route.size()) - 2; // Limita todos os movimentos aos clientes.
    int neighborhood = 0; // Inicia pela vizinhanca 2-opt.
    while (neighborhood < 3) { // Para somente quando as tres vizinhancas falham na mesma rota.
        auto bestRoute = route; // Mantem a rota atual se nenhum vizinho melhorar.
        double bestCost = currentCost; // Compara todos os vizinhos com o melhor custo da varredura.
        const double tolerance = 1e-9 * std::max(1.0, std::abs(currentCost)); // Evita aceitar ruido numerico como melhora.
        for (int first = 1; first <= lastCustomer; ++first) { // Preserva os depositos em todas as vizinhancas.
            const int firstDestination = neighborhood == 1 ? 1 : first + 1; // Relocate testa as duas direcoes; 2-opt e swap evitam pares repetidos.
            for (int last = firstDestination; last <= lastCustomer; ++last) { // Enumera os destinos validos da vizinhanca atual.
                if (first == last) { // Exclui a reinsercao de um cliente na mesma posicao.
                    continue; // Evita avaliar um movimento sem efeito.
                } // Encerra a verificacao de destino.
                auto candidate = route; // Gera cada vizinho a partir da mesma rota atual.
                if (neighborhood == 0) { // Aplica a primeira vizinhanca do VND.
                    std::reverse(candidate.begin() + first, candidate.begin() + last + 1); // Executa 2-opt sem mover os depositos.
                } else if (neighborhood == 1) { // Aplica o movimento 1-point.
                    const int customer = candidate[first]; // Preserva o cliente antes de remove-lo.
                    candidate.erase(candidate.begin() + first); // Retira o cliente de sua posicao original.
                    candidate.insert(candidate.begin() + last, customer); // Usa last como posicao final na rota apos a reinsercao.
                } else { // Aplica o movimento 2-point.
                    std::swap(candidate[first], candidate[last]); // Troca dois clientes sem alterar os demais.
                } // Encerra a construcao do vizinho.
                const double candidateCost = evaluateRoute(instance, candidate, edges); // Reparticiona sem filtrar pelo custo do caminhao sozinho.
                if (candidateCost + tolerance < bestCost) { // Aceita somente melhora do objetivo combinado.
                    bestCost = candidateCost; // Guarda o melhor tempo desta varredura.
                    bestRoute = candidate; // Preserva a ordem que produziu esse tempo.
                } // Encerra a comparacao do vizinho.
            } // Encerra os finais de trecho.
        } // Encerra a varredura completa da vizinhanca.
        if (bestCost + tolerance < currentCost) { // Aceita a melhor melhora da vizinhanca atual.
            route = bestRoute; // Atualiza a ordem somente depois de concluir a varredura.
            currentCost = bestCost; // Atualiza o objetivo combinado aceito.
            neighborhood = 0; // Reinicia no 2-opt depois de qualquer melhora.
        } else { // Mantem a rota quando nao houve melhora.
            ++neighborhood; // Avanca de 2-opt para relocate e depois para swap.
        } // Encerra a transicao entre vizinhancas.
    } // Encerra o VND em um otimo local das tres vizinhancas, respeitada a tolerancia.
    return route; // Devolve a ordem melhorada com os depositos preservados.
} // Encerra a busca local.

Solution ExactPartitioning::Optimize(TSPD& instance) { // Mantem a estrutura necessaria para a etapa corrigida.
    if (!std::isfinite(instance.getTruckSpeed()) || instance.getTruckSpeed() <= 0.0 || // Mantem a estrutura necessaria para a etapa corrigida.
        !std::isfinite(instance.getDroneSpeed()) || instance.getDroneSpeed() <= 0.0) // Mantem a estrutura necessaria para a etapa corrigida.
        throw std::invalid_argument("Truck and drone speeds must be positive and finite"); // Mantem a estrutura necessaria para a etapa corrigida.
    if(instance.getDimension() > 20)
        throw std::length_error("Exact Partitioning must be used in instances shorther than 20 clients");
    const auto initialRoute = buildInitialRoute(instance); // Constroi a ordem inicial antes da busca local.
    const auto edges = instance.getEdges(); // Reutiliza as mesmas arestas em todos os vizinhos.
    const auto route = iterativeImprovement(instance, initialRoute, edges); // Melhora a ordem com VND avaliado pelo particionamento exato do TSP-D.
    evaluateRoute(instance, route, edges); // Restaura P e M da rota aceita, pois o ultimo vizinho testado pode ter sido rejeitado.
    auto result = getSolution(route, edges); // Preserva o resultado TSP-D antes de anexar a base TSP.
    result.setTspRoute(route, instance.calculateTspCost(route)); // Guarda a rota completa e seu custo sem entregas do drone.
    return result; // Retorna os dois custos sem alterar o objetivo da busca local.
} 
} 
