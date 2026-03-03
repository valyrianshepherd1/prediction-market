#include <drogon/drogon.h>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace fs = std::filesystem;

static fs::path exeDirFromArgv0(char **argv) {
    // argv[0] в IDE может быть относительным
    fs::path p = argv && argv[0] ? fs::path(argv[0]) : fs::path{};
    if (p.empty())
        return fs::current_path();

    if (p.is_relative())
        p = fs::absolute(p);

    // weakly_canonical не падает, если части пути не существуют
    std::error_code ec;
    p = fs::weakly_canonical(p, ec);
    if (ec)
        p = fs::absolute(argv[0]);

    return p.parent_path();
}

static fs::path resolveConfigPath(int argc, char **argv) {
    // 1) --config <path>
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string_view(argv[i]) == "--config") {
            return fs::path(argv[i + 1]);
        }
    }

    // 2) ENV PM_CONFIG
    if (const char *env = std::getenv("PM_CONFIG"); env && *env) {
        return fs::path(env);
    }

    // 3) рядом с бинарём: <exe_dir>/config/config.json
    const auto exeDir = exeDirFromArgv0(argv);
    const auto candidate = exeDir / "config" / "config.json";
    if (fs::exists(candidate))
        return candidate;

    // 4) из текущей директории: ./config/config.json
    const auto cwdCandidate = fs::current_path() / "config" / "config.json";
    if (fs::exists(cwdCandidate))
        return cwdCandidate;

    // 5) удобный fallback: ./backend/config/config.json
    const auto repoCandidate = fs::current_path() / "backend" / "config" / "config.json";
    if (fs::exists(repoCandidate))
        return repoCandidate;

    throw std::runtime_error(
        "config.json not found. Use --config <path> or set PM_CONFIG env var.");
}

int main(int argc, char **argv) {
    try {
        fs::path configPath = resolveConfigPath(argc, argv);

        if (configPath.is_relative())
            configPath = fs::absolute(configPath);

        if (!fs::exists(configPath)) {
            throw std::runtime_error("Config file does not exist: " + configPath.string());
        }

        fs::current_path(configPath.parent_path());

        drogon::app().loadConfigFile(configPath.string());
        drogon::app().run();
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "Failed to start: " << e.what() << "\n";
        return 1;
    }
}
