#include "game/camera.h"
#include "game/gdk.h"

#include "game/game.h"
#include "game/registry.h"
#include "game/render.h"
#include "game/window.h"

#include "simcoe/math/math.h"
#include "simcoe/simcoe.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"

#include <filesystem>

using namespace simcoe;
using namespace simcoe::input;

struct ImGuiRuntime {
    ImGuiRuntime() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        auto& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    }

    ~ImGuiRuntime() {
        ImGui::DestroyContext();
    }
};

struct Camera final : input::ITarget {
    Camera(game::Input& game, math::float3 position, float fov) 
        : camera(position, { 0, 0, 1 }, fov) 
        , input(game)
    { 
        debug = game::debug.newEntry({ "Camera" }, [&] {
            ImGui::SliderFloat("FOV", &camera.fov, 45.f, 120.f);
            ImGui::InputFloat3("Position", &camera.position.x);
            ImGui::InputFloat3("Direction", &camera.direction.x);
        });

        game.add(this);
    }

    void accept(const input::State& state) override {
        if (console.update(state.key[Key::keyTilde])) {
            gInputLog.info("console: {}", console ? "enabled" : "disabled");
        }

        ImGui::SetNextFrameWantCaptureKeyboard(console);
        ImGui::SetNextFrameWantCaptureMouse(console);

        if (console) { return; }

        float x = getAxis(state, Key::keyA, Key::keyD);
        float y = getAxis(state, Key::keyS, Key::keyW);
        float z = getAxis(state, Key::keyQ, Key::keyE);

        float yaw = state.axis[Axis::mouseHorizontal] * 0.01f;
        float pitch = -state.axis[Axis::mouseVertical] * 0.01f;

        camera.move({ x, y, z });
        camera.rotate(yaw, pitch);
    }

    void update(system::Window& window) {
        input.mouse.captureInput(!console);
        window.hideCursor(!console);
    }

    game::ICamera *getCamera() { return &camera; }

private:
    float getAxis(const input::State& state, Key low, Key high) {
        if (state.key[low] == 0 && state.key[high] == 0) { return 0.f; }

        return state.key[low] > state.key[high] ? -1.f : 1.f;
    }

    system::Timer timer;
    input::Toggle console = false;

    game::FirstPerson camera;
    game::Input& input;

    std::unique_ptr<util::Entry> debug;
};

static void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

namespace std {
    template<>
    struct hash<math::int2> {
        size_t operator()(const math::int2& v) const {
            return std::hash<int>()(v.x) ^ std::hash<int>()(v.y);
        }
    };
}

using math::int2;
using math::float2;

enum TileType {
    eStart,
    ePlayer,
    eGoal,
    eBasic,
    ePath,
    eImpassable,
    eTerrain
};

struct Tile {
    int traverseCost = 1;

    float currentCost = 0.f;
};

struct Grid {
    Grid(int size)
        : size(size)
        , tiles(new Tile[size * size])
    { 
        setTraverseCost(2, 2, 2);
        setTraverseCost(2, 3, 2);
        setTraverseCost(2, 4, 2);
        setTraverseCost(2, 5, 2);

        setTraverseCost(3, 2, 2);
        setTraverseCost(3, 3, 2);

        setTraverseCost(4, 2, 2);

        setTraverseCost(5, 2, 2);

        setImpassable(6, 2);
        setImpassable(6, 3);
        setImpassable(6, 4);

        calcCosts();
    }

    TileType getType(int x, int y) {
        if (!path.empty()) {
            auto [px, py] = path[distance];
            if (x == px && y == py) { return ePlayer; }
        }
        if (x == player[0] && y == player[1]) { return eStart; }
        if (x == goal[0] && y == goal[1]) { return eGoal; }
        if (getTraverseCost(x, y) == INT_MAX) { return eImpassable; }
        if (getTraverseCost(x, y) > 1) { return eTerrain; }

        bool isPath = std::find(path.begin(), path.end(), int2{ x, y }) != path.end();
        if (isPath) { return ePath; }

        return eBasic;
    }

    int getTraverseCost(int x, int y) {
        return tiles[x + y * size].traverseCost;
    }

    void setTraverseCost(int x, int y, int it) {
        tiles[x + y * size].traverseCost = it;
    }

    void setImpassable(int x, int y) {
        setTraverseCost(x, y, INT_MAX);
    }

    template<typename F>
    void calcTileValues(F&& func) {
        for (int y = 0; y < size; y++) {
            for (int x = 0; x < size; x++) {
                tiles[x + y * size].currentCost = func(x, y);
            }
        }
    }

    int size;

    Tile *tiles;

    int player[2] = { 1, 1 };
    int goal[2] = { 7, 8 };

    std::vector<int2> path;
    int distance = 0;

    std::string error;

    enum CostFunction : int { eEuclid, eManhatten, eTotal } cost = eEuclid;
    std::array<float(*)(float2), eTotal> costFunctions = {
        [](float2 a) { return std::sqrt(a.x * a.x + a.y * a.y); },
        [](float2 a) { return a.x + a.y; }
    };

    void calcCosts() {
        calcTileValues([&](int x, int y) -> float {
            float2 it = { float(goal[0] - x), float(goal[1] - y) };
            return costFunctions[cost](it) * getTraverseCost(x, y);
        });
    }

    float getCost(int x, int y) {
        return tiles[x + y * size].currentCost;
    }

    void toggleTile(int x, int y) {
        bool recalcPath = !path.empty();
        distance = 0;
        path.clear();
        error.clear();

        switch (getType(x, y)) {
        case eBasic:
            setTraverseCost(x, y, 2);
            break;
        case eTerrain:
            setImpassable(x, y);
            break;
        case eImpassable:
            setTraverseCost(x, y, 1);
            break;
        default:
            gLog.info("Can't toggle tile type {}", (int)getType(x, y));
            break;
        }

        calcCosts();
        if (recalcPath) {
            doAStar();
        }
    }

    void doAStar() {
        auto getNeighbors = [&](int x, int y) -> std::vector<int2> {
            std::vector<int2> neighbors;

            // add all 8 neighbors

            for (int i = -1; i <= 1; i++) {
                for (int j = -1; j <= 1; j++) {
                    if (i == 0 && j == 0) { continue; }

                    int nx = x + i;
                    int ny = y + j;

                    if (nx < 0 || nx >= size || ny < 0 || ny >= size) { continue; }

                    if (getTraverseCost(nx, ny) == INT_MAX) { continue; }

                    neighbors.push_back({ nx, ny });
                }
            }

            return neighbors;
        };

        // find the shortest path from start to gual using an A* search using the tiles currentCost as the heuristic
        
        std::unordered_set<int2> open = { int2 { player[0], player[1] } };

        std::unordered_map<int2, int2> cameFrom;
        std::unordered_map<int2, float> gscore;
        std::unordered_map<int2, float> fscore;

        gscore[{ player[0], player[1] }] = 0;
        fscore[{ player[0], player[1] }] = getCost(player[0], player[1]);

        auto getScore = [&](auto& map, const int2& it) {
            auto it2 = map.find(it);
            if (it2 == map.end()) { return FLT_MAX; }
            return it2->second;
        };

        while (!open.empty()) {
            auto current = *std::min_element(open.begin(), open.end(), [&](const int2& a, const int2& b) {
                return getScore(fscore, a) < getScore(fscore, b);
            });

            // if we are at the goal, we are done
            // reconstruct the path and return
            if (current.x == goal[0] && current.y == goal[1]) {
                path.clear();
                auto it = current;
                while (it != int2{ player[0], player[1] }) {
                    path.push_back(it);
                    it = cameFrom[it];
                }
                std::reverse(path.begin(), path.end());
                distance = 0;
                return;
            }

            open.erase(current);

            for (auto neighbor : getNeighbors(current.x, current.y)) {
                float tentative_gscore = getScore(gscore, current) + getTraverseCost(neighbor.x, neighbor.y);

                if (tentative_gscore < getScore(gscore, neighbor)) {
                    cameFrom[neighbor] = current;
                    gscore[neighbor] = tentative_gscore;
                    fscore[neighbor] = getScore(gscore, neighbor) + getCost(neighbor.x, neighbor.y);
                    open.insert(neighbor);
                }
            }
        }

        // no path found

        path.clear();
    
        error = "No path found";
    }
};

auto newGame() {
    return game::debug.newEntry({ "Game", true }, [grid = new Grid(10)] {
        auto drawTile = [&](int x, int y) -> std::string {
            std::string label;
            switch (grid->getType(x, y)) {
            case ePlayer: 
                ImGui::PushStyleColor(ImGuiCol_Button, { 0, 0.5f, 0, 1 });
                return "Player"; 
            case eGoal: 
                ImGui::PushStyleColor(ImGuiCol_Button, { 1, 0, 0, 1 });
                return "Goal"; 
            case eTerrain:
                ImGui::PushStyleColor(ImGuiCol_Button, { 0.5f, 0.5f, 0.5f, 1 });
                return std::format("{:.3f}##{}x{}", grid->getCost(x, y), x, y); 
            case eBasic: 
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_Button));
                return std::format("{:.3f}##{}x{}", grid->getCost(x, y), x, y); 
            case ePath:
                ImGui::PushStyleColor(ImGuiCol_Button, { 0.5f, 0.5f, 1, 1 });
                return std::format("Path##{}x{}", x, y);
            case eStart:
                ImGui::PushStyleColor(ImGuiCol_Button, { 0, 0.5f, 0, 1 });
                return "Start";
            case eImpassable:
                ImGui::PushStyleColor(ImGuiCol_Button, { 0, 0, 0, 1 });
                return std::format("No##{}x{}", x, y);

            default:
                ImGui::PushStyleColor(ImGuiCol_Button, { 1, 1, 0, 1 });
                return "Unknown";
            }
        };

        bool dirty = false;

        dirty |= ImGui::SliderInt2("Start", grid->player, 0, grid->size);
        dirty |= ImGui::SliderInt2("Goal", grid->goal, 0, grid->size);

        ImGui::SliderInt("Distance", &grid->distance, 0, std::max(int(grid->path.size()), 1) - 1);
        ImGui::SameLine();
        HelpMarker("Move the player along the path");

        if (ImGui::Combo("Cost function", (int*)&grid->cost, "Euclidean\0Manhattan\0\0", 2)) {
            dirty = true;
        }

        if (!grid->error.empty()) {
            ImGui::Text("Error: %s", grid->error.c_str());
        }

        if (ImGui::Button("A*")) {
            grid->doAStar();
            grid->distance = 0;
        }

        if (dirty) {
            grid->distance = 0;
            grid->calcCosts();
            grid->error = "";
        }

        ImGui::Separator();
        for (int x = 0; x < grid->size; x++) {
            for (int y = 0; y < grid->size; y++) {
                auto id = drawTile(x, y);                

                if (ImGui::Button(id.c_str(), { 64, 64 })) {
                    grid->toggleTile(x, y);
                }

                ImGui::PopStyleColor();
                if (y < grid->size - 1) { ImGui::SameLine(); }
            }
        }
    });
}

int commonMain() {
    game::Info detail;

    simcoe::addSink(&detail.sink);
    gLog.info("cwd: {}", std::filesystem::current_path().string());

    system::System system;
    ImGuiRuntime imgui;
    game::gdk::Runtime gdk;
    game::Input input;
    Camera camera { input, { 0, 0, 50 }, 90.f };

    game::Window window { input.mouse, input.keyboard };
    window.hideCursor(false);

    detail.windowResolution = window.size();
    detail.renderResolution = { 1920, 1080 };
    detail.pCamera = camera.getCamera();

    render::Context context { window, { } };
    game::Scene scene { context, detail, input.manager };

    ImGui_ImplWin32_Init(window.getHandle());
    ImGui_ImplWin32_EnableDpiAwareness();

    auto game = newGame();

    scene.start();

    while (window.poll()) {
        input.poll();
        scene.execute();

        // TODO: move camera
    }

    scene.stop();

    ImGui_ImplWin32_Shutdown();

    gLog.info("bye bye");

    return 0;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return commonMain();
}

int main(int, const char **) {
    return commonMain();
}
