#include "Graphic.hpp"
#include "../utils/utils.hpp"



namespace tspd::graphic{

    void Graphic::_configureView(sf::View& view, const vector<node::Node>& nodes, float& tamanho_view) {
        if (nodes.empty()) return;

        auto [min_x_it, max_x_it] = std::minmax_element(
            nodes.begin(), nodes.end(),
            [](const auto& a, const auto& b) { return a.getX() < b.getX(); }
        );
        auto [min_y_it, max_y_it] = std::minmax_element(
            nodes.begin(), nodes.end(),
            [](const auto& a, const auto& b) { return a.getY() < b.getY(); }
        );

        float min_x = min_x_it->getX();
        float max_x = max_x_it->getX();
        float min_y = min_y_it->getY();
        float max_y = max_y_it->getY();

        float centro_x = (min_x + max_x) / 2.0f;
        float centro_y = (min_y + max_y) / 2.0f;

        float largura_grafo = max_x - min_x;
        float altura_grafo = max_y - min_y;

        if (largura_grafo <= 0.001f) largura_grafo = 1.0f;
        if (altura_grafo <= 0.001f)  altura_grafo  = 1.0f;

        float maior_dimensao = std::max(largura_grafo, altura_grafo);
        
        tamanho_view = maior_dimensao * _WINDOW_VIEW_ZOOM_;

        view.setCenter({centro_x, centro_y});
        view.setSize({tamanho_view, tamanho_view});
    }

    Graphic::Graphic(TSPD tspd) : tspd_(tspd) {}

    vector<sf::CircleShape> Graphic::_createVertices(const vector<node::Node>& nodes, float raio) const {
        vector<sf::CircleShape> vertices;

        for (size_t i = 0; i < nodes.size(); i++) {
            sf::CircleShape vertice(raio);
            if (i == 0) {
                vertice.setFillColor(_DEPOSIT_COLOR_);
                vertice.setOutlineColor(_DEPOSIT_OUTLINE_COLOR_);
                vertice.setOutlineThickness(raio * _DEPOSIT_OUTLINE_THICKNESS_RATIO_);
            } else {
                vertice.setFillColor(_VERTICES_COLOR_);
            }

            vertice.setOrigin({raio, raio});
            vertice.setPosition({static_cast<float>(nodes[i].getX()), static_cast<float>(nodes[i].getY())});

            vertices.push_back(vertice);
        }

        return vertices;
    }

    sf::Vector2f Graphic::_getNodePosition(int node_id, const vector<node::Node>& nodes) const {
        const auto node_it = std::find_if(
            nodes.begin(), nodes.end(),
            [node_id](const node::Node& node) { return node.getId() == node_id; }
        );

        return {static_cast<float>(node_it->getX()), static_cast<float>(node_it->getY())};
    }

    void Graphic::_drawLine(sf::RenderWindow& window,
                            const sf::Vector2f& inicio,
                            const sf::Vector2f& fim,
                            const sf::Color& cor,
                            float espessura) const {
        const float delta_x = fim.x - inicio.x;
        const float delta_y = fim.y - inicio.y;
        const float comprimento = std::sqrt(delta_x * delta_x + delta_y * delta_y);

        if (comprimento == 0.f)
            return;

        sf::RectangleShape linha({comprimento, espessura});
        linha.setFillColor(cor);
        linha.setOrigin({0.f, espessura / 2.f});
        linha.setPosition(inicio);
        linha.setRotation(sf::radians(std::atan2(delta_y, delta_x)));

        window.draw(linha);
    }

    void Graphic::_drawRoute(sf::RenderWindow& window,
                             const vector<int>& rota,
                             const vector<node::Node>& nodes,
                             const sf::Color& cor,
                             bool tracejada,
                             float espessura) const {
        if (rota.size() < 2)
            return;

        for (size_t i = 0; i < rota.size(); i++) {
            const sf::Vector2f inicio = _getNodePosition(rota[i], nodes);
            const sf::Vector2f fim = _getNodePosition(rota[(i + 1) % rota.size()], nodes);

            if (!tracejada) {
                _drawLine(window, inicio, fim, cor, espessura);
                continue;
            }

            const float delta_x = fim.x - inicio.x;
            const float delta_y = fim.y - inicio.y;
            const float comprimento = std::sqrt(delta_x * delta_x + delta_y * delta_y);
            const float tamanho_traco = espessura * 6.f;
            const float tamanho_espaco = espessura * 4.f;

            for (float inicio_traco = 0.f;
                 inicio_traco < comprimento;
                 inicio_traco += tamanho_traco + tamanho_espaco) {
                const float fim_traco = std::min(inicio_traco + tamanho_traco, comprimento);
                const float proporcao_inicio = inicio_traco / comprimento;
                const float proporcao_fim = fim_traco / comprimento;

                _drawLine(window,
                          {inicio.x + delta_x * proporcao_inicio, inicio.y + delta_y * proporcao_inicio},
                          {inicio.x + delta_x * proporcao_fim, inicio.y + delta_y * proporcao_fim},
                          cor,
                          espessura);
            }
        }
    }

    void Graphic::_drawLegend(sf::RenderWindow& window,
                              const Solution& solution,
                              const sf::Font& font) const {
        constexpr float margem = 16.f;
        constexpr float largura = 310.f;
        constexpr float altura = 118.f;
        constexpr unsigned int tamanho_texto = 18;

        const float posicao_y = static_cast<float>(window.getSize().y) - altura - margem;

        sf::RectangleShape fundo({largura, altura});
        fundo.setPosition({margem, posicao_y});
        fundo.setFillColor(sf::Color(255, 255, 255, 230));
        fundo.setOutlineColor(sf::Color(100, 100, 100));
        fundo.setOutlineThickness(1.f);

        sf::Text titulo(font, "Legenda de custos", tamanho_texto);
        titulo.setPosition({margem + 12.f, posicao_y + 8.f});
        titulo.setFillColor(sf::Color::Black);

        sf::Text custo_caminhao(font, "Custo do caminhao: " + std::to_string(solution.getTruckCost()), tamanho_texto);
        custo_caminhao.setPosition({margem + 12.f, posicao_y + 38.f});
        custo_caminhao.setFillColor(sf::Color::Blue);

        sf::Text custo_drone(font, "Custo do drone: " + std::to_string(solution.getDroneCost()), tamanho_texto);
        custo_drone.setPosition({margem + 12.f, posicao_y + 63.f});
        custo_drone.setFillColor(sf::Color::Green);

        sf::Text custo_total(font, "Custo total: " + std::to_string(solution.getTotalCost()), tamanho_texto);
        custo_total.setPosition({margem + 12.f, posicao_y + 88.f});
        custo_total.setFillColor(sf::Color::Black);

        window.draw(fundo);
        window.draw(titulo);
        window.draw(custo_caminhao);
        window.draw(custo_drone);
        window.draw(custo_total);
    }

    void Graphic::draw(){

        sf::RenderWindow window(sf::VideoMode(_WINDOW_SIZE_), "Grafo TSPD - " + tspd_.getName());

        vector<node::Node> nodes = tspd_.getNodes();
        if(nodes.empty())
            nodes = ::tspd::utils::calculateNodesFromDistances(tspd_.getEdges(), tspd_.getDimension());

        sf::View view;

        float tamanho_view = 0.f;
        _configureView(view, nodes, tamanho_view);

        window.setView(view);

        float raio = tamanho_view * _NODE_RADIUS_RATIO_;
        vector<sf::CircleShape> vertices = _createVertices(nodes, raio);

        while (window.isOpen()) {
            while (const auto event = window.pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    window.close();
                }
            }

            window.clear(_WINDOW_BACKGROUND_COLOR_);

            for (const auto& vertice : vertices) {
                window.draw(vertice);
            }

            window.display();
        }

    }

    void Graphic::drawSolution(Solution& s){
        sf::RenderWindow window(sf::VideoMode(_WINDOW_SIZE_), "solution");

        vector<node::Node> nodes = tspd_.getNodes();
        if(nodes.empty())
            nodes = ::tspd::utils::calculateNodesFromDistances(tspd_.getEdges(), tspd_.getDimension());

        sf::View view;
        float tamanho_view = 0.f;
        _configureView(view, nodes, tamanho_view);

        const float raio = tamanho_view * _NODE_RADIUS_RATIO_;
        const float espessura_aresta = tamanho_view * 0.0025f;
        const vector<sf::CircleShape> vertices = _createVertices(nodes, raio);
        const vector<int> truck_nodes = s.getTruckNodes();
        const vector<int> drone_nodes = s.getDroneNodes();

        sf::Font font;
        const bool legenda_disponivel = font.openFromFile("C:/Windows/Fonts/arial.ttf");

        while (window.isOpen()) {
            while (const auto event = window.pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    window.close();
                }
            }

            window.clear(_WINDOW_BACKGROUND_COLOR_);
            window.setView(view);

            _drawRoute(window, truck_nodes, nodes, sf::Color::Blue, false, espessura_aresta);
            _drawRoute(window, drone_nodes, nodes, sf::Color::Green, true, espessura_aresta);

            for (const auto& vertice : vertices) {
                window.draw(vertice);
            }

            if (legenda_disponivel) {
                window.setView(window.getDefaultView());
                _drawLegend(window, s, font);
            }

            window.display();
        }
    }

}
