#include "AirwayGraph.hpp"
#include "FleetManager.hpp"
#include "OrderScheduler.hpp"
#include "RoutingEngine.hpp"
#include <iostream>
#include <string>
#include <limits>
#include <vector>
#include <iomanip>
#include <cstdlib>
#include <algorithm>
#include <unordered_map>
#include "MissionLogger.hpp"
 
int promptInt(const std::string& label, const std::string& example, int min_val = 0) {
    int value;
    while (true) {
        std::cout << label << " (e.g., " << example << "): ";
        if (std::cin >> value && value >= min_val) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return value;
        }
        std::cout << "  [!] Invalid input. Enter an integer >= " << min_val << ".\n";
        if (std::cin.eof()) { std::cout << "\nInput closed. Exiting.\n"; std::exit(1); }
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

double promptDouble(const std::string& label, const std::string& example, double min_val = -1e9) {
    double value;
    while (true) {
        std::cout << label << " (e.g., " << example << "): ";
        if (std::cin >> value && value >= min_val) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return value;
        }
        std::cout << "  [!] Invalid decimal number.\n";
        if (std::cin.eof()) { std::cout << "\nInput closed. Exiting.\n"; std::exit(1); }
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

Vector3D promptCoordinates(const std::string& label, const std::string& example) {
    double x, y, z;
    while (true) {
        std::cout << label << " (e.g., " << example << "): ";
        if (std::cin >> x >> y >> z) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return {x, y, z};
        }
        std::cout << "  [!] Invalid 3D coordinates. Enter three space-separated numbers (X Y Z).\n";
        if (std::cin.eof()) { std::cout << "\nInput closed. Exiting.\n"; std::exit(1); }
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

std::string promptString(const std::string& label, const std::string& example) {
    std::string value;
    while (true) {
        std::cout << label << " (e.g., " << example << "): ";
        if (std::cin >> value && !value.empty()) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return value;
        }
        std::cout << "  [!] Field cannot be empty.\n";
        if (std::cin.eof()) { std::cout << "\nInput closed. Exiting.\n"; std::exit(1); }
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

// Keeps asking until the user enters a node ID that really exists in the graph.
int promptNodeId(const AirwayGraph& g, const std::string& label, const std::string& example, int min_id = 0) {
    while (true) {
        int id = promptInt(label, example, min_id);
        if (g.nodes.count(id)) return id;
        std::cout << "  [!] Node " << id << " does not exist. Valid IDs are " << min_id << " to "
                  << g.nodes.size() - 1 << ".\n";
    }
}

int main() {
    std::cout << COLOR_CYAN << "╔══════════════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║   AUTONOMOUS DRONE FLEET ROUTING MANAGEMENT SYSTEM (PHASE-II ENGINE)     ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════════════════╝\n" << COLOR_RESET;

    AirwayGraph graph;

    // 1. Central Depot
    std::cout << "\n>>> STEP 1: Central Launch Depot Setup\n";
    Vector3D depot_coords = promptCoordinates("  Enter Depot Coordinates [X Y Z]", "0 0 50");
    graph.addWaypoint(0, "Central_Depot", depot_coords, true);
    std::cout << "  [OK] Central Depot created at ID [0].\n";

    // 2. Waypoints
    std::cout << "\n>>> STEP 2: Delivery Waypoints Registration\n";
    int num_waypoints = promptInt("  Total delivery locations/zones", "3", 1);
    for (int i = 1; i <= num_waypoints; ++i) {
        std::cout << "\n  --- Location #" << i << " (Node ID: " << i << ") ---\n";
        std::string label = promptString("  Location Name/Label", (i == 1 ? "School" : (i == 2 ? "Temple" : "College")));
        Vector3D coords = promptCoordinates("  Location Coordinates [X Y Z]", (i == 1 ? "1000 500 60" : (i == 2 ? "1500 1200 70" : "2500 1800 65")));
        graph.addWaypoint(i, label, coords, false);
    }

    // 3. Corridors
    std::cout << "\n>>> STEP 3: Corridor Connections Setup\n";
    int num_corridors = promptInt("  Total corridors connecting zones", "3", 1);
    for (int i = 1; i <= num_corridors; ++i) {
        int u = promptNodeId(graph, "  Corridor #" + std::to_string(i) + " Start Node ID", "0");
        int v = promptNodeId(graph, "  Corridor #" + std::to_string(i) + " End Node ID", std::to_string(u == 0 ? 1 : u + 1));
        if (!graph.addCorridor(u, v, CorridorStatus::OPEN_AVAILABLE)) {
    std::cout << "  [!] Corridor skipped (same node, or it already exists). Please re-enter.\n";
    --i;
    continue;
}
std::cout << "  [OK] Corridor added: [" << graph.nodes.at(u).label << "] <---> [" << graph.nodes.at(v).label << "]\n";
    }

    // 4. Set Blocked / Red Corridors
    std::cout << "\n>>> STEP 4: Configure Restricted Airspace (RED ROUTES)\n";
    int num_blocked = promptInt("  How many corridors are BLOCKED (e.g., severe weather / no-fly)?", "1", 0);
    for (int i = 1; i <= num_blocked; ++i) {
        int bu = promptNodeId(graph, "  Blocked Route #" + std::to_string(i) + " Start Node ID", "2");
        int bv = promptNodeId(graph, "  Blocked Route #" + std::to_string(i) + " End Node ID", "3");
        graph.setCorridorStatus(bu, bv, CorridorStatus::BLOCKED_RESTRICTED);
        std::cout << COLOR_RED << "  [X] Corridor [" << graph.nodes[bu].label << "] <-> [" 
                  << graph.nodes[bv].label << "] marked as BLOCKED.\n" << COLOR_RESET;
    }

    // 5. Environmental Wind
    std::cout << "\n>>> STEP 5: Atmospheric Vector\n";
    Vector3D wind_v = promptCoordinates("  Wind Vector [Vx Vy Vz in m/s]", "-3.0 1.0 0");
    WindField ambient_wind{wind_v};

    // 6. Fleet
    std::cout << "\n>>> STEP 6: Drone Registration\n";
    FleetManager fleet;
    int num_drones = promptInt("  Number of drones in fleet", "2", 1);
    for (int i = 1; i <= num_drones; ++i) {
        std::cout << "\n  --- Drone #" << i << " ---\n";
        std::string callsign = promptString("  Drone Call-sign", (i == 1 ? "UAV-ALPHA" : "UAV-BETA"));
        double payload = promptDouble("  Max Payload (kg)", (i == 1 ? "8.0" : "12.0"), 0.1);
        double battery = promptDouble("  Battery Capacity (Wh)", (i == 1 ? "600" : "800"), 10.0);
        double speed = promptDouble("  Airspeed (m/s)", (i == 1 ? "16" : "14"), 1.0);
        fleet.registerDrone({callsign, payload, battery, battery, speed, 0, DroneStatus::IDLE});
    }

    // 7. Orders
    std::cout << "\n>>> STEP 7: Customer Delivery Orders\n";
    OrderScheduler scheduler;
    int num_orders = promptInt("  Total customer orders to schedule", "2", 1);
    for (int i = 1; i <= num_orders; ++i) {
        std::cout << "\n  --- Order #" << i << " ---\n";
        int order_id = promptInt("  Order ID", std::to_string(1100 + i), 1);
        int dest_node = promptNodeId(graph, "  Destination Node ID", (i == 1 ? "3" : "2"), 1);
        double weight = promptDouble("  Package Weight (kg)", (i == 1 ? "8.7" : "7.6"), 0.1);
        double deadline = promptDouble("  Deadline (seconds)", (i == 1 ? "278" : "237"), 1.0);
        scheduler.enqueueOrder({order_id, dest_node, weight, deadline});
    }

    // 8. Dispatch & Evaluation
    std::cout << "\n" << COLOR_YELLOW << ">>> RUNNING A* DISPATCH ORCHESTRATION..." << COLOR_RESET << "\n";

    MissionLogger logger("mission_log.csv");
    std::unordered_map<std::string, double> free_at;   // time (s) when each drone is back at the depot
    const double RESERVE = 1.20;                        // keep a 20% battery safety margin
    int accepted = 0, rejected = 0;
    double total_energy = 0.0, makespan = 0.0;

    auto routeString = [&](const std::vector<int>& path) {
        std::string r;
        for (size_t k = 0; k < path.size(); ++k) {
            r += graph.nodes.at(path[k]).label;
            if (k + 1 < path.size()) r += " -> ";
        }
        return r;
    };

    while (scheduler.hasPendingOrders()) {
        DeliveryOrder order = scheduler.popNextOrder();
        const std::string dest = graph.nodes.at(order.target_node_id).label;

        std::vector<std::string> candidates = fleet.getEligibleDrones(order.payload_kg);
        if (candidates.empty()) {
            std::cout << COLOR_RED << "  [X] Order #" << order.order_id
                      << " DEFERRED: no drone with >= " << order.payload_kg
                      << " kg capacity and >= 30% battery.\n" << COLOR_RESET;
            logger.log(order.order_id, "-", dest, "", order.payload_kg, 0, 0, "DEFERRED_NO_DRONE");
            ++rejected;
            continue;
        }

        // Try every eligible drone and keep the one that needs the least energy.
        // stage = furthest check any drone passed: 0 none, 1 reachable, 2 can return, 3 battery ok
        int stage = 0;
        std::string best_id;
        PathResult best_fwd, best_back;
        double best_need = std::numeric_limits<double>::infinity();

        for (const auto& id : candidates) {
            Drone* d = fleet.getDrone(id);
            PathResult fwd = RoutingEngine::computeAStarRoute(graph, d->current_node_id, order.target_node_id,
                                                              d->cruise_airspeed, order.payload_kg, ambient_wind);
            if (!fwd.reachable) continue;
            stage = std::max(stage, 1);

            // Return leg: empty drone flies back to the depot (node 0)
            PathResult back = RoutingEngine::computeAStarRoute(graph, order.target_node_id, 0,
                                                               d->cruise_airspeed, 0.0, ambient_wind);
            if (!back.reachable) continue;
            stage = std::max(stage, 2);

            double need = (fwd.total_energy_wh + back.total_energy_wh) * RESERVE;
            if (need > d->current_battery_wh) continue;
            stage = std::max(stage, 3);

            if (free_at[id] + fwd.total_time_sec > order.deadline_sec) continue;   // would arrive late

            if (need < best_need) { best_need = need; best_id = id; best_fwd = fwd; best_back = back; }
        }

        if (best_id.empty()) {
            static const char* reasons[] = {"UNREACHABLE", "NO_RETURN_PATH", "LOW_BATTERY", "DEADLINE_MISSED"};
            std::cout << COLOR_RED << "  [X] Order #" << order.order_id << " REJECTED (" << reasons[stage]
                      << ") -> " << dest << "\n" << COLOR_RESET;
            logger.log(order.order_id, "-", dest, "", order.payload_kg, 0, 0, reasons[stage]);
            ++rejected;
            continue;
        }

        double mission_energy = best_fwd.total_energy_wh + best_back.total_energy_wh;
        double mission_time   = best_fwd.total_time_sec + best_back.total_time_sec;
        double arrival        = free_at[best_id] + best_fwd.total_time_sec;
        free_at[best_id]     += mission_time;
        makespan              = std::max(makespan, free_at[best_id]);
        total_energy         += mission_energy;
        ++accepted;

        // Drone flew out and back: drain the battery and mark it idle at the depot again
        fleet.updateTelemetry(best_id, 0, mission_energy, DroneStatus::IDLE);

        std::cout << COLOR_GREEN << "  [✓] Order #" << order.order_id << " Assigned to " << best_id
                  << " | Route: " << routeString(best_fwd.path)
                  << " | ETA: " << std::fixed << std::setprecision(1) << arrival << "s"
                  << " (deadline " << order.deadline_sec << "s)"
                  << " | Round-trip energy: " << mission_energy << " Wh\n" << COLOR_RESET;

        for (size_t k = 0; k + 1 < best_fwd.path.size(); ++k)
            graph.setCorridorStatus(best_fwd.path[k], best_fwd.path[k + 1], CorridorStatus::RUNNING_MISSION);

        logger.log(order.order_id, best_id, dest, routeString(best_fwd.path), order.payload_kg,
                   arrival, mission_energy, "ASSIGNED");
    }

    // Fleet performance summary (makespan + energy, as promised in the project objectives)
    std::cout << "\n" << COLOR_CYAN << "=== FLEET PERFORMANCE SUMMARY ===" << COLOR_RESET << "\n";
    std::cout << std::fixed << std::setprecision(1);
    std::cout << "  Orders accepted : " << accepted << "   rejected/deferred: " << rejected << "\n";
    std::cout << "  Total energy    : " << total_energy << " Wh\n";
    std::cout << "  Makespan        : " << makespan << " s\n";
    std::cout << "  Drone batteries after all missions:\n";
    fleet.printStatus();
    std::cout << "  Mission log saved to mission_log.csv\n\n";

    // 9. Display Visual Elements
    graph.renderAsciiRadar();
    graph.renderCorridorStatusMatrix();
    graph.exportHtmlVisualizer("airspace_map.html");

    std::cout << "\nPhase-II Demonstration Complete.\n";
    return 0;
}