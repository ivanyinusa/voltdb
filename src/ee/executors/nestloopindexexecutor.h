/* This file is part of VoltDB.
 * Copyright (C) 2008-2014 VoltDB Inc.
 *
 * This file contains original code and/or modifications of original code.
 * Any modifications made by VoltDB Inc. are licensed under the following
 * terms and conditions:
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with VoltDB.  If not, see <http://www.gnu.org/licenses/>.
 */
/* Copyright (C) 2008 by H-Store Project
 * Brown University
 * Massachusetts Institute of Technology
 * Yale University
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef HSTORENESTLOOPINDEXEXECUTOR_H
#define HSTORENESTLOOPINDEXEXECUTOR_H

#include "executors/abstractexecutor.h"

#include "common/tabletuple.h"
#include "plannodes/limitnode.h"

#include "boost/scoped_array.hpp"

#include <string>

namespace voltdb {

class AbstractExpression;
class PersistentTable;
class TableCatalogDelegate;

/**
 * Nested loop for IndexScan.
 * This is the implementation of usual nestloop which receives
 * one input table (<i>outer</i> table) and repeatedly does indexscan
 * on another table (<i>inner</i> table) with inner table's index.
 * This executor is faster than HashMatchJoin and MergeJoin if only one
 * of underlying tables has low selectivity.
 */
class NestLoopIndexExecutor : public AbstractExecutor
{
public:
    NestLoopIndexExecutor() { }
    ~NestLoopIndexExecutor() { }
protected:
    bool p_init(TempTableLimits* limits);
    bool p_execute();
private:
    PersistentTable* getInnerTargetTable();

    IndexLookupType m_lookupType;
    std::string m_index_name;
    JoinType m_join_type;
    TableCatalogDelegate* m_inner_target_tcd;

    // TODO: Document
    boost::scoped_array<AbstractExpression*> m_search_key_array_ptr;

    SortDirectionType m_sort_direction;

    int m_num_of_search_keys;
    AbstractExpression* m_end_expression;
    AbstractExpression* m_post_expression;
    AbstractExpression* m_initial_expression;
    // null row predicate for underflow edge case
    AbstractExpression* m_skip_null_predicate;
    AbstractExpression* m_prejoin_expression;
    AbstractExpression* m_where_expression;
    const AbstractExpression* const* m_output_expression_array;
    SortDirectionType m_sortDirection;
    StandAloneTupleStorage m_null_tuple;
    StandAloneTupleStorage m_index_values;
    LimitPlanNode::InlineState m_inlineLimitOffset;
};

}

#endif
