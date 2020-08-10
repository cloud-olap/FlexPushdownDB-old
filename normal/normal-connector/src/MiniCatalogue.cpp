//
// Created by Yifei Yang on 7/15/20.
//

#include <string>
#include <normal/connector/MiniCatalogue.h>

normal::connector::MiniCatalogue::MiniCatalogue(
        const std::shared_ptr<std::vector<std::string>> &tables,
        const std::shared_ptr<std::unordered_map<std::string, std::shared_ptr<std::vector<std::string>>>> &schemas,
        const std::shared_ptr<std::unordered_map<std::string, int>> &columnLengthMap,
        const std::shared_ptr<std::vector<std::string>> &defaultJoinOrder) :
        tables_(tables),
        schemas_(schemas),
        columnLengthMap_(columnLengthMap),
        defaultJoinOrder_(defaultJoinOrder) {}

std::shared_ptr<normal::connector::MiniCatalogue> normal::connector::MiniCatalogue::defaultMiniCatalogue() {
  // star join order
  auto defaultJoinOrder = std::make_shared<std::vector<std::string>>();
  defaultJoinOrder->emplace_back("supplier");
  defaultJoinOrder->emplace_back("date");
  defaultJoinOrder->emplace_back("customer");
  defaultJoinOrder->emplace_back("part");

  // schemas
  auto cols_supplier = std::vector<std::string>{"s_suppkey", "s_name", "s_address", "s_city", "s_nation", "s_region",
                                                "s_phone"};
  auto cols_date = std::vector<std::string>{"d_datekey", "d_date", "d_dayofweek", "d_month", "d_year", "d_yearmonthnum",
                                            "d_yearmonth", "d_daynuminweek", "d_daynuminmonth", "d_daynuminyear",
                                            "d_monthnuminyear", "d_weeknuminyear", "d_sellingseason", "d_lastdayinweekfl",
                                            "d_lastdayinmonthfl", "d_holidayfl", "d_weekdayfl"};
  auto cols_customer = std::vector<std::string>{"c_custkey", "c_name", "c_address", "c_city", "c_nation", "c_region",
                                                "c_phone", "c_mktsegment"};
  auto cols_part = std::vector<std::string>{"p_partkey", "p_name", "p_mfgr", "p_category", "p_brand1", "p_color",
                                            "p_type", "p_size", "p_container"};
  auto cols_lineorder = std::vector<std::string>{"lo_orderkey", "lo_linenumber", "lo_custkey", "lo_partkey",
                                                 "lo_suppkey", "lo_orderdate", "lo_orderpriority", "lo_shippriority",
                                                 "lo_quantity", "lo_extendedprice", "lo_ordtotalprice", "lo_discount",
                                                 "lo_revenue", "lo_supplycost", "lo_tax", "lo_commitdate",
                                                 "lo_shipmode"};
  auto schemas = std::make_shared<std::unordered_map<std::string, std::shared_ptr<std::vector<std::string>>>>();
  schemas->insert({"supplier", std::make_shared<std::vector<std::string>>(cols_supplier)});
  schemas->insert({"date", std::make_shared<std::vector<std::string>>(cols_date)});
  schemas->insert({"customer", std::make_shared<std::vector<std::string>>(cols_customer)});
  schemas->insert({"part", std::make_shared<std::vector<std::string>>(cols_part)});
  schemas->insert({"lineorder", std::make_shared<std::vector<std::string>>(cols_lineorder)});

  // tables
  auto tables = std::make_shared<std::vector<std::string>>();
  for (const auto &schema: *schemas) {
    tables->push_back(schema.first);
  }

  // columnLengthMap
  auto columnLengthMap = std::make_shared<std::unordered_map<std::string, int>>();
  columnLengthMap->emplace("lo_orderkey", 4);
  columnLengthMap->emplace("lo_linenumber", 4);
  columnLengthMap->emplace("lo_custkey", 4);
  columnLengthMap->emplace("lo_partkey", 4);
  columnLengthMap->emplace("lo_suppkey", 4);
  columnLengthMap->emplace("lo_orderdate", 4);
  columnLengthMap->emplace("lo_orderpriority", 15);
  columnLengthMap->emplace("lo_shippriority", 1);
  columnLengthMap->emplace("lo_quantity", 4);
  columnLengthMap->emplace("lo_extendedprice", 4);
  columnLengthMap->emplace("lo_ordtotalprice", 4);
  columnLengthMap->emplace("lo_discount", 4);
  columnLengthMap->emplace("lo_revenue", 4);
  columnLengthMap->emplace("lo_supplycost", 4);
  columnLengthMap->emplace("lo_tax", 4);
  columnLengthMap->emplace("lo_commitdate", 4);
  columnLengthMap->emplace("lo_shipmode", 10);

  columnLengthMap->emplace("p_partkey", 4);
  columnLengthMap->emplace("p_name", 22);
  columnLengthMap->emplace("p_mfgr", 6);
  columnLengthMap->emplace("p_category", 7);
  columnLengthMap->emplace("p_brand1", 9);
  columnLengthMap->emplace("p_color", 11);
  columnLengthMap->emplace("p_type", 25);
  columnLengthMap->emplace("p_size", 4);
  columnLengthMap->emplace("p_container", 10);

  columnLengthMap->emplace("s_suppkey", 4);
  columnLengthMap->emplace("s_name", 25);
  columnLengthMap->emplace("s_address", 25);
  columnLengthMap->emplace("s_city", 10);
  columnLengthMap->emplace("s_nation", 15);
  columnLengthMap->emplace("s_region", 12);
  columnLengthMap->emplace("s_phone", 15);

  columnLengthMap->emplace("c_custkey", 4);
  columnLengthMap->emplace("c_name", 25);
  columnLengthMap->emplace("c_address", 25);
  columnLengthMap->emplace("c_city", 10);
  columnLengthMap->emplace("c_nation", 15);
  columnLengthMap->emplace("c_region", 12);
  columnLengthMap->emplace("c_phone", 15);
  columnLengthMap->emplace("c_mktsegment", 10);

  columnLengthMap->emplace("d_datekey", 4);
  columnLengthMap->emplace("d_date", 18);
  columnLengthMap->emplace("d_dayofweek", 8);
  columnLengthMap->emplace("d_month", 9);
  columnLengthMap->emplace("d_year", 4);
  columnLengthMap->emplace("d_yearmonthnum", 4);
  columnLengthMap->emplace("d_yearmonth", 7);
  columnLengthMap->emplace("d_daynuminweek", 4);
  columnLengthMap->emplace("d_daynuminmonth", 4);
  columnLengthMap->emplace("d_daynuminyear", 4);
  columnLengthMap->emplace("d_monthnuminyear", 4);
  columnLengthMap->emplace("d_weeknuminyear", 4);
  columnLengthMap->emplace("d_sellingseason", 12);
  columnLengthMap->emplace("d_lastdayinweekfl", 1);
  columnLengthMap->emplace("d_lastdayinmonthfl", 1);
  columnLengthMap->emplace("d_holidayfl", 1);
  columnLengthMap->emplace("d_weekdayfl", 1);

  return std::make_shared<MiniCatalogue>(tables, schemas, columnLengthMap, defaultJoinOrder);
}

std::shared_ptr<std::vector<std::string>> normal::connector::MiniCatalogue::tables() {
  return tables_;
}

std::shared_ptr<std::vector<std::string>> normal::connector::MiniCatalogue::defaultJoinOrder() {
  return defaultJoinOrder_;
}

std::string normal::connector::MiniCatalogue::findTableOfColumn(std::string columnName) {
  for (const auto &schema: *schemas_) {
    for (const auto &existColumnName: *(schema.second)) {
      if (existColumnName == columnName) {
        return schema.first;
      }
    }
  }
  throw std::runtime_error("Column " + columnName + " not found");
}

double normal::connector::MiniCatalogue::lengthFraction(std::string columnName) {
  auto thisLength = columnLengthMap_->find(columnName)->second;
  auto tableName = findTableOfColumn(columnName);
  int allLength = 0;
  for (auto const &otherColumnName: *schemas_->find(tableName)->second) {
    allLength += columnLengthMap_->find(otherColumnName)->second;
  }
  return (double)thisLength / (double)allLength;
}
