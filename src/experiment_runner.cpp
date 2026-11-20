#include "experiment_runner.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <ctime>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <locale>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "agatz_solution/exact_partiotioning_algorithm/exactPartitioning.hpp"
#include "agatz_solution/greedy_heuristic/greedy_heuristic.hpp"
#ifdef TSPD_ENABLE_GRAPHICS
#include "graphic/graphic.hpp"
#endif
#include "instance_reader/instance_reader.hpp"
#include "TSP/TSPSolver.hpp"
#include "hgvns/HGVNS.hpp"
#include "hga_tac/HGATAC.hpp"

namespace tspd::experiments {
namespace {
namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;
using Strategy = ::tspd::solution::SolutionStrategy;
using Instance = ::tspd::tspd::TSPD;
using Solution = ::tspd::solution::Solution;

struct Algorithm {
    std::string id;
    bool stochastic;
    std::function<std::unique_ptr<Strategy>(std::uint32_t)> create;
};

const std::vector<Algorithm>& registry() {
    static const std::vector<Algorithm> algorithms{
        {"tsp", false, [](std::uint32_t) { return std::make_unique<tsp::TSPSolver>(); }},
        {"greedy", false, [](std::uint32_t) { return std::make_unique<greedyHeuristic::GreedyHeuristic>(); }},
        {"exact-partitioning", false, [](std::uint32_t) { return std::make_unique<exactPartitioning::ExactPartitioning>(); }},
        {"hgvns", true, [](std::uint32_t seed) { return std::make_unique<hgvns::HGVNS>(seed); }},
        {"hgvns-pd-waiting-rvnd", true, [](std::uint32_t seed) {
            return std::make_unique<hgvns::HGVNS>(seed, hgvns::BlockSelection::Waiting,
                hgvns::BlockParameters{}, hgvns::PDPlacement::AfterRVND);
        }},
        {"hga-tac", true, [](std::uint32_t seed) { return std::make_unique<hga_tac::HGATAC>(seed); }},
        {"hga-tac-plus", true, [](std::uint32_t seed) { return std::make_unique<hga_tac::HGATAC>(seed, true); }}
    };
    return algorithms;
}

void usage(const char* program) {
    std::cout << "Uso: " << program
              << " <arquivo.txt|diretorio> [--graphics] [--algorithms id,id] [--seed numero | --seeds inicio:fim]\n"
              << "--graphics: abre a visualizacao de um arquivo individual no Windows.\n"
              << "Diretorios: busca recursiva, sem graficos; solutions/ e ignorado.\n"
              << "Sem --algorithms: executa todos; com --seeds, somente os estocasticos.\n"
              << "TSP: calculado uma vez por instancia e compartilhado com as heuristicas.\n"
              << "--seed: semente de HGVNS e HGA-TAC, inteiro sem sinal de 32 bits; padrao 0.\n"
              << "--seeds: intervalo inclusivo (1:9) ou lista (1,3,7), ate 1000 sementes distintas.\n"
              << "O lote --seeds e sequencial, sem graficos, e gera dois CSVs por semente.\n"
              << "Algoritmos:";
    for (const auto& algorithm : registry()) std::cout << ' ' << algorithm.id;
    std::cout << "\nResultados: results/rodada-<data-hora>.csv e results/analise-<data-hora>.csv\n";
}

struct Options {
    fs::path input;
    bool graphics = false;
    std::vector<std::uint32_t> seeds{0};
    bool seedBatch = false;
    std::set<std::string> selected;
};

std::uint32_t parseSeed(const std::string& value, const std::string& option) {
    if (value.empty() || !std::all_of(value.begin(), value.end(), [](unsigned char c) { return std::isdigit(c); }))
        throw std::invalid_argument(option + " exige um inteiro sem sinal de 32 bits.");
    std::uint64_t number = 0;
    for (const char digit : value) {
        number = number * 10 + static_cast<unsigned>(digit - '0');
        if (number > std::numeric_limits<std::uint32_t>::max())
            throw std::invalid_argument(option + " excede o limite de 32 bits.");
    }
    return static_cast<std::uint32_t>(number);
}

std::vector<std::uint32_t> parseSeeds(const std::string& list) {
    constexpr std::size_t maxSeeds = 1000;
    std::vector<std::uint32_t> seeds;
    std::set<std::uint32_t> seen;
    std::size_t begin = 0;
    do {
        const auto end = list.find(',', begin);
        const auto item = list.substr(begin, end == std::string::npos ? end : end - begin);
        const auto colon = item.find(':');
        const auto first = parseSeed(item.substr(0, colon), "--seeds");
        const auto last = colon == std::string::npos ? first : parseSeed(item.substr(colon + 1), "--seeds");
        if (first > last) throw std::invalid_argument("--seeds exige inicio <= fim; exemplo: --seeds 1:9.");
        if (static_cast<std::uint64_t>(last) - first + 1 > maxSeeds)
            throw std::invalid_argument("--seeds aceita ate 1000 sementes distintas por lote.");
        // O contador de 64 bits permite incluir 4294967295 sem transbordar.
        for (std::uint64_t seed = first; seed <= last; ++seed) {
            if (seen.insert(static_cast<std::uint32_t>(seed)).second) {
                if (seeds.size() == maxSeeds)
                    throw std::invalid_argument("--seeds aceita ate 1000 sementes distintas por lote.");
                seeds.push_back(static_cast<std::uint32_t>(seed));
            }
        }
        if (end == std::string::npos) break;
        begin = end + 1;
    } while (true);
    return seeds;
}

Options parse(int argc, char* argv[]) {
    Options options;
    bool singleSeed = false;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--graphics") {
            options.graphics = true;
        } else if (argument == "--no-graphics") {
            options.graphics = false;
        } else if (argument == "--seed") {
            if (i + 1 == argc) throw std::invalid_argument("--seed exige um inteiro sem sinal de 32 bits.");
            options.seeds = {parseSeed(argv[++i], "--seed")};
            singleSeed = true;
        } else if (argument == "--seeds") {
            if (i + 1 == argc)
                throw std::invalid_argument("--seeds exige um intervalo (1:9) ou lista (1,3,7).");
            if (options.seedBatch) throw std::invalid_argument("Informe --seeds uma unica vez.");
            options.seeds = parseSeeds(argv[++i]);
            options.seedBatch = true;
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
    if (singleSeed && options.seedBatch)
        throw std::invalid_argument("Use --seed ou --seeds, nunca as duas opcoes juntas.");
    if (options.seedBatch) {
        for (const auto& algorithm : registry())
            if (options.selected.count(algorithm.id) && !algorithm.stochastic)
                throw std::invalid_argument("Algoritmo '" + algorithm.id +
                    "' e deterministico. --seeds aceita somente metodos estocasticos.");
    }
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

struct Measurement {
    std::optional<double> cost, seconds;
    std::string status = "not_selected", error;
};

// Explicit optimum metadata; a DP reference by itself is not an optimality certificate.
std::optional<double> knownOptimum(const fs::path& path) {
    if (!fs::exists(path)) return std::nullopt;
    std::ifstream input(path);
    input.imbue(std::locale::classic());
    double value;
    std::string extra;
    if (!(input >> value) || !std::isfinite(value) || value < 0 || (input >> extra))
        throw std::invalid_argument("Otimo conhecido invalido: " + path.string());
    return value;
}

void analysisHeader(std::ostream& output) {
    output << "name,instance_path,nodes,seed,optimal_tspd_cost,optimal_source,"
              "literature_tspd_cost,literature_tsp_cost,tsp_result,tsp_seconds,tsp_status,tsp_error";
    for (const auto& algorithm : registry()) if (algorithm.id != "tsp")
        output << ',' << algorithm.id << "_result," << algorithm.id << "_seconds,"
               << algorithm.id << "_total_seconds," << algorithm.id << "_status," << algorithm.id << "_error";
    output << '\n';
}

struct ReportFiles {
    std::uint32_t seed;
    fs::path detailPath, analysisPath;
    std::size_t successes = 0, failures = 0;
};

std::ofstream openOutput(const fs::path& path, std::ios::openmode mode = std::ios::out) {
    std::ofstream output;
    output.exceptions(std::ios::failbit | std::ios::badbit);
    output.open(path, mode);
    output.imbue(std::locale::classic());
    output << std::setprecision(17);
    return output;
}

std::vector<ReportFiles> createReports(const Options& options) {
    fs::create_directories("results");
    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::ostringstream timestamp;
    timestamp << std::put_time(std::localtime(&now), "%Y%m%d-%H%M%S");
    std::vector<ReportFiles> reports;
    for (unsigned suffix = 0;; ++suffix) {
        reports.clear();
        bool collision = false;
        for (const auto seed : options.seeds) {
            auto name = timestamp.str();
            if (suffix) name += "-" + std::to_string(suffix);
            if (options.seedBatch) name += "-seed-" + std::to_string(seed);
            const auto detail = fs::path("results") / ("rodada-" + name + ".csv");
            const auto analysis = fs::path("results") / ("analise-" + name + ".csv");
            if (fs::exists(detail) || fs::exists(analysis)) { collision = true; break; }
            reports.push_back({seed, detail, analysis});
        }
        if (!collision) break;
    }
    for (const auto& files : reports) {
        auto report = openOutput(files.detailPath);
        report << "name,algorithm,tspd_cost,tsp_cost,literature_tspd_cost,literature_tsp_cost,solve_seconds,status,error";
        // Preserva o formato antigo para --seed e para a execucao padrao.
        if (options.seedBatch) report << ",seed,instance_path";
        report << '\n';
        report.flush();
        auto analysis = openOutput(files.analysisPath);
        analysisHeader(analysis);
        analysis.flush();
    }
    return reports;
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
            if ((options.selected.empty() || options.selected.count(a.id)) &&
                (!options.seedBatch || a.stochastic)) algorithms.push_back(a);
#ifdef TSPD_ENABLE_GRAPHICS
        const bool graphics = options.graphics && !options.seedBatch && fs::is_regular_file(options.input);
#else
        const bool graphics = false;
#endif
        auto reports = createReports(options);
        std::size_t successes = 0, failures = 0, completed = 0, failedInstances = 0;
        const auto start = Clock::now();
        std::cout << "Instancias: " << found.files.size() << " | Algoritmos: " << algorithms.size()
                  << " | Graficos: " << (graphics ? "sim" : "nao")
                  << '\n';
        if (options.seedBatch) {
            std::cout << "Sementes (" << options.seeds.size() << "):";
            for (const auto seed : options.seeds) std::cout << ' ' << seed;
            std::cout << '\n';
        } else {
            if (options.selected.empty() || std::any_of(options.selected.begin(), options.selected.end(),
                    [](const std::string& id) { return id.rfind("hgvns", 0) == 0; }))
                std::cout << "Semente HGVNS: " << options.seeds.front() << '\n';
            if (options.selected.empty() || options.selected.count("hga-tac") || options.selected.count("hga-tac-plus"))
                std::cout << "Semente HGA-TAC: " << options.seeds.front() << '\n';
        }
        for (const auto& files : reports)
            std::cout << "CSV: " << fs::absolute(files.detailPath).string()
                      << "\nCSV analise: " << fs::absolute(files.analysisPath).string() << '\n';
        for (std::size_t index = 0; index < found.files.size(); ++index) {
            const auto& path = found.files[index];
            std::cout << '[' << index + 1 << '/' << found.files.size() << "] " << path.string() << std::endl;
            std::optional<Instance> source;
            std::string readError;
            std::optional<double> literatureTspdCost, literatureTspCost;
            std::optional<double> optimum;
            const auto optimumPath = path.parent_path() / "solutions" / (path.stem().string() + "-optimal.txt");
            try {
                if (!isInstance(path)) throw std::invalid_argument("Formato de instancia nao suportado.");
                source.emplace(instance_reader::InstanceReader::readInstance(path.string()));
                validate(*source);
                optimum = knownOptimum(optimumPath);
                if (source->hasLiteratureTspSolution() && source->getLiteratureTspSolution().hasTotalCost())
                    literatureTspCost = source->getLiteratureTspSolution().getTotalCost();
                // O leitor usa TSP como fallback; nao o apresenta como referencia TSP-D.
                const auto dpPath = path.parent_path() / "solutions" /
                    (path.stem().string() + "-DP" + path.extension().string());
                if (source->hasLiteratureSolution() && source->getLiteratureSolution().hasTotalCost() &&
                    (fs::exists(dpPath) || !source->hasLiteratureTspSolution()))
                    literatureTspdCost = source->getLiteratureSolution().getTotalCost();
            } catch (const std::exception& error) { readError = error.what(); }
            std::optional<Solution> tspSolution;
            double tspSeconds = 0;
            std::string tspError;
            if (readError.empty()) {
                const auto tspStart = Clock::now();
                try {
                    auto instance = *source;
                    tsp::TSPSolver solver;
                    tspSolution = solver.Solve(instance);
                    tspSeconds = solver.getDuration().count();
                } catch (const std::exception& error) {
                    tspSeconds = elapsed(tspStart);
                    tspError = std::string("Falha ao calcular o TSP inicial: ") + error.what();
                }
            }
            bool instanceFailed = false;
            for (auto& files : reports) {
                const auto seed = files.seed;
                if (options.seedBatch) std::cout << "  Semente: " << seed << std::endl;
                // Abre somente o par em uso; o numero de sementes nao acumula descritores.
                auto report = openOutput(files.detailPath, std::ios::app);
                auto analysis = openOutput(files.analysisPath, std::ios::app);
                std::vector<Measurement> measurements(registry().size());
                for (const auto& algorithm : algorithms) {
                    std::optional<double> cost;
                    std::optional<double> tspCost;
                    double solveSeconds = 0;
                    std::string error = readError.empty() ? tspError : readError;
                    if (!tspError.empty()) solveSeconds = tspSeconds;
                    if (error.empty()) {
                        const auto solveStart = Clock::now();
                        try {
                            auto instance = *source;
                            if (!tspSolution)
                                throw std::logic_error("TSPSolver nao disponibilizou uma solucao TSP inicial.");
                            auto solution = [&]() {
                                if (algorithm.id == "tsp") {
                                    // A linha TSP reutiliza a solucao e o tempo ja calculados.
                                    solveSeconds = tspSeconds;
                                    return *tspSolution;
                                }
                                auto solver = algorithm.create(seed);
                                auto result = solver->Solve(instance, *tspSolution);
                                solveSeconds = solver->getDuration().count();
                                if (const auto* hgvns = dynamic_cast<const hgvns::HGVNS*>(solver.get())) {
                                    const auto& stats = hgvns->statistics();
                                    if (stats.pdCalls) std::cout << "    PD: chamadas=" << stats.pdCalls
                                        << ", ordens=" << stats.ordersEvaluated
                                        << ", blocos melhorados=" << stats.blocksAccepted << '\n';
                                }
                                return result;
                            }();
                            if (!std::isfinite(solution.getTotalCost()))
                                throw std::runtime_error("Algoritmo retornou custo nao finito.");
                            cost = solution.getTotalCost();
                            tspCost = solution.getTspCost();
#ifdef TSPD_ENABLE_GRAPHICS
                            if (graphics) {
                                graphic::Graphic view(instance);
                                view.drawSolution(solution);
                            }
#endif
                        } catch (const std::exception& exception) {
                            if (!cost) solveSeconds = elapsed(solveStart);
                            error = exception.what();
                        }
                    }
                    if (error.empty()) {
                        ++successes;
                        ++files.successes;
                        std::cout << "  " << algorithm.id << ": custo=" << *cost
                                  << ", tempo=" << solveSeconds << "s\n";
                    } else {
                        ++failures;
                        ++files.failures;
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
                           << (error.empty() ? "success" : "error") << ',' << csv(error);
                    if (options.seedBatch) report << ',' << seed << ',' << csv(fs::absolute(path).string());
                    report << '\n';
                    report.flush();
                    const auto slot = std::find_if(registry().begin(), registry().end(),
                        [&](const Algorithm& a) { return a.id == algorithm.id; }) - registry().begin();
                    measurements[slot] = {cost, readError.empty() && tspError.empty() ? std::optional<double>(solveSeconds) : std::nullopt,
                                          error.empty() ? "success" : "error", error};
                }
                analysis << csv(path.filename().string()) << ',' << csv(fs::absolute(path).string()) << ',';
                if (source) analysis << source->getDimension();
                analysis << ',' << seed << ',';
                writeCost(analysis, optimum);
                analysis << ',' << csv(optimum ? fs::absolute(optimumPath).string() : "") << ',';
                writeCost(analysis, literatureTspdCost);
                analysis << ',';
                writeCost(analysis, literatureTspCost);
                analysis << ',';
                if (tspSolution) analysis << tspSolution->getTotalCost();
                analysis << ',';
                if (readError.empty()) analysis << tspSeconds;
                const auto initialError = readError.empty() ? tspError : readError;
                analysis << ',' << (initialError.empty() ? "success" : "error") << ',' << csv(initialError);
                for (std::size_t i = 0; i < registry().size(); ++i) if (registry()[i].id != "tsp") {
                    const auto& measurement = measurements[i];
                    analysis << ',';
                    writeCost(analysis, measurement.cost);
                    analysis << ',';
                    writeCost(analysis, measurement.seconds);
                    analysis << ',';
                    if (measurement.seconds && tspSolution) analysis << tspSeconds + *measurement.seconds;
                    analysis << ',' << measurement.status << ',' << csv(measurement.error);
                }
                analysis << '\n';
                analysis.flush();
            }
            if (instanceFailed) ++failedInstances;
            else ++completed;
        }
        std::cout << "Rodada concluida em " << elapsed(start) << "s.\n"
                  << "Instancias selecionadas: " << found.files.size()
                  << " | Concluidas: " << completed << " | Com falhas: " << failedInstances << '\n'
                  << "Execucoes com sucesso: " << successes << " | Com falhas: " << failures
                  << " | Erros de descoberta: " << found.errors << '\n';
        for (const auto& files : reports) {
            if (options.seedBatch)
                std::cout << "Semente " << files.seed << ": sucesso=" << files.successes
                          << " | falhas=" << files.failures << '\n';
            std::cout << "CSV: " << fs::absolute(files.detailPath).string()
                      << "\nCSV analise: " << fs::absolute(files.analysisPath).string() << '\n';
        }
        return failures || found.errors ? 1 : 0;
    } catch (const std::exception& error) {
        std::cerr << "Erro: " << error.what() << '\n';
        usage(argv[0]);
        return 1;
    }
}
} // namespace tspd::experiments
