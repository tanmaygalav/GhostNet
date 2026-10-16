#include "ghostnet/scenario_loader.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char* argv[]) {
    std::string scenario_json_content;

    if (argc >= 3 && std::string(argv[1]) == "--scenario") {
        std::ifstream file(argv[2]);
        if (!file.is_open()) {
            std::cerr << "[GHOSTNET CLI ERROR] Unable to open scenario file: " << argv[2] << "\n";
            return 1;
        }
        std::ostringstream ss;
        ss << file.rdbuf();
        scenario_json_content = ss.str();
    } else {
        // Read directly from stdin
        std::ostringstream ss;
        ss << std::cin.rdbuf();
        scenario_json_content = ss.str();
    }

    if (scenario_json_content.empty()) {
        std::cerr << "Usage: ghostnet_cli --scenario <path_to_scenario.json>\n"
                  << "       cat scenario.json | ghostnet_cli\n";
        return 1;
    }

    try {
        auto scenario = ghostnet::ScenarioLoader::from_string(scenario_json_content);

        std::cerr << "[GHOSTNET] Starting discrete-event simulation...\n";
        std::cerr << "[GHOSTNET] Nodes: " << scenario.topology.node_count()
                  << " | Links: " << scenario.topology.link_count()
                  << " | Seed: " << scenario.engine_config.seed << "\n";

        ghostnet::SimulationEngine engine(scenario.engine_config, std::move(scenario.topology));

        for (const auto& w : scenario.workloads) {
            engine.add_workload(w);
        }

        auto status = engine.run();
        auto metrics = engine.get_metrics();

        nlohmann::json output = ghostnet::ScenarioLoader::metrics_to_json(
            status, engine.current_time(), engine.processed_event_count(), metrics
        );

        // Authoritative machine-readable output sent exclusively to stdout
        std::cout << output.dump(2) << std::endl;

        std::cerr << "[GHOSTNET] Run finished with status: " << output["status"] << "\n";
        return 0;

    } catch (const std::exception& ex) {
        std::cerr << "[GHOSTNET CLI ERROR] " << ex.what() << "\n";
        return 1;
    }
}