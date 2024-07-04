//
// Created by Yifei Yang on 2/9/21.
//

#ifndef FPDB_FPDB_UTIL_INCLUDE_FPDB_UTIL_UTIL_H
#define FPDB_FPDB_UTIL_INCLUDE_FPDB_UTIL_UTIL_H

#include <tl/expected.hpp>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>

using namespace std;

namespace fpdb::util {
  /**
   * File utils
   * @param filePath
   * @return
   */
  string readFile(const string& filePath);
  vector<string> readFileByLine(const string& filePath);
  void writeFile(const string& filePath, const string& content);
  int64_t getFileSize(const string& filePath);

  /**
   * Config utils
   * @param fileName
   * @return
   */
  unordered_map<string, string> readConfig(const string &fileName);

  /**
   * Read ip of all remote nodes
   * @param isCompute
   * @return
   */
  vector<string> readRemoteIps(bool isCompute = true);

  /**
   * Parsing bool string
   * @param stringToParse
   * @return
   */
  bool parseBool(const string& stringToParse);

  /**
   * Union func
   */
  template<typename T>
  vector<T> union_(const vector<T> &vec1, const set<T> &set2) {
    set<T> unionSet{vec1.begin(), vec1.end()};
    unionSet.insert(set2.begin(), set2.end());
    return vector<T>{unionSet.begin(), unionSet.end()};
  }

  template<typename T>
  vector<T> union_(const vector<T> &vec1, const vector<T> &vec2) {
    set<T> unionSet{vec1.begin(), vec1.end()};
    unionSet.insert(vec2.begin(), vec2.end());
    return vector<T>{unionSet.begin(), unionSet.end()};
  }

  template<typename T>
  vector<T> union_(const vector<set<T>> &sets) {
    set<T> unionSet;
    for (const auto &set: sets) {
      unionSet.insert(set.begin(), set.end());
    }
    return vector<T>{unionSet.begin(), unionSet.end()};
  }

  /**
   * Check if set1 is a subset of set2
   */
  template<typename T>
  bool isSubSet(const set<T> &set1, const set<T> &set2) {
    for (const auto &e1: set1) {
      if (set2.find(e1) == set2.end()) {
        return false;
      }
    }
    return true;
  }

  template<typename T>
  bool isSubSet(const unordered_set<T> &set1, const unordered_set<T> &set2) {
    for (const auto &e1: set1) {
      if (set2.find(e1) == set2.end()) {
        return false;
      }
    }
    return true;
  }

  template<typename T>
  vector<set<T>> splitToUnarySet(const vector<T> vec) {
    vector<set<T>> sets;
    for (const auto &e: vec) {
      sets.emplace_back(set<T>{e});
    }
    return sets;
  }

  size_t hashCombine(const vector<size_t> &hashes);

  /**
   * Bitmap util
   */
  void setBit(vector<int64_t> &bitmap, int64_t n);
  void unsetBit(vector<int64_t> &bitmap, int64_t n);
  bool getBit(const vector<int64_t> &bitmap, int64_t n);

  /**
   * Given a start and finish number, will create pairs of numbers from start to finish (inclusive)
   * evenly split across the number of ranges given.
   *
   * E.g.
   * ranges(0,8,3) -> [[0,1,2][3,4,5][6,7,8]]
   * ranges(0,9,3) -> [[0,1,2,3][4,5,6,7][8,9]]
   * ranges(0,10,3) -> [[0,1,2,3][4,5,6,7][8,9,10]]
   *
   * @tparam T
   * @param start
   * @param finish
   * @param numRanges
   * @return
   */
  template<typename T>
  static vector<pair<T, T>> ranges(T start, T finish, T numRanges) {
    vector<pair<T, T>> result;

    T rangeSize = ((finish - start) / numRanges) + 1;

    for (int i = 0; i < numRanges; ++i) {
      T rangeStart = rangeSize * i;
      T rangeStop = min((rangeStart + rangeSize) - 1, finish);
      result.push_back(pair{rangeStart, rangeStop});
    }

    return result;
  }

  bool isInteger(const string& str);
  tl::expected<string, string> execCmd(const char *cmd);
  tl::expected<string, string> getLocalIp();
  tl::expected<string, string> getLocalPrivateIp();
}


#endif //FPDB_FPDB_UTIL_INCLUDE_FPDB_UTIL_UTIL_H
