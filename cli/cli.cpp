#include "cli.hpp"
#include "../common/template.pb.h"
#include <notcurses/notcurses.h>

struct GameUI {
    ncplane* world;
    ncplane* info;
    ncplane* options;
};

GameUI createUI(ncplane* stdplane)
{
    unsigned rows, cols;
    ncplane_dim_yx(stdplane, &rows, &cols);

    constexpr unsigned border = 1;

    if (rows < 5 || cols < 10)
        return {};

    const unsigned content_rows = rows - 2 * border;
    const unsigned content_cols = cols - 2 * border;

    // Sidebar = 1/4 de la largeur
    const unsigned sidebar_cols = content_cols / 4;
    const unsigned world_cols = content_cols - sidebar_cols;

    // Options = 1/4 de la hauteur de la sidebar
    const unsigned options_rows = content_rows / 4;
    const unsigned info_rows = content_rows - options_rows;

    GameUI ui{};

    // ┌───────────────────────────────┬──────────┐
    // │                               │          │
    // │                               │   info   │
    // │            world              │          │
    // │                               ├──────────┤
    // │                               │ options  │
    // └───────────────────────────────┴──────────┘

    ncplane_options world_opts{
        .y = static_cast<int>(border),
        .x = static_cast<int>(border),
        .rows = content_rows,
        .cols = world_cols,
        .userptr = nullptr,
        .name = "world",
        .resizecb = nullptr,
        .flags = 0,
        .margin_b = 0,
        .margin_r = 0,
    };

    ui.world = ncplane_create(stdplane, &world_opts);

    ncplane_options info_opts{
        .y = static_cast<int>(border),
        .x = static_cast<int>(border + world_cols),
        .rows = info_rows,
        .cols = sidebar_cols,
        .userptr = nullptr,
        .name = "info",
        .resizecb = nullptr,
        .flags = 0,
        .margin_b = 0,
        .margin_r = 0,
    };

    ui.info = ncplane_create(stdplane, &info_opts);

    ncplane_options options_opts{
        .y = static_cast<int>(border + info_rows),
        .x = static_cast<int>(border + world_cols),
        .rows = options_rows,
        .cols = sidebar_cols,
        .userptr = nullptr,
        .name = "options",
        .resizecb = nullptr,
        .flags = 0,
        .margin_b = 0,
        .margin_r = 0,
    };

    ui.options = ncplane_create(stdplane, &options_opts);

    if (!ui.world || !ui.info || !ui.options) {
        if (ui.world)
            ncplane_destroy(ui.world);
        if (ui.info)
            ncplane_destroy(ui.info);
        if (ui.options)
            ncplane_destroy(ui.options);

        return {};
    }

    return ui;
}

void drawBorder(ncplane* plane)
{
    unsigned rows, cols;
    ncplane_dim_yx(plane, &rows, &cols);

    if (rows < 2 || cols < 2)
        return;

    // Coins
    ncplane_putstr_yx(plane, 0, 0, "┌");
    ncplane_putstr_yx(plane, 0, cols - 1, "┐");
    ncplane_putstr_yx(plane, rows - 1, 0, "└");
    ncplane_putstr_yx(plane, rows - 1, cols - 1, "┘");

    // Haut / bas
    for (unsigned x = 1; x < cols - 1; ++x) {
        ncplane_putstr_yx(plane, 0, x, "─");
        ncplane_putstr_yx(plane, rows - 1, x, "─");
    }

    // Gauche / droite
    for (unsigned y = 1; y < rows - 1; ++y) {
        ncplane_putstr_yx(plane, y, 0, "│");
        ncplane_putstr_yx(plane, y, cols - 1, "│");
    }
}

bool initRender(notcurses* nc, ncplane* stdplane)
{
    GameUI ui = createUI(stdplane);

    if (!ui.world || !ui.info || !ui.options)
        return false;

    drawBorder(ui.world);
    drawBorder(ui.info);
    drawBorder(ui.options);

    ncplane_putstr_yx(ui.info, 1, 2, "INFORMATIONS");

    ncplane_putstr_yx(ui.options, 1, 2, "OPTIONS");
    ncplane_putstr_yx(ui.options, 3, 2, "1. Attaquer");
    ncplane_putstr_yx(ui.options, 4, 2, "2. Inventaire");
    ncplane_putstr_yx(ui.options, 5, 2, "3. Fuir");

    // Exemple de contenu du monde
    ncplane_putstr_yx(ui.world, 2, 2, "WORLD");

    // Joueur
    ncplane_putstr_yx(ui.world, 10, 20, "█");

    return notcurses_render(nc) >= 0;
}

void game_loop(Socket server_socket)
{

    struct notcurses_options ncopt{};

    struct notcurses* nc = notcurses_init(&ncopt, stdout);
    struct ncplane* stdplane = notcurses_stdplane(nc);

    if (!initRender(nc, stdplane)) {
        notcurses_stop(nc);
        return;
    }

    // while (true) {

    //     std::string client_response{};

    //     game::WorldDelta change{};
    //     game::RcvStatus status{};
    //     auto server_response =
    //         server_socket.receive_message<game::WorldDelta>(game::RcvStatus & status);
    //     if (game::RcvStatus::OK) {
    //         switch (server_response.delta_type_case()) {

    //         case game::WorldDelta::kUnauthorizedCommand:
    //             handleUnauthorizedCommand(server_response, stdplane);
    //         case game::WorldDelta::kPlayerConnected:
    //             handlePlayerConnected(server_response, stdplane);
    //         case game::WorldDelta::kPlayerDisconnected:
    //             handlePlayerDisconnected(server_response, stdplane);
    //         case game::WorldDelta::kWorldInitiation:
    //             handleWorldInit(server_response, stdplane);
    //         case game::WorldDelta::kMessageReceived:
    //             handleMessageReceived(server_response, stdplane);
    //         case game::WorldDelta::kPlayerInventoryChanged:
    //             handlePlayerInventoryChanged(server_response, stdplane);
    //         case game::WorldDelta::kNpcInventoryChanged:
    //             handleNpcInventoryChanged(server_response, stdplane);
    //         case game::WorldDelta::kPlayerHpChanged:
    //             handlePlayerHpChanged(server_response, stdplane);
    //         case game::WorldDelta::kNpcHpChanged:
    //             handleNpcHpChanged(server_response, stdplane);
    //         case game::WorldDelta::kPlayerMoved:
    //             handlePlayerMoved(server_response, stdplane);
    //         case game::WorldDelta::kNpcMoved:
    //             hanleNpcMoved(server_response stdplane);
    //         case game::WorldDelta::kDoorUnlock:
    //             handleDoorUnlock(server_response stdplane);
    //         case game::WorldDelta::kPlayerStateChanged:
    //             handlePlayerStateChanged(server_response, stdplane);
    //         case game::WorldDelta::DELTA_TYPE_NOT_SET:
    //             break;
    //         }
    //     }
    //     else {
    //         break;
    //     }
    // }
    sleep(20);
    notcurses_stop(nc);
    server_socket.close();
}

int main()
{
    Socket server_socket(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
    if (server_socket.get() < 0)
        return (perror("Socket creation error"), EXIT_FAILURE);

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1332);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(server_socket.get(), reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0)
        return (perror("connection error"), EXIT_FAILURE);

    game_loop(std::move(server_socket));
    return (0);
}
