//
// Created by Yifei Yang on 11/15/21.
//

#include <fpdb/cache/Util.h>
#include <fpdb/catalogue/obj-store/ObjStorePartition.h>
#include <fpdb/util/Util.h>

using namespace fpdb::catalogue::obj_store;
using namespace fpdb::util;

std::vector<std::string> split(const std::string& str, const std::string& splitStr) {
  std::vector<std::string> res;
  std::string::size_type pos1, pos2;
  pos2 = str.find(splitStr);
  pos1 = 0;
  while (pos2 != std::string::npos)
  {
    res.push_back(str.substr(pos1, pos2 - pos1));
    pos1 = pos2 + 1;
    pos2 = str.find(splitStr, pos1);
  }
  if (pos1 < str.length()) {
    res.push_back(str.substr(pos1));
  }
  return res;
}

namespace fpdb::cache {

unordered_map<shared_ptr<SegmentKey>, size_t, cache::SegmentKeyPointerHash, cache::SegmentKeyPointerPredicate>
  Util::readSegmentKeySize(const std::string& s3Bucket,
                           const std::string& schemaName,
                           filesystem::path &filePath) {
  unordered_map<shared_ptr<SegmentKey>, size_t, SegmentKeyPointerHash, SegmentKeyPointerPredicate> res;
  for (auto const &str: readFileByLine(filePath)) {
    auto splitRes = split(str, ",");
    string objectName = splitRes[0];
    string column = splitRes[1];
    unsigned long startOffset = stoul(splitRes[2]);
    unsigned long endOffset  = stoul(splitRes[3]);
    size_t sizeInBytes;
    sscanf(splitRes[4].c_str(), "%zu", &sizeInBytes);

    string s3Object = schemaName + objectName;
    // create the SegmentKey
    auto segmentPartition = make_shared<ObjStorePartition>(s3Bucket, s3Object);
    auto segmentRange = SegmentRange::make(startOffset, endOffset);
    auto segmentKey = SegmentKey::make(segmentPartition, column, segmentRange);
    res.emplace(segmentKey, sizeInBytes);
  }
  return res;
}

}
