#ifndef MISSION_LOGGER_HPP
#define MISSION_LOGGER_HPP

#include <fstream>
#include <string>

// Writes one CSV row per dispatch decision. The file closes automatically
// when the object goes out of scope (RAII).
class MissionLogger {
private:
    std::ofstream out;

public:
    explicit MissionLogger(const std::string& filename) : out(filename) {
        if (out) out << "order_id,drone,destination,route,payload_kg,eta_sec,energy_wh,status\n";
    }

    void log(int order_id, const std::string& drone, const std::string& dest,
             const std::string& route, double payload_kg, double eta_sec,
             double energy_wh, const std::string& status) {
        if (!out) return;
        out << order_id << ',' << drone << ',' << dest << ",\"" << route << "\","
            << payload_kg << ',' << eta_sec << ',' << energy_wh << ',' << status << '\n';
    }
};

#endif // MISSION_LOGGER_HPP