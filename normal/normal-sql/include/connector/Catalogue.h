//
// Created by matt on 27/3/20.
//

#ifndef NORMAL_NORMAL_SQL_SRC_CATALOGUE_H
#define NORMAL_NORMAL_SQL_SRC_CATALOGUE_H

#include <string>
#include <unordered_map>
#include <memory>

#include "CatalogueEntry.h"

class Catalogue {

private:
  std::string name_;
  std::unordered_map<std::string, std::shared_ptr<CatalogueEntry>> entries_;

public:
  explicit Catalogue(std::string Name);
  virtual ~Catalogue() = default;

  [[nodiscard]] const std::string &getName() const;
  void put(std::shared_ptr<CatalogueEntry> entry);
  std::shared_ptr<CatalogueEntry> getEntry(std::string alias);
  std::string toString();
};

#endif //NORMAL_NORMAL_SQL_SRC_CATALOGUE_H
