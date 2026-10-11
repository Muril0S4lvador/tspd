#include "graphic.hpp"



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

    void Graphic::_drawNodeNames(sf::RenderWindow& window,
                                 const vector<node::Node>& nodes,
                                 const sf::View& view,
                                 float radius,
                                 const sf::Font& font) const {
        const float pixel_radius = radius * window.getSize().x / view.getSize().x;
        for (size_t i = 0; i < nodes.size(); ++i) {
            const std::string name = nodes[i].getName();
            sf::Text text(font, sf::String::fromUtf8(
                name.begin(), name.end()), 13);
            const auto bounds = text.getLocalBounds();
            text.setOrigin(bounds.position + bounds.size / 2.f);
            const float scale = std::min(1.f, pixel_radius * 1.5f /
                std::max(1.f, std::max(bounds.size.x, bounds.size.y)));
            text.setScale({scale, scale});
            const auto pixel = window.mapCoordsToPixel(_getNodePosition(nodes[i].getId(), nodes), view);
            text.setPosition({static_cast<float>(pixel.x), static_cast<float>(pixel.y)});
            text.setFillColor(i == 0 ? sf::Color::Black : sf::Color::White);
            window.draw(text);
        }
    }

    void Graphic::_drawEdgeTooltip(sf::RenderWindow& window,
                                   const vector<int>& truckRoute,
                                   const vector<int>& droneRoute,
                                   const vector<node::Node>& nodes,
                                   const sf::View& view,
                                   const sf::Font& font) const {
        if (!window.hasFocus()) return;
        const auto mouse = sf::Mouse::getPosition(window);
        if (mouse.x < 0 || mouse.y < 0 || mouse.x >= static_cast<int>(window.getSize().x) ||
            mouse.y >= static_cast<int>(window.getSize().y)) return;

        std::string description;
        const auto inspectRoute = [&](const vector<int>& route, const std::string& type) {
            if (route.size() < 2) return;
            float closest = 7.f;
            int first = -1, second = -1;
            for (size_t i = 0; i < route.size(); ++i) {
                const int a = route[i], b = route[(i + 1) % route.size()];
                if (a == b) continue;
                const auto start = window.mapCoordsToPixel(_getNodePosition(a, nodes), view);
                const auto end = window.mapCoordsToPixel(_getNodePosition(b, nodes), view);
                const float dx = static_cast<float>(end.x - start.x);
                const float dy = static_cast<float>(end.y - start.y);
                const float length_squared = dx * dx + dy * dy;
                const float t = length_squared == 0.f ? 0.f : std::clamp(
                    ((mouse.x - start.x) * dx + (mouse.y - start.y) * dy) / length_squared, 0.f, 1.f);
                const float distance = std::hypot(mouse.x - start.x - t * dx,
                                                  mouse.y - start.y - t * dy);
                if (distance < closest) {
                    closest = distance;
                    first = a;
                    second = b;
                }
            }
            if (first == -1) return;
            const auto name = [&](int id) {
                return std::find_if(nodes.begin(), nodes.end(),
                    [id](const node::Node& n) { return n.getId() == id; })->getName();
            };
            if (!description.empty()) description += "\n\n";
            description += name(first) + " -- " + name(second) + "\n" + type + "\n" +
                           std::to_string(tspd_.getDistance(first, second)) + " Weight";
        };
        inspectRoute(truckRoute, "Truck");
        inspectRoute(droneRoute, "Drone");
        if (description.empty()) return;

        sf::Text text(font, sf::String::fromUtf8(description.begin(), description.end()), 16);
        text.setFillColor(sf::Color::Black);
        const auto bounds = text.getLocalBounds();
        const sf::Vector2f size = bounds.size + sf::Vector2f{20.f, 20.f};
        const float x = std::clamp(mouse.x + 14.f, 0.f,
            std::max(0.f, static_cast<float>(window.getSize().x) - size.x));
        const float y = std::clamp(mouse.y + 14.f, 0.f,
            std::max(0.f, static_cast<float>(window.getSize().y) - size.y));
        sf::RectangleShape background(size);
        background.setPosition({x, y});
        background.setFillColor(sf::Color(255, 255, 255, 245));
        background.setOutlineColor(sf::Color(100, 100, 100));
        background.setOutlineThickness(1.f);
        text.setPosition(sf::Vector2f{x + 10.f, y + 10.f} - bounds.position);
        window.draw(background);
        window.draw(text);
    }

    void Graphic::draw(){

        sf::RenderWindow window(sf::VideoMode(_WINDOW_SIZE_), "Grafo TSPD - " + tspd_.getName());

        vector<node::Node> nodes = tspd_.getNodes();
        sf::View view;

        float tamanho_view = 0.f;
        _configureView(view, nodes, tamanho_view);

        window.setView(view);

        float raio = tamanho_view * _NODE_RADIUS_RATIO_;
        vector<sf::CircleShape> vertices = _createVertices(nodes, raio);
        sf::Font font;
        const bool font_available = font.openFromFile("C:/Windows/Fonts/arial.ttf");

        while (window.isOpen()) {
            while (const auto event = window.pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    window.close();
                }
            }

            window.clear(_WINDOW_BACKGROUND_COLOR_);
            window.setView(view);

            for (const auto& vertice : vertices) {
                window.draw(vertice);
            }

            if (font_available) {
                window.setView(window.getDefaultView());
                _drawNodeNames(window, nodes, view, raio, font);
            }
            window.display();
        }

    }

    void Graphic::drawSolution(Solution& s){
        sf::RenderWindow window(sf::VideoMode(_WINDOW_SIZE_), "solution");

        vector<node::Node> nodes = tspd_.getNodes();
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
                _drawNodeNames(window, nodes, view, raio, font);
                _drawLegend(window, s, font);
                _drawEdgeTooltip(window, truck_nodes, drone_nodes, nodes, view, font);
            }

            window.display();
        }
    }

}
