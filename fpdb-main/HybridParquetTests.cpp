//
// Created by Yifei Yang on 5/23/24.
//

#include "test-base/TestUtil.h"
#include <fmt/format.h>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Need 2 params.\n");
    return 1;
  }
  int modeParam = std::stoi(argv[1]);
  int cacheSizeParam = std::stoi(argv[2]);
  std::shared_ptr<Mode> mode;
  switch (modeParam) {
    case 1:{
      mode = Mode::pullupMode();
      break;
    }
    case 2:{
      mode = Mode::pushdownOnlyMode();
      break;
    }
    case 3:{
      mode = Mode::cachingOnlyMode();
      break;
    }
    case 4:{
      mode = Mode::hybridMode();
      break;
    }
    default: {
      printf("Unknown mode: '%d'\n", modeParam);
      return 1;
    }
  }

  std::vector<std::string> queryFileNames;
  for (int i = 1; i <= 100; ++i) {
    queryFileNames.emplace_back(fmt::format("generated/{}.sql", i));
  }

  fpdb::main::test::TestUtil testUtil("ssb-sf100-sortlineorder/parquet_150MB/",
                                      queryFileNames,
                                      32,
                                      false,
                                      ObjStoreType::FPDB_STORE,
                                      mode,
                                      CachingPolicyType::LFU,
                                      cacheSizeParam * 1L * 1024 * 1024 * 1024);
  std::set<int> fixLayout;
  for (int i = 50; i < 100; ++i) {
    fixLayout.emplace(i);
  }
  testUtil.setFixLayoutIndices(fixLayout);
  testUtil.runTest();

  return 0;
}