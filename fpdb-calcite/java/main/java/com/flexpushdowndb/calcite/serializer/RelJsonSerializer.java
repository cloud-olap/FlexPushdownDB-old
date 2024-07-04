package com.flexpushdowndb.calcite.serializer;

import org.apache.calcite.adapter.enumerable.*;
import org.apache.calcite.rel.RelFieldCollation;
import org.apache.calcite.rel.RelNode;
import org.apache.calcite.rel.core.AggregateCall;
import org.apache.calcite.rel.core.Join;
import org.apache.calcite.rel.core.Sort;
import org.apache.calcite.rex.*;
import org.json.JSONArray;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;

public final class RelJsonSerializer {
  private final RelNode relNode;
  private final Set<EnumerableHashJoin> pushableHashJoins;

  public RelJsonSerializer(RelNode relNode, Set<EnumerableHashJoin> pushableHashJoins) {
    this.relNode = relNode;
    this.pushableHashJoins = pushableHashJoins;
  }

  public static JSONObject serialize(RelNode relNode, Set<EnumerableHashJoin> pushableHashJoins) {
    JSONArray outputFieldsJArr = new JSONArray();
    for (String fieldName: relNode.getRowType().getFieldNames()) {
      outputFieldsJArr.put(fieldName);
    }
    return new JSONObject()
            .put("plan", new RelJsonSerializer(relNode, pushableHashJoins).serialize())
            .put("outputFields", outputFieldsJArr);
  }

  private JSONObject serialize() {
    return serializeRel(relNode);
  }

  private JSONObject serializeRel(RelNode relNode) {
    JSONObject jo;
    if (relNode instanceof EnumerableTableScan) {
      jo = serializeEnumerableTableScan((EnumerableTableScan) relNode);
    } else if (relNode instanceof EnumerableFilter) {
      jo = serializeEnumerableFilter((EnumerableFilter) relNode);
    } else if (relNode instanceof EnumerableHashJoin || relNode instanceof EnumerableNestedLoopJoin) {
      jo = serializeJoin((Join) relNode);
    } else if (relNode instanceof EnumerableProject) {
      jo = serializeEnumerableProject((EnumerableProject) relNode);
    } else if (relNode instanceof EnumerableAggregate || relNode instanceof EnumerableSortedAggregate) {
      jo = serializeEnumerableAggregate((EnumerableAggregateBase) relNode);
    } else if (relNode instanceof EnumerableSort) {
      jo = serializeEnumerableSort((EnumerableSort) relNode);
    } else if (relNode instanceof EnumerableLimitSort) {
      jo = serializeEnumerableLimitSort((EnumerableLimitSort) relNode);
    } else {
      throw new UnsupportedOperationException("Serialize unsupported RelNode: " + relNode.getClass().getSimpleName());
    }
    return jo;
  }

  private JSONArray serializeRelInputs(RelNode relNode) {
    JSONArray jArr = new JSONArray();
    for (RelNode input: relNode.getInputs()) {
      jArr.put(serializeRel(input));
    }
    return jArr;
  }

  private JSONObject serializeEnumerableTableScan(EnumerableTableScan scan) {
    JSONObject jo = new JSONObject();
    // operator name
    jo.put("operator", scan.getClass().getSimpleName());
    // schema
    jo.put("schema", scan.getTable().getQualifiedName().get(0));
    // table
    jo.put("table", scan.getTable().getQualifiedName().get(1));
    // input operators
    jo.put("inputs", serializeRelInputs(scan));
    return jo;
  }

  private JSONObject serializeEnumerableFilter(EnumerableFilter filter) {
    JSONObject jo = new JSONObject();
    // operator name
    jo.put("operator", filter.getClass().getSimpleName());
    // filter condition
    jo.put("condition", RexJsonSerializer.serialize(filter.getCondition(),
                                                    filter.getRowType().getFieldNames(),
                                                    filter.getCluster().getRexBuilder()));
    // input operators
    jo.put("inputs", serializeRelInputs(filter));
    return jo;
  }

  private JSONObject serializeJoin(Join join) {
    JSONObject jo = new JSONObject();
    // operator name
    jo.put("operator", join.getClass().getSimpleName());

    // join may incur field renames, if left and right inputs have overlapping field names
    List<String> leftFieldNames = join.getInput(0).getRowType().getFieldNames();
    List<String> rightFieldNames = join.getInput(1).getRowType().getFieldNames();
    List<String> outputFieldNames = join.getRowType().getFieldNames();

    // left
    JSONArray leftFieldRenamesJArr = new JSONArray();
    for (int i = 0; i < leftFieldNames.size(); ++i) {
      String inputFieldName = leftFieldNames.get(i);
      String outputFieldName = outputFieldNames.get(i);
      if (!inputFieldName.equals(outputFieldName)) {
        leftFieldRenamesJArr.put(new JSONObject()
                .put("old", inputFieldName)
                .put("new", outputFieldName));
      }
    }
    if (!leftFieldRenamesJArr.isEmpty()) {
      jo.put("leftFieldRenames", leftFieldRenamesJArr);
      jo.put("leftFieldNames", leftFieldNames);
    }

    // right
    JSONArray rightFieldRenamesJArr = new JSONArray();
    for (int i = leftFieldNames.size(); i < outputFieldNames.size(); ++i) {
      String inputFieldName = rightFieldNames.get(i - leftFieldNames.size());
      String outputFieldName = outputFieldNames.get(i);
      if (!inputFieldName.equals(outputFieldName)) {
        rightFieldRenamesJArr.put(new JSONObject()
                .put("old", inputFieldName)
                .put("new", outputFieldName));
      }
    }
    if (!rightFieldRenamesJArr.isEmpty()) {
      jo.put("rightFieldRenames", rightFieldRenamesJArr);
      jo.put("rightFieldNames", rightFieldNames);
    }

    // join condition
    List<String> inputFieldNames = new ArrayList<>(outputFieldNames);
    if (inputFieldNames.size() < leftFieldNames.size() + rightFieldNames.size()) {
      // if it's SEMI
      inputFieldNames.addAll(rightFieldNames);
    }
    if (!join.getCondition().isAlwaysTrue()) {
      jo.put("condition", RexJsonSerializer.serialize(join.getCondition(),
                                                      inputFieldNames,
                                                      join.getCluster().getRexBuilder()));
    }

    // join type
    jo.put("joinType", join.getJoinType());

    // pushable hash joins
    jo.put("pushable", join instanceof EnumerableHashJoin
            && pushableHashJoins != null
            && pushableHashJoins.contains((EnumerableHashJoin) join));

    // input operators
    jo.put("inputs", serializeRelInputs(join));
    return jo;
  }

  private JSONObject serializeEnumerableProject(EnumerableProject project) {
    JSONObject jo = new JSONObject();
    // operator name
    jo.put("operator", project.getClass().getSimpleName());

    // project fields
    List<String> inputFieldNames = project.getInput().getRowType().getFieldNames();
    List<String> outputFieldNames = project.getRowType().getFieldNames();
    JSONArray fields = new JSONArray();
    int projectId = 0;
    for (RexNode rexNode: project.getProjects()) {
      JSONObject fieldJObj = new JSONObject();
      fieldJObj.put("name", outputFieldNames.get(projectId));
      fieldJObj.put("expr", RexJsonSerializer.serialize(rexNode, inputFieldNames, project.getCluster().getRexBuilder()));
      fields.put(fieldJObj);
      ++projectId;
    }

    jo.put("fields", fields);
    // input operators
    jo.put("inputs", serializeRelInputs(project));
    return jo;
  }

  private JSONObject serializeEnumerableAggregate(EnumerableAggregateBase aggregate) {
    JSONObject jo = new JSONObject();
    // operator name
    jo.put("operator", aggregate.getClass().getSimpleName());

    // group fields
    List<String> inputFieldNames = aggregate.getInput().getRowType().getFieldNames();
    List<String> outputFieldNames = aggregate.getRowType().getFieldNames();
    JSONArray groupFieldsJArr = new JSONArray();
    for (int fieldIndex: aggregate.getGroupSet().asList()) {
      groupFieldsJArr.put(inputFieldNames.get(fieldIndex));
    }
    jo.put("groupFields", groupFieldsJArr);

    // aggregations
    JSONArray aggListJArr = new JSONArray();
    int outputFieldId = groupFieldsJArr.length();
    for (AggregateCall aggCall: aggregate.getAggCallList()){
      JSONObject aggCallJObj = new JSONObject();
      aggCallJObj.put("aggFunction", aggCall.getAggregation().kind.name());
      aggCallJObj.put("aggOutputField", outputFieldNames.get(outputFieldId));
      if (!aggCall.getArgList().isEmpty()) {
        // Haven't got an aggregation function with multiple arguments
        int aggFieldId = aggCall.getArgList().get(0);
        aggCallJObj.put("aggInputField", inputFieldNames.get(aggFieldId));
      }
      aggListJArr.put(aggCallJObj);
      ++outputFieldId;
    }
    jo.put("aggregations", aggListJArr);

    // input operators
    jo.put("inputs", serializeRelInputs(aggregate));
    return jo;
  }

  private JSONArray serializeSortFields(Sort sort) {
    List<String> inputFieldNames = sort.getInput().getRowType().getFieldNames();
    JSONArray sortFieldsJArr = new JSONArray();
    for (RelFieldCollation collation: sort.getCollation().getFieldCollations()) {
      JSONObject sortFieldJObj = new JSONObject();
      sortFieldJObj.put("field", inputFieldNames.get(collation.getFieldIndex()));
      sortFieldJObj.put("direction", collation.getDirection());
      sortFieldsJArr.put(sortFieldJObj);
    }
    return sortFieldsJArr;
  }

  private JSONObject serializeEnumerableSort(EnumerableSort sort) {
    JSONObject jo = new JSONObject();
    // operator name
    jo.put("operator", sort.getClass().getSimpleName());
    // sort fields
    jo.put("sortFields", serializeSortFields(sort));
    // input operators
    jo.put("inputs", serializeRelInputs(sort));
    return jo;
  }

  private JSONObject serializeEnumerableLimitSort(EnumerableLimitSort limitSort) {
    JSONObject jo = new JSONObject();
    // operator name
    jo.put("operator", limitSort.getClass().getSimpleName());
    // sort fields
    jo.put("sortFields", serializeSortFields(limitSort));
    // limit
    assert limitSort.fetch != null;
    jo.put("limit", RexJsonSerializer.serialize(limitSort.fetch, null, limitSort.getCluster().getRexBuilder()));
    // input operators
    jo.put("inputs", serializeRelInputs(limitSort));
    return jo;
  }
}
