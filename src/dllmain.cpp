#define WIN32_LEAN_AND_MEAN
#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <windows.h>
#include <filesystem>
#include <memory>
#include <thread>

#include <elden-x/params.hpp>
#include <elden-x/singletons.hpp>
#include <elden-x/utils/modutils.hpp>

#include "erdyes_apply_materials.hpp"
#include "erdyes_config.hpp"
#include "erdyes_local_player.hpp"
#include "erdyes_messages.hpp"
#include "erdyes_talkscript.hpp"

using namespace std;

static thread mod_thread;

static void setup_mod() {
    modutils::initialize();
    er::FD4::find_singletons();

    er::CS::SoloParamRepository::wait_for_params();

    spdlog::info("Sleeping for {}ms...", erdyes::config::initialize_delay);
    this_thread::sleep_for(chrono::milliseconds(erdyes::config::initialize_delay));

    spdlog::info("Hooking messages...");
    erdyes::setup_messages();

    erdyes::local_player::init();

    spdlog::info("Hooking talkscripts...");
    erdyes::setup_talkscript();

    erdyes::apply_materials_init();

    modutils::enable_hooks();
    spdlog::info("Initialized mod");
}

static shared_ptr<spdlog::logger> make_logger(const filesystem::path &path) {
    auto logger = make_shared<spdlog::logger>("dyes");
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] %^[%l]%$ %v");
    logger->sinks().push_back(
        make_shared<spdlog::sinks::daily_file_sink_st>(path.string(), 0, 0, false, 5));
    spdlog::set_default_logger(logger);
    return logger;
}

static void enable_debug_logging(shared_ptr<spdlog::logger> logger) {
    AllocConsole();
    FILE *stream;
    freopen_s(&stream, "CONOUT$", "w", stdout);
    freopen_s(&stream, "CONOUT$", "w", stderr);
    freopen_s(&stream, "CONIN$", "r", stdin);
    logger->sinks().push_back(make_shared<spdlog::sinks::stdout_color_sink_st>());
    logger->flush_on(spdlog::level::info);
    logger->set_level(spdlog::level::trace);
}

bool WINAPI DllMain(HINSTANCE dll_instance, unsigned int fdw_reason, void *lpv_reserved) {
    if (fdw_reason == DLL_PROCESS_ATTACH) {
        wchar_t dll_filename[MAX_PATH] = {0};
        GetModuleFileNameW(dll_instance, dll_filename, MAX_PATH);
        auto folder = filesystem::path(dll_filename).parent_path();

        auto logger = make_logger(folder / "logs" / "erdyes.log");

#ifdef _DEBUG
        enable_debug_logging(logger);
#endif

#ifdef PROJECT_VERSION
        spdlog::info("Armor dye mod version {}", PROJECT_VERSION);
#endif

        erdyes::load_config(folder / "erdyes.ini");

#ifndef _DEBUG
        if (erdyes::config::debug) {
            enable_debug_logging(logger);
        }
#endif

        mod_thread = thread([]() {
            try {
                setup_mod();
            } catch (runtime_error const &e) {
                spdlog::error("Error initializing mod: {}", e.what());
                modutils::deinitialize();
            }
        });
    } else if (fdw_reason == DLL_PROCESS_DETACH && lpv_reserved != nullptr) {
        try {
            mod_thread.join();
            modutils::deinitialize();
            spdlog::info("Deinitialized mod");
        } catch (runtime_error const &e) {
            spdlog::error("Error deinitializing mod: {}", e.what());
            spdlog::shutdown();
            return false;
        }
        spdlog::shutdown();
    }
    return true;
}