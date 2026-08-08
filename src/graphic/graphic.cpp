#include "Graphic.hpp"

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

    void Graphic::draw(){

        sf::RenderWindow window(sf::VideoMode(_WINDOW_SIZE_), "Grafo TSPD - " + tspd_.getName());

        vector<node::Node> nodes = tspd_.getNodes();
        sf::View view;

        float tamanho_view = 0.f;
        _configureView(view, nodes, tamanho_view);

        window.setView(view);

        std::vector<sf::CircleShape> vertices;
        float raio = tamanho_view * _NODE_RADIUS_RATIO_;

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
}