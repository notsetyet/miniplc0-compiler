#include "analyser.h"
#include "c0-vm-cpp/src/instruction.h"
#include "c0-vm-cpp/src/function.h"
#include "c0-vm-cpp/src/file.h"
#include "c0-vm-cpp/src/util/print.hpp"
#include "3rd_party/fmt/include/fmt/core.h"
#include "fmts.hpp"

#include <climits>
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <cstdint>
#include <string>
#include <variant>
#include <utility>

namespace miniplc0 {
	std::pair<std::vector<std::string>, std::optional<CompilationError>> Analyser::Analyse() {
		synT.clear();
		output.clear();
		functions.clear();
		constants.clear();
		_instructions.clear();
		auto err = analyseMain();
		if (err.has_value()) {
			return std::make_pair(std::vector<std::string>(), err);
		}
		else {
			printFile();
			return std::make_pair(output, std::optional<CompilationError>());
		}
	}

	std::optional<CompilationError> Analyser::analyseMain() {
		addTable();
		while (true) {
			auto next = nextToken();
			if (!next.has_value()) {
				return {};
			}
			if (next.value().GetType() == TokenType::CONST) {
				//const must be var
				std::vector<vm::Instruction> instrucs;
				int varNum = 0;
				vm::Instruction instr;
				instr.op = vm::OpCode::snew;
				instr.x = 0;
				_instructions.emplace_back(instr);
				int rec = _instructions.size() - 1;				
				auto err = analyseVariableDeclaration(instrucs,varNum);
				_instructions.at(rec).x = varNum;
				if (err.has_value())
					return err;
				
			}
			else {
				if (!isTypeSpecifier(next.value().GetType())) {
					unreadToken();
					break;
				}
				next = nextToken();
				if (!next.has_value()) {
					unreadToken();
					unreadToken();
					break;
				}
				if (next.value().GetType() != TokenType::IDENTIFIER) {
					unreadToken();
					unreadToken();
					break;
				}
				next = nextToken();
				if (!next.has_value()) {
					unreadToken();
					unreadToken();
					unreadToken();
					break;
				}
				if (next.value().GetType() == TokenType::LEFT_BRACKET) {
					unreadToken();
					unreadToken();
					unreadToken();
					break;
				}
				unreadToken();
				unreadToken();
				unreadToken();
				std::vector<vm::Instruction> instrucs;
				int varNum = 0;
				vm::Instruction instr;
				instr.op = vm::OpCode::snew;
				instr.x = 0;
				_instructions.emplace_back(instr);
				int rec = _instructions.size() - 1;
				auto err = analyseVariableDeclaration(instrucs, varNum);
				_instructions.at(rec).x = varNum;
				if (err.has_value())
					return err;
			}
		}

		while (true) {
			auto next = nextToken();
			if (!next.has_value()) {
				return {};
			}
			unreadToken();
			if (!isTypeSpecifier(next.value().GetType())) {
				break;
			}
			auto err = analyseFunc();
			if (err.has_value())
				return err;
		}
		return {};
	}


	/*<variable - declaration> :: =
		[<const - qualifier>]<type - specifier> < init - declarator - list>';'
		< init - declarator - list > :: =
		<init - declarator>{ ',' < init - declarator > }
		<init - declarator> :: =
		<identifier>[<initializer>]
		<initializer> :: =
		'=' < expression >*/
	std::optional<CompilationError> Analyser::analyseVariableDeclaration(std::vector<vm::Instruction>& instrucs,int& varNum) {
		auto next = nextToken();
		if (!next.has_value()) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidVariableDeclaration);
		}
		bool isConst = false;
		bool isInt = false;
		bool isVoid = false;
		bool isDouble = false;
		if (next.value().GetType() == TokenType::CONST) {
			isConst = true;
		}
		else unreadToken();
		next = nextToken();
		TokenType type = next.value().GetType();
		switch (type)
		{
		case TokenType::INT:
			isInt = true;
			break;
		case TokenType::VOID:
			isVoid = true;
			break;
		case TokenType::DOUBLE:
			isDouble = true;
			break;
		default:
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSpecifier);
		}
		next = nextToken();
		if (!next.has_value()) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
		}
		if (next.value().GetType() != TokenType::IDENTIFIER) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
		}
		unreadToken();
		if (isInt == true) {
			addIntVariable(next.value());
		}
		else if (isVoid == true) {
			addVoidVariable(next.value());
		}
		else if (isDouble == true) {

		}
		if (isConst == true) {
			addConstant(next.value());
		}
		auto err = analyseInitDeclaratorList(type,instrucs,varNum);
		if (err.has_value())
			return err;
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::SEMICOLON) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
		}
		return {};
	}
	/*<init - declarator> :: =
		<identifier>[<initializer>]*/
	std::optional<CompilationError> Analyser::analyseInitDeclarator(const TokenType& tp, std::vector<vm::Instruction>& instrucs,int& varNum) {
		auto next = nextToken();		
		if (!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidIdentifier);
		}
		auto name = next.value().GetValueString();
		next = nextToken();
		if (!next.has_value()) {
			return {};
		}
		if (next.value().GetType() != TokenType::EQUAL_SIGN) {
			if (isConstant(name)) {//需要显式赋值
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrConstantNeedValue);
			}
			unreadToken();
			std::vector<struct Variable> tmp = synT[_level];
			struct Variable vtmp;
			vtmp.name = name;
			vtmp._level = _level;
			vtmp.type = tp;
			tmp.emplace_back(vtmp);
			synT.erase(_level);
			synT.emplace(_level, tmp);
			int sizes = (int)tmp.size() - 1;
			vm::Instruction instr;
			instr.op = vm::OpCode::loada;
			instr.y = sizes;
			instr.x = 0;
			if (_level == 0) {
				_instructions.emplace_back(instr);
			}
			else now_func.instructions.emplace_back(instr);
			return {};
		}
		unreadToken();//unread '='
		addInitializedVariable(next.value());
		std::vector<struct Variable> tmp = synT[_level];
		struct Variable vtmp;
		vtmp.name = name;
		vtmp._level = _level;
		vtmp.type = tp;
		tmp.emplace_back(vtmp);
		synT.erase(_level);
		synT.emplace(_level, tmp);
		vm::Instruction instr;
		instr.op = vm::OpCode::loada;
		int sizes = (int)tmp.size() - 1;
		instr.y = sizes;
		instr.x = 0;
		if (_level == 0) {
			_instructions.emplace_back(instr);
		}
		else now_func.instructions.emplace_back(instr);
		auto err = analyseInitializer(instrucs);
		if (err.has_value())
			return err;		
		return {};
	}
	/*< init - declarator - list > :: =
		<init - declarator>{ ',' < init - declarator > }*/
	std::optional<CompilationError> Analyser::analyseInitDeclaratorList(const TokenType& tp, std::vector<vm::Instruction>& instrucs,int& varNum) {
		auto err = analyseInitDeclarator(tp,instrucs,varNum);
		if (err.has_value())
			return err;
		varNum++;
		while (true) {
			auto next = nextToken();
			if (!next.has_value()) {
				return {};
			}
			if (next.value().GetType() != TokenType::COMMA) {
				unreadToken();
				return {};
			}
			err = analyseInitDeclarator(tp,instrucs,varNum);
			if (err.has_value())
				return err;
			varNum++;
		}
		return {};
	}
	/*<initializer> :: =
		'=' < expression >*/
	std::optional<CompilationError> Analyser::analyseInitializer(std::vector<vm::Instruction>& instrucs) {
		auto next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::EQUAL_SIGN) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNotInitialized);
		}
		auto err = analyseExpression(instrucs);
		if (err.second.has_value())
			return err.second;
		auto type = err.first;
		if (type.has_value() && type.value().GetType() == TokenType::UNSIGNED_INTEGER) {
			vm::Instruction instr;
			instr.op = vm::OpCode::istore;
			if (_level == 0) {
				_instructions.emplace_back(instr);
			}
			else now_func.instructions.emplace_back(instr);
			return {};
		}
		return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrCalWithVoid);
	}



	/*<statement - seq> :: =
	{ <statement> }
		<statement> :: =
		<compound - statement>	{
		| <condition - statement>	if switch
		| <loop - statement>	while do for
		| <jump - statement>	break continue return
		| <print - statement>	print
		| <scan - statement>	scan
		| < assignment - expression>';'		identifier =
		| < function - call>';'		identifier (
		| ';'*/
	std::optional<CompilationError> Analyser::analyseStatement(std::vector<vm::Instruction>& instrucs) {
		auto next = nextToken();
		if (!next.has_value()) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		}
		if (next.value().GetType() == TokenType::SEMICOLON) {
			return {};
		}
		if (next.value().GetType() == TokenType::IF
			|| next.value().GetType() == TokenType::SWITCH) {
			unreadToken();
			auto err = analyseConditionStatement(instrucs);
			if (err.has_value())
				return err;
			return {};
		}
		else if (next.value().GetType() == TokenType::LEFT_MID_BRACKET) {
			unreadToken();
			auto err = analyseCompoundStatement(instrucs);
			if (err.has_value()) {
				return err;
			}
			return {};
		}
		else if (next.value().GetType() == TokenType::RETURN
			|| next.value().GetType() == TokenType::BREAK
			|| next.value().GetType() == TokenType::CONTINUE) {
			unreadToken();
			auto err = analyseJmpStatement(instrucs);
			if (err.has_value())
				return err;
			
			return {};
		}
		else if (next.value().GetType() == TokenType::WHILE
			|| next.value().GetType() == TokenType::DO
			|| next.value().GetType() == TokenType::FOR) {
			unreadToken();
			auto err = analyseLoopStatement(instrucs);
			if (err.has_value())
				return err;
			return {};
		}
		else if (next.value().GetType() == TokenType::PRINT) {
			unreadToken();
			auto err = analysePrintStatement(instrucs);
			if (err.has_value())
				return err;
			return {};
		}
		else if (next.value().GetType() == TokenType::SCAN) {
			unreadToken();
			auto err = analyseScanStatement(instrucs);
			if (err.has_value())
				return err;
			return {};
		}
		else if (next.value().GetType() == TokenType::IDENTIFIER) {
			auto name = next.value().GetValueString();
			next = nextToken();
			if (!next.has_value()) {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
			}
			//assign
			else if (next.value().GetType() == TokenType::EQUAL_SIGN) {
				unreadToken();
				unreadToken();
				auto err = analyseAssignmentStatement(instrucs);
				if (err.has_value()) {
					return err;
				}
				next = nextToken();
				if (!next.has_value() || next.value().GetType() != TokenType::SEMICOLON) {
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
				}
				return {};
			}
			//funccall
			else if (next.value().GetType() == TokenType::LEFT_BRACKET) {
				unreadToken();
				unreadToken();
				auto err = analyseFuncCall(instrucs);
				if (err.has_value()) {
					return err;
				}
				next = nextToken();
				if (!next.has_value() || next.value().GetType() != TokenType::SEMICOLON) {
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
				}
				return {};
			}
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		}
		return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
	}
	//{statement}
	std::optional<CompilationError> Analyser::analyseStatementSequence(std::vector<vm::Instruction>& instrucs) {
		while (true) {			
			// 预读
			auto next = nextToken();
			if (!next.has_value()) {
				return {};
			}
			unreadToken();
			if (next.value().GetType() != TokenType::IDENTIFIER &&//读到头了
				next.value().GetType() != TokenType::PRINT &&
				next.value().GetType() != TokenType::SEMICOLON &&
				next.value().GetType() != TokenType::WHILE &&
				next.value().GetType() != TokenType::DO &&
				next.value().GetType() != TokenType::FOR &&
				next.value().GetType() != TokenType::IF &&
				next.value().GetType() != TokenType::SCAN &&
				next.value().GetType() != TokenType::RETURN &&
				next.value().GetType() != TokenType::BREAK &&
				next.value().GetType() != TokenType::CONTINUE &&
				next.value().GetType() != TokenType::LEFT_MID_BRACKET) {
				return {};
			}
			auto err = analyseStatement(instrucs);
			if (err.has_value())
				return err;
		}
		return {};
	}



	/*<expression> :: =
		<additive - expression>
		<additive - expression> :: =
		<multiplicative - expression>{ <additive - operator><multiplicative - expression> }
		<multiplicative - expression> :: =
		<unary - expression>{ <multiplicative - operator><unary - expression> }
		<unary - expression> :: =
		[<unary - operator>]<primary - expression>
		*/
	std::pair<std::optional<Token>, std::optional<CompilationError>> Analyser::analyseExpression(std::vector<vm::Instruction>& instrucs) {
		auto err = analyseAddiExpr(instrucs);
		return err;
	}

	std::optional<CompilationError> Analyser::analyseExpressionList(std::vector<vm::Instruction>& instrucs) {
		auto err = analyseExpression(instrucs).second;
		if (err.has_value()) {
			return  err;
		}
		while (true) {
			auto next = nextToken();
			if (!next.has_value())
				return {};
			if (next.value().GetType() != TokenType::COMMA) {
				unreadToken();
				return {};
			}
			err = analyseExpression(instrucs).second;
			if (err.has_value()) {
				return err;
			}
		}
		return {};
	}
	/*<function - call> :: =
		< identifier> '('[<expression - list>] ')'*/
	std::optional<CompilationError> Analyser::analyseFuncCall(std::vector<vm::Instruction>& instrucs) {
		auto next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
		}
		if (!isDeclared(next.value().GetValueString())) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNotInitialized);
		}
		auto name = next.value().GetValueString();
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncomleteBracket);
		}
		next = nextToken();
		if (!next.has_value()) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncomleteBracket);
		}
		else if (next.value().GetType() == TokenType::RIGHT_BRACKET) {			
			vm::Instruction instr;
			instr.op = vm::OpCode::call;
			instr.x = findFunc(name);
			if (_level == 0) {
				_instructions.emplace_back(instr);
			}
			else now_func.instructions.emplace_back(instr);
			return {};
		}
		//应该是没有或者是expr list
		else {
			unreadToken();
			auto err = analyseExpressionList(instrucs);
			if (err.has_value()) {
				return err;
			}				
		}
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncomleteBracket);
		}
		vm::Instruction instr;
		instr.op = vm::OpCode::call;
		instr.x = findFunc(name);
		if (_level == 0) {
			_instructions.emplace_back(instr);
		}
		else now_func.instructions.emplace_back(instr);
		return {};
	}
	std::pair<std::optional<Token>, std::optional<CompilationError>> Analyser::analyseAddiExpr(std::vector<vm::Instruction>& instrucs) {
		auto err = analyseMultiExpr(instrucs);
		if (err.second.has_value()) {
			return err;
		}
		
		while (true) {
			auto next = nextToken();
			if (!next.has_value()) {
				return err;
			}
			auto type = next.value().GetType();
			bool addOP = false;
			if (type == TokenType::PLUS_SIGN) {
				addOP = true;
			}
			else if (type == TokenType::MINUS_SIGN) {
			}
			else {
				unreadToken();
				return err;
			}
			err = analyseMultiExpr(instrucs);
			if (err.second.has_value()) {
				return err;
			}
			if (err.first.value().GetType() == TokenType::VOID) {
				return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrCalWithVoid));
			}
			if (addOP) {
				vm::Instruction instr;
				instr.op = vm::OpCode::iadd;
				if (_level == 0) {
					_instructions.emplace_back(instr);
				}
				else now_func.instructions.emplace_back(instr);
			}
			else {
				vm::Instruction instr;
				instr.op = vm::OpCode::isub;
				if (_level == 0) {
					_instructions.emplace_back(instr);
				}
				else now_func.instructions.emplace_back(instr);
			}
		}
		//should not reach here
		return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrUnexpected));
	}
	/*<multiplicative - expression> :: =
		<unary - expression>{ <multiplicative - operator><unary - expression> }*/
	std::pair<std::optional<Token>, std::optional<CompilationError>> Analyser::analyseMultiExpr(std::vector<vm::Instruction>& instrucs) {
		auto err = analyseUnaryExpr(instrucs);
		if (err.second.has_value()) {
			return err;
		}
		
		while (true) {
			auto next = nextToken();
			if (!next.has_value()) {
				return err;
			}
			auto type = next.value().GetType();
			bool mulOP = false;
			if (type == TokenType::MULTIPLICATION_SIGN) {
				mulOP = true;
			}
			else if (type == TokenType::DIVISION_SIGN) {
			}
			else {
				unreadToken();
				return err;
			}
			err = analyseUnaryExpr(instrucs);
			if (err.second.has_value()) {
				return err;
			}
			if (err.first.value().GetType() == TokenType::VOID) {
				return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrCalWithVoid));
			}
			if (mulOP) {
				vm::Instruction instr;
				instr.op = vm::OpCode::imul;
				if (_level == 0) {
					_instructions.emplace_back(instr);
				}
				else now_func.instructions.emplace_back(instr);
			}
			else {
				vm::Instruction instr;
				instr.op = vm::OpCode::idiv;
				if (_level == 0) {
					_instructions.emplace_back(instr);
				}
				else now_func.instructions.emplace_back(instr);
			}
		}
		//should not reach here
		return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrUnexpected));
	}
	/*{'(' < type - specifier > ')'}<unary - expression>*/
	std::pair<std::optional<Token>, std::optional<CompilationError>> Analyser::analyseCastExpr() {
		auto next = nextToken();
		if (!next.has_value()) {
			return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier));
		}
		if (next.value().GetType() != TokenType::LEFT_BRACKET) {
			std::vector<vm::Instruction> instr;
			auto err = analyseUnaryExpr(instr);
			return err;
		}
		else {
			next = nextToken();
			if (!isTypeSpecifier(next.value().GetType())) {
				return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrTypeErr));
			}
			TokenType type = next.value().GetType();
			if (type == TokenType::VOID) {
				return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrTypeErr));
			}
			next = nextToken();
			if (next.value().GetType() != TokenType::RIGHT_BRACKET) {
				return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncomleteBracket));
			}
			std::vector<vm::Instruction> instr;
			auto err = analyseUnaryExpr(instr);
			if (err.second.has_value()) {
				return err;
			}
			else {
				auto name = err.first.value().GetValueString();
				return std::make_pair(std::make_optional<Token>(type, name, _current_pos, _current_pos), std::optional<CompilationError>());
			}
		}
	}

	/*<unary - expression> :: =
		[<unary - operator>]<primary - expression>*/
	std::pair<std::optional<Token>, std::optional<CompilationError>> Analyser::analyseUnaryExpr(std::vector<vm::Instruction>& instrucs) {
		auto next = nextToken();
		auto type = next.value().GetType();
		bool minNum = false;
		int16_t unaryType = 0;
		switch (type)
		{
		case miniplc0::PLUS_SIGN:
			break;
		case miniplc0::MINUS_SIGN:
			unaryType = -1;
			minNum = true;
			break;
		default:
			unreadToken();
			break;
		}
		//analysePrimary
		/*<primary - expression> :: =
		'(' < expression > ')'
		| <identifier>
		| <integer - literal>
		| <function - call>*/
		//注意返回的类型
		next = nextToken();
		if (!next.has_value()) {
			return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression));
		}
		//(expr)
		if (next.value().GetType() == TokenType::LEFT_BRACKET) {
			auto err = analyseExpression(instrucs);
			if (err.second.has_value())
				return err;
			next = nextToken();
			if (!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET) {
				return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression));
			}
			if (unaryType == -1) {
				vm::Instruction instr;
				instr.op = vm::OpCode::ineg;
				if (_level == 0) {
					_instructions.emplace_back(instr);
				}
				else now_func.instructions.emplace_back(instr);
			}
			return err;
		}
		else if (next.value().GetType() == TokenType::IDENTIFIER) {
			auto name = next.value().GetValueString();
			auto id = next;
			if (!isDeclared(name)) {
				return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNotInitialized));
			}
			next = nextToken();
			unreadToken();
			//funccall
			if (next.has_value() && next.value().GetType() == TokenType::LEFT_BRACKET) {
				unreadToken();
				//call instruction
				auto err = analyseFuncCall(instrucs);
				if (err.has_value()) {
					return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression));
				}
				//what's the type still problems
				//pos is not important here
				if (unaryType == -1) {
					vm::Instruction instr;
					instr.op = vm::OpCode::ineg;
					if (_level == 0) {
						_instructions.emplace_back(instr);
					}
					else now_func.instructions.emplace_back(instr);
				}
				TokenType funcType = funcs[name];
				if (funcType == TokenType::INT) {
					funcType = TokenType::UNSIGNED_INTEGER;
				}
				return std::make_pair(std::make_optional<Token>(funcType, name, _current_pos, _current_pos), std::optional<CompilationError>());
			}
			//if it's already at the top of stack
			uint16_t level_diff=-1;
			uint16_t offset=-1;
			TokenType type=TokenType::VOID;
			//deal loada
			for (int ind = _level; ind >= 0; ind--) {
				std::vector<struct Variable> tmp = synT[ind];
				uint16_t index = 0;
				bool flag = false;
				for (std::vector<struct Variable>::iterator it = tmp.begin();
					it != tmp.end(); it++,index++) {
					struct Variable vtmp = *it;
					if (vtmp.name == name) {
						type = vtmp.type;
						offset = index;
						level_diff = ind == 0 ? 1 : 0;
						flag = true;
						break;
					}
				}
				if (flag)break;
			}
			if (type == TokenType::VOID) {
				return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrCalWithVoid));
			}
			vm::Instruction instr;
			instr.op = vm::OpCode::loada;
			instr.x = level_diff;
			instr.y = offset;
			if (_level == 0) {
				_instructions.emplace_back(instr);
			}
			else now_func.instructions.emplace_back(instr);
			vm::Instruction instr2;
			instr2.op = vm::OpCode::iload;
			if (_level == 0) {
				_instructions.emplace_back(instr2);
			}
			else now_func.instructions.emplace_back(instr2);
			if (unaryType == -1) {
				//if i should judge it is a int??
				vm::Instruction instr3;
				instr3.op = vm::OpCode::ineg;
				if (_level == 0) {
					_instructions.emplace_back(instr3);
				}
				else now_func.instructions.emplace_back(instr3);
			}
			return std::make_pair(std::make_optional<Token>(TokenType::UNSIGNED_INTEGER, name, _current_pos, _current_pos), std::optional<CompilationError>());
		}
		else if (next.value().GetType() == TokenType::UNSIGNED_INTEGER) {	
			vm::Instruction instr;
			instr.op = vm::OpCode::ipush;
			std::stringstream ss;
			ss << next.value().GetValueString();
			int32_t tmp;
			ss >> tmp;
			if (ss.eof() && !ss.fail()) {
				instr.x = tmp;
			}
			//十六进制
			else {
				return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrUnexpected));
			}
			if (_level == 0) {
				_instructions.emplace_back(instr);
			}
			else now_func.instructions.emplace_back(instr);
			if (minNum == true) {
				vm::Instruction instr2;
				instr2.op = vm::OpCode::ineg;
				if (_level == 0) {
					_instructions.emplace_back(instr2);
				}
				else now_func.instructions.emplace_back(instr2);
			}
			return std::make_pair(std::make_optional<Token>(TokenType::UNSIGNED_INTEGER, next.value().GetValueString(), _current_pos, _current_pos), std::optional<CompilationError>());
		}
		else if (next.value().GetType() == TokenType::S_CHAR) {		
			int id = -1;
			std::string name= "\"" + next.value().GetValueString() + "\"";
			int index = 0;
			for (std::vector<vm::Constant>::iterator it = constants.begin(); it != constants.end();
				it++) {
				vm::Constant ctmp = *it;
				std::string stmp = std::get<std::string>(ctmp.value);
				if (stmp == name) {
					id = index;
					break;
				}
				index++;
			}
			vm::Constant constant;
			constant.type = vm::Constant::Type::STRING;
			constant.value = "\"" + next.value().GetValueString() + "\"";
			if(id==-1)constants.emplace_back(constant);
			vm::Instruction instr;
			instr.op = vm::OpCode::loadc;
			if (id == -1)instr.x = constants.size() - 1;
			else instr.x = id;
			if (_level == 0) {
				_instructions.emplace_back(instr);
			}
			else now_func.instructions.emplace_back(instr);
			return std::make_pair(std::make_optional<Token>(TokenType::S_CHAR, next.value().GetValueString(), _current_pos, _current_pos), std::optional<CompilationError>());
		}
		else if (next.value().GetType() == TokenType::C_CHAR) {
			int id = -1;
			std::string name = "\"" + next.value().GetValueString() + "\"";
			int index = 0;
			for (std::vector<vm::Constant>::iterator it = constants.begin(); it != constants.end();
				it++) {
				vm::Constant ctmp = *it;
				std::string stmp = std::get<std::string>(ctmp.value);
				if (stmp == name) {
					id = index;
					break;
				}
				index++;
			}
			vm::Constant constant;
			constant.type = vm::Constant::Type::STRING;
			constant.value = "\"" + next.value().GetValueString() + "\"";
			if (id == -1)constants.emplace_back(constant);
			vm::Instruction instr;
			instr.op = vm::OpCode::loadc;
			if (id == -1)instr.x = constants.size() - 1;
			else instr.x = id;
			if (_level == 0) {
				_instructions.emplace_back(instr);
			}
			else now_func.instructions.emplace_back(instr);
			return std::make_pair(std::make_optional<Token>(TokenType::C_CHAR, next.value().GetValueString(), _current_pos, _current_pos), std::optional<CompilationError>());
		}
		return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression));
	}

	//// <assignment-expression> ::= 
	//<identifier><assignment - operator><expression>
	//// 需要补全
	std::optional<CompilationError> Analyser::analyseAssignmentStatement(std::vector<vm::Instruction>& instrucs) {
		auto next = nextToken();	
		if (!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
		}
		if (!isDeclared(next.value().GetValueString())) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNotInitialized);
		}
		if (isConstant(next.value().GetValueString())) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrAssignToConstant);
		}
		if (isUninitializedVariable(next.value().GetValueString())) {
			addInitializedVariable(next.value());
		}
		auto name = next.value().GetValueString();
		//if it's at the top of stack
		uint16_t level_diff = -1;
		uint16_t offset = -1;
		//deal loada
		for (int ind = _level; ind >= 0; ind--) {
			std::vector<struct Variable> tmp = synT[ind];
			uint16_t index = 0;
			bool flag = false;
			for (std::vector<struct Variable>::iterator it = tmp.begin();
				it != tmp.end(); it++, index++) {
				struct Variable vtmp = *it;
				if (vtmp.name == name) {
					offset = index;
					level_diff = ind == 0 ? 1 : 0;
					flag = true;
					break;
				}
			}
			if (flag)break;
		}
		vm::Instruction instr;
		instr.op = vm::OpCode::loada;
		instr.x = level_diff;
		instr.y = offset;
		if (_level == 0) {
			_instructions.emplace_back(instr);
		}
		else now_func.instructions.emplace_back(instr);
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::EQUAL_SIGN) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedEqualSign);
		}
		auto err = analyseExpression(instrucs);
		if (err.second.has_value()) {
			return err.second;
		}
		auto type = err.first;
		if (type.has_value() && type.value().GetType() == TokenType::INT) {
			vm::Instruction instr;
			instr.op = vm::OpCode::istore;
			if (_level == 0) {
				_instructions.emplace_back(instr);
			}
			else now_func.instructions.emplace_back(instr);
		}
		if (type.has_value() && type.value().GetType() == TokenType::VOID) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrCalWithVoid);
		}
		if (type.has_value() && type.value().GetType() == TokenType::UNSIGNED_INTEGER) {
			vm::Instruction instr;
			instr.op = vm::OpCode::istore;
			if (_level == 0) {
				_instructions.emplace_back(instr);
			}
			else now_func.instructions.emplace_back(instr);
		}
		return {};
	}

	/*<return-statement> :: = 'return'[<expression>] ';'*/
	//the type is needed
	std::optional<CompilationError> Analyser::analyseReturnStatement(std::vector<vm::Instruction>& instrucs) {
		auto next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::RETURN) {
			std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		}
		isReturned = true;
		next = nextToken();
		if (next.value().GetType() == TokenType::SEMICOLON) {
			vm::Instruction instr;
			instr.op = vm::OpCode::ret;
			if (_level == 0) {
				_instructions.emplace_back(instr);
			}
			else now_func.instructions.emplace_back(instr);
			return {};
		}
		else
			unreadToken();
		auto err = analyseExpression(instrucs);
		if (err.second.has_value())
			return err.second;
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::SEMICOLON) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
		}
		if (err.first.has_value() && err.first.value().GetType() == TokenType::UNSIGNED_INTEGER) {
			vm::Instruction instr;
			instr.op = vm::OpCode::iret;
			if (_level == 0) {
				_instructions.emplace_back(instr);
			}
			else now_func.instructions.emplace_back(instr);
		}		
		else { 
			vm::Instruction instr;
			instr.op = vm::OpCode::ret;
			if (_level == 0) {
				_instructions.emplace_back(instr);
			}
			else now_func.instructions.emplace_back(instr);
		}
		return {};
	}

	/*<function - definition> :: =
		<type - specifier><identifier><parameter - clause><compound - statement>*/
	std::optional<CompilationError> Analyser::analyseFunc() {
		//'void'|'int'|'char'|'double'		
		auto next = nextToken();
		if (!next.has_value()) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSpecifier);
		}
		TokenType type = next.value().GetType();
		if (!isTypeSpecifier(type)) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSpecifier);
		}
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
		}
		//	函数和const不能同名
		if (isConstant(next.value().GetValueString())) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrDuplicateDeclaration);
		}
		auto id = next.value();
		addFunc(id,type);
		vm::Function funcs;
		//
		int ide = -1;
		std::string name = "\"" + id.GetValueString() + "\"";
		int index = 0;
		for (std::vector<vm::Constant>::iterator it = constants.begin(); it != constants.end();
			it++) {
			vm::Constant ctmp = *it;
			std::string stmp = std::get<std::string>(ctmp.value);
			if (stmp == name) {
				ide = index;
				break;
			}
			index++;
		}
		//
		funcs.nameIndex = ide;
		funcs.level = _level + 1;
		uint16_t parasize = 0;
		auto err = analysePara(parasize);
		if (err.has_value()) {
			return err;
		}
		funcs.paramSize = parasize;
		now_func = funcs;
		std::vector<vm::Instruction> instrucs;
		err = analyseCompoundStatement(instrucs);
		if (err.has_value()) {
			return err;
		}
		functions.emplace_back(now_func);
		return {};
	}
	/*<parameter - clause> :: =
		'('[<parameter - declaration - list>] ')'
		< parameter - declaration - list > :: =
		<parameter - declaration>{ ',' < parameter - declaration > }
		<parameter - declaration> :: =
		[<const - qualifier>]<type - specifier><identifier>*/
	std::optional<CompilationError> Analyser::analysePara(uint16_t& parasize) {
		auto next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		}
		next = nextToken();
		if (!next.has_value()) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		}
		//要么没有要么就是参数声明，不能是其他的,也就是直接到了右括号的位置
		if (next.value().GetType() == TokenType::RIGHT_BRACKET) {
			return {};
		}
		unreadToken();
		auto err = analyseParaDeclaration();
		if (err.has_value()) {
			return err;
		}
		parasize++;
		//LIST
		while (true) {
			next = nextToken();
			if (!next.has_value()) {				
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncomleteBracket);
			}
			if (next.value().GetType() != TokenType::COMMA) {
				unreadToken();
				break;
			}
			err = analyseParaDeclaration();
			if (err.has_value()) {
				return err;
			}
			parasize++;
		}
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncomleteBracket);
		}
		return {};
	}
	//<parameter-declaration>
	/*<parameter - declaration> :: =
		[<const - qualifier>]<type - specifier><identifier>*/
	std::optional<CompilationError> Analyser::analyseParaDeclaration() {
		auto next = nextToken();
		bool constFlag = false;
		if (!next.has_value() || next.value().GetType() != TokenType::CONST) {
			unreadToken();
		}
		else {
			constFlag = true;
		}
		next = nextToken();
		if (!next.has_value()) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSpecifier);
		}
		TokenType type = next.value().GetType();
		if (!isTypeSpecifier(type)) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSpecifier);
		}
		if (type == TokenType::VOID) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrVoidPara);
		}
		next = nextToken();
		auto name = next.value().GetValueString();
		auto id = next.value();
		if (!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
		}
		std::vector<struct Variable> tmp;
		if (synT.size() ==(size_t)_level + 1) {
			tmp = synT[_level + 1];
		}
		struct Variable vtmp;
		vtmp.name = name;
		vtmp._level = _level+1;
		
		//作用域
		if (isDeclared(name)) {//notice type specifier
			if (constFlag) {
				//error
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrDuplicateDeclaration);
			}
			else {
				//search the specifier
				switch (type)	//	area
				{
				case miniplc0::VOID:
					if (isVoidVariable(name)) {
						return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrDuplicateDeclaration);
					}
					vtmp.type = TokenType::VOID;
					tmp.emplace_back(vtmp);
					synT.erase(_level + 1);
					synT.emplace(_level + 1, tmp);
					break;
				case miniplc0::INT:
					if (isIntVariable(name)) {
						return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrDuplicateDeclaration);
					}
					vtmp.type = TokenType::INT;
					tmp.emplace_back(vtmp);
					synT.erase(_level + 1);
					synT.emplace(_level + 1, tmp);
					break;
				case miniplc0::CHAR:
					break;
				case miniplc0::DOUBLE:
					vtmp.type = TokenType::DOUBLE;
					tmp.emplace_back(vtmp);
					synT.erase(_level + 1);
					synT.emplace(_level + 1, tmp);
					break;
				default://一般不会到这里
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSpecifier);
				}
			}
		}
		else if (constFlag) {
			addConstant(next.value());
			switch (type)	//	area
			{
			case miniplc0::VOID:
				_add(next.value(), _voids);
				vtmp.type = TokenType::VOID;
				tmp.emplace_back(vtmp);
				synT.erase(_level + 1);
				synT.emplace(_level + 1, tmp);
				break;
			case miniplc0::INT:
				_add(next.value(), _ints);
				vtmp.type = TokenType::INT;
				tmp.emplace_back(vtmp);
				synT.erase(_level + 1);
				synT.emplace(_level + 1, tmp);
				break;
			case miniplc0::CHAR:
				break;
			case miniplc0::DOUBLE:
				vtmp.type = TokenType::DOUBLE;
				tmp.emplace_back(vtmp);
				synT.erase(_level + 1);
				synT.emplace(_level + 1, tmp);
				break;
			default://一般不会到这里
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSpecifier);
			}
		}
		//不是const也不是decalared
		switch (type)	//	area
		{
		case miniplc0::VOID:
			_add(next.value(), _voids);
			vtmp.type = TokenType::VOID;
			tmp.emplace_back(vtmp);
			synT.erase(_level + 1);
			synT.emplace(_level + 1, tmp);
			break;
		case miniplc0::INT:
			_add(next.value(), _ints);
			vtmp.type = TokenType::INT;
			tmp.emplace_back(vtmp);
			synT.erase(_level + 1);
			synT.emplace(_level + 1, tmp);
			break;
		case miniplc0::CHAR:
			break;
		case miniplc0::DOUBLE:
			vtmp.type = TokenType::DOUBLE;
			tmp.emplace_back(vtmp);
			synT.erase(_level + 1);
			synT.emplace(_level + 1, tmp);
			break;
		default://一般不会到这里
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSpecifier);
		}
		return {};
	}
	//<compound-statement>::='{' {<variable-declaration>} <statement-seq> '}'
	std::optional<CompilationError> Analyser::analyseCompoundStatement(std::vector<vm::Instruction>& instrucs) {
		auto next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::LEFT_MID_BRACKET) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		}
		addTable();
		int varNum = 0;
		while (true) {
			next = nextToken();
			if (!next.has_value()) {
				return {};
			}
			if (next.value().GetType() == TokenType::CONST) {
				//const must be var
				std::vector<vm::Instruction> instrucs;
				int varNum = 0;
				vm::Instruction instr;
				instr.op = vm::OpCode::snew;
				instr.x = 0;
				now_func.instructions.emplace_back(instr);
				int rec = now_func.instructions.size() - 1;
				auto err = analyseVariableDeclaration(instrucs, varNum);
				now_func.instructions.at(rec).x = varNum;
				if (err.has_value())
					return err;				
			}
			else {
				if (!isTypeSpecifier(next.value().GetType())) {
					unreadToken();
					break;
				}
				next = nextToken();
				if (!next.has_value()) {
					unreadToken();
					unreadToken();
					break;
				}
				if (next.value().GetType() != TokenType::IDENTIFIER) {
					unreadToken();
					unreadToken();
					break;
				}
				next = nextToken();
				if (!next.has_value()) {
					unreadToken();
					unreadToken();
					unreadToken();
					break;
				}
				if (next.value().GetType() == TokenType::LEFT_BRACKET) {
					unreadToken();
					unreadToken();
					unreadToken();
					break;
				}
				unreadToken();
				unreadToken();
				unreadToken();
				int varNum = 0;
				vm::Instruction instr;
				instr.op = vm::OpCode::snew;
				instr.x = 0;
				now_func.instructions.emplace_back(instr);
				int rec = now_func.instructions.size() - 1;
				auto err = analyseVariableDeclaration(instrucs, varNum);
				now_func.instructions.at(rec).x = varNum;
				if (err.has_value())
					return err;
			}
		}
		auto err = analyseStatementSequence(instrucs);
		if (err.has_value()) {
			return err;
		}
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::RIGHT_MID_BRACKET) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncomleteBracket);
		}
		delTable();
		return {};
	}


	/*<scan - statement> :: =
		'scan' '(' < identifier > ')' ';'
		< print - statement > :: =
		'print' '('[<printable - list>] ')' ';'
		< printable - list > :: =
		<printable>{ ',' < printable > }
		<printable> :: =
		<expression> | <string - literal>*/
	std::optional<CompilationError> Analyser::analysePrintList(std::vector<vm::Instruction>& instrucs) {
		auto err = analysePrint(instrucs);
		if (err.has_value())
			return err;
		while (true) {
			auto next = nextToken();
			if (!next.has_value()) {
				return {};
			}
			if (next.value().GetType() != TokenType::COMMA) {
				unreadToken();
				return {};
			}
			//一个print有多个<printable>时，<printable>之间输出一个空格(bipush 32 + cprint)
			vm::Instruction instr;
			instr.op = vm::OpCode::bipush;
			instr.x = 32;
			if (_level == 0) {
				_instructions.emplace_back(instr);
			}
			else now_func.instructions.emplace_back(instr);
			err = analysePrint(instrucs);
			if (err.has_value())
				return err;
		}
	}
	std::optional<CompilationError> Analyser::analysePrint(std::vector<vm::Instruction>& instrucs) {
		auto err = analyseExpression(instrucs);
		if (err.second.has_value())
			return err.second;
		auto type = err.first;
		vm::Instruction instr;
		if (type.has_value() && type.value().GetType() == TokenType::S_CHAR) {
			instr.op = vm::OpCode::sprint;
			if (_level == 0) {
				_instructions.emplace_back(instr);
			}
			else now_func.instructions.emplace_back(instr);
		}
		else if (type.has_value() && type.value().GetType() == TokenType::C_CHAR) {
			instr.op = vm::OpCode::cprint;
			if (_level == 0) {
				_instructions.emplace_back(instr);
			}
			else now_func.instructions.emplace_back(instr);
		}
		else {
			instr.op = vm::OpCode::iprint;
			if (_level == 0) {
				_instructions.emplace_back(instr);
			}
			else now_func.instructions.emplace_back(instr);
		}			
		return {};
	}

	//< print - statement > :: =
	//	'print' '('[<printable - list>] ')' ';'
	std::optional<CompilationError> Analyser::analysePrintStatement(std::vector<vm::Instruction>& instrucs) {
		auto next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::PRINT) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		}
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		}
		next = nextToken();
		if (!next.has_value()) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		}
		TokenType type = next.value().GetType();
		if (type == TokenType::RIGHT_BRACKET) {

		}
		else {
			unreadToken();
			auto err = analysePrintList(instrucs);
			if (err.has_value()) {
				return err;
			}
			next = nextToken();
			if (!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET) {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
			}
		}
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::SEMICOLON) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
		}
		return {};
	}
	//scan statement
	std::optional<CompilationError> Analyser::analyseScanStatement(std::vector<vm::Instruction>& instrucs) {
		auto next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::SCAN) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		}
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncomleteBracket);
		}
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::IDENTIFIER) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
		}
		//要求必须是可以修改的
		if (isConstant(next.value().GetValueString())) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrAssignToConstant);
		}
		auto name = next.value().GetValueString();
		//if i should judge it
		uint16_t level_diff = -1;
		uint16_t offset = -1;
		//deal loada
		for (int ind = _level; ind >= 0; ind--) {
			std::vector<struct Variable> tmp = synT[ind];
			uint16_t index = 0;
			bool flag = false;
			for (std::vector<struct Variable>::iterator it = tmp.begin();
				it != tmp.end(); it++, index++) {
				struct Variable vtmp = *it;
				if (vtmp.name == name) {
					offset = index;
					level_diff = ind == 0 ? 1 : 0;
					flag = true;
					break;
				}
			}
			if (flag)break;
		}
		vm::Instruction instr;
		instr.op = vm::OpCode::loada;
		instr.x = level_diff;
		instr.y = offset;
		if (_level == 0) {
			_instructions.emplace_back(instr);
		}
		else now_func.instructions.emplace_back(instr);
		vm::Instruction instr2;
		instr2.op = vm::OpCode::iscan;
		if (_level == 0) {
			_instructions.emplace_back(instr2);
		}
		else now_func.instructions.emplace_back(instr2);
		vm::Instruction instr3;
		instr3.op = vm::OpCode::istore;
		if (_level == 0) {
			_instructions.emplace_back(instr3);
		}
		else now_func.instructions.emplace_back(instr3);
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncomleteBracket);
		}
		next = nextToken();
		if (!next.has_value() || next.value().GetType() != TokenType::SEMICOLON) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
		}
		return {};
	}

	/*<loop - statement> :: =
			'while' '(' < condition > ')' < statement >
			| 'do' < statement > 'while' '(' < condition > ')' ';'
			| 'for' '(' < for - init - statement > [<condition>]';'[<for - update - expression>]')' < statement >*/
	std::optional<CompilationError> Analyser::analyseLoopStatement(std::vector<vm::Instruction>& instrucs) {
		auto next = nextToken();
		if (!next.has_value()) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		}
		TokenType type = next.value().GetType();
		//while(condition)statement
		if (type == TokenType::WHILE) {
			next = nextToken();
			if (!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET) {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncomleteBracket);
			}
			int reco = now_func.instructions.size();
			//analysecondition
			auto err = analyseExpression(instrucs);
			if (err.second.has_value())
				return err.second;
			auto tk = err.first.value().GetType();
			if (tk == TokenType::VOID) {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrCalWithVoid);
			}
			auto next = nextToken();
			if (!next.has_value());
			TokenType type = next.value().GetType();
			if (!isRelationalOP(type)) {
				unreadToken();
			}
			err = analyseExpression(instrucs);
			if (err.second.has_value())
				return err.second;
			tk = err.first.value().GetType();
			if (tk == TokenType::VOID) {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrCalWithVoid);
			}
			//jmp to where
			vm::Instruction instr;
			instr.op = vm::OpCode::icmp;
			now_func.instructions.emplace_back(instr);
			recPos = now_func.instructions.size()-1;
			switch (type)
			{
			case miniplc0::LESS_SIGN:
				instr.op = vm::OpCode::jge;
				instr.x = 0;
				now_func.instructions.emplace_back(instr);
				recPos = now_func.instructions.size()-1;
				break;
			case miniplc0::LESS_EQAUL_SIGN:
				instr.op = vm::OpCode::jg;
				instr.x = 0;
				now_func.instructions.emplace_back(instr);
				recPos = now_func.instructions.size()-1;
				break;
			case miniplc0::MORE_SIGN:
				instr.op = vm::OpCode::jle;
				instr.x = 0;
				now_func.instructions.emplace_back(instr);
				recPos = now_func.instructions.size()-1;
				break;
			case miniplc0::MORE_EQUAL_SIGN:
				instr.op = vm::OpCode::jl;
				instr.x = 0;
				now_func.instructions.emplace_back(instr);
				recPos = now_func.instructions.size()-1;
				break;
			case miniplc0::NOT_EQUAL_SIGN:
				instr.op = vm::OpCode::je;
				instr.x = 0;
				now_func.instructions.emplace_back(instr);
				recPos = now_func.instructions.size()-1;
				break;
			case miniplc0::TOTAL_EQUAL_SIGN:
				instr.op = vm::OpCode::jne;
				instr.x = 0;
				now_func.instructions.emplace_back(instr);
				recPos = now_func.instructions.size()-1;
				break;
			default:
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrUnexpected);
			}
			next = nextToken();
			if (!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET) {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncomleteBracket);
			}
			int rec = recPos;
			auto err2 = analyseStatement(instrucs);
			if (err2.has_value()) {
				return err2;
			}
			vm::Instruction instr2;
			instr2.op = vm::OpCode::jmp;
			instr2.x = reco;
			now_func.instructions.emplace_back(instr2);
			//while can't be in the global text		if it is assigned??
			now_func.instructions.at(rec).x = now_func.instructions.size();
			return {};
		}
		//do statement while(condition);
		else if (type == TokenType::DO) {
			auto err = analyseStatement(instrucs);
			if (err.has_value())
				return err;
			next = nextToken();
			if (!next.has_value() || next.value().GetType() != TokenType::WHILE) {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
			}
			next = nextToken();
			if (!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET) {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncomleteBracket);
			}
			err = analyseCondition(instrucs);
			if (err.has_value())
				return err;
			next = nextToken();
			if (!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET) {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncomleteBracket);
			}
			return {};
		}
		//for
		else if (type == TokenType::FOR) {
			next = nextToken();
			if (!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET) {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncomleteBracket);
			}
			auto err = analyseForInitStatement(instrucs);
			if (err.has_value())
				return err;
			next = nextToken();
			if (!next.has_value()) {

			}
			else if (next.value().GetType() != TokenType::IF &&
				next.value().GetType() != TokenType::SWITCH) {
				unreadToken();
			}
			else {
				err = analyseCondition(instrucs);
				if (err.has_value())
					return err;
			}
			next = nextToken();
			if (!next.has_value() || next.value().GetType() != TokenType::SEMICOLON) {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
			}
			next = nextToken();
			if (!next.has_value()) {
				return {};
			}
			unreadToken();
			if (next.value().GetType() == TokenType::IDENTIFIER) {
				err = analyseForupdateStatement(instrucs);
				if (err.has_value())
					return err;
			}
			return {};
		}
		return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
	}
	///*<for - init - statement> :: =
	//	[<assignment - expression>{',' < assignment - expression > }]';'*/
	std::optional<CompilationError> Analyser::analyseForInitStatement(std::vector<vm::Instruction>& instrucs) {
		auto next = nextToken();
		if (!next.has_value()) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		}
		if (next.value().GetType() == TokenType::SEMICOLON) {
			return {};
		}
		else
			unreadToken();
		auto err = analyseAssignmentStatement(instrucs);
		if (err.has_value())
			return err;
		while (true) {
			next = nextToken();
			if (!next.has_value()) {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
			}
			if (next.value().GetType() != TokenType::COMMA) {
				unreadToken();
				break;
			}
			err = analyseAssignmentStatement(instrucs);
			if (err.has_value())
				return err;
		}
		next = nextToken();
		if (!next.has_value()) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
		}
		else if (next.value().GetType() != TokenType::SEMICOLON) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
		}
		return {};
	}
	/*<for - update - expression> :: =
		(<assignment - expression> | <function - call>){ ','(<assignment - expression> | <function - call>) }*/
		//()表示二选一
	std::optional<CompilationError> Analyser::analyseForupdateStatement(std::vector<vm::Instruction>& instrucs) {
		auto next = nextToken();
		if (!next.has_value()) {
			return {};
		}
		if (next.value().GetType() != TokenType::IDENTIFIER) {
			unreadToken();
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNeedIdentifier);
		}
		next = nextToken();
		unreadToken();
		unreadToken();
		if (!next.has_value()) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		}
		if (next.value().GetType() == TokenType::EQUAL_SIGN) {
			auto err = analyseAssignmentStatement(instrucs);
			if (err.has_value())
				return err;
		}
		else if (next.value().GetType() == TokenType::LEFT_BRACKET) {
			auto err = analyseFuncCall(instrucs);
			if (err.has_value())
				return err;
		}
		else {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		}
		while (true) {
			next = nextToken();
			if (!next.has_value()) {
				return {};
			}
			if (next.value().GetType() != TokenType::COMMA) {
				unreadToken();
				return {};
			}
			next = nextToken();
			if (!next.has_value()) {
				return {};
			}
			if (next.value().GetType() != TokenType::IDENTIFIER) {
				unreadToken();
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
			}
			next = nextToken();
			unreadToken();
			unreadToken();
			if (!next.has_value()) {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
			}
			if (next.value().GetType() == TokenType::EQUAL_SIGN) {
				auto err = analyseAssignmentStatement(instrucs);
				if (err.has_value())
					return err;
			}
			else if (next.value().GetType() == TokenType::LEFT_BRACKET) {
				auto err = analyseFuncCall(instrucs);
				if (err.has_value())
					return err;
			}
			else {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
			}
		}
		return {};
	}
	/*<jump - statement> :: =
		'break' ';'
		| 'continue' ';'
		| <return-statement>
		<return-statement> :: = 'return'[<expression>] ';'*/
	std::optional<CompilationError> Analyser::analyseJmpStatement(std::vector<vm::Instruction>& instrucs) {
		auto next = nextToken();
		if (!next.has_value()) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		}
		TokenType type = next.value().GetType();
		if (type == TokenType::BREAK) {
			next = nextToken();
			if (!next.has_value() || next.value().GetType() != TokenType::SEMICOLON) {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
			}
			return {};
		}
		else if (type == TokenType::CONTINUE) {
			next = nextToken();
			if (!next.has_value() || next.value().GetType() != TokenType::SEMICOLON) {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoSemicolon);
			}
			return {};
		}
		else {//直接else避免回溯
			unreadToken();
			auto err = analyseReturnStatement(instrucs);
			if (err.has_value()) {
				return err;
			}
			return {};
		}
	}

	/*<condition> :: =
			<expression>[<relational - operator><expression>]

			<condition - statement> :: =
			'if' '(' < condition > ')' < statement > ['else' < statement > ]
			| 'switch' '(' < condition > ')' '{' {<labeled - statement>} '}'*/
	std::optional<CompilationError> Analyser::analyseCondition(std::vector<vm::Instruction>& instrucs) {
		//true or false; not void
		//expr必须有值
		auto err = analyseExpression(instrucs);
		if (err.second.has_value())
			return err.second;
		auto tk = err.first.value().GetType();
		if (tk == TokenType::VOID) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrCalWithVoid);
		}
		auto next = nextToken();
		if (!next.has_value())
			return {};
		TokenType type = next.value().GetType();
		if (!isRelationalOP(type)) {
			unreadToken();
			return {};
		}
		err = analyseExpression(instrucs);
		if (err.second.has_value())
			return err.second;
		tk = err.first.value().GetType();
		if (tk == TokenType::VOID) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrCalWithVoid);
		}
		//jmp to where
		vm::Instruction instr;
		instr.op = vm::OpCode::icmp;
		now_func.instructions.emplace_back(instr);
		recPos = now_func.instructions.size()-1;
		switch (type)
		{
		case miniplc0::LESS_SIGN:
			instr.op = vm::OpCode::jge;	
			instr.x = 0;
			now_func.instructions.emplace_back(instr);
			recPos = now_func.instructions.size()-1;
			break;
		case miniplc0::LESS_EQAUL_SIGN:
			instr.op = vm::OpCode::jg;
			instr.x = 0;
			now_func.instructions.emplace_back(instr);
			recPos = now_func.instructions.size()-1;
			break;
		case miniplc0::MORE_SIGN:
			instr.op = vm::OpCode::jle;
			instr.x = 0;
			now_func.instructions.emplace_back(instr);
			recPos = now_func.instructions.size()-1;
			break;
		case miniplc0::MORE_EQUAL_SIGN:
			instr.op = vm::OpCode::jl;
			instr.x = 0;
			now_func.instructions.emplace_back(instr);
			recPos = now_func.instructions.size()-1;
			break;
		case miniplc0::NOT_EQUAL_SIGN:
			instr.op = vm::OpCode::je;
			instr.x = 0;
			now_func.instructions.emplace_back(instr);
			recPos = now_func.instructions.size()-1;
			break;
		case miniplc0::TOTAL_EQUAL_SIGN:
			instr.op = vm::OpCode::jne;
			instr.x = 0;
			now_func.instructions.emplace_back(instr);
			recPos = now_func.instructions.size()-1;
			break;
		default:
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrUnexpected);
		}
		return {};
	}
	/*
		<condition - statement> :: =
		'if' '(' < condition > ')' < statement > ['else' < statement > ]
		| 'switch' '(' < condition > ')' '{' {<labeled - statement>} '}'*/
	std::optional<CompilationError> Analyser::analyseConditionStatement(std::vector<vm::Instruction>& instrucs) {
		auto next = nextToken();
		if (!next.has_value()) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
		}
		if (next.value().GetType() == TokenType::IF) {
			next = nextToken();
			if (!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET) {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncomleteBracket);
			}
			//analyseCondition
			auto err = analyseExpression(instrucs);
			if (err.second.has_value())
				return err.second;
			auto tk = err.first.value().GetType();
			if (tk == TokenType::VOID) {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrCalWithVoid);
			}
			auto next = nextToken();
			if (!next.has_value());
			TokenType type = next.value().GetType();
			if (!isRelationalOP(type)) {
				unreadToken();
			}
			err = analyseExpression(instrucs);
			if (err.second.has_value())
				return err.second;
			tk = err.first.value().GetType();
			if (tk == TokenType::VOID) {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrCalWithVoid);
			}
			//jmp to where
			vm::Instruction instr;
			instr.op = vm::OpCode::icmp;
			now_func.instructions.emplace_back(instr);
			recPos = now_func.instructions.size();
			vm::Instruction instr2;
			switch (type)
			{
			case miniplc0::LESS_SIGN:
				instr2.op = vm::OpCode::jge;
				instr2.x = 0;
				now_func.instructions.emplace_back(instr2);
				recPos = now_func.instructions.size()-1;
				break;
			case miniplc0::LESS_EQAUL_SIGN:
				instr2.op = vm::OpCode::jg;
				instr2.x = 0;
				now_func.instructions.emplace_back(instr2);
				recPos = now_func.instructions.size()-1;
				break;
			case miniplc0::MORE_SIGN:
				instr2.op = vm::OpCode::jle;
				instr2.x = 0;
				now_func.instructions.emplace_back(instr2);
				recPos = now_func.instructions.size()-1;
				break;
			case miniplc0::MORE_EQUAL_SIGN:
				instr2.op = vm::OpCode::jl;
				instr2.x = 0;
				now_func.instructions.emplace_back(instr2);
				recPos = now_func.instructions.size()-1;
				break;
			case miniplc0::NOT_EQUAL_SIGN:
				instr2.op = vm::OpCode::je;
				instr2.x = 0;
				now_func.instructions.emplace_back(instr2);
				recPos = now_func.instructions.size()-1;
				break;
			case miniplc0::TOTAL_EQUAL_SIGN:
				instr2.op = vm::OpCode::jne;
				instr2.x = 0;
				now_func.instructions.emplace_back(instr2);
				recPos = now_func.instructions.size()-1;
				break;
			default:
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrUnexpected);
			}
			//analyseCondition	end
			next = nextToken();
			if (!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET) {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncomleteBracket);
			}
			auto err2 = analyseStatement(instrucs);
			if (err2.has_value()) {
				return err2;
			}
			//here to get the jmp pos	the precise pos
			int rec = recPos;
			next = nextToken();
			now_func.instructions.at(rec).x = now_func.instructions.size();
			if (!next.has_value()) {
				return {};
			}
			else if (next.value().GetType() != TokenType::ELSE) {
				unreadToken();
				return {};
			}
			//it is else statement
			err2 = analyseStatement(instrucs);
			if (err2.has_value()) {
				return err2;
			}
			return {};
		}
		else if (next.value().GetType() == TokenType::SWITCH) {
			next = nextToken();
			if (!next.has_value() || next.value().GetType() != TokenType::LEFT_BRACKET) {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncomleteBracket);
			}
			auto err = analyseCondition(instrucs);
			if (err.has_value()) {
				return err;
			}
			next = nextToken();
			if (!next.has_value() || next.value().GetType() != TokenType::RIGHT_BRACKET) {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncomleteBracket);
			}
			next = nextToken();
			if (!next.has_value() || next.value().GetType() != TokenType::LEFT_MID_BRACKET) {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncomleteBracket);
			}
			addTable();
			next = nextToken();
			if (!next.has_value()) {

			}
			else if (next.value().GetType() != TokenType::CASE && next.value().GetType() != TokenType::DEFAULT) {
				unreadToken();
			}
			else {
				while (true) {
					next = nextToken();
					if (!next.has_value()) {
						break;
					}
					if (next.value().GetType() != TokenType::CASE && next.value().GetType() != TokenType::DEFAULT) {
						unreadToken();
						break;
					}
					auto err = analyseLabeledStatement(instrucs);
					if (err.has_value())
						return err;
				}
			}
			next = nextToken();
			if (!next.has_value() || next.value().GetType() != TokenType::RIGHT_MID_BRACKET) {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncomleteBracket);
			}
			delTable();
			return {};
		}
		//else 
		return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrIncompleteExpression);
	}

	/*<labeled - statement> :: =
		'case' (<integer - literal> | <char - literal>) ':' < statement >
		| 'default' ':' < statement >*/
	std::optional<CompilationError> Analyser::analyseLabeledStatement(std::vector<vm::Instruction>& instrucs) {
		auto next = nextToken();
		if (!next.has_value()) {
			return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidLabeledStatement);
		}
		if (next.value().GetType() == TokenType::CASE) {
			next = nextToken();
			if (!next.has_value()) {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidLabeledStatement);
			}
			else if (next.value().GetType() == TokenType::UNSIGNED_INTEGER) {
				next = nextToken();
				if (!next.has_value() || next.value().GetType() != TokenType::MAO_SIGN) {
					return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidLabeledStatement);
				}
			}
			else {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidLabeledStatement);
			}
			auto err = analyseStatement(instrucs);
			if (err.has_value())
				return err;
			return {};
		}
		else if (next.value().GetType() == TokenType::DEFAULT) {
			next = nextToken();
			if (!next.has_value() || next.value().GetType() != TokenType::MAO_SIGN) {
				return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrNoColon);
			}
			auto err = analyseStatement(instrucs);
			if (err.has_value())
				return err;
			return {};
		}
		return std::make_optional<CompilationError>(_current_pos, ErrorCode::ErrInvalidLabeledStatement);
	}

	std::optional<Token> Analyser::nextToken() {
		if (_offset == _tokens.size())
			return {};
		// 考虑到 _tokens[0..._offset-1] 已经被分析过了
		// 所以我们选择 _tokens[0..._offset-1] 的 EndPos 作为当前位置
		_current_pos = _tokens[_offset].GetEndPos();
		return _tokens[_offset++];
	}

	void Analyser::unreadToken() {
		if (_offset == 0)
			DieAndPrint("analyser unreads token from the begining.");
		_current_pos = _tokens[_offset - 1].GetEndPos();
		_offset--;
	}

	void Analyser::_add(const Token& tk, std::map<std::string, int32_t>& mp) {
		if (tk.GetType() != TokenType::IDENTIFIER)
			DieAndPrint("only identifier can be added to the table.");
		mp[tk.GetValueString()] = _level;
		//_nextTokenIndex++;
	}

	void Analyser::addVariable(const Token& tk) {
		_add(tk, _vars);
	}

	void Analyser::addConstant(const Token& tk) {
		_add(tk, _consts);
	}

	void Analyser::addInitializedVariable(const Token& tk) {
		//erase uninitialized
		_uninitialized_vars.erase(tk.GetValueString());
		//_vars[tk.GetValueString()] = getIndex(tk.GetValueString());
	}

	void Analyser::addUninitializedVariable(const Token& tk) {
		_add(tk, _uninitialized_vars);
	}

	void Analyser::addIntVariable(const Token& tk) {
		addUninitializedVariable(tk);
		_add(tk, _ints);
	}
	void Analyser::addVoidVariable(const Token& tk) {
		addUninitializedVariable(tk);
		_add(tk, _voids);
	}

	void Analyser::addFunc(const Token& tk,const TokenType& type) {
		//addUninitializedVariable(tk);
		_add(tk, _consts);
		_add(tk, _funcs);
		funcs.insert(std::make_pair(tk.GetValueString(), type));
		recFunc.emplace_back(tk.GetValueString());
		//where is the nameindex
		vm::Constant constant;
		constant.type = vm::Constant::Type::STRING;
		std::string name = tk.GetValueString();
		name = "\"" + name + "\"";
		constant.value = name;
		constants.emplace_back(constant);
	}

	int32_t Analyser::getIndex(const std::string& s) {//补全
		if (_uninitialized_vars.find(s) != _uninitialized_vars.end())
			return _uninitialized_vars[s];
		else if (_vars.find(s) != _vars.end())
			return _vars[s];
		else
			return _consts[s];
	}

	bool Analyser::isDeclared(const std::string& s) {
		return isConstant(s) || isUninitializedVariable(s) || isIntVariable(s) || isVoidVariable(s);
	}

	bool Analyser::isUninitializedVariable(const std::string& s) {
		return _uninitialized_vars.find(s) != _uninitialized_vars.end();
	}
	bool Analyser::isInitializedVariable(const std::string& s) {
		return _uninitialized_vars.find(s) == _uninitialized_vars.end();
	}

	bool Analyser::isConstant(const std::string& s) {
		return _consts.find(s) != _consts.end();
	}

	bool Analyser::isIntVariable(const std::string& s) {
		//示例
		/*for (std::vector<Table>::iterator i = synT.begin(); i != synT.end(); i++) {
			Table t = *i;
			if (t.it.names.find(s) != t.it.names.end())return true;
		}
		return false;*/
		return _ints.find(s) != _ints.end();
	}

	bool Analyser::isVoidVariable(const std::string& s) {
		return _voids.find(s) != _voids.end();
	}

	bool Analyser::isFunc(const std::string& s) {
		return _funcs.find(s) != _funcs.end();
	}

	bool Analyser::isRelationalOP(const TokenType& type) {
		if (type == LESS_SIGN ||
			type == LESS_EQAUL_SIGN ||
			type == MORE_SIGN ||
			type == MORE_EQUAL_SIGN ||
			type == NOT_EQUAL_SIGN ||
			type == TOTAL_EQUAL_SIGN)return true;
		return false;
	}

	bool Analyser::isTypeSpecifier(const TokenType& type) {
		if (type == INT || type == VOID || type == DOUBLE || type == CHAR) {
			return true;
		}
		return false;
	}
	void Analyser::addTable()
	{
		_level++;
	}
	void Analyser::delTable()
	{
		synT.erase(_level);
		_level--;
	}

	uint16_t Analyser::findFunc(const std::string& s) {
		for (uint16_t index = 0; index < recFunc.size();index++) {
			if (recFunc[index] == s)return index;
			index++;
		}
		return -1;
	}

	void Analyser::printFile() {		
		int i;

		i = 0;
		output.emplace_back(".constants:");
		for (auto& constant : constants) {
			std::stringstream ss;
			//println(out, i++, constant);
			ss << i++;
			ss << " ";
			ss << "S ";
			std::string s;
			for (auto ch : std::get<vm::str_t>(constant.value)) {
				/*switch (ch)
				{
				case '\\': s += "\\x5C"; break;
				case '\'': s += "\\x27"; break;
				case '\"': s += "\\x22"; break;
				case '\n': s += "\\x0A"; break;
				case '\r': s += "\\x0D"; break;
				case '\t': s += "\\x09"; break;
				default:   s += ch;   break;
				}*/
				s += ch;
			}
			ss << s;
			output.emplace_back(ss.str());
		}

		i = 0;
		output.emplace_back(".start:");
		for (auto& ins : _instructions) {
			//println(out, i++, ins);
			std::stringstream ss;
			ss << i;
			i++;
			ss << "	";
			const char* name = vm::nameOfOpCode.at(ins.op);
			if (auto it = vm::paramSizeOfOpCode.find(ins.op); it != vm::paramSizeOfOpCode.end()) {
				switch (it->second.size()) {
				case 0: {
					//print(out, name); 
					ss << name;
					break;
				}
				case 1: {
					//print(out, name, ins.x);
					ss << name;
					ss << " ";
					ss << ins.x;
					break;
				}
				case 2: {
					//printfmt(out, "{} {},{}", name, ins.x, ins.y); 
					ss << name;
					ss << " ";
					ss << ins.x;
					ss << ", ";
					ss << ins.y;
					break;
				}
				default: {
					//print(out, "????"); break;
					ss << "????";
					break;
				}
				}
				output.emplace_back(ss.str());
			}
			else {
				//print(out, name);
				ss << name;
				output.emplace_back(ss.str());
			}
		}

		std::vector<std::string> names;
		i = 0;
		//println(out, ".functions:");
		output.emplace_back(".functions:");
		for (auto& fun : functions) {
			std::stringstream ss;
			std::string name = std::get<std::string>(constants.at(fun.nameIndex).value);
			names.push_back(name);
			//println(out, i++, fun.nameIndex, fun.paramSize, fun.level, "#", name);
			int sizes = name.size() - 2;
			ss << i++ << " " << fun.nameIndex << " " << fun.paramSize << " " << fun.level;
			output.emplace_back(ss.str());
		}

		i = 0;
		for (auto& fun : functions) {
			std::stringstream ss;
			//printfmt(out, ".F{}: # {}", i, names.at(i)); println(out);
			int sizes = (int)names.at(i).size() - 2;
			ss << ".F" << i;
			output.emplace_back(ss.str());
			int j = 0;
			for (auto& ins : fun.instructions) {
				std::stringstream ss2;
				//println(out, j++, ins);
				ss2 << j++ << "    ";
				const char* name = vm::nameOfOpCode.at(ins.op);
				if (auto it = vm::paramSizeOfOpCode.find(ins.op); it != vm::paramSizeOfOpCode.end()) {
					switch (it->second.size()) {
					case 0: {
						//print(out, name); 
						ss2 << name;
						break;
					}
					case 1: {
						//print(out, name, ins.x);
						ss2 << name;
						ss2 << " ";
						ss2 << ins.x;
						break;
					}
					case 2: {
						//printfmt(out, "{} {},{}", name, ins.x, ins.y); 
						ss2 << name;
						ss2 << " ";
						ss2 << ins.x;
						ss2 << ", ";
						ss2 << ins.y;
						break;
					}
					default: {
						//print(out, "????"); break;
						ss2 << "????";
						break;
					}
					}
					output.emplace_back(ss2.str());
				}
				else {
					//print(out, name);
					ss2 << name;
					output.emplace_back(ss2.str());
				}
			}
			++i;
		}
		/*for (auto it : output) {
			std::cout << it << "\n";
		}*/
		
	}

	void Analyser::output_binary(Analyser& analyser,std::ostream& out) {
		auto p = analyser.Analyse();
		if (p.second.has_value()) {
			fmt::print(stderr, "Syntactic analysis error: {}\n", p.second.value());
			exit(2);
		}
		char bytes[8];
		const auto writeNBytes = [&](void* addr, int count) {
			assert(0 < count && count <= 8);
			char* p = reinterpret_cast<char*>(addr) + (count - 1);
			for (int i = 0; i < count; ++i) {
				bytes[i] = *p--;
			}
			out.write(bytes, count);
		};

		// magic
		out.write("\x43\x30\x3A\x29", 4);
		// version
		out.write("\x00\x00\x00\x01", 4);
		// constants_count
		vm::u2 constants_count = constants.size();
		writeNBytes(&constants_count, sizeof constants_count);
		// constants
		for (auto& constant : constants) {
			switch (constant.type)
			{
			case vm::Constant::Type::STRING: {
				out.write("\x00", 1);
				std::string v = std::get<vm::str_t>(constant.value);
				vm::u2 len = v.length();
				writeNBytes(&len, sizeof len);
				out.write(v.c_str(), len);
			} break;
			case vm::Constant::Type::INT: {
				out.write("\x01", 1);
				vm::int_t v = std::get<vm::int_t>(constant.value);
				writeNBytes(&v, sizeof v);
			} break;
			case vm::Constant::Type::DOUBLE: {
				out.write("\x02", 1);
				vm::double_t v = std::get<vm::double_t>(constant.value);
				writeNBytes(&v, sizeof v);
			} break;
			default: assert(("unexpected error", false)); break;
			}
		}

		auto to_binary = [&](const std::vector<vm::Instruction>& v) {
			vm::u2 instructions_count = v.size();
			writeNBytes(&instructions_count, sizeof instructions_count);
			for (auto& ins : v) {
				vm::u1 op = static_cast<vm::u1>(ins.op);
				writeNBytes(&op, sizeof op);
				if (auto it = vm::paramSizeOfOpCode.find(ins.op); it != vm::paramSizeOfOpCode.end()) {
					auto paramSizes = it->second;
					switch (paramSizes[0]) {
#define CASE(n) case n: { vm::u##n x = ins.x; writeNBytes(&x, n); }
						CASE(1); break;
						CASE(2); break;
						CASE(4); break;
#undef CASE
					default: assert(("unexpected error", false));
					}
					if (paramSizes.size() == 2) {
						switch (paramSizes[1]) {
#define CASE(n) case n: { vm::u##n y = ins.y; writeNBytes(&y, n); }
							CASE(1); break;
							CASE(2); break;
							CASE(4); break;
#undef CASE
						default: assert(("unexpected error", false));
						}
					}
				}
			}
		};

		// start
		to_binary(_instructions);
		// functions_count
		vm::u2 functions_count = functions.size();
		writeNBytes(&functions_count, sizeof functions_count);
		// functions
		for (auto& fun : functions) {
			vm::u2 v;
			v = fun.nameIndex; writeNBytes(&v, sizeof v);
			v = fun.paramSize; writeNBytes(&v, sizeof v);
			v = fun.level;     writeNBytes(&v, sizeof v);
			to_binary(fun.instructions);
		}
	}
}

