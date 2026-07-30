#pragma once

#include <vector>
#include <algorithm>
#include <numeric>

#include <iostream>

#include <SFML/Graphics.hpp>

#include "../tspd/tspd.hpp"

using std::vector;
using tspd::tspd::TSPD;

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