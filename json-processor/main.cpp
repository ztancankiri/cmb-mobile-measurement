#include <iostream>
#include <vector>
#include <unordered_set>
#include <fstream>
#include "stdlib.h"
#include <nlohmann/json.hpp>
#include <dirent.h>
#include <sys/stat.h>
#include <chrono>
#include <unordered_set>
#include <unordered_map>

#define START_TIME 1642611600000000000
#define FINISH_TIME 1642806000000000000
#define HOUR_NSEC 3600000000000

#define SIZE ((FINISH_TIME - START_TIME) / HOUR_NSEC)
#define OFFSET(x) ((x - START_TIME) / HOUR_NSEC)

using json = nlohmann::json;

struct Entry {
    std::unordered_map<std::string, long> protocols;
    std::unordered_set<std::string> dns;
    std::unordered_set<std::string> ipv4;
    std::unordered_set<std::string> ipv6;
};

std::vector<std::string> get_files_in_directory(std::string sDir) {
    std::vector<std::string> result;

    DIR* dp;
    struct dirent* dirp;

    if ((dp = opendir(sDir.c_str())) == NULL) {
        return result;
    }

    while ((dirp = readdir(dp)) != NULL) {
        std::string filename(dirp->d_name);

        if (dirp->d_type == DT_REG) {
            std::stringstream ss_path;
            ss_path << sDir << "/" << filename;

            result.push_back(ss_path.str());
        }
    }

    closedir(dp);
    return result;
}

void read_json(json &obj, std::string filename) {
    std::ifstream file(filename);
    file >> obj;
    file.close();
}

void dump_json(json obj, std::string filename, int indent = 0) {
    std::ofstream file(filename);
    file << (indent == 0 ? obj.dump() : obj.dump(indent));
    file.close();
}

void wrap_json_file(std::string filename) {
    std::string body = "[";
    
    std::ifstream file(filename);
    std::string line;

    while (std::getline(file, line)) {
        body += line + ',';
    }

    body[body.length() - 1] = ']';
    file.close();

    std::ofstream output(filename);
    output << body;
    output.close();
}

bool protocolExists(std::string protocol, json protocols) {
    for (auto &iterator : protocols) {
        if (((std::string)iterator).compare(protocol) == 0) {
            return true;
        }
    }

    return false;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Missing arguments." << std::endl;
        std::cerr << "\tUsage: " << argv[0] << " <wrap_flag (0 or 1)> <directory_path>" << std::endl;
        return 1;
    }
    int wrap_flag = atoi(argv[1]);
    std::string dir_path(argv[2]);

    auto t_start = std::chrono::high_resolution_clock::now();

    std::vector<std::string> filenames = get_files_in_directory(dir_path);

    std::vector<std::unordered_map<std::string, Entry>> entries;
    entries.resize(SIZE);

    if (wrap_flag) {
        for (auto &filename : filenames) {
            wrap_json_file(filename);
        }
    }

    for (auto &filename : filenames) {
        json obj;
        read_json(obj, filename);

        for (auto &member : obj) {
            long currentOffset = OFFSET((long)member["ts"]);

            if (currentOffset >= 0 && currentOffset < SIZE) {
                std::string srcMac = member["srcMac"];
                std::string dstMac = member["dstMac"];
                
                if (entries[currentOffset].find(srcMac) == entries[currentOffset].end()) { // Not Found
                    struct Entry entry;
                    entries[currentOffset][srcMac] = entry;

                    entries[currentOffset][srcMac].protocols["UDP"] = 0;
                    entries[currentOffset][srcMac].protocols["TCP"] = 0;
                    entries[currentOffset][srcMac].protocols["IPv4"] = 0;
                    entries[currentOffset][srcMac].protocols["IPv6"] = 0;
                    entries[currentOffset][srcMac].protocols["ARP"] = 0;
                    entries[currentOffset][srcMac].protocols["ICMP"] = 0;
                    entries[currentOffset][srcMac].protocols["HTTP"] = 0;
                    entries[currentOffset][srcMac].protocols["SSH"] = 0;
                    entries[currentOffset][srcMac].protocols["SSL"] = 0;
                    entries[currentOffset][srcMac].protocols["DNS"] = 0;
                }

                if (entries[currentOffset].find(dstMac) == entries[currentOffset].end()) { // Not Found
                    struct Entry entry;
                    entries[currentOffset][dstMac] = entry;

                    entries[currentOffset][dstMac].protocols["UDP"] = 0;
                    entries[currentOffset][dstMac].protocols["TCP"] = 0;
                    entries[currentOffset][dstMac].protocols["IPv4"] = 0;
                    entries[currentOffset][dstMac].protocols["IPv6"] = 0;
                    entries[currentOffset][dstMac].protocols["ARP"] = 0;
                    entries[currentOffset][dstMac].protocols["ICMP"] = 0;
                    entries[currentOffset][dstMac].protocols["HTTP"] = 0;
                    entries[currentOffset][dstMac].protocols["SSH"] = 0;
                    entries[currentOffset][dstMac].protocols["SSL"] = 0;
                    entries[currentOffset][dstMac].protocols["DNS"] = 0;
                }

                for (auto &protocol : member["protocols"]) {
                    std::string p = protocol;
                    entries[currentOffset][srcMac].protocols[p]++;
                    entries[currentOffset][dstMac].protocols[p]++;
                }

                if (protocolExists("DNS", member["protocols"])) {
                    for (auto &domain : member["domains"]) {
                        std::string d = domain;
                        entries[currentOffset][srcMac].dns.insert(d);
                        entries[currentOffset][dstMac].dns.insert(d);
                    }
                }

                if (protocolExists("IPv4", member["protocols"])) {
                    std::string srcIP = member["srcIP"];
                    std::string dstIP = member["dstIP"];
                        
                    entries[currentOffset][srcMac].ipv4.insert(srcIP);
                    entries[currentOffset][srcMac].ipv4.insert(dstIP);
                    entries[currentOffset][dstMac].ipv4.insert(srcIP);
                    entries[currentOffset][dstMac].ipv4.insert(dstIP);
                }
                else if (protocolExists("IPv6", member["protocols"])) {
                    std::string srcIP = member["srcIP"];
                    std::string dstIP = member["dstIP"];
                        
                    entries[currentOffset][srcMac].ipv6.insert(srcIP);
                    entries[currentOffset][srcMac].ipv6.insert(dstIP);
                    entries[currentOffset][dstMac].ipv6.insert(srcIP);
                    entries[currentOffset][dstMac].ipv6.insert(dstIP);
                }
            }
        }
    }

    json result = json::array();

    for (int i = 0; i < SIZE; i++) {
        result.push_back(json::array());

        for (auto kv : entries[i]) {
            json entry;
            entry["MAC"] = kv.first;
            entry["protocols"] = kv.second.protocols;
            entry["dns"] = kv.second.dns;
            entry["ipv4"] = kv.second.ipv4;
            entry["ipv6"] = kv.second.ipv6;
            result[i].push_back(entry);
        }
    }

    dump_json(result, "result.json", 4);

    auto t_end = std::chrono::high_resolution_clock::now();
    double elapsed_time = std::chrono::duration<double, std::milli>(t_end-t_start).count() / 1000;
    std::cout << "Time elapsed: " << elapsed_time << " seconds" << std::endl;
    return 0;
}