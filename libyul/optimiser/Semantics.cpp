/*(
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
 * Specific AST walkers that collect semantical facts.
 */

#include <libyul/optimiser/Semantics.h>

#include <libyul/Exceptions.h>
#include <libyul/AsmData.h>
#include <libyul/Dialect.h>
#include <libyul/backends/evm/EVMDialect.h>

#include <libevmasm/SemanticInformation.h>

#include <libdevcore/CommonData.h>

using namespace std;
using namespace dev;
using namespace yul;

MovableChecker::MovableChecker(Dialect const& _dialect):
	m_dialect(_dialect)
{
}

MovableChecker::MovableChecker(Dialect const& _dialect, Expression const& _expression):
	MovableChecker(_dialect)
{
	visit(_expression);
}

void MovableChecker::operator()(Identifier const& _identifier)
{
	ASTWalker::operator()(_identifier);
	m_variableReferences.emplace(_identifier.name);
}

void MovableChecker::operator()(FunctionalInstruction const& _instr)
{
	ASTWalker::operator()(_instr);

	if (!eth::SemanticInformation::movable(_instr.instruction))
		m_movable = false;
	if (!eth::SemanticInformation::sideEffectFree(_instr.instruction))
		m_sideEffectFree = false;
	if (eth::SemanticInformation::invalidatesStorage(_instr.instruction))
		m_invalidatesStorage = true;
}

void MovableChecker::operator()(FunctionCall const& _functionCall)
{
	ASTWalker::operator()(_functionCall);

	if (BuiltinFunction const* f = m_dialect.builtin(_functionCall.functionName.name))
	{
		if (!f->movable)
			m_movable = false;
		if (!f->sideEffectFree)
			m_sideEffectFree = false;
		if (f->invalidatesStorage)
			m_invalidatesStorage = true;
	}
	else
	{
		m_movable = false;
		m_sideEffectFree = false;
		m_invalidatesStorage = true;
	}
}

void MovableChecker::visit(Statement const&)
{
	assertThrow(false, OptimizerException, "Movability for statement requested.");
}


bool InvalidationChecker::invalidatesStorage(Dialect const& _dialect, Block const& _block)
{
	InvalidationChecker ic(_dialect);
	ic(_block);
	return ic.m_invalidates;
}

bool InvalidationChecker::invalidatesStorage(Dialect const& _dialect, Expression const& _expression)
{
	InvalidationChecker ic(_dialect);
	ic.visit(_expression);
	return ic.m_invalidates;
}

void InvalidationChecker::operator()(FunctionalInstruction const& _fun)
{
	if (eth::SemanticInformation::invalidatesStorage(_fun.instruction))
		m_invalidates = true;
}

void InvalidationChecker::operator()(FunctionCall const& _functionCall)
{
	if (BuiltinFunction const* f = m_dialect.builtin(_functionCall.functionName.name))
	{
		if (f->invalidatesStorage)
			m_invalidates = true;
	}
	else
		m_invalidates = true;
}

pair<TerminationFinder::ControlFlow, size_t> TerminationFinder::firstUnconditionalControlFlowChange(
	vector<Statement> const& _statements
)
{
	for (size_t i = 0; i < _statements.size(); ++i)
	{
		ControlFlow controlFlow = controlFlowKind(_statements[i]);
		if (controlFlow != ControlFlow::FlowOut)
			return {controlFlow, i};
	}
	return {ControlFlow::FlowOut, size_t(-1)};
}

TerminationFinder::ControlFlow TerminationFinder::controlFlowKind(Statement const& _statement)
{
	if (
		_statement.type() == typeid(ExpressionStatement) &&
		isTerminatingBuiltin(boost::get<ExpressionStatement>(_statement))
	)
		return ControlFlow::Terminate;
	else if (_statement.type() == typeid(Break))
		return ControlFlow::Break;
	else if (_statement.type() == typeid(Continue))
		return ControlFlow::Continue;
	else
		return ControlFlow::FlowOut;
}

bool TerminationFinder::isTerminatingBuiltin(ExpressionStatement const& _exprStmnt)
{
	if (_exprStmnt.expression.type() == typeid(FunctionalInstruction))
		return eth::SemanticInformation::terminatesControlFlow(
			boost::get<FunctionalInstruction>(_exprStmnt.expression).instruction
		);
	else if (_exprStmnt.expression.type() == typeid(FunctionCall))
		if (auto const* dialect = dynamic_cast<EVMDialect const*>(&m_dialect))
			if (auto const* builtin = dialect->builtin(boost::get<FunctionCall>(_exprStmnt.expression).functionName.name))
				if (builtin->instruction)
					return eth::SemanticInformation::terminatesControlFlow(*builtin->instruction);
	return false;
}
