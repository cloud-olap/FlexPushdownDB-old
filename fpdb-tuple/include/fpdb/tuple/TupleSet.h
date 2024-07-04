//
// Created by matt on 12/12/19.
//

#ifndef FPDB_FPDB_TUPLE_INCLUDE_FPDB_TUPLE_TUPLESET_H
#define FPDB_FPDB_TUPLE_INCLUDE_FPDB_TUPLE_TUPLESET_H

#include <fpdb/tuple/arrow/Arrays.h>
#include <fpdb/tuple/arrow/TableHelper.h>
#include <fpdb/tuple/Globals.h>
#include <fpdb/tuple/Schema.h>
#include <fpdb/tuple/Column.h>
#include <fpdb/tuple/TupleSetShowOptions.h>
#include <fpdb/tuple/serialization/ArrowSerializer.h>
#include <fpdb/caf/CAFUtil.h>
#include <arrow/api.h>
#include <arrow/csv/api.h>
#include <tl/expected.hpp>
#include <memory>
#include <vector>

namespace arrow { class Table; }
namespace arrow::csv { class TableReader; }

namespace fpdb::tuple {

/**
 * A list of tuples/rows/records. Really just encapsulates Arrow tables and record batches. Hiding
 * some of the rough edges.
 */
class TupleSet : public std::enable_shared_from_this<TupleSet> {

public:
  explicit TupleSet() = default;
  explicit TupleSet(std::shared_ptr<arrow::Table> table);

  static std::shared_ptr<TupleSet> make(const std::shared_ptr<arrow::Table> &table);
  static std::shared_ptr<TupleSet> make(const std::shared_ptr<arrow::Schema>& schema);
  static std::shared_ptr<TupleSet> make(const std::shared_ptr<arrow::Schema>& schema,
                                        const std::vector<std::shared_ptr<arrow::Array>>& values);
  static std::shared_ptr<TupleSet> make(const std::shared_ptr<arrow::Schema>& schema,
                                        const std::vector<std::shared_ptr<arrow::ChunkedArray>>& values);
  static std::shared_ptr<TupleSet> make(const std::shared_ptr<arrow::Schema>& schema,
                                        const std::vector<std::shared_ptr<Column>>& columns);
  static std::shared_ptr<TupleSet> make(const std::vector<std::shared_ptr<Column>>& columns);
  static tl::expected<std::shared_ptr<TupleSet>, std::string>
  make(const std::vector<std::shared_ptr<arrow::RecordBatch>> &recordBatches);
  static tl::expected<std::shared_ptr<TupleSet>, std::string>
  make(const std::shared_ptr<arrow::csv::TableReader> &tableReader);
  static std::shared_ptr<TupleSet> makeWithNullTable();
  static std::shared_ptr<TupleSet> makeWithEmptyTable();

  bool valid() const;
  bool validate() const;
  void clear();
  int64_t numRows() const;
  int numColumns() const;
  size_t size() const;
  std::shared_ptr<arrow::Schema> schema() const;
  std::shared_ptr<arrow::Table> table() const;
  void table(const std::shared_ptr<arrow::Table> &table);

  /**
   * Concatenate tupleSets.
   */
  static tl::expected<std::shared_ptr<TupleSet>, std::string> concatenate(const std::vector<std::shared_ptr<TupleSet>>& tupleSets);

  /**
   * Append tupleSets.
   */
  tl::expected<void, std::string> append(const std::vector<std::shared_ptr<TupleSet>>& tupleSet);
  tl::expected<void, std::string> append(const std::shared_ptr<TupleSet>& tupleSet);

  /**
   * Get column.
   */
  tl::expected<std::shared_ptr<Column>, std::string> getColumnByName(const std::string &name) const;
  tl::expected<std::shared_ptr<Column>, std::string> getColumnByIndex(const int &columnIndex) const;

  /**
   * Project specified columns, ignore non-existing ones.
   * @param columnNames
   * @return
   */
  tl::expected<std::shared_ptr<TupleSet>, std::string> projectExist(const std::vector<std::string> &columnNames) const;
  static std::shared_ptr<arrow::RecordBatch> projectExist(const std::shared_ptr<arrow::RecordBatch> &recordBatch,
                                                          const std::vector<std::string> &columnNames);
  static std::shared_ptr<arrow::RecordBatch> projectExist(const arrow::RecordBatch &recordBatch,
                                                          const std::vector<std::string> &columnNames);

  /**
   * Project specified columns
   * @param columnIds
   * @return
   */
  tl::expected<std::shared_ptr<TupleSet>, std::string> project(const std::vector<int> &columnIds) const;

  /**
   * Rename columns.
   */
  tl::expected<void, std::string> renameColumns(const std::vector<std::string>& columnNames);
  tl::expected<void, std::string> renameColumns(const std::unordered_map<std::string, std::string> &columnRenames);

  /**
   * Invokes combineChunks on the underlying table
   *
   * @return
   */
  tl::expected<void, std::string> combine();

  /**
   * Returns the tuple set pretty printed as a string
   *
   * @return
   */
  std::string showString();
  std::string showString(TupleSetShowOptions options);

  /**
   * Returns a short string representing the tuple set
   * @return
   */
  std::string toString() const;

  /**
   * Perform a custom function which returns an scalar value on the table.
   * @param fn
   * @return
   */
  std::shared_ptr<arrow::Scalar> visit(const std::function<std::shared_ptr<arrow::Scalar>(
          std::shared_ptr<arrow::Scalar>, arrow::RecordBatch &)>& fn);

  /**
   * Returns an element from the tupleset given and column name and row number.
   *
   * @tparam ARROW_TYPE
   * @tparam C_TYPE
   * @param columnName
   * @param row
   * @return
   */
  template<typename ARROW_TYPE, typename C_TYPE = typename ARROW_TYPE::c_type>
  tl::expected<C_TYPE, std::string> value(const std::string &columnName, int row){
    return TableHelper::value<ARROW_TYPE, C_TYPE>(columnName, row, *table_);
  }

  /**
   * Returns an string element from the tupleset given and column name and row number.
   * @param columnName
   * @param row
   * @return
   */
  tl::expected<std::string, std::string> stringValue(const std::string &columnName, int row){
	  return TableHelper::value<::arrow::StringType, std::string>(columnName, row, *table_);
  }


  /**
   * Returns an element from the tupleset given and column number and row number.
   *
   * @tparam ARROW_TYPE
   * @tparam C_TYPE
   * @param columnName
   * @param row
   * @return
   */
  template<typename ARROW_TYPE, typename C_TYPE = typename ARROW_TYPE::c_type>
  tl::expected<C_TYPE, std::string> value(int column, int row){
	return TableHelper::value<ARROW_TYPE, C_TYPE>(column, row, *table_);
  }

  /**
   * Returns a column from the tupleset as a vector given a column name
   *
   * @tparam ARROW_TYPE
   * @tparam C_TYPE
   * @param columnName
   * @param row
   * @return
   */
  template<typename ARROW_TYPE, typename C_TYPE = typename ARROW_TYPE::c_type>
  tl::expected<std::shared_ptr<std::vector<C_TYPE>>, std::string> vector(const std::string &columnName){
	return TableHelper::vector<ARROW_TYPE, C_TYPE>(*table_->GetColumnByName(columnName));
  }

private:
  static std::vector<std::shared_ptr<arrow::Table>>
  tupleSetVectorToArrowTableVector(const std::vector<std::shared_ptr<TupleSet>> &tupleSets);

  std::shared_ptr<arrow::Table> table_;

// caf inspect
public:
  template <class Inspector>
  friend bool inspect(Inspector& f, TupleSet& tupleSet) {
    auto toBytes = [&tupleSet]() -> decltype(auto) {
      return ArrowSerializer::table_to_bytes(tupleSet.table_);
    };
    auto fromBytes = [&tupleSet](const std::vector<std::uint8_t> &bytes) {
      tupleSet.table_ = ArrowSerializer::bytes_to_table(bytes);
      return true;
    };
    return f.object(tupleSet).fields(f.field("table", toBytes, fromBytes));
  }
};

}

using TupleSetPtr = std::shared_ptr<fpdb::tuple::TupleSet>;

CAF_BEGIN_TYPE_ID_BLOCK(TupleSet, fpdb::caf::CAFUtil::TupleSet_first_custom_type_id)
CAF_ADD_TYPE_ID(TupleSet, (fpdb::tuple::TupleSet))
CAF_END_TYPE_ID_BLOCK(TupleSet)

namespace caf {
template <>
struct inspector_access<TupleSetPtr> : variant_inspector_access<TupleSetPtr> {
  // nop
};
} // namespace caf

#endif //FPDB_FPDB_TUPLE_INCLUDE_FPDB_TUPLE_TUPLESET_H
