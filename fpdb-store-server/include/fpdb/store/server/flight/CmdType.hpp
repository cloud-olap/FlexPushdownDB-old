//
// Created by matt on 4/2/22.
//

#ifndef FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_CMDTYPE_HPP
#define FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_CMDTYPE_HPP

#include <memory>
#include <string>

namespace fpdb::store::server::flight {

enum class CmdTypeId {
  GET_OBJECT,
  SELECT_OBJECT_CONTENT,
  PUT_BITMAP,
  CLEAR_BITMAP,
  CLEAR_TABLE,
  PUT_ADAPT_PUSHDOWN_METRICS,
  CLEAR_ADAPT_PUSHDOWN_METRICS,
  SET_ADAPT_PUSHDOWN,
  PUSHBACK_COMPLETE,
  PUSHBACK_DOUBLE_EXEC,
  SET_NUM_REQ_TO_TAIL
};

class CmdType {
public:
  CmdType(CmdTypeId id, std::string name);

  [[nodiscard]] CmdTypeId id() const;

  [[nodiscard]] const std::string& name() const;

  static std::shared_ptr<CmdType> get_object();

  static std::shared_ptr<CmdType> select_object_content();

  static std::shared_ptr<CmdType> put_bitmap();

  static std::shared_ptr<CmdType> clear_bitmap();

  static std::shared_ptr<CmdType> clear_table();

  static std::shared_ptr<CmdType> put_adapt_pushdown_metrics();

  static std::shared_ptr<CmdType> clear_adapt_pushdown_metrics();

  static std::shared_ptr<CmdType> set_adapt_pushdown();

  static std::shared_ptr<CmdType> pushback_complete();

  static std::shared_ptr<CmdType> pushback_double_exec();

  static std::shared_ptr<CmdType> set_num_req_to_tail();

private:
  CmdTypeId id_;
  std::string name_;
};

} // namespace fpdb::store::server::flight

#endif // FPDB_FPDB_STORE_SERVER_INCLUDE_FPDB_STORE_SERVER_FLIGHT_CMDTYPE_HPP
