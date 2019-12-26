#pragma once

#include "error/error.h"
#include "tokenizer/token.h"
#include "c0-vm-cpp/src/instruction.h"
#include "c0-vm-cpp/src/function.h"
#include "c0-vm-cpp/src/constant.h"
#include "c0-vm-cpp/src/file.h"

#include <vector>
#include <optional>
#include <utility>
#include <map>
#include <cstdint>
#include <cstddef> // for std::size_t
#include <stack>
#include <set>

namespace miniplc0 {

	struct Variable {
		std::string name;
		uint16_t _level;
		TokenType type;
	};

	class Analyser final {
	private:
		using uint64_t = std::uint64_t;
		using int64_t = std::int64_t;
		using uint32_t = std::uint32_t;
		using int32_t = std::int32_t;
		using uint16_t = std::uint16_t;
	public:
		Analyser(std::vector<Token> v)
			: _tokens(std::move(v)), _offset(0), _instructions({}), _current_pos(0, 0),
			_uninitialized_vars({}), _vars({}), _consts({}), _ints({}), _voids({}), _funcs({}),
			_nextTokenIndex(0), _level(-1) {}
		Analyser(Analyser&&) = delete;
		Analyser(const Analyser&) = delete;
		Analyser& operator=(Analyser) = delete;

		// 唯一接口
		std::pair<std::vector<std::string>, std::optional<CompilationError>> Analyse();
		void output_binary(Analyser&,std::ostream&);
	private:
		// 所有的递归子程序

		// <主过程>
		std::optional<CompilationError> analyseMain();
		
		std::optional<CompilationError> analyseVariableDeclaration(std::vector<vm::Instruction>&,int&);
		std::optional<CompilationError> analyseInitDeclarator(const TokenType&, std::vector<vm::Instruction>&,int&);
		std::optional<CompilationError> analyseInitDeclaratorList(const TokenType&, std::vector<vm::Instruction>&,int&);
		std::optional<CompilationError> analyseInitializer(std::vector<vm::Instruction>&);
		// <语句序列>
		std::optional<CompilationError> analyseStatementSequence(std::vector<vm::Instruction>&);
		std::optional<CompilationError> analyseStatement(std::vector<vm::Instruction>&);
		// <表达式>
		std::pair<std::optional<Token>, std::optional<CompilationError>> analyseExpression(std::vector<vm::Instruction>&);
		std::optional<CompilationError> analyseExpressionList(std::vector<vm::Instruction>&);
		std::optional<CompilationError> analyseFuncCall(std::vector<vm::Instruction>&);
		// <赋值语句>
		std::optional<CompilationError> analyseAssignmentStatement(std::vector<vm::Instruction>&);

		//<function-definition>
		std::optional<CompilationError>analyseFunc();
		//<parameter-clause>+list
		std::optional<CompilationError>analysePara(uint16_t&);
		//<parameter-declaration>+list
		std::optional<CompilationError>analyseParaDeclaration();
		//<compound-statement>
		std::optional<CompilationError> analyseCompoundStatement(std::vector<vm::Instruction>&);
		
		//condition-statement
		std::optional<CompilationError> analyseConditionStatement(std::vector<vm::Instruction>&);
		//loop statement
		std::optional<CompilationError> analyseLoopStatement(std::vector<vm::Instruction>&);
		std::optional<CompilationError> analyseForInitStatement(std::vector<vm::Instruction>&);
		std::optional<CompilationError> analyseForupdateStatement(std::vector<vm::Instruction>&);
		//jmp statement
		std::optional<CompilationError> analyseJmpStatement(std::vector<vm::Instruction>&);
		//print statement
		std::optional<CompilationError> analysePrintStatement(std::vector<vm::Instruction>&);
		std::optional<CompilationError> analysePrintList(std::vector<vm::Instruction>&);
		std::optional<CompilationError> analysePrint(std::vector<vm::Instruction>&);
		//scan statement
		std::optional<CompilationError> analyseScanStatement(std::vector<vm::Instruction>&);
		//assign
		//function call
		//
		std::optional<CompilationError> analyseReturnStatement(std::vector<vm::Instruction>&);
		//condition
		std::optional<CompilationError> analyseCondition(std::vector<vm::Instruction>&);
		//expr
		std::pair<std::optional<Token>, std::optional<CompilationError>> analyseAddiExpr(std::vector<vm::Instruction>&);
		std::pair<std::optional<Token>, std::optional<CompilationError>> analyseMultiExpr(std::vector<vm::Instruction>&);
		std::pair<std::optional<Token>, std::optional<CompilationError>> analyseUnaryExpr(std::vector<vm::Instruction>&);

		std::pair<std::optional<Token>, std::optional<CompilationError>> analyseCastExpr();

		std::optional<CompilationError> analyseLabeledStatement(std::vector<vm::Instruction>&);

		// Token 缓冲区相关操作

		// 返回下一个 token
		std::optional<Token> nextToken();
		// 回退一个 token
		void unreadToken();

		// 下面是符号表相关操作

		// helper function
		void _add(const Token&, std::map<std::string, int32_t>&);
		// 添加变量、常量、未初始化的变量
		void addVariable(const Token&);
		void addConstant(const Token&);
		void addUninitializedVariable(const Token&);

		void addInitializedVariable(const Token&);

		void addIntVariable(const Token&);
		void addVoidVariable(const Token&);
		void addFunc(const Token&,const TokenType&);
		// 是否被声明过
		bool isDeclared(const std::string&);
		// 是否是未初始化的变量
		bool isUninitializedVariable(const std::string&);
		// 是否是已初始化的变量
		bool isInitializedVariable(const std::string&);
		// 是否是常量
		bool isConstant(const std::string&); 
		bool isIntVariable(const std::string&);
		bool isVoidVariable(const std::string&);
		bool isFunc(const std::string&);
		// 获得 {变量，常量} 在栈上的偏移
		int32_t getIndex(const std::string&);
		bool isRelationalOP(const TokenType&);
		bool isTypeSpecifier(const TokenType&);
		//synT的处理
		void addTable();
		void delTable();
		//加上层次的处理
		void insertVoidVar(const std::string&);
		void insertIntVar(const std::string&);
		uint16_t findFunc(const std::string&);
		void printFile();
		

	private:
		std::vector<Token> _tokens;
		std::size_t _offset;
		std::pair<uint64_t, uint64_t> _current_pos;
		std::map<std::string, int32_t> _uninitialized_vars;
		std::map<std::string, int32_t> _vars;
		std::map<std::string, int32_t> _consts;
		std::map<std::string, int32_t> _ints;
		std::map<std::string, int32_t> _voids;
		std::map<std::string, int32_t> _funcs;
		// 下一个 token 在栈的偏移
		int32_t _nextTokenIndex;
		//符号表
		uint16_t _level;
		std::map<std::string, TokenType> funcs;
		std::vector<std::string> recFunc;
		//for the output
		std::vector<vm::Constant> constants;
		std::vector<vm::Function> functions;
		std::vector<vm::Instruction> _instructions;//start instructions

		std::map<uint16_t, std::vector<struct Variable>> synT;
		vm::Function now_func;
		int recPos;
		bool isReturned;
		bool returnJudge;
		
		std::vector<std::string> output;
		std::vector<std::string>bin_output;
	};
}
