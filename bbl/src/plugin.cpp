#include "plugin.hpp"
#include "bbl-plugin.h"

#include <spdlog/spdlog.h>

#include <cstdlib>

namespace bbl {

Plugin::Plugin(PluginHandle handle,
               std::string filename,
               std::string name,
               PluginExec fn_exec)
    : _handle(handle), filename(std::move(filename)), name(std::move(name)),
      fn_exec(fn_exec) {}

Plugin::Plugin(Plugin&& rhs) {
    _handle = rhs._handle;
    rhs._handle = NULL;

    filename = std::move(rhs.filename);

    fn_exec = rhs.fn_exec;
    rhs.fn_exec = nullptr;
}

Plugin::~Plugin() {
#ifdef _WIN32
    FreeLibrary(_handle);
#else

#endif
}

#ifdef _WIN32
static std::vector<Plugin>
get_plugins(std::vector<std::string> const& plugin_paths) {
    std::vector<Plugin> result;

    WIN32_FIND_DATA file_data;
    for (auto const& path : plugin_paths) {
        std::string pattern = fmt::format("{}/*.dll", path);
        HANDLE file_handle = FindFirstFile(pattern.c_str(), &file_data);

        if (file_handle == (void*)ERROR_INVALID_HANDLE ||
            file_handle == (void*)ERROR_FILE_NOT_FOUND) {
            continue;
        }

        do {
            std::string filename =
                fmt::format("{}/{}", path, file_data.cFileName);
            HINSTANCE handle = LoadLibrary(filename.c_str());

            if (!handle) {
                SPDLOG_ERROR("could not load {}", filename);
                continue;
            }

            PluginInit fn_init =
                (PluginInit)GetProcAddress(handle, "bbl_plugin_init");

            if (fn_init == nullptr) {
                SPDLOG_ERROR("could not load bbl_plugin_init function from {}",
                             filename);
                continue;
            }

            char const* name = "";
            int version = 0;
            fn_init(&name, &version);

            if (version != BBL_PLUGIN_API_VERSION) {
                SPDLOG_ERROR("plugin {} has API version {}, which is not "
                             "compatible with {}",
                             filename,
                             version,
                             BBL_PLUGIN_API_VERSION);
                continue;
            }

            PluginExec fn_exec = (PluginExec)GetProcAddress(handle, "bbl_plugin_exec");
            if (fn_exec == nullptr) {
                SPDLOG_ERROR("could not load bbl_plugin_exec function from {}",
                             filename);
                continue;
            }

            result.emplace_back(Plugin(handle, filename, name, fn_exec));

        } while (FindNextFile(file_handle, &file_data));
    }

    return result;
}

#else

static std::vector<Plugin>
get_plugins(std::vector<std::string> const& plugin_paths) {
    std::vector<Plugin> result;
    for (auto const& path : plugin_paths) {
    }
    return result;
}

#endif

PluginManager::PluginManager() {
#define _CRT_SECURE_NO_WARNINGS
    // parse BBL_PLUGIN_PATH env var and load all plugins found within
    char* plugin_path_ev = std::getenv("BBL_PLUGIN_PATH");

    if (plugin_path_ev == nullptr || strlen(plugin_path_ev) == 0) {
        return;
    }

    std::vector<std::string> plugin_paths;

#ifdef _WIN32
    char const* delim = ";";
#else
    char const* delim = ":";
#endif

    char* token = strtok(plugin_path_ev, delim);
    while (token != nullptr) {
        SPDLOG_INFO("{}", token);
        plugin_paths.push_back(std::string(token));
        token = strtok(NULL, delim);
    }

    _plugins = get_plugins(plugin_paths);
}

void PluginManager::exec(bbl_context_t cpp_ctx, bbl_capi_t capi, char const* output_path) {
    for (auto const& plugin: _plugins) {
        plugin.fn_exec(cpp_ctx, capi, output_path);
    }
}

} // namespace bbl