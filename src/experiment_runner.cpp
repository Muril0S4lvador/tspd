#include "experiment_runner.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <locale>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "agatz_solution/exact_partiotioning_algorithm/exactPartitioning.hpp"
#include "agatz_solution/greedy_heuristic/greedy_heuristic.hpp"
#include "graphic/graphic.hpp"
#include "instance_reader/instance_reader.hpp"

namespace tspd::experiments {
namespace {
namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;
using Strategy = ::tspd::solution::SolutionStrategy;
using Instance = ::tspd::tspd::TSPD;

struct Algorithm {
    std::string id;
    std::function<std::unique_ptr<Strategy>()> create;
};

const std::vector<Algorithm>& registry() {
    static const std::vector<Algorithm> algorithms{
        {"greedy", [] { return std::make_unique<greedyHeuristic::GreedyHeuristic>(); }},
        {"exact-partitioning", [] { return std::make_unique<exactPartitioning::ExactPartitioning>(); }}
    };
    return algorithms;
}

void usage(const char* program) {
    std::cout << "Uso: " << program
              << " <arquivo.txt|diretorio> [--no-graphics] [--algorithms id,id]\n"
              << "Diretorios: busca recursiva, sem graficos; solutions/ e ignorado.\n"
              << "Sem --algorithms: executa todos os algoritmos cadastrados.\n"
              << "Algoritmos:";
    for (const auto& algorithm : registry()) std::cout << ' ' << algorithm.id;
    std::cout << "\nResultados: results/rodada-<data-hora>.csv\n";
}

struct Options {
    fs::path input;
    bool noGraphics = false;
    std::set<std::string> selected;
};

Options parse(int argc, char* argv[]) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--no-graphics") {
            options.noGraphics = true;
        } else if (argument == "--algorithms") {
            if (i + 1 == argc || std::string(argv[i + 1]).rfind("--", 0) == 0)
                throw std::invalid_argument("--algorithms exige uma lista de identificadores.");
            const std::string list = argv[++i];
            std::size_t begin = 0;
            do {
                const auto end = list.find(',', begin);
                const auto id = list.substr(begin, end == std::string::npos ? end : end - begin);
                if (std::none_of(registry().begin(), registry().end(), [&](const Algorithm& a) {
                    return a.id == id;
                })) throw std::invalid_argument("Algoritmo desconhecido: '" + id + "'.");
                options.selected.insert(id);
                if (end == std::string::npos) break;
                begin = end + 1;
            } while (true);
        } else if (argument.rfind("--", 0) == 0) {
            throw std::invalid_argument("Opcao desconhecida: " + argument);
        } else {
            if (!options.input.empty()) throw std::invalid_argument("Informe somente um caminho.");
            options.input = argument;
        }
    }
    if (options.input.empty()) throw std::invalid_argument("Informe um arquivo ou diretorio.");
    return options;
}

// Usa a assinatura do formato suportado pelo InstanceReader, nao apenas a extensao.
bool isInstance(const fs::path& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("Nao foi possivel ler: " + path.string());
    std::string line;
    while (std::getline(input, line)) {
        if (line.find("/*The speed of the Truck*/") != std::string::npos) return true;
    }
    return false;
}

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return text;
}

struct Discovery {
    std::vector<fs::path> files;
    std::size_t errors = 0;
};

Discovery discover(const fs::path& path) {
    if (!fs::exists(path)) throw std::invalid_argument("Caminho inexistente: " + path.string());
    if (fs::is_regular_file(path)) return {{path}, 0};
    if (!fs::is_directory(path)) throw std::invalid_argument("Caminho nao e arquivo nem diretorio.");
    Discovery found;
    // Nao segue links de diretorio para evitar ciclos e execucoes duplicadas.
    fs::recursive_directory_iterator it(path), end;
    while (it != end) {
        const auto entry = *it;
        try {
            if (entry.is_directory() && lower(entry.path().filename().string()) == "solutions") {
                it.disable_recursion_pending();
            } else if (entry.is_regular_file() && lower(entry.path().extension().string()) == ".txt"
                       && isInstance(entry.path())) {
                found.files.push_back(entry.path());
            }
        } catch (const std::exception& error) {
            ++found.errors;
            std::cerr << "Erro na descoberta: " << error.what() << '\n';
        }
        std::error_code error;
        it.increment(error);
        if (error) {
            ++found.errors;
            std::cerr << "Erro ao percorrer " << entry.path().string() << ": " << error.message() << '\n';
        }
    }
    std::sort(found.files.begin(), found.files.end());
    return found;
}

std::string csv(const std::string& value) {
    std::string quoted = "\"";
    for (char c : value) {
        if (c == '"') quoted += '"';
        quoted += c;
    }
    return quoted + '"';
}

fs::path reportPath() {
    fs::create_directories("results");
    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::ostringstream name;
    name << "rodada-" << std::put_time(std::localtime(&now), "%Y%m%d-%H%M%S");
    auto path = fs::path("results") / (name.str() + ".csv");
    for (unsigned suffix = 1; fs::exists(path); ++suffix)
        path = fs::path("results") / (name.str() + "-" + std::to_string(suffix) + ".csv");
    return path;
}

void validate(const Instance& instance) {
    const auto nodes = instance.getNodes();
    if (instance.getDimension() < 1 || nodes.size() != static_cast<std::size_t>(instance.getDimension()))
        throw std::invalid_argument("Instancia sem nos validos ou com dimensao inconsistente.");
    if (!std::isfinite(instance.getTruckSpeed()) || instance.getTruckSpeed() <= 0 ||
        !std::isfinite(instance.getDroneSpeed()) || instance.getDroneSpeed() <= 0)
        throw std::invalid_argument("Fatores de caminhao e drone devem ser positivos e finitos.");
    for (const auto& node : nodes)
        if (node.getId() < 0 || node.getName().empty() ||
            !std::isfinite(node.getX()) || !std::isfinite(node.getY()))
            throw std::invalid_argument("Instancia com deposito ou coordenadas de nos invalidos.");
}

double elapsed(Clock::time_point start) {
    return std::chrono::duration<double>(Clock::now() - start).count();
}

void writeCost(std::ostream& output, const std::optional<double>& cost) {
    if (cost && std::isfinite(*cost)) output << *cost;
}
} // namespace

int run(int argc, char* argv[]) {
    if (argc == 2 && std::string(argv[1]) == "--help") { usage(argv[0]); return 0; }
    try {
        const auto options = parse(argc, argv);
        const auto found = discover(options.input);
        if (found.files.empty()) throw std::invalid_argument("Nenhuma instancia encontrada no caminho informado.");
        std::vector<Algorithm> algorithms;
        for (const auto& a : registry())
            if (options.selected.empty() || options.selected.count(a.id)) algorithms.push_back(a);
        const bool graphics = fs::is_regular_file(options.input) && !options.noGraphics;
        const auto outputPath = reportPath();
        std::ofstream report(outputPath);
        report.exceptions(std::ios::failbit | std::ios::badbit);
        report.imbue(std::locale::classic());
        report << std::setprecision(17)
               << "name,algorithm,tspd_cost,tsp_cost,literature_tspd_cost,literature_tsp_cost,solve_seconds,status,error\n";
        std::size_t successes = 0, failures = 0, completed = 0, failedInstances = 0;
        const auto start = Clock::now();
        std::cout << "Instancias: " << found.files.size() << " | Algoritmos: " << algorithms.size()
                  << " | Graficos: " << (graphics ? "sim" : "nao")
                  << "\nCSV: " << fs::absolute(outputPath).string() << '\n';
        for (std::size_t index = 0; index < found.files.size(); ++index) {
            const auto& path = found.files[index];
            std::cout << '[' << index + 1 << '/' << found.files.size() << "] " << path.string() << std::endl;
            std::optional<Instance> source;
            std::string readError;
            std::optional<double> literatureTspdCost, literatureTspCost;
            try {
                if (!isInstance(path)) throw std::invalid_argument("Formato de instancia nao suportado.");
                source.emplace(instance_reader::InstanceReader::readInstance(path.string()));
                validate(*source);
                if (source->hasLiteratureTspSolution() && source->getLiteratureTspSolution().hasTotalCost())
                    literatureTspCost = source->getLiteratureTspSolution().getTotalCost();
                // O leitor usa TSP como fallback; nao o apresenta como referencia TSP-D.
                const auto dpPath = path.parent_path() / "solutions" /
                    (path.stem().string() + "-DP" + path.extension().string());
                if (source->hasLiteratureSolution() && source->getLiteratureSolution().hasTotalCost() &&
                    (fs::exists(dpPath) || !source->hasLiteratureTspSolution()))
                    literatureTspdCost = source->getLiteratureSolution().getTotalCost();
            } catch (const std::exception& error) { readError = error.what(); }
            bool instanceFailed = false;
            for (const auto& algorithm : algorithms) {
                std::optional<double> cost;
                std::optional<double> tspCost;
                double solveSeconds = 0;
                std::string error = readError;
                if (error.empty()) {
                    const auto solveStart = Clock::now();
                    try {
                        auto instance = *source;
                        auto solver = algorithm.create();
                        auto solution = solver->Solve(instance);
                        solveSeconds = solver->getDuration().count();
                        if (!std::isfinite(solution.getTotalCost()))
                            throw std::runtime_error("Algoritmo retornou custo nao finito.");
                        cost = solution.getTotalCost();
                        tspCost = solution.getTspCost();
                        if (graphics) {
                            graphic::Graphic view(instance);
                            view.drawSolution(solution);
                        }
                    } catch (const std::exception& exception) {
                        if (!cost) solveSeconds = elapsed(solveStart);
                        error = exception.what();
                    }
                }
                if (error.empty()) {
                    ++successes;
                    std::cout << "  " << algorithm.id << ": custo=" << *cost
                              << ", tempo=" << solveSeconds << "s\n";
                } else {
                    ++failures;
                    instanceFailed = true;
                    std::cerr << "  " << algorithm.id << ": " << error << '\n';
                }
                report << csv(path.filename().string()) << ',' << csv(algorithm.id) << ',';
                writeCost(report, cost);
                report << ',';
                writeCost(report, tspCost);
                report << ',';
                writeCost(report, literatureTspdCost);
                report << ',';
                writeCost(report, literatureTspCost);
                report << ',' << solveSeconds << ','
                       << (error.empty() ? "success" : "error") << ',' << csv(error) << '\n';
                report.flush();
            }
            if (instanceFailed) ++failedInstances;
            else ++completed;
        }
        std::cout << "Rodada concluida em " << elapsed(start) << "s.\n"
                  << "Instancias selecionadas: " << found.files.size()
                  << " | Concluidas: " << completed << " | Com falhas: " << failedInstances << '\n'
                  << "Execucoes com sucesso: " << successes << " | Com falhas: " << failures
                  << " | Erros de descoberta: " << found.errors
                  << "\nCSV: " << fs::absolute(outputPath).string() << '\n';
        return failures || found.errors ? 1 : 0;
    } catch (const std::exception& error) {
        std::cerr << "Erro: " << error.what() << '\n';
        usage(argv[0]);
        return 1;
    }
}
} // namespace tspd::experiments
