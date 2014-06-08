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
#include "upsertexecutor.h"

#include "common/tabletuple.h"
#include "common/FatalException.hpp"
#include "plannodes/upsertnode.h"
#include "storage/persistenttable.h"
#include "storage/tableiterator.h"
#include "storage/temptable.h"
#include "storage/ConstraintFailureException.h"

namespace voltdb {

void UpsertExecutor::p_initMore()
{
    VOLT_TRACE("init Upsert Executor");

    UpsertPlanNode* node = dynamic_cast<UpsertPlanNode*>(m_abstractNode);
    assert(node);
    assert(m_input_tables.size() == 1);

    // Target table must be a PersistentTable and must not be NULL
    PersistentTable *persistentTarget = dynamic_cast<PersistentTable*>(getTargetTable());
    assert(persistentTarget);

    TempTable* input_table = getTempInputTable(); //input table should be temptable
    m_partitionColumnIsString = false;
    m_partitionColumn = persistentTarget->partitionColumn();
    if (m_partitionColumn != -1) {
        if (input_table->schema()->columnType(m_partitionColumn) == VALUE_TYPE_VARCHAR) {
            m_partitionColumnIsString = true;
        }
    }

    m_multiPartition = node->isMultiPartition();
}

bool UpsertExecutor::p_execute()
{
    VOLT_DEBUG("execute Upsert Executor");

    TempTable* input_table = m_input_tables[0].getTempTable(); //input table should be temptable
    assert(input_table);

    // Target table can be StreamedTable or PersistentTable and must not be NULL
    // Update target table reference from table delegate
    PersistentTable* targetTable = dynamic_cast<PersistentTable*>(getTargetTable());
    assert(targetTable);
    assert (targetTable->columnCount() == input_table->columnCount());
    TableTuple targetTuple = TableTuple(targetTable->schema());

    TableTuple tbTuple = TableTuple(input_table->schema());

    VOLT_DEBUG("INPUT TABLE: %s\n", input_table->debug().c_str());
    assert ( ! input_table->isTempTableEmpty());

    // count the number of successful inserts
    int modifiedTuples = 0;

    TableIterator iterator = input_table->iterator();
    while (iterator.next(tbTuple)) {
        VOLT_DEBUG("Upserting tuple '%s' into target table '%s' with table schema: %s",
                tbTuple.debug(targetTable->name()).c_str(), targetTable->name().c_str(),
                targetTable->schema()->debug().c_str());

        // if there is a partition column for the target table
        if (m_partitionColumn != -1) {
            // get the value for the partition column
            NValue value = tbTuple.getNValue(m_partitionColumn);
            bool isLocal = m_engine->isLocalSite(value);

            // if it doesn't map to this site
            if (!isLocal) {
                if (!m_multiPartition) {
                    throw ConstraintFailureException(
                            dynamic_cast<PersistentTable*>(targetTable),
                            tbTuple,
                            "Mispartitioned tuple in single-partition upsert statement.");
                }
                continue;
            }
        }

        // look up the tuple whether it exists already
        if (targetTable->primaryKeyIndex() == NULL) {
            VOLT_ERROR("No primary keys were found in our target table '%s'",
                    targetTable->name().c_str());
        }
        assert(targetTable->primaryKeyIndex() != NULL);
        TableTuple existsTuple = targetTable->lookupTuple(tbTuple);

        if (existsTuple.isNullTuple()) {
            // try to put the tuple into the target table
            if (!targetTable->insertTuple(tbTuple)) {
                VOLT_ERROR("Failed to insert tuple from input table '%s' into"
                        " target table '%s'",
                        input_table->name().c_str(),
                        targetTable->name().c_str());
                return false;
            }
        } else {
            // tuple exists already, try to update the tuple instead
            targetTuple.move(tbTuple.address());
            TableTuple &tempTuple = targetTable->getTempTupleInlined(targetTuple);

            if (!targetTable->updateTupleWithSpecificIndexes(existsTuple, tempTuple,
                    targetTable->allIndexes())) {
                VOLT_INFO("Failed to update existsTuple from table '%s'",
                        targetTable->name().c_str());
                return false;
            }
        }

        // successfully inserted
        modifiedTuples++;
    }

    setModifiedTuples(modifiedTuples);
    VOLT_DEBUG("Finished upserting tuple");
    return true;
}

}
