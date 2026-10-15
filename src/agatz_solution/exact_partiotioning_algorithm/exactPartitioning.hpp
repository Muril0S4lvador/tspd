#pragma once // Disponibiliza a declaracao usada nesta implementacao.
#include "../../solution/solution_strategy.hpp" // Disponibiliza a declaracao usada nesta implementacao.
#include "../../tspd/tspd.hpp" // Disponibiliza a declaracao usada nesta implementacao.
namespace tspd::exactPartitioning { // Mantem a estrutura necessaria para a etapa corrigida.
class ExactPartitioning : public ::tspd::solution::SolutionStrategy { // Mantem a estrutura necessaria para a etapa corrigida.
private: // Mantem a estrutura necessaria para a etapa corrigida.
    inline static std::vector<std::vector<double>> T_; // Guarda a melhor duracao por trecho.
    inline static std::vector<std::vector<int>> M_; // Guarda a posicao do cliente aereo ou -1.
    inline static std::vector<double> V_; // Mantem a estrutura necessaria para a etapa corrigida.
    inline static std::vector<int> P_; // Guarda posicoes predecessoras, nao IDs.
    inline static int n_; // Dimensao dos nos distintos para consultar arestas.
    inline static double droneSpeed_; // Evita truncar fatores fracionarios.
    inline static double truckSpeed_; // Evita truncar fatores fracionarios.
    static void initializingParameters(::tspd::tspd::TSPD& instance, const std::vector<int>& route); // Usa o tamanho da rota fechada.
    static double getDroneCost(const std::vector<int>& route, int start, int end, const std::vector<::tspd::edge::Edge>& edges); // Evita copias e converte posicoes.
    static double getTruckCost(const std::vector<int>& route, int start, int end, const std::vector<::tspd::edge::Edge>& edges); // Calcula uma ligacao direta.
    static void BestImprovementByNode(const std::vector<int>& route, const std::vector<::tspd::edge::Edge>& edges); // Preserva as entradas e evita copias.
    static void ChoosingBestSequence(); // Mantem a estrutura necessaria para a etapa corrigida.
    static double evaluateRoute(::tspd::tspd::TSPD& instance, const std::vector<int>& route, const std::vector<::tspd::edge::Edge>& edges); // Avalia cada ordem pelo tempo do particionamento exato.
    static std::vector<int> iterativeImprovement(::tspd::tspd::TSPD& instance, const std::vector<int>& initialRoute, const std::vector<::tspd::edge::Edge>& edges); // Executa VND com 2-opt, relocate e swap, reiniciando apos melhorias.
    static ::tspd::solution::Solution getSolution(const std::vector<int>& route, const std::vector<::tspd::edge::Edge>& edges); // Corrige o retorno e recebe dados de reconstrucao.
public: // Mantem a estrutura necessaria para a etapa corrigida.
    std::string getName() const override { return "Exact Partitioning"; } // Mantem a estrutura necessaria para a etapa corrigida.
    solution::Solution Optimize(::tspd::tspd::TSPD& instance) override; // Mantem a estrutura necessaria para a etapa corrigida.
}; // Encerra o bloco desta etapa.
} // Encerra o bloco desta etapa.
