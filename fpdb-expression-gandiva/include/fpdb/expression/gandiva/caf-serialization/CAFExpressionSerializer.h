//
// Created by Yifei Yang on 1/15/22.
//

#ifndef FPDB_FPDB_EXPRESSION_GANDIVA_INCLUDE_FPDB_EXPRESSION_GANDIVA_CAFSERIALIZATION_CAFEXPRESSIONSERIALIZER_H
#define FPDB_FPDB_EXPRESSION_GANDIVA_INCLUDE_FPDB_EXPRESSION_GANDIVA_CAFSERIALIZATION_CAFEXPRESSIONSERIALIZER_H

#include <fpdb/expression/gandiva/Add.h>
#include <fpdb/expression/gandiva/And.h>
#include <fpdb/expression/gandiva/Cast.h>
#include <fpdb/expression/gandiva/Column.h>
#include <fpdb/expression/gandiva/DateAdd.h>
#include <fpdb/expression/gandiva/DateExtract.h>
#include <fpdb/expression/gandiva/Divide.h>
#include <fpdb/expression/gandiva/EqualTo.h>
#include <fpdb/expression/gandiva/GreaterThan.h>
#include <fpdb/expression/gandiva/GreaterThanOrEqualTo.h>
#include <fpdb/expression/gandiva/If.h>
#include <fpdb/expression/gandiva/In.h>
#include <fpdb/expression/gandiva/IsNull.h>
#include <fpdb/expression/gandiva/LessThan.h>
#include <fpdb/expression/gandiva/LessThanOrEqualTo.h>
#include <fpdb/expression/gandiva/Like.h>
#include <fpdb/expression/gandiva/Multiply.h>
#include <fpdb/expression/gandiva/Not.h>
#include <fpdb/expression/gandiva/NotEqualTo.h>
#include <fpdb/expression/gandiva/NumericLiteral.h>
#include <fpdb/expression/gandiva/Or.h>
#include <fpdb/expression/gandiva/StringLiteral.h>
#include <fpdb/expression/gandiva/Substr.h>
#include <fpdb/expression/gandiva/Subtract.h>
#include <fpdb/caf/CAFUtil.h>

using namespace fpdb::expression::gandiva;
using ExpressionPtr = std::shared_ptr<fpdb::expression::gandiva::Expression>;


CAF_BEGIN_TYPE_ID_BLOCK(Expression, fpdb::caf::CAFUtil::Expression_first_custom_type_id)
CAF_ADD_TYPE_ID(Expression, (ExpressionPtr))
CAF_ADD_TYPE_ID(Expression, (Add))
CAF_ADD_TYPE_ID(Expression, (And))
CAF_ADD_TYPE_ID(Expression, (Cast))
CAF_ADD_TYPE_ID(Expression, (fpdb::expression::gandiva::Column))
CAF_ADD_TYPE_ID(Expression, (DateAdd))
CAF_ADD_TYPE_ID(Expression, (DateExtract))
CAF_ADD_TYPE_ID(Expression, (Divide))
CAF_ADD_TYPE_ID(Expression, (EqualTo))
CAF_ADD_TYPE_ID(Expression, (GreaterThan))
CAF_ADD_TYPE_ID(Expression, (GreaterThanOrEqualTo))
CAF_ADD_TYPE_ID(Expression, (If))
CAF_ADD_TYPE_ID(Expression, (In<arrow::Int32Type, int32_t>))
CAF_ADD_TYPE_ID(Expression, (In<arrow::Int64Type, int64_t>))
CAF_ADD_TYPE_ID(Expression, (In<arrow::DoubleType, double>))
CAF_ADD_TYPE_ID(Expression, (In<arrow::Date64Type, int64_t>))
CAF_ADD_TYPE_ID(Expression, (In<arrow::StringType, string>))
CAF_ADD_TYPE_ID(Expression, (IsNull))
CAF_ADD_TYPE_ID(Expression, (LessThan))
CAF_ADD_TYPE_ID(Expression, (LessThanOrEqualTo))
CAF_ADD_TYPE_ID(Expression, (Like))
CAF_ADD_TYPE_ID(Expression, (Multiply))
CAF_ADD_TYPE_ID(Expression, (Not))
CAF_ADD_TYPE_ID(Expression, (NotEqualTo))
CAF_ADD_TYPE_ID(Expression, (NumericLiteral<arrow::Int32Type>))
CAF_ADD_TYPE_ID(Expression, (NumericLiteral<arrow::Int64Type>))
CAF_ADD_TYPE_ID(Expression, (NumericLiteral<arrow::DoubleType>))
CAF_ADD_TYPE_ID(Expression, (NumericLiteral<arrow::BooleanType>))
CAF_ADD_TYPE_ID(Expression, (NumericLiteral<arrow::Date64Type>))
CAF_ADD_TYPE_ID(Expression, (Or))
CAF_ADD_TYPE_ID(Expression, (StringLiteral))
CAF_ADD_TYPE_ID(Expression, (Substr))
CAF_ADD_TYPE_ID(Expression, (Subtract))
CAF_END_TYPE_ID_BLOCK(Expression)

// Variant-based approach on OperatorPtr
namespace caf {

template<>
struct variant_inspector_traits<ExpressionPtr> {
  using value_type = ExpressionPtr;

  // Lists all allowed types and gives them a 0-based index.
  static constexpr type_id_t allowed_types[] = {
          type_id_v<none_t>,
          type_id_v<Add>,
          type_id_v<And>,
          type_id_v<Cast>,
          type_id_v<fpdb::expression::gandiva::Column>,
          type_id_v<DateAdd>,
          type_id_v<DateExtract>,
          type_id_v<Divide>,
          type_id_v<EqualTo>,
          type_id_v<GreaterThan>,
          type_id_v<GreaterThanOrEqualTo>,
          type_id_v<If>,
          type_id_v<In<arrow::Int32Type, int32_t>>,
          type_id_v<In<arrow::Int64Type, int64_t>>,
          type_id_v<In<arrow::DoubleType, double>>,
          type_id_v<In<arrow::Date64Type, int64_t>>,
          type_id_v<In<arrow::StringType, string>>,
          type_id_v<IsNull>,
          type_id_v<LessThan>,
          type_id_v<LessThanOrEqualTo>,
          type_id_v<Like>,
          type_id_v<Multiply>,
          type_id_v<Not>,
          type_id_v<NotEqualTo>,
          type_id_v<NumericLiteral<arrow::Int32Type>>,
          type_id_v<NumericLiteral<arrow::Int64Type>>,
          type_id_v<NumericLiteral<arrow::DoubleType>>,
          type_id_v<NumericLiteral<arrow::BooleanType>>,
          type_id_v<NumericLiteral<arrow::Date64Type>>,
          type_id_v<Or>,
          type_id_v<StringLiteral>,
          type_id_v<Substr>,
          type_id_v<Subtract>
  };

  // Returns which type in allowed_types corresponds to x.
  static auto type_index(const value_type &x) {
    if (!x)
      return 0;
    else if (x->getType() == ADD)
      return 1;
    else if (x->getType() == AND)
      return 2;
    else if (x->getType() == CAST)
      return 3;
    else if (x->getType() == COLUMN)
      return 4;
    else if (x->getType() == DATE_ADD)
      return 5;
    else if (x->getType() == DATE_EXTRACT)
      return 6;
    else if (x->getType() == DIVIDE)
      return 7;
    else if (x->getType() == EQUAL_TO)
      return 8;
    else if (x->getType() == GREATER_THAN)
      return 9;
    else if (x->getType() == GREATER_THAN_OR_EQUAL_TO)
      return 10;
    else if (x->getType() == IF)
      return 11;
    else if (x->getType() == IN && x->getTypeString() == "In<Int32>")
      return 12;
    else if (x->getType() == IN && x->getTypeString() == "In<Int64>")
      return 13;
    else if (x->getType() == IN && x->getTypeString() == "In<Double>")
      return 14;
    else if (x->getType() == IN && x->getTypeString() == "In<Date64>")
      return 15;
    else if (x->getType() == IN && x->getTypeString() == "In<String>")
      return 16;
    else if (x->getType() == IS_NULL)
      return 17;
    else if (x->getType() == LESS_THAN)
      return 18;
    else if (x->getType() == LESS_THAN_OR_EQUAL_TO)
      return 19;
    else if (x->getType() == LIKE)
      return 20;
    else if (x->getType() == MULTIPLY)
      return 21;
    else if (x->getType() == NOT)
      return 22;
    else if (x->getType() == NOT_EQUAL_TO)
      return 23;
    else if (x->getType() == NUMERIC_LITERAL && x->getTypeString() == "NumericLiteral<Int32>")
      return 24;
    else if (x->getType() == NUMERIC_LITERAL && x->getTypeString() == "NumericLiteral<Int64>")
      return 25;
    else if (x->getType() == NUMERIC_LITERAL && x->getTypeString() == "NumericLiteral<Double>")
      return 26;
    else if (x->getType() == NUMERIC_LITERAL && x->getTypeString() == "NumericLiteral<Boolean>")
      return 27;
    else if (x->getType() == NUMERIC_LITERAL && x->getTypeString() == "NumericLiteral<Date64>")
      return 28;
    else if (x->getType() == OR)
      return 29;
    else if (x->getType() == STRING_LITERAL)
      return 30;
    else if (x->getType() == SUBSTR)
      return 31;
    else if (x->getType() == SUBTRACT)
      return 32;
    else
      return -1;
  }

  // Applies f to the value of x.
  template<class F>
  static auto visit(F &&f, const value_type &x) {
    switch (type_index(x)) {
      case 1:
        return f(dynamic_cast<Add &>(*x));
      case 2:
        return f(dynamic_cast<And &>(*x));
      case 3:
        return f(dynamic_cast<Cast &>(*x));
      case 4:
        return f(dynamic_cast<fpdb::expression::gandiva::Column &>(*x));
      case 5:
        return f(dynamic_cast<DateAdd &>(*x));
      case 6:
        return f(dynamic_cast<DateExtract &>(*x));
      case 7:
        return f(dynamic_cast<Divide &>(*x));
      case 8:
        return f(dynamic_cast<EqualTo &>(*x));
      case 9:
        return f(dynamic_cast<GreaterThan &>(*x));
      case 10:
        return f(dynamic_cast<GreaterThanOrEqualTo &>(*x));
      case 11:
        return f(dynamic_cast<If &>(*x));
      case 12:
        return f(dynamic_cast<In<arrow::Int32Type, int32_t> &>(*x));
      case 13:
        return f(dynamic_cast<In<arrow::Int64Type, int64_t> &>(*x));
      case 14:
        return f(dynamic_cast<In<arrow::DoubleType, double> &>(*x));
      case 15:
        return f(dynamic_cast<In<arrow::Date64Type, int64_t> &>(*x));
      case 16:
        return f(dynamic_cast<In<arrow::StringType, string> &>(*x));
      case 17:
        return f(dynamic_cast<IsNull &>(*x));
      case 18:
        return f(dynamic_cast<LessThan &>(*x));
      case 19:
        return f(dynamic_cast<LessThanOrEqualTo &>(*x));
      case 20:
        return f(dynamic_cast<Like &>(*x));
      case 21:
        return f(dynamic_cast<Multiply &>(*x));
      case 22:
        return f(dynamic_cast<Not &>(*x));
      case 23:
        return f(dynamic_cast<NotEqualTo &>(*x));
      case 24:
        return f(dynamic_cast<NumericLiteral<arrow::Int32Type> &>(*x));
      case 25:
        return f(dynamic_cast<NumericLiteral<arrow::Int64Type> &>(*x));
      case 26:
        return f(dynamic_cast<NumericLiteral<arrow::DoubleType> &>(*x));
      case 27:
        return f(dynamic_cast<NumericLiteral<arrow::BooleanType> &>(*x));
      case 28:
        return f(dynamic_cast<NumericLiteral<arrow::Date64Type> &>(*x));
      case 29:
        return f(dynamic_cast<Or &>(*x));
      case 30:
        return f(dynamic_cast<StringLiteral &>(*x));
      case 31:
        return f(dynamic_cast<Substr &>(*x));
      case 32:
        return f(dynamic_cast<Subtract &>(*x));
      default: {
        none_t dummy;
        return f(dummy);
      }
    }
  }

  // Assigns a value to x.
  template<class U>
  static void assign(value_type &x, U value) {
    if constexpr (std::is_same<U, none_t>::value)
      x.reset();
    else
      x = std::make_shared<U>(std::move(value));
  }

  // Create a default-constructed object for `type` and then call the
  // continuation with the temporary object to perform remaining load steps.
  template<class F>
  static bool load(type_id_t type, F continuation) {
    switch (type) {
      default:
        return false;
      case type_id_v<none_t>: {
        none_t dummy;
        continuation(dummy);
        return true;
      }
      case type_id_v<Add>: {
        auto tmp = Add{};
        continuation(tmp);
        return true;
      }
      case type_id_v<And>: {
        auto tmp = And{};
        continuation(tmp);
        return true;
      }
      case type_id_v<Cast>: {
        auto tmp = Cast{};
        continuation(tmp);
        return true;
      }
      case type_id_v<fpdb::expression::gandiva::Column>: {
        auto tmp = fpdb::expression::gandiva::Column{};
        continuation(tmp);
        return true;
      }
      case type_id_v<DateAdd>: {
        auto tmp = DateAdd{};
        continuation(tmp);
        return true;
      }
      case type_id_v<DateExtract>: {
        auto tmp = DateExtract{};
        continuation(tmp);
        return true;
      }
      case type_id_v<Divide>: {
        auto tmp = Divide{};
        continuation(tmp);
        return true;
      }
      case type_id_v<EqualTo>: {
        auto tmp = EqualTo{};
        continuation(tmp);
        return true;
      }
      case type_id_v<GreaterThan>: {
        auto tmp = GreaterThan{};
        continuation(tmp);
        return true;
      }
      case type_id_v<GreaterThanOrEqualTo>: {
        auto tmp = GreaterThanOrEqualTo{};
        continuation(tmp);
        return true;
      }
      case type_id_v<If>: {
        auto tmp = If{};
        continuation(tmp);
        return true;
      }
      case type_id_v<In<arrow::Int32Type, int32_t>>: {
        auto tmp = In<arrow::Int32Type, int32_t>{};
        continuation(tmp);
        return true;
      }
      case type_id_v<In<arrow::Int64Type, int64_t>>: {
        auto tmp = In<arrow::Int64Type, int64_t>{};
        continuation(tmp);
        return true;
      }
      case type_id_v<In<arrow::DoubleType, double>>: {
        auto tmp = In<arrow::DoubleType, double>{};
        continuation(tmp);
        return true;
      }
      case type_id_v<In<arrow::Date64Type, int64_t>>: {
        auto tmp = In<arrow::Date64Type, int64_t>{};
        continuation(tmp);
        return true;
      }
      case type_id_v<In<arrow::StringType, string>>: {
        auto tmp = In<arrow::StringType, string>{};
        continuation(tmp);
        return true;
      }
      case type_id_v<IsNull>: {
        auto tmp = IsNull{};
        continuation(tmp);
        return true;
      }
      case type_id_v<LessThan>: {
        auto tmp = LessThan{};
        continuation(tmp);
        return true;
      }
      case type_id_v<LessThanOrEqualTo>: {
        auto tmp = LessThanOrEqualTo{};
        continuation(tmp);
        return true;
      }
      case type_id_v<Like>: {
        auto tmp = Like{};
        continuation(tmp);
        return true;
      }
      case type_id_v<Multiply>: {
        auto tmp = Multiply{};
        continuation(tmp);
        return true;
      }
      case type_id_v<Not>: {
        auto tmp = Not{};
        continuation(tmp);
        return true;
      }
      case type_id_v<NotEqualTo>: {
        auto tmp = NotEqualTo{};
        continuation(tmp);
        return true;
      }
      case type_id_v<NumericLiteral<arrow::Int32Type>>: {
        auto tmp = NumericLiteral<arrow::Int32Type>{};
        continuation(tmp);
        return true;
      }
      case type_id_v<NumericLiteral<arrow::Int64Type>>: {
        auto tmp = NumericLiteral<arrow::Int64Type>{};
        continuation(tmp);
        return true;
      }
      case type_id_v<NumericLiteral<arrow::DoubleType>>: {
        auto tmp = NumericLiteral<arrow::DoubleType>{};
        continuation(tmp);
        return true;
      }
      case type_id_v<NumericLiteral<arrow::BooleanType>>: {
        auto tmp = NumericLiteral<arrow::BooleanType>{};
        continuation(tmp);
        return true;
      }
      case type_id_v<NumericLiteral<arrow::Date64Type>>: {
        auto tmp = NumericLiteral<arrow::Date64Type>{};
        continuation(tmp);
        return true;
      }
      case type_id_v<Or>: {
        auto tmp = Or{};
        continuation(tmp);
        return true;
      }
      case type_id_v<StringLiteral>: {
        auto tmp = StringLiteral{};
        continuation(tmp);
        return true;
      }
      case type_id_v<Substr>: {
        auto tmp = Substr{};
        continuation(tmp);
        return true;
      }
      case type_id_v<Subtract>: {
        auto tmp = Subtract{};
        continuation(tmp);
        return true;
      }
    }
  }
};

template <>
struct inspector_access<ExpressionPtr> : variant_inspector_access<ExpressionPtr> {
  // nop
};

} // namespace caf

#endif //FPDB_FPDB_EXPRESSION_GANDIVA_INCLUDE_FPDB_EXPRESSION_GANDIVA_CAFSERIALIZATION_CAFEXPRESSIONSERIALIZER_H
