#include <iostream>
#include <string>
#include <chrono>
#include <exception>
#include <clocale>

#ifdef _WIN32
#include <windows.h>
#endif

#include <SFML/Graphics.hpp>

#include "instance_reader/instance_reader.hpp"
#include "node/node.hpp"
#include "edge/edge.hpp"
#include "tspd/tspd.hpp"
#include "graphic/graphic.hpp"
#include "solution/solution.hpp"
#include "agatz_solution/greedy_heuristic/greedy_heuristic.hpp"

void setInterpreter();



int main(int argc, char* argv[]) {
    setInterpreter();

    if (argc < 2) {
        std::cerr << "Erro: Nenhum arquivo de instancia foi fornecido.\n";
        std::cerr << "Uso: " << argv[0] << " <caminho_para_instancia.tsp>\n";
        std::cerr << "Exemplo: " << argv[0] << " data/descompressed/a280/a280.tsp\n";
        return 1;
    }

    std::string caminho_instancia = argv[1];

    try {
        std::cout << "==========================================\n";
        std::cout << "  Iniciando Solver TSP/TSPD\n";
        std::cout << "==========================================\n";
        std::cout << "Instância selecionada: " << caminho_instancia << "\n\n";

        // 2. Início da medição de tempo
        
        auto inicio = std::chrono::high_resolution_clock::now();
        std::cout << "[1/2] Lendo dados da instância...\n";
        tspd::tspd::TSPD instancia = tspd::instance_reader::InstanceReader::readInstance(caminho_instancia);
        auto fim = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> tempo_execucao = fim - inicio;

        std::cout << instancia;
        std::vector<tspd::node::Node> nodes = instancia.getNodes();
        std::vector<tspd::edge::Edge> edges = instancia.getEdges();
        tspd::solution::Solution s = tspd::greedyHeuristic::GreedyHeuristic::getGreedyHeuristicSolution(instancia);

        tspd::graphic::Graphic graphic(instancia);

        graphic.drawSolution(s);

        // 5. Exibição dos resultados
        std::cout << "\n\n==========================================\n";
        std::cout << "  Execucao concluida com sucesso!\n";
        std::cout << "  Tempo de execucao: " << tempo_execucao.count() << " segundos\n";
        std::cout << "==========================================\n";

        std::cout << s;

    } catch (const std::exception& e) {
        // Captura erros lançados durante a execução (ex: arquivo não encontrado, erro de leitura)
        std::cerr << "\nERRO DURANTE A EXECUCAO: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

// Força o terminal do Windows a interpretar UTF-8
void setInterpreter(){
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif
    std::setlocale(LC_ALL, "pt_BR.UTF-8");
}