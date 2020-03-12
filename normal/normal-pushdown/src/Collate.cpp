//
// Created by matt on 5/12/19.
//

#include "normal/pushdown/Collate.h"

#include <vector>                      // for vector

#include <arrow/table.h>               // for ConcatenateTables, Table (ptr ...
#include <arrow/pretty_print.h>

#include <normal/core/TupleMessage.h>
#include <normal/core/CompleteMessage.h>

#include "normal/pushdown/Globals.h"

namespace normal::pushdown {

void Collate::onStart() {
  SPDLOG_DEBUG("Starting");
}

Collate::Collate(std::string name) : Operator(std::move(name)) {
}

void Collate::onReceive(const normal::core::Envelope &message) {
  if (message.message().type() == "StartMessage") {
    this->onStart();
  } else if (message.message().type() == "TupleMessage") {
    auto tupleMessage = dynamic_cast<const normal::core::TupleMessage &>(message.message());
    this->onTuple(tupleMessage);
  } else if (message.message().type() == "CompleteMessage") {
    auto completeMessage = dynamic_cast<const normal::core::CompleteMessage &>(message.message());
    this->onComplete(completeMessage);
  } else {
    throw;
  }
}

void Collate::onComplete(const normal::core::CompleteMessage &msg) {
  ctx()->operatorActor()->quit();
}

void Collate::show() {

  assert(tuples_);

  SPDLOG_DEBUG("{}  |  Show:\n{}", this->name(), tuples_->toString());
}

std::shared_ptr<normal::core::TupleSet> Collate::tuples() {

  assert(tuples_);

  return tuples_;
}
void Collate::onTuple(const normal::core::TupleMessage& message) {

  SPDLOG_DEBUG("Received tuples");

  if (!tuples_) {
    assert(message.tuples());
    tuples_ = message.tuples();
  } else {
    auto tables = std::vector<std::shared_ptr<arrow::Table>>();
    std::shared_ptr<arrow::Table> table;
    tables.push_back(message.tuples()->table());
    tables.push_back(tuples_->table());
    arrow::ConcatenateTables(tables, &table);
    tuples_->table(table);
  }
}

}