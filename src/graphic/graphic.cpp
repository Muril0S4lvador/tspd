#include "graphic.hpp"

#include <iomanip>
#include <map>
#include <sstream>
#include <utility>

namespace {
    constexpr float overlayPanelBandHeight = 152.f;

    enum class OverlaySource { Solution, Literature };
    enum class OverlayVehicle { Truck, Drone };

    struct EdgeMembership {
        OverlaySource source;
        OverlayVehicle vehicle;
        int weight;
    };

    struct OverlayGeometry {
        int first;
        int second;
        vector<EdgeMembership> memberships;
    };

    using GeometryMap = std::map<std::pair<int, int>, OverlayGeometry>;

    struct RenderedEdge {
        const OverlayGeometry* geometry;
        OverlayVehicle vehicle;
        sf::Vector2f start;
        sf::Vector2f end;
        sf::Color color;
    };

    constexpr sf::Color solutionColor(35, 47, 62);
    constexpr sf::Color literatureColor(59, 130, 246);
    constexpr sf::Color commonColor(34, 197, 94);

    void addMembership(GeometryMap& geometries,
                       int first,
                       int second,
                       OverlaySource source,
                       OverlayVehicle vehicle,
                       int weight) {
        if (first == second) return;
        const auto key = std::minmax(first, second);
        auto [it, inserted] = geometries.try_emplace(
            std::pair<int, int>{key.first, key.second},
            OverlayGeometry{key.first, key.second, {}});
        auto& memberships = it->second.memberships;
        const auto duplicate = std::find_if(memberships.begin(), memberships.end(),
            [source, vehicle, weight](const EdgeMembership& membership) {
                return membership.source == source && membership.vehicle == vehicle &&
                       membership.weight == weight;
            });
        if (duplicate == memberships.end())
            memberships.push_back({source, vehicle, weight});
        (void)inserted;
    }

    GeometryMap buildOverlay(const TSPD& tspd,
                             const vector<int>& truckRoute,
                             const vector<int>& droneRoute) {
        GeometryMap geometries;
        const auto addRoute = [&](const vector<int>& route, OverlayVehicle vehicle) {
            if (route.size() < 2) return;
            for (size_t i = 0; i < route.size(); ++i) {
                const int first = route[i];
                const int second = route[(i + 1) % route.size()];
                if (first == second) continue;
                addMembership(geometries, first, second, OverlaySource::Solution,
                              vehicle, tspd.getDistance(first, second));
            }
        };
        addRoute(truckRoute, OverlayVehicle::Truck);
        addRoute(droneRoute, OverlayVehicle::Drone);

        if (tspd.hasLiteratureSolution()) {
            const auto& literature = tspd.getLiteratureSolution();
            for (const auto& edge : literature.getTruckEdges())
                addMembership(geometries, edge.getAId(), edge.getBId(),
                              OverlaySource::Literature, OverlayVehicle::Truck,
                              edge.getWeight());
            for (const auto& edge : literature.getDroneEdges())
                addMembership(geometries, edge.getAId(), edge.getBId(),
                              OverlaySource::Literature, OverlayVehicle::Drone,
                              edge.getWeight());
        }
        return geometries;
    }

    bool hasVehicle(const OverlayGeometry& geometry, OverlayVehicle vehicle) {
        return std::any_of(geometry.memberships.begin(), geometry.memberships.end(),
            [vehicle](const EdgeMembership& membership) {
                return membership.vehicle == vehicle;
            });
    }

    sf::Color edgeColor(const OverlayGeometry& geometry, OverlayVehicle vehicle) {
        bool hasSolution = false;
        bool hasLiterature = false;
        for (const auto& membership : geometry.memberships) {
            if (membership.vehicle != vehicle) continue;
            hasSolution |= membership.source == OverlaySource::Solution;
            hasLiterature |= membership.source == OverlaySource::Literature;
        }
        if (hasSolution && hasLiterature) return commonColor;
        return hasSolution ? solutionColor : literatureColor;
    }

    sf::Vector2f nodePosition(int nodeId, const vector<tspd::node::Node>& nodes) {
        const auto node = std::find_if(nodes.begin(), nodes.end(),
            [nodeId](const tspd::node::Node& candidate) { return candidate.getId() == nodeId; });
        if (node == nodes.end()) return {};
        return {static_cast<float>(node->getX()), static_cast<float>(node->getY())};
    }

    vector<RenderedEdge> renderedEdges(sf::RenderTarget& target,
                                       const GeometryMap& geometries,
                                       const vector<tspd::node::Node>& nodes,
                                       const sf::View& view) {
        vector<RenderedEdge> rendered;
        for (const auto& [key, geometry] : geometries) {
            (void)key;
            const auto startPixel = target.mapCoordsToPixel(nodePosition(geometry.first, nodes), view);
            const auto endPixel = target.mapCoordsToPixel(nodePosition(geometry.second, nodes), view);
            const sf::Vector2f start{static_cast<float>(startPixel.x), static_cast<float>(startPixel.y)};
            const sf::Vector2f end{static_cast<float>(endPixel.x), static_cast<float>(endPixel.y)};
            const sf::Vector2f delta = end - start;
            const float length = std::hypot(delta.x, delta.y);
            if (length <= 0.f) continue;

            const bool truck = hasVehicle(geometry, OverlayVehicle::Truck);
            const bool drone = hasVehicle(geometry, OverlayVehicle::Drone);
            const sf::Vector2f normal{-delta.y / length, delta.x / length};
            const auto add = [&](OverlayVehicle vehicle, float offset) {
                const sf::Vector2f displacement = normal * offset;
                rendered.push_back({&geometry, vehicle,
                                    start + displacement, end + displacement,
                                    edgeColor(geometry, vehicle)});
            };
            if (truck && drone) {
                add(OverlayVehicle::Truck, -3.f);
                add(OverlayVehicle::Drone, 3.f);
            } else if (truck) {
                add(OverlayVehicle::Truck, 0.f);
            } else if (drone) {
                add(OverlayVehicle::Drone, 0.f);
            }
        }
        return rendered;
    }

    void drawDottedLine(sf::RenderTarget& target,
                        const sf::Vector2f& start,
                        const sf::Vector2f& end,
                        const sf::Color& color,
                        float thickness) {
        const sf::Vector2f delta = end - start;
        const float length = std::hypot(delta.x, delta.y);
        if (length <= 0.f) return;

        const float radius = std::max(1.5f, thickness * 0.65f);
        const float spacing = std::max(5.f, radius * 3.2f);
        for (float along = 0.f; along <= length; along += spacing) {
            sf::CircleShape dot(radius);
            dot.setOrigin({radius, radius});
            dot.setPosition(start + delta * (along / length));
            dot.setFillColor(color);
            target.draw(dot);
        }
    }

    void drawSolidLine(sf::RenderTarget& target,
                       const sf::Vector2f& start,
                       const sf::Vector2f& end,
                       const sf::Color& color,
                       float thickness) {
        const sf::Vector2f delta = end - start;
        const float length = std::hypot(delta.x, delta.y);
        if (length <= 0.f) return;
        sf::RectangleShape line({length, thickness});
        line.setFillColor(color);
        line.setOrigin({0.f, thickness / 2.f});
        line.setPosition(start);
        line.setRotation(sf::radians(std::atan2(delta.y, delta.x)));
        target.draw(line);
    }

    void drawOverlayEdges(sf::RenderTarget& target,
                          const vector<RenderedEdge>& edges,
                          float thickness) {
        for (const auto& edge : edges) {
            if (edge.vehicle == OverlayVehicle::Drone)
                drawDottedLine(target, edge.start, edge.end, edge.color, thickness);
            else
                drawSolidLine(target, edge.start, edge.end, edge.color, thickness);
        }
    }

    const RenderedEdge* nearestRenderedEdge(const vector<RenderedEdge>& edges,
                                            const sf::Vector2f& point,
                                            float hitRadius = 8.f) {
        const RenderedEdge* closestEdge = nullptr;
        float closestDistance = hitRadius;
        for (const auto& edge : edges) {
            const float dx = edge.end.x - edge.start.x;
            const float dy = edge.end.y - edge.start.y;
            const float lengthSquared = dx * dx + dy * dy;
            const float t = lengthSquared == 0.f ? 0.f : std::clamp(
                ((point.x - edge.start.x) * dx + (point.y - edge.start.y) * dy) /
                    lengthSquared,
                0.f, 1.f);
            const float distance = std::hypot(point.x - edge.start.x - t * dx,
                                              point.y - edge.start.y - t * dy);
            if (distance < closestDistance) {
                closestDistance = distance;
                closestEdge = &edge;
            }
        }
        return closestEdge;
    }
}


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

    void Graphic::_drawLegend(sf::RenderTarget& target,
                              const Solution& solution,
                              const sf::Font& font) const {
        constexpr float margin = 16.f;
        constexpr float panelWidth = 350.f;
        constexpr float rowHeight = 22.f;
        constexpr float textSize = 15.f;
        const bool hasLiterature = tspd_.hasLiteratureSolution();
        const vector<std::pair<std::string, sf::Color>> sourceRows = hasLiterature
            ? vector<std::pair<std::string, sf::Color>>{
                {"Solution", solutionColor},
                {"Literature", literatureColor},
                {"Aresta em comum (mesmo tipo)", commonColor}}
            : vector<std::pair<std::string, sf::Color>>{{"Solution", solutionColor}};
        const float rowCount = static_cast<float>(sourceRows.size() + 2);
        const float panelHeight = rowCount * rowHeight + 18.f;
        const float panelX = margin;
        const float panelY = static_cast<float>(target.getSize().y) - panelHeight - margin;

        sf::RectangleShape background({panelWidth, panelHeight});
        background.setPosition({panelX, panelY});
        background.setFillColor(sf::Color(255, 255, 255, 238));
        background.setOutlineColor(sf::Color(100, 100, 100));
        background.setOutlineThickness(1.f);
        target.draw(background);

        const auto drawRow = [&](const std::string& label,
                                 const sf::Color& color,
                                 OverlayVehicle vehicle,
                                 size_t row) {
            const float y = panelY + 10.f + static_cast<float>(row) * rowHeight;
            if (vehicle == OverlayVehicle::Truck)
                drawSolidLine(target, {panelX + 12.f, y + 8.f}, {panelX + 52.f, y + 8.f},
                              color, 3.f);
            else
                drawDottedLine(target, {panelX + 12.f, y + 8.f}, {panelX + 52.f, y + 8.f},
                               color, 3.f);
            sf::Text text(font, sf::String::fromUtf8(label.begin(), label.end()),
                          static_cast<unsigned int>(textSize));
            text.setFillColor(sf::Color(35, 35, 35));
            text.setPosition({panelX + 64.f, y});
            target.draw(text);
        };

        size_t row = 0;
        for (const auto& [label, color] : sourceRows)
            drawRow(label, color, OverlayVehicle::Truck, row++);
        drawRow("Truck — linha sólida", sf::Color(85, 85, 85),
                OverlayVehicle::Truck, row++);
        drawRow("Drone — linha pontilhada", sf::Color(85, 85, 85),
                OverlayVehicle::Drone, row);

        vector<std::string> costRows{"Custo Solution: " + std::to_string(solution.getTotalCost())};
        if (hasLiterature) {
            const auto& literature = tspd_.getLiteratureSolution();
            if (literature.hasTotalCost()) {
                std::ostringstream literatureCost;
                literatureCost << std::fixed << std::setprecision(6)
                               << literature.getTotalCost();
                costRows.push_back("Custo Literature: " + literatureCost.str());
            } else {
                costRows.push_back("Custo Literature: indisponível");
            }
        }
        constexpr float costPanelWidth = 310.f;
        const float costPanelHeight = 18.f + static_cast<float>(costRows.size()) * 26.f;
        const float costPanelX = static_cast<float>(target.getSize().x) - costPanelWidth - margin;
        const float costPanelY = static_cast<float>(target.getSize().y) - costPanelHeight - margin;
        sf::RectangleShape costBackground({costPanelWidth, costPanelHeight});
        costBackground.setPosition({costPanelX, costPanelY});
        costBackground.setFillColor(sf::Color(255, 255, 255, 238));
        costBackground.setOutlineColor(sf::Color(100, 100, 100));
        costBackground.setOutlineThickness(1.f);
        target.draw(costBackground);

        for (size_t i = 0; i < costRows.size(); ++i) {
            sf::Text text(font, sf::String::fromUtf8(costRows[i].begin(), costRows[i].end()), 15);
            text.setFillColor(sf::Color(35, 35, 35));
            text.setPosition({costPanelX + 10.f,
                              costPanelY + 8.f + static_cast<float>(i) * 26.f});
            target.draw(text);
        }
    }

    void Graphic::_drawNodeNames(sf::RenderTarget& target,
                                 const vector<node::Node>& nodes,
                                 const sf::View& view,
                                 float radius,
                                 const sf::Font& font) const {
        const float pixel_radius = radius * target.getSize().x / view.getSize().x;
        for (size_t i = 0; i < nodes.size(); ++i) {
            const std::string name = nodes[i].getName();
            sf::Text text(font, sf::String::fromUtf8(
                name.begin(), name.end()), 13);
            const auto bounds = text.getLocalBounds();
            text.setOrigin(bounds.position + bounds.size / 2.f);
            const float scale = std::min(1.f, pixel_radius * 1.5f /
                std::max(1.f, std::max(bounds.size.x, bounds.size.y)));
            text.setScale({scale, scale});
            const auto pixel = target.mapCoordsToPixel(_getNodePosition(nodes[i].getId(), nodes), view);
            text.setPosition({static_cast<float>(pixel.x), static_cast<float>(pixel.y)});
            text.setFillColor(i == 0 ? sf::Color::Black : sf::Color::White);
            target.draw(text);
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
            mouse.y >= static_cast<int>(window.getSize().y) ||
            mouse.y >= static_cast<int>(window.getSize().y - overlayPanelBandHeight)) return;

        const GeometryMap geometries = buildOverlay(tspd_, truckRoute, droneRoute);
        const auto rendered = renderedEdges(window, geometries, nodes, view);
        const RenderedEdge* closestEdge = nearestRenderedEdge(
            rendered, {static_cast<float>(mouse.x), static_cast<float>(mouse.y)});
        if (!closestEdge) return;

        const auto name = [&](int id) {
            const auto node = std::find_if(nodes.begin(), nodes.end(),
                [id](const node::Node& candidate) { return candidate.getId() == id; });
            return node == nodes.end() ? std::to_string(id) : node->getName();
        };
        const auto sourceName = [](OverlaySource source) {
            return source == OverlaySource::Solution ? "Solution" : "Literature";
        };
        const auto vehicleName = [](OverlayVehicle vehicle) {
            return vehicle == OverlayVehicle::Truck ? "Truck" : "Drone";
        };

        std::string description = name(closestEdge->geometry->first) + " — " +
                                  name(closestEdge->geometry->second);
        for (const auto& membership : closestEdge->geometry->memberships) {
            description += "\n";
            description += sourceName(membership.source);
            description += " • ";
            description += vehicleName(membership.vehicle);
            description += "\nPeso: ";
            description += std::to_string(membership.weight);
        }

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

        const float windowWidth = static_cast<float>(window.getSize().x);
        const float windowHeight = static_cast<float>(window.getSize().y);
        const float graphHeight = std::max(1.f, windowHeight - overlayPanelBandHeight);
        view.setViewport(sf::FloatRect({0.f, 0.f}, {1.f, graphHeight / windowHeight}));

        if (!nodes.empty()) {
            const auto [minX, maxX] = std::minmax_element(nodes.begin(), nodes.end(),
                [](const auto& left, const auto& right) { return left.getX() < right.getX(); });
            const auto [minY, maxY] = std::minmax_element(nodes.begin(), nodes.end(),
                [](const auto& left, const auto& right) { return left.getY() < right.getY(); });
            const float width = std::max(1.f, static_cast<float>(maxX->getX() - minX->getX()));
            const float height = std::max(1.f, static_cast<float>(maxY->getY() - minY->getY()));
            const float viewAspect = windowWidth / graphHeight;
            tamanho_view = std::max(width, height * viewAspect) * _WINDOW_VIEW_ZOOM_ * 1.05f;
            view.setSize({tamanho_view, tamanho_view / viewAspect});
            view.setCenter({static_cast<float>((minX->getX() + maxX->getX()) / 2.0),
                            static_cast<float>((minY->getY() + maxY->getY()) / 2.0)});
        }

        const float raio = tamanho_view * _NODE_RADIUS_RATIO_;
        const float espessura_aresta = 3.f;
        const vector<sf::CircleShape> vertices = _createVertices(nodes, raio);
        const vector<int> truck_nodes = s.getTruckNodes();
        const vector<int> drone_nodes = s.getDroneNodes();
        const GeometryMap geometries = buildOverlay(tspd_, truck_nodes, drone_nodes);

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

            const auto edges = renderedEdges(window, geometries, nodes, view);
            window.setView(window.getDefaultView());
            drawOverlayEdges(window, edges, espessura_aresta);

            window.setView(view);
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
