/* This file is part of VoltDB.
 * Copyright (C) 2008-2016 VoltDB Inc.
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

package org.voltdb.expressions;

import java.util.Map;

import org.json_voltpatches.JSONArray;
import org.json_voltpatches.JSONException;
import org.json_voltpatches.JSONObject;
import org.json_voltpatches.JSONStringer;
import org.voltdb.VoltType;
import org.voltdb.types.ExpressionType;

import com.google_voltpatches.common.collect.ImmutableSortedMap;

/**
 * A special case expression that has no sql equivalent and must be
 * generated by system code. This expression type is only used
 * in elastic partition hashing to dynamically program the
 * determination of the target partition for tuples whose partition
 * key hash values fall within various ranges.
 */
public class HashRangeExpression extends AbstractValueExpression {
    private static final String HASH_COLUMN_MEMBER_NAME = "HASH_COLUMN";
    private static final String RANGES_MEMBER_NAME = "RANGES";
    private static final String RANGE_START_MEMBER_NAME = "RANGE_START";
    private static final String RANGE_END_MEMBER_NAME = "RANGE_END";

    // Members would be final except for requirements of the clone method.
    private ImmutableSortedMap<Integer, Integer> m_ranges;
    private int m_hashColumn = Integer.MIN_VALUE;

    // For deserialization.
    public HashRangeExpression() {
        super(ExpressionType.HASH_RANGE);
        setValueType(VoltType.BOOLEAN);
    }

    public HashRangeExpression(Map<Integer, Integer> ranges, int hashColumn) {
        this();
        m_ranges = ImmutableSortedMap.copyOf(ranges);
        m_hashColumn = hashColumn;
    }

    @Override
    public Object clone() {
        HashRangeExpression clone = (HashRangeExpression)super.clone();
        clone.m_ranges = m_ranges;
        clone.m_hashColumn = m_hashColumn;
        return clone;
    }

    @Override
    public void validate() throws Exception {
        super.validate();

        if ((m_right != null) || (m_left != null)) {
            throw new Exception("ERROR: A Hash Range expression has child expressions for '" + this + "'");
        }
        if (m_hashColumn == Integer.MIN_VALUE) {
            throw new Exception("ERROR: A Hash Range has no hash column set for '" + this + "'");
        }
        if (m_ranges == null) {
            throw new Exception("ERROR: A Hash Range has no ranges set for '" + this + "'");
        }
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof HashRangeExpression == false) {
            return false;
        }
        HashRangeExpression expr = (HashRangeExpression) obj;

        if ((m_ranges == null) != (expr.m_ranges == null)) {
            return false;
        }
        if ((m_hashColumn == Integer.MIN_VALUE) != (expr.m_hashColumn == Integer.MIN_VALUE)) {
            return false;
        }
        if (m_ranges != null) { // Implying both sides non-null
            if (m_ranges.equals(expr.m_ranges) == false) {
                return false;
            }
        }
        if (m_hashColumn != Integer.MIN_VALUE) { // Implying both sides non-null
            if (m_hashColumn != expr.m_hashColumn) {
                return false;
            }
        }
        return true;
    }

    @Override
    public int hashCode() {
        // based on implementation of equals
        // No need to factor in superclass attributes, since equals doesn't.
        int result = 0;
        if (m_ranges != null) {
            result += m_ranges.hashCode();
        }
        if (m_hashColumn != Integer.MIN_VALUE) {
            result += m_hashColumn;
        }
        return result;
    }

    @Override
    public void toJSONString(JSONStringer stringer) throws JSONException {
        super.toJSONString(stringer);
        stringer.key(HASH_COLUMN_MEMBER_NAME).value(m_hashColumn);
        stringer.key(RANGES_MEMBER_NAME).array();
        for (Map.Entry<Integer, Integer> e : m_ranges.entrySet()) {
            stringer.object();
            stringer.key(RANGE_START_MEMBER_NAME).value(e.getKey().intValue());
            stringer.key(RANGE_END_MEMBER_NAME).value(e.getValue().intValue());
            stringer.endObject();
        }
        stringer.endArray();
    }

    @Override
    protected void loadFromJSONObject(JSONObject obj) throws JSONException {
        m_hashColumn = obj.getInt(HASH_COLUMN_MEMBER_NAME);
        JSONArray array = obj.getJSONArray(RANGES_MEMBER_NAME);
        ImmutableSortedMap.Builder<Integer, Integer> b = ImmutableSortedMap.naturalOrder();
        for (int ii = 0; ii < array.length(); ii++) {
            JSONObject range = array.getJSONObject(ii);
            b.put(range.getInt(RANGE_START_MEMBER_NAME), range.getInt(RANGE_END_MEMBER_NAME));
        }
        m_ranges = b.build();
    }

    @Override
    public String explain(String impliedTableName) {
        return "hash range";
    }
}
