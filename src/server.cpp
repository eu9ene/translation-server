
#include <algorithm>
#include <cstdlib>
#include <future>
#include <iostream>
#include <sstream>
#include <map>
#include <filesystem>

#include "translation.h"
#include "crow.h"

void run(int port, int workers, crow::LogLevel logLevel) {
    marian::bergamot::TranslatorWrapper wrapper(workers);
    wrapper.loadModels("/models");

    crow::SimpleApp app;

    CROW_ROUTE(app, "/v1/translate")
            .methods("POST"_method)
                    ([&wrapper](const crow::request &req) {
                        auto x = crow::json::load(req.body);
                        if (!x) {
                            CROW_LOG_WARNING << "Bad json: " << x;
                            return crow::response(400);
                        }

                        std::string from = (std::string) x["from"];
                        std::string to = (std::string) x["to"];
                        std::string input = (std::string) x["text"];

                        if (!wrapper.isSupported(from, to)) {
                            CROW_LOG_WARNING << "Language pair is not supported: " << from << to;
                            return crow::response(400);
                        }
                        CROW_LOG_DEBUG << "Starting translation from " << from << " to " << to << ": " << input;
                        auto result = wrapper.translate(from, to, input);
                        CROW_LOG_DEBUG << "Finished translation from " << from << " to " << to << ": " << result;

                        crow::json::wvalue jsonRes;
                        jsonRes["result"] = result;
                        return crow::response(jsonRes);
                    });

    CROW_ROUTE(app, "/__heartbeat__")
            ([] {
                return "Ready";
            });

    CROW_ROUTE(app, "/__lbheartbeat__")
            ([] {
                return "Ready";
            });

    app
    .port(port)
    .loglevel(logLevel)
    .multithreaded()
    .run();
}

std::string getEnvVar(const std::string &key, const std::string &defaultVal) {
    char *val = getenv(key.c_str());
    return val == NULL ? std::string(defaultVal) : std::string(val);
}

int main(int argc, char *argv[]) {
    auto port = std::stoi(getEnvVar("PORT", "8000"));

    auto logLevelVar = getEnvVar("LOG_LEVEL", "INFO");
    auto logLevel = crow::LogLevel::Info;
    if (logLevelVar == "WARNING")
        logLevel = crow::LogLevel::Warning;
    else if (logLevelVar == "ERROR")
        logLevel = crow::LogLevel::Error;
    else if (logLevelVar == "INFO")
        logLevel = crow::LogLevel::Info;
    else if (logLevelVar == "DEBUG")
        logLevel = crow::LogLevel::Debug;
    else {
        throw std::invalid_argument("Unknown logging level");
    }

    auto workers = std::stoi(getEnvVar("NUM_WORKERS", "1"));
    if (workers == 0)
        workers = std::thread::hardware_concurrency();

    run(port, workers, logLevel);

    return 0;
}
