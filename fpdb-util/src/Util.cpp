//
// Created by Yifei Yang on 2/9/21.
//

#include <fpdb/util/Util.h>
#include <fmt/format.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>

string fpdb::util::readFile(const string& filePath) {
  ifstream inFile(filePath);
  if (!inFile.good()) {
    throw runtime_error(fmt::format("File not exists: {}", filePath));
  }
  ostringstream buf;
  char ch;
  while (buf && inFile.get(ch)) {
    buf.put(ch);
  }
  return buf.str();
}

vector<string> fpdb::util::readFileByLine(const string &filePath) {
  ifstream inFile(filePath);
  if (!inFile.good()) {
    throw runtime_error(fmt::format("File not exists: {}", filePath));
  }
  vector<string> lines;
  string line;
  while (getline(inFile, line)) {
    lines.emplace_back(line);
  }
  return lines;
}

void fpdb::util::writeFile(const string& filePath, const string& content) {
  ofstream outFile(filePath);
  if (!outFile.good()) {
    throw runtime_error(fmt::format("Error when writing file: {}", filePath));
  }
  outFile << content;
  outFile.flush();
  outFile.close();
}

int64_t fpdb::util::getFileSize(const string& filePath) {
  std::filesystem::path fsPath(filePath);
  return std::filesystem::file_size(fsPath);
}

unordered_map<string, string> fpdb::util::readConfig(const string &fileName) {
  unordered_map<string, string> configMap;
  string configPath = filesystem::current_path()
          .parent_path()
          .append("resources/config")
          .append(fileName)
          .string();
  vector<string> lines = readFileByLine(configPath);
  for(auto const &line: lines) {
    auto pos = line.find('=');
    if (pos == string::npos) {
      continue;
    }
    auto key = line.substr(0, pos);
    auto value = line.substr(pos + 1);
    configMap[key] = value;
  }
  return configMap;
}

vector<string> fpdb::util::readRemoteIps(bool isCompute) {
  // read cluster ips
  string clusterIpFileName = isCompute ? "cluster_ips" : "fpdb-store_ips";
  string clusterIpFilePath = filesystem::current_path()
          .parent_path()
          .append("resources/config")
          .append(clusterIpFileName)
          .string();
  auto clusterIps = readFileByLine(clusterIpFilePath);

  // need to remove local ip if it is for the compute cluster
  if (isCompute == false) {
    return clusterIps;
  }
  auto expLocalIp = getLocalIp();
  if (!expLocalIp) {
    throw runtime_error(expLocalIp.error());
  }
  auto localIpIt = std::find(clusterIps.begin(), clusterIps.end(), *expLocalIp);
  if (localIpIt != clusterIps.end()) {
    clusterIps.erase(localIpIt);
  }
  return clusterIps;
}

bool fpdb::util::parseBool(const string& stringToParse) {
  if (stringToParse == "TRUE") {
    return true;
  } else {
    return false;
  }
}

size_t fpdb::util::hashCombine(const vector<size_t> &hashes) {
  // reference: https://stackoverflow.com/questions/20511347/a-good-hash-function-for-a-vector
  std::size_t seed = hashes.size();
  for (auto hash: hashes) {
    seed ^= hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  }
  return seed;
}

void fpdb::util::setBit(vector<int64_t> &bitmap, int64_t n) {
  bitmap[n / 64] |= 1UL << (n % 64);
}

void fpdb::util::unsetBit(vector<int64_t> &bitmap, int64_t n) {
  bitmap[n / 64] &= ~(1UL << (n % 64));
}

bool fpdb::util::getBit(const vector<int64_t> &bitmap, int64_t n) {
  return (bitmap[n / 64] >> (n % 64)) & 1UL;
}

bool fpdb::util::isInteger(const string& str) {
  return !str.empty() && std::find_if(str.begin(), str.end(),
                                      [](unsigned char c) { return !std::isdigit(c); }) == str.end();
}

tl::expected<string, string> fpdb::util::execCmd(const char *cmd) {
  char buffer[128];
  string result;
  FILE* pipe = popen(cmd, "r");
  if (!pipe) {
    return tl::make_unexpected("popen() failed!");
  }
  while (fgets(buffer, sizeof buffer, pipe) != nullptr) {
    result += buffer;
  }
  pclose(pipe);
  return result;
}

tl::expected<string, string> fpdb::util::getLocalIp() {
  return execCmd("curl -s ifconfig.me");
}

tl::expected<string, string> fpdb::util::getLocalPrivateIp() {
  auto expIp = execCmd("hostname -I");
  if (!expIp.has_value()) {
    return expIp;
  }
  auto ip = *expIp;
  return ip.substr(0, ip.size() - 2);
}
