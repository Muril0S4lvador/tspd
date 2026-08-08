#pragma once

#include <vector>
#include <algorithm>
#include <numeric>

#include <iostream>

#include <SFML/Graphics.hpp>

#include "../tspd/tspd.hpp"

using std::vector;
using tspd::tspd::TSPD;

#define _WINDOW_SIZE_ {800u, 800u} // u - unsigned int
#define _WINDOW_VIEW_ZOOM_ 1.07f
#define _WINDOW_BACKGROUND_COLOR_ sf::Color(240, 240, 240)
// representa a proporção do tamanho da tela que o nó vai ocupar
#define _NODE_RADIUS_RATIO_ 0.005f

#define _DEPOSIT_COLOR_ sf::Color(0, 255, 255)
#define _DEPOSIT_OUTLINE_COLOR_ sf::Color::Black
#define _DEPOSIT_OUTLINE_THICKNESS_RATIO_ 0.6f

#define _VERTICES_COLOR_ sf::Color(50, 50, 50)

namespace tspd::graphic{
    class Graphic{
        private:
            TSPD tspd_;

            void _configureView(sf::View& view, const vector<node::Node>& nodes, float& tamanho_view);

        public:
            Graphic(TSPD tspd);

            void draw();
    };
}