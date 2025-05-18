#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <regex>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <numeric>
#include <algorithm>
#include <filesystem>

int log(std::wstring msg);
int log(std::string msg);

// Global variable to store the last read position
std::streampos lastReadPos = 0;
std::string filePath = "";
int lastEpoch = 0;
struct Hit {
    long long epoch;
    int damage;
};
std::vector<Hit> stats;
const long dpsInactivitySeconds = 6;

int hasNoLogs () {
  return filePath == "" ? 1 : 0;
}

std::string getFormattedLocalDate() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm;
#ifdef _MSC_VER
    localtime_s(&now_tm, &now_c);
#else
    localtime_r(&now_c, &now_tm);
#endif
    std::stringstream ss;
    ss << std::put_time(&now_tm, "%a %b %d %H:%M:%S %Y");
    return ss.str();
}

long long epochLocal() {
    std::string localDateStr = getFormattedLocalDate();
    std::istringstream ss(localDateStr);
    std::tm t{};
    ss >> std::get_time(&t, "%a %b %d %H:%M:%S %Y");
    if (ss.fail()) {
        log("Error parsing formatted local date: " + localDateStr);
        return -1;
    }
    std::time_t timeSinceEpoch = mktime(&t);
    if (timeSinceEpoch == -1) {
      log("Error converting tm to time_t for formatted local date: " + localDateStr);
      return -1;
    }
    return static_cast<long long>(timeSinceEpoch);
}

// Helper function to parse a date string (e.g., "[05/10 00:46:00]") to epoch seconds
long long parseDateToEpoch(const std::string& dateStr) {
    std::istringstream ss(dateStr);
    std::tm t{};
    ss >> std::get_time(&t, "%a %b %d %H:%M:%S %Y");
    if (ss.fail()) {
        std::cerr << "Error parsing date: " << dateStr << std::endl;
        return -1; // Indicate an error
    }
    // mktime interprets the tm struct as local time, so we need to adjust to UTC if that's desired
    std::time_t timeSinceEpoch = mktime(&t);
    if (timeSinceEpoch == -1) {
        std::cerr << "Error converting tm to time_t for: " << dateStr << std::endl;
        return -1;
    }
    return static_cast<long long>(timeSinceEpoch);
}

// Function to get the size of a file
long long getFileSize(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (file.is_open()) {
        long long size = file.tellg();
        file.close();
        return size;
    } else {
      //std::cerr << "Error opening file: " << filePath << std::endl;
        return -1; // Indicate an error
    }
}

// Function to read from a specific position in a file until the end
std::string readFileFromPosition(const std::string& filePath, std::streampos startPos) {
    std::ifstream file(filePath, std::ios::binary);
    std::string content;
    if (file.is_open()) {
        file.seekg(startPos);
        std::stringstream buffer;
        buffer << file.rdbuf();
        content = buffer.str();
        file.close();
    } else {
      //std::cerr << "Error opening file: " << filePath << std::endl;
    }
    return content;
}

// Function to read the file from the last read position until the end,
// accumulating the content into a single string
std::string slurpOnceAsString(const std::string& filePath) {
    long long fileSize = getFileSize(filePath);
    if (fileSize == -1) {
        return "";
    }

    if (lastReadPos >= fileSize) {
        return ""; // Nothing new to read
    }

    std::string content = readFileFromPosition(filePath, lastReadPos);
    lastReadPos += content.length();
    return content;
}

// Function to read the file from the last read position until the end,
// splitting the content into a vector of strings by "\r\n"
std::vector<std::string> slurpOnceAsLines(const std::string& filePath) {
    long long fileSize = getFileSize(filePath);
    if (fileSize == -1) {
      return {};
    }

    if (lastReadPos >= fileSize) {
      return {}; // Nothing new to read
    }

    std::string content = readFileFromPosition(filePath, lastReadPos);
    lastReadPos += content.length();

    std::vector<std::string> lines;
    std::string line;
    std::istringstream contentStream(content);
    while (std::getline(contentStream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back(); // Remove the trailing '\r' if present
        }
        lines.push_back(line);
    }
    return lines;
}

std::vector<std::string> slurpAsLines(const std::string& filePath) {
    std::ifstream inputFile(filePath);
    std::vector<std::string> lines;
    std::string line;

    if (inputFile.is_open()) {
        while (std::getline(inputFile, line)) {
            lines.push_back(line);
        }
        inputFile.close();
        return lines;
    }
    return {};
}

std::string getNewestLogFile()
{
  std::vector<std::string> lines = slurpAsLines("dpsgirl.conf");
  std::string newestLog = "";
  // std::time_t newestTime = 0;
  long long newestTime = 0;

  for (const auto& line : lines) {
    if (!std::filesystem::exists(line)) {
      continue;
    }
    std::filesystem::path p = line;
    auto lastWriteTime = std::filesystem::last_write_time(p);
    auto systemClockTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(lastWriteTime);
    auto epochTime = systemClockTime.time_since_epoch();
    std::time_t t = std::chrono::duration_cast<std::chrono::seconds>(epochTime).count();

    if (t > newestTime || newestTime == 0)
      {
        newestTime = t;
        newestLog = line;
      }
  }

  if (newestLog != filePath) {
    if (newestLog != "")
      log("Parsing the newest log file in your dpsgirl.conf:  " + newestLog);
    else
      log("Your config file may have disappeared, no logs available!");

    filePath = newestLog;
    lastReadPos = 0;
    stats.clear();
  }

  return newestLog;
}

std::vector<Hit> getHits() {
    std::vector<Hit> hits;
    std::vector<std::string> lines = slurpOnceAsLines(getNewestLogFile());
    std::regex damageRegex(".* points of .*damage.*");
    std::regex exclusionRegex("(\\] a|healed|YOU|absorbed|shielded)");
    std::regex extractionRegex("\\[(.*?)\\][^\\d]* (\\d+)"); // Match date in [], then a space and digits

    for (const auto& line : lines) {
      if (std::regex_match(line, damageRegex) && !std::regex_search(line, exclusionRegex)) {
            std::smatch match;
            if (std::regex_search(line, match, extractionRegex) && match.size() == 3) {
                std::string dateStr = match[1].str();
                std::string damageStr = match[2].str();
                long long epoch = parseDateToEpoch(dateStr);
                try {
                    int damage = std::stoi(damageStr);
                    if (epoch != -1) {
                      lastEpoch = epoch;
                      hits.push_back({epoch, damage});
                    }
                } catch (const std::invalid_argument& e) {
                    std::cerr << "Invalid damage value: " << damageStr << " in line: " << line << std::endl;
                } catch (const std::out_of_range& e) {
                    std::cerr << "Damage value out of range: " << damageStr << " in line: " << line << std::endl;
                }
            }
        }
    }
    return hits;
}

// Function to get the inactivity duration in seconds
long long getInactivitySeconds() {
  auto nowEpoch = epochLocal();
  return nowEpoch - lastEpoch;
}

// Function to update the stats with new hits and clear if inactive
void updateStats() {
    std::vector<Hit> newHits = getHits();
    stats.insert(stats.end(), newHits.begin(), newHits.end());

    if (getInactivitySeconds() > dpsInactivitySeconds) {
      stats.clear();
    }
}

// Function to get the duration of the recorded hits in seconds
long long getDurationSeconds() {
    if (!stats.empty()) {
        long long startEpoch = stats.front().epoch;
        long long endEpoch = stats.back().epoch;
        return std::max(1LL, endEpoch - startEpoch);
    } else {
        return 1; // Avoid division by zero if no stats yet
    }
}

// Function to get the sum of all damage recorded
long long getSumDamage() {
    if (!stats.empty()) {
        return std::accumulate(stats.begin(), stats.end(), 0LL,
                               [](long long sum, const Hit& hit) {
                                   return sum + hit.damage;
                               });
    } else {
        return 0;
    }
}

// Function to calculate the damage per second (DPS)
int getDps() {
    long long totalDamage = getSumDamage();
    long long duration = getDurationSeconds();

    if (duration > 0) {
        return static_cast<int>(static_cast<double>(totalDamage) / duration);
    } else {
        return 0;
    }
}

// int main() {

//     // Create a dummy log file for testing
//     // std::ofstream outfile(filePath);
//     // outfile << "Line 1\r\nLine 2\r\nLine 3\r\n";
//     // outfile.close();

//     // std::cout << "First read (single string): " << slurpOnceAsString(filePath) << std::endl;
//     // std::cout << "Second read (single string): " << slurpOnceAsString(filePath) << std::endl;
//     // std::cout << "Third read (single string): " << slurpOnceAsString(filePath) << std::endl;
//     // std::cout << "Fourth read (single string): " << slurpOnceAsString(filePath) << std::endl; // Should be empty

//     // lastReadPos = 0; // Reset for the lines version

//     // std::cout << "\nFirst read (lines):\n";
//     // for (const auto& line : slurpOnceAsLines(filePath)) {
//     //     std::cout << line << std::endl;
//     // }

//     // std::cout << "\nSecond read (lines):\n";
//     // for (const auto& line : slurpOnceAsLines(filePath)) {
//     //     std::cout << line << std::endl;
//     // }

//     // std::cout << "\nThird read (lines):\n";
//     // for (const auto& line : slurpOnceAsLines(filePath)) {
//     //     std::cout << line << std::endl;
//     // }

//     // std::cout << "\nFourth read (lines):\n";
//     // for (const auto& line : slurpOnceAsLines(filePath)) {
//     //     std::cout << line << std::endl;
//     // }

//     // std::vector<Hit> firstHits = getHits();
//     // std::cout << "First batch of hits:\n";
//     // for (const auto& hit : firstHits) {
//     //     std::cout << "Epoch: " << hit.epoch << ", Damage: " << hit.damage << std::endl;
//     // }

//     // std::vector<Hit> secondHits = getHits();
//     // std::cout << "\nSecond batch of hits (if any new lines were added):\n";
//     // for (const auto& hit : secondHits) {
//     //     std::cout << "Epoch: " << hit.epoch << ", Damage: " << hit.damage << std::endl;
//     // }

//     // std::cout << "Formatted local date: " << getFormattedLocalDate() << std::endl;
//     // long long localEpoch = epochLocal();
//     // if (localEpoch != -1) {
//     //     std::cout << "Epoch for local time: " << localEpoch << std::endl;
//     // }

//     updateStats();
//     std::cout << "Current DPS: " << getDps() << ", Stats count: " << stats.size() << ", Inactivity: " << getInactivitySeconds() << "s" << std::endl;

//     return 0;
// }
