/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <map>
#include <set>

/**
 * Data structure that keeps track of values and keys of a mapping.
 */
template <class T>
struct InvertibleMap
{
	std::map<T, T> values;
	// references[x] == {y | values[y] == x}
	std::map<T, std::set<T>> references;

	void set(T _key, T _value)
	{
		if (values.count(_key))
			references[values[_key]].erase(_key);
		values[_key] = _value;
		references[_value].insert(_key);
	}

	void eraseKey(T _key)
	{
		if (values.count(_key))
			references[values[_key]].erase(_key);
		values.erase(_key);
	}

	void eraseValue(T _value)
	{
		if (references.count(_value))
		{
			for (T v: references[_value])
				values.erase(v);
			references.erase(_value);
		}
	}

	void clear()
	{
		values.clear();
		references.clear();
	}
};
