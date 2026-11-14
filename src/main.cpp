#include <cstdlib>
#include <optional>
#include <string>
#include <vector>

#include "game/game.hpp"
#include "persistence/data_dir.hpp"
#include "persistence/env_file.hpp"

int main(int argc, char** argv) {
    const std::vector<std::string> args(argv, argv + argc);

    // `.env` in the working directory (project root in dev) fills in variables that are not
    // set in the real environment.
    const char* process_dir = std::getenv("PROJECT_MINE_DATA_DIR");
    const auto data_dir_value = persistence::env_value(
        "PROJECT_MINE_DATA_DIR",
        process_dir ? std::optional<std::string>(process_dir) : std::nullopt,
        persistence::read_env_file(".env"));

    game::Game game(persistence::resolve_data_dir(args, data_dir_value));
    game.start_up();
    game.run();
    return 0;
}
