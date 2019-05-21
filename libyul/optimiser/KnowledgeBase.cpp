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
/**
 * Class that can answer questions about values of variables and their relations.
 */

#include <libyul/optimiser/KnowledgeBase.h>

#include <libyul/AsmData.h>
#include <libyul/Utilities.h>
#include <libyul/optimiser/SimplificationRules.h>
#include <libyul/optimiser/Semantics.h>

#include <libdevcore/CommonData.h>

using namespace yul;
using namespace dev;

bool KnowledgeBase::knownToBeDifferent(YulString _a, YulString _b) const
{
	// Try to use the simplification rules together with the
	// current values to turn `sub(_a, _b)` into a nonzero constant.
	// If that fails, try `eq(_a, _b)`.

	Expression expr1 = simplify(FunctionCall{{}, {{}, "sub"_yulstring}, make_vector<Expression>(Identifier{{}, _a}, Identifier{{}, _b})});
	if (expr1.type() == typeid(Literal))
		return valueOfLiteral(boost::get<Literal>(expr1)) != 0;

	Expression expr2 = simplify(FunctionCall{{}, {{}, "eq"_yulstring}, make_vector<Expression>(Identifier{{}, _a}, Identifier{{}, _b})});
	if (expr2.type() == typeid(Literal))
		return valueOfLiteral(boost::get<Literal>(expr2)) == 0;

	return false;
}

Expression KnowledgeBase::simplify(Expression _expression) const
{
	// TODO we might want to include some recursion limiter.

	if (_expression.type() == typeid(FunctionCall))
		for (Expression& arg: boost::get<FunctionCall>(_expression).arguments)
			arg = simplify(arg);
	else if (_expression.type() == typeid(FunctionalInstruction))
		for (Expression& arg: boost::get<FunctionalInstruction>(_expression).arguments)
			arg = simplify(arg);

	if (auto match = SimplificationRules::findFirstMatch(_expression, m_dialect, m_variableValues))
		return simplify(match->action().toExpression(locationOf(_expression)));

	return _expression;
}