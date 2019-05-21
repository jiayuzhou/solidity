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
 * Specific AST walkers that collect semantical facts.
 */

#pragma once

#include <libyul/optimiser/ASTWalker.h>

#include <set>

namespace yul
{
struct Dialect;

/**
 * Specific AST walker that determines whether an expression is movable, side-effect free
 * or whether it invalidates storage.
 */
class MovableChecker: public ASTWalker
{
public:
	explicit MovableChecker(Dialect const& _dialect);
	MovableChecker(Dialect const& _dialect, Expression const& _expression);

	void operator()(Identifier const& _identifier) override;
	void operator()(FunctionalInstruction const& _functionalInstruction) override;
	void operator()(FunctionCall const& _functionCall) override;

	/// Disallow visiting anything apart from Expressions (this throws).
	void visit(Statement const&) override;
	using ASTWalker::visit;

	bool movable() const { return m_movable; }
	bool sideEffectFree() const { return m_sideEffectFree; }
	bool invalidatesStorage() const { return m_invalidatesStorage; }

	std::set<YulString> const& referencedVariables() const { return m_variableReferences; }

private:
	Dialect const& m_dialect;
	/// Which variables the current expression references.
	std::set<YulString> m_variableReferences;
	/// Is the current expression movable or not.
	bool m_movable = true;
	/// Is the current expression side-effect free, i.e. can be removed
	/// without changing the semantics.
	bool m_sideEffectFree = true;
	bool m_invalidatesStorage = false;
};

class InvalidationChecker: public ASTWalker
{
public:
	/// @returns true if some part of the block might invalidate storage.
	static bool invalidatesStorage(Dialect const& _dialect, Block const& _block);
	static bool invalidatesStorage(Dialect const& _dialect, Expression const& _expression);
private:
	explicit InvalidationChecker(Dialect const& _dialect): m_dialect(_dialect) {}

public:
	using ASTWalker::operator();
	void operator()(FunctionalInstruction const&) override;
	void operator()(FunctionCall const& _functionCall) override;

private:
	Dialect const& m_dialect;
	bool m_invalidates = false;
};

/**
 * Helper class to find "irregular" control flow.
 * This includes termination, break and continue.
 */
class TerminationFinder
{
public:
	enum class ControlFlow { FlowOut, Break, Continue, Terminate };

	TerminationFinder(Dialect const& _dialect): m_dialect(_dialect) {}

	/// @returns the index of the first statement in the provided sequence
	/// that is an unconditional ``break``, ``continue`` or a
	/// call to a terminating builtin function.
	/// If control flow can continue at the end of the list,
	/// returns `FlowOut` and ``size_t(-1)``.
	/// The function might return ``FlowOut`` even though control
	/// flow cannot actually continue.
	std::pair<ControlFlow, size_t> firstUnconditionalControlFlowChange(
		std::vector<Statement> const& _statements
	);

	/// @returns the control flow type of the given statement.
	/// This function could return FlowOut even if control flow never continues.
	ControlFlow controlFlowKind(Statement const& _statement);

	/// @returns true if the expression statement is a direct
	/// call to a builtin terminating function like
	/// ``stop``, ``revert`` or ``return``.
	bool isTerminatingBuiltin(ExpressionStatement const& _exprStmnt);

private:
	Dialect const& m_dialect;
};

}
