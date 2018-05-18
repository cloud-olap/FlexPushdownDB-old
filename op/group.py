# -*- coding: utf-8 -*-
"""Group by with aggregate support

"""

import itertools
from operator import itemgetter
from op.operator_base import Operator
from sql.aggregate_expr import AggregateExpr


class Group(Operator):
    """A crude group by operator. Allows multiple grouping columns to be specified along with multiple aggregate
    expressions.

    """

    def __init__(self, group_col_indexes, aggregate_expr_strs):
        """Creates a new group by operator.

        :param group_col_indexes: The indexes of the columns to group by
        :param aggregate_expr_strs: The list of aggregate expressions
        """

        Operator.__init__(self)

        self.group_col_indexes = group_col_indexes
        self.aggregate_expr_strs = aggregate_expr_strs
        self.aggregate_exprs = list(AggregateExpr(a_expr) for a_expr in self.aggregate_expr_strs)

        # Dict of tuples hashed by the values of each grouping columns
        self.tuples = {}

    # noinspection PyUnusedLocal
    def on_receive(self, t, producer):
        """ Handles the event of receiving a new tuple from a producer. Applies each aggregate function to the tuple and
        then inserts those aggregates into the tuples dict indexed by the grouping columns values.

        Once downstream producers are completed the tuples will be send to downstream consumers.

        :param t: The received tuples
        :param producer: The producer of the tuple
        :return: None
        """

        # print("Group | {}".format(t))

        def get_items():
            """Needed to handle single item tuples properly, note the trailing comma

            """
            if len(self.group_col_indexes) == 1:
                return itemgetter(*self.group_col_indexes)(t),
            else:
                return itemgetter(*self.group_col_indexes)(t)

        # Create a tuple of columns to group by, we use this as the key for a dict of groups and their associated
        # aggregate values
        group_cols_tuple = get_items()
        # print(group_cols_tuple)

        # Get the current list of aggregates for this group
        group_aggregate_vals = self.tuples.get(group_cols_tuple, list(itertools.repeat(0, len(self.aggregate_exprs))))
        # print(group_aggregate_vals)

        for i in range(0, len(self.aggregate_exprs)):
            e = self.aggregate_exprs[i]
            v = group_aggregate_vals[i]
            e.val = v
            e.eval(t)
            group_aggregate_vals[i] = e.val

        # print(list(group_cols_tuple) + group_aggregate_vals)

        self.tuples[group_cols_tuple] = group_aggregate_vals

    def on_producer_completed(self, producer):
        """Handles the event where the producer has completed producing all the tuples it will produce. Once this
        occurs the tuples can be sent to consumers downstream.

        :param producer: The producer that has completed
        :return: None
        """

        for k, v in self.tuples.items():
            if not self.is_completed():
                t = list(k) + v
                self.send(t)
            else:
                break

        Operator.on_producer_completed(self, producer)
