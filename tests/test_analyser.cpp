#include "catch2/catch.hpp"

#include "instruction/instruction.h"
#include "tokenizer/tokenizer.h"
#include "analyser/analyser.h"
#include "simple_vm.hpp"
#include "c0-vm-cpp/src/file.h"

#include <sstream>
#include <vector>

TEST_CASE("missing_begin_end", "[invalid]") {
	std::string input =
		"int N=0xbabe;\n"
		"int max;\n"

		"int fib(int n) {\n"
			"if (n <= 0) return 0;\n"
			"/*else if (n == 1) return 1;\n*/"
			"//else return fib(n-2);\n"
		"}\n"

		"int main() {\n"
			"int i = 0;\n"
			"int f;\n"
			"scan(max);\n"
			/*"int pi = pi * 3;\n"
			"if (pi < max) {\n"
				"max = pi;\n"
			"}\n"*/
			"while (i < max) {\n"
				"f = fib(i);\n"
				"if (f < N) {\n"
					"print(N,max);\n"
				"}\n"
				"else {\n"
					"print(\"fib\", i, \"=\", f, \">=\", N);\n"
				"}\n"
				"i = i + 1;\n"
			"}\n"
			"return 0;\n"
		"}\n "
		;
	std::stringstream ss;
	ss.str(input);
	miniplc0::Tokenizer tkz(ss);
	auto tokens = tkz.AllTokens();
	//std::cout << tokens.second.value().GetPos().first<< " " << tokens.second.value().GetPos().second << "\n";
	if (tokens.second.has_value())
		FAIL();
	miniplc0::Analyser analyser(tokens.first);
	auto instructions = analyser.Analyse();
	//std::cout << instructions.second.value().GetCode()<< "\n";
	//std::cout << "OK\n";
	REQUIRE((!instructions.second.has_value()));
}


TEST_CASE("missing_begin_end2", "[invalid]") {
	std::string input =
		"int g0 = 42;\n"

	"int fun(int num) {\n"
		"return -num;\n"
	"}\n"

	"int main() {\n"
		"return fun(-123456);\n"
	"}\n"
		;
	std::stringstream ss;
	ss.str(input);
	miniplc0::Tokenizer tkz(ss);
	auto tokens = tkz.AllTokens();
	if (tokens.second.has_value())
		FAIL();
	miniplc0::Analyser analyser(tokens.first);
	auto instructions = analyser.Analyse();
	//std::cout << instructions.second.value().GetPos().first<<" "<<instructions.second.value().GetPos().second<< "\n";
	std::cout << "\n";
	REQUIRE((!instructions.second.has_value()));
}

TEST_CASE("missing_semicolon", "[invalid]") {
	std::string input =
		"void hanoi(int n, int a, int b, int c) {\n"
		"if (n == 1) {\n"
			"print(a, \"->\", c);\n"
		"}\n"
		"else {\n"
			"hanoi(n - 1, a, c, b);\n"
			"print(a, \"->\", c);\n"
			"hanoi(n - 1, b, a, c);\n"

		"}\n"
	"}\n"

	"int main() {\n"
		"hanoi(3, 'a', 'b', 'c');\n"
		"return 0;\n"
	"}\n"
		;
	std::stringstream ss;
	ss.str(input);
	miniplc0::Tokenizer tkz(ss);
	auto tokens = tkz.AllTokens();
	if (tokens.second.has_value())
		FAIL();
	miniplc0::Analyser analyser(tokens.first);
	auto instructions = analyser.Analyse();
	/*std::cout << instructions.second.value().GetCode() << "\n";
	std::cout << instructions.second.value().GetPos().first << " " << instructions.second.value().GetPos().second << "\n";*/
	std::cout << "\n";
	REQUIRE((!instructions.second.has_value()));
}
//
////TEST_CASE("redeclaration", "[invalid]") {
////	std::string input =
////		"void func() {}\n"
////		"int main() {\n"
////			"int a = a + func();\n"
////			"}\n"
////		;
////	std::stringstream ss;
////	ss.str(input);
////	miniplc0::Tokenizer tkz(ss);
////	auto tokens = tkz.AllTokens();
////	if (tokens.second.has_value())
////		FAIL();
////	miniplc0::Analyser analyser(tokens.first);
////	auto instructions = analyser.Analyse();
////	std::cout << instructions.second.has_value() << "\n";
////	REQUIRE((instructions.second.has_value()));
////}
////
//TEST_CASE("uninit", "[invalid]") {
//	std::string input =
//		"int a=0xffff;\n"
//		;
//	std::stringstream ss;
//	ss.str(input);
//	std::ifstream in;
//	in.open("D:\\study\\±àÒëÔ­Àí\\C0\\miniplc0-compiler\\build\\in.txt");
//	miniplc0::Tokenizer tkz(in);
//	auto tokens = tkz.AllTokens();
//	if (tokens.second.has_value())
//		FAIL();
//	miniplc0::Analyser analyser(tokens.first);
//	auto instructions = analyser.Analyse();
//	//std::cout << instructions.second.value().GetCode() << "\n";
//	REQUIRE((!instructions.second.has_value()));
//}
//
//TEST_CASE("var_const", "[invalid]") {
//	std::string input =
//		"begin\n"
//		"var a;\n"
//		"const b = 1;\n"
//		"end\n"
//		;
//	std::stringstream ss;
//	ss.str(input);
//	miniplc0::Tokenizer tkz(ss);
//	auto tokens = tkz.AllTokens();
//	if (tokens.second.has_value())
//		FAIL();
//	miniplc0::Analyser analyser(tokens.first);
//	auto instructions = analyser.Analyse();
//	REQUIRE((instructions.second.has_value()));
//}
//
//TEST_CASE("assign", "[valid]") {
//	std::string input =
//		"begin\n"
//		"var a = 1;\n"
//		"var b;\n"
//		"var c;\n"
//		"var d;\n"
//		"var e;\n"
//		"b = a;\n"
//		"e = b;\n"
//		"d = e;\n"
//		"c = a;\n"
//		"print(c);\n"
//		"end\n"
//		;
//	std::stringstream ss;
//	ss.str(input);
//	miniplc0::Tokenizer tkz(ss);
//	auto tokens = tkz.AllTokens();
//	if (tokens.second.has_value())
//		FAIL();
//	miniplc0::Analyser analyser(tokens.first);
//	auto instructions = analyser.Analyse();
//	if (instructions.second.has_value())
//		FAIL();
//	miniplc0::VM vm(instructions.first);
//	std::vector<int32_t> writes = vm.Run();
//	std::vector<int32_t> output = {};
//	output.emplace_back(1);
//	REQUIRE((writes == output));
//}
//
//TEST_CASE("declaration", "[valid]") {
//	std::string input =
//		"begin\n"
//		"const abc = 123;\n"
//		"var ABC = 456;\n"
//		"print(abc);\n"
//		"print(ABC);\n"
//		"end\n"
//		;
//	std::stringstream ss;
//	ss.str(input);
//	miniplc0::Tokenizer tkz(ss);
//	auto tokens = tkz.AllTokens();
//	if (tokens.second.has_value())
//		FAIL();
//	miniplc0::Analyser analyser(tokens.first);
//	auto instructions = analyser.Analyse();
//	if (instructions.second.has_value())
//		FAIL();
//	miniplc0::VM vm(instructions.first);
//	std::vector<int32_t> writes = vm.Run();
//	std::vector<int32_t> output = {};
//	output.emplace_back(123);
//	output.emplace_back(456);
//	REQUIRE((writes == output));
//}
//
//TEST_CASE("init", "[valid]") {
//	std::string input =
//		"begin\n"
//		"var a = 0;\n"
//		"var b = 1;\n"
//		"var c = a+b;\n"
//		"a = b;\n"
//		"c = c;\n"
//		"c = a+b;\n"
//		"a = b;\n"
//		"b = c;\n"
//		"print(a);\n"
//		"print(b);\n"
//		"print(c);\n"
//		"end\n"
//		;
//	std::stringstream ss;
//	ss.str(input);
//	miniplc0::Tokenizer tkz(ss);
//	auto tokens = tkz.AllTokens();
//	if (tokens.second.has_value())
//		FAIL();
//	miniplc0::Analyser analyser(tokens.first);
//	auto instructions = analyser.Analyse();
//	if (instructions.second.has_value()) {
//		std::cout << instructions.second.value().GetCode() << "\n";
//		FAIL();
//	}
//	miniplc0::VM vm(instructions.first);
//	std::vector<int32_t> writes = vm.Run();
//	std::vector<int32_t> output = {};
//	output.emplace_back(1);
//	output.emplace_back(2);
//	output.emplace_back(2);
//	REQUIRE((writes == output));
//}
//
//TEST_CASE("no_begin", "[invalid]") {
//	std::string input =
//		"end"
//		;
//	std::stringstream ss;
//	ss.str(input);
//	miniplc0::Tokenizer tkz(ss);
//	auto tokens = tkz.AllTokens();
//	if (tokens.second.has_value())
//		FAIL();
//	miniplc0::Analyser analyser(tokens.first);
//	auto instructions = analyser.Analyse();
//	REQUIRE((instructions.second.has_value()));
//}
//
//TEST_CASE("no_end", "[invalid]") {
//	std::string input =
//		"begin"
//		;
//	std::stringstream ss;
//	ss.str(input);
//	miniplc0::Tokenizer tkz(ss);
//	auto tokens = tkz.AllTokens();
//	if (tokens.second.has_value())
//		FAIL();
//	miniplc0::Analyser analyser(tokens.first);
//	auto instructions = analyser.Analyse();
//	REQUIRE((instructions.second.has_value()));
//}
//
//TEST_CASE("no_semicolon", "[invalid]") {
//	std::string input =
//		"begin\n"
//		"   const a = 1\n"
//		"end"
//		;
//	std::stringstream ss;
//	ss.str(input);
//	miniplc0::Tokenizer tkz(ss);
//	auto tokens = tkz.AllTokens();
//	if (tokens.second.has_value())
//		FAIL();
//	miniplc0::Analyser analyser(tokens.first);
//	auto instructions = analyser.Analyse();
//	REQUIRE((instructions.second.has_value()));
//}
//
//TEST_CASE("const_no_value", "[invalid]") {
//	std::string input =
//		"begin\n"
//		"   const a;\n"
//		"   const b;\n"
//		"end"
//		;
//	std::stringstream ss;
//	ss.str(input);
//	miniplc0::Tokenizer tkz(ss);
//	auto tokens = tkz.AllTokens();
//	if (tokens.second.has_value())
//		FAIL();
//	miniplc0::Analyser analyser(tokens.first);
//	auto instructions = analyser.Analyse();
//	REQUIRE((instructions.second.has_value()));
//}
//
//TEST_CASE("const_duplicate", "[invalid]") {
//	std::string input =
//		"begin\n"
//		"   const a = 1;\n"
//		"   const a = 2;\n"
//		"end"
//		;
//	std::stringstream ss;
//	ss.str(input);
//	miniplc0::Tokenizer tkz(ss);
//	auto tokens = tkz.AllTokens();
//	if (tokens.second.has_value())
//		FAIL();
//	miniplc0::Analyser analyser(tokens.first);
//	auto instructions = analyser.Analyse();
//	REQUIRE((instructions.second.has_value()));
//}
//
//TEST_CASE("var_duplicate", "[invalid]") {
//	std::string input =
//		"begin\n"
//		"   var a;\n"
//		"   var a;\n"
//		"end"
//		;
//	std::stringstream ss;
//	ss.str(input);
//	miniplc0::Tokenizer tkz(ss);
//	auto tokens = tkz.AllTokens();
//	if (tokens.second.has_value())
//		FAIL();
//	miniplc0::Analyser analyser(tokens.first);
//	auto instructions = analyser.Analyse();
//	REQUIRE((instructions.second.has_value()));
//}
//
//TEST_CASE("assign_to_const", "[invalid]") {
//	std::string input =
//		"begin\n"
//		"   const a = 1;\n"
//		"   a = 2;\n"
//		"end"
//		;
//	std::stringstream ss;
//	ss.str(input);
//	miniplc0::Tokenizer tkz(ss);
//	auto tokens = tkz.AllTokens();
//	if (tokens.second.has_value())
//		FAIL();
//	miniplc0::Analyser analyser(tokens.first);
//	auto instructions = analyser.Analyse();
//	REQUIRE((instructions.second.has_value()));
//}
//
//TEST_CASE("assign_no_value", "[invalid]") {
//	std::string input =
//		"begin\n"
//		"   var a;\n"
//		"   a = ;\n"
//		"end"
//		;
//	std::stringstream ss;
//	ss.str(input);
//	miniplc0::Tokenizer tkz(ss);
//	auto tokens = tkz.AllTokens();
//	if (tokens.second.has_value())
//		FAIL();
//	miniplc0::Analyser analyser(tokens.first);
//	auto instructions = analyser.Analyse();
//	REQUIRE((instructions.second.has_value()));
//}
//
//TEST_CASE("print_no_value", "[invalid]") {
//	std::string input =
//		"begin\n"
//		"   print();\n"
//		"end"
//		;
//	std::stringstream ss;
//	ss.str(input);
//	miniplc0::Tokenizer tkz(ss);
//	auto tokens = tkz.AllTokens();
//	if (tokens.second.has_value())
//		FAIL();
//	miniplc0::Analyser analyser(tokens.first);
//	auto instructions = analyser.Analyse();
//	REQUIRE((instructions.second.has_value()));
//}
//
//TEST_CASE("valid_empty", "[valid]") {
//	std::string input =
//		"begin\n"
//		"end"
//		;
//	std::stringstream ss;
//	ss.str(input);
//	miniplc0::Tokenizer tkz(ss);
//	auto tokens = tkz.AllTokens();
//	if (tokens.second.has_value())
//		FAIL();
//	miniplc0::Analyser analyser(tokens.first);
//	auto instructions = analyser.Analyse();
//	if (instructions.second.has_value())
//		FAIL();
//	miniplc0::VM vm(instructions.first);
//	std::vector<int32_t> writes = vm.Run();
//	std::vector<int32_t> output = {};
//	REQUIRE((writes == output));
//}
//
//TEST_CASE("valid_empty_statement", "[valid]") {
//	std::string input =
//		"begin\n"
//		"   ;;;;;;;;;;;;\n"
//		"   ;;;;;\n"
//		"end"
//		;
//	std::stringstream ss;
//	ss.str(input);
//	miniplc0::Tokenizer tkz(ss);
//	auto tokens = tkz.AllTokens();
//	if (tokens.second.has_value())
//		FAIL();
//	miniplc0::Analyser analyser(tokens.first);
//	auto instructions = analyser.Analyse();
//	if (instructions.second.has_value())
//		FAIL();
//	miniplc0::VM vm(instructions.first);
//	std::vector<int32_t> writes = vm.Run();
//	std::vector<int32_t> output = {};
//	REQUIRE((writes == output));
//}
//
//TEST_CASE("valid_overview", "[valid]") {
//	std::string input =
//		"begin\n"
//		"   const a = 1;\n"
//		"   const b = -2147483647;\n"
//		"   var c;\n"
//		"   var d = a + b + 6;\n"
//		"   c = 3;\n"
//		"   d = 5;\n"
//		"   print((-d + c) * a);\n"
//		"   ;\n"
//		"end\n"
//		;
//	std::stringstream ss;
//	ss.str(input);
//	miniplc0::Tokenizer tkz(ss);
//	auto tokens = tkz.AllTokens();
//	if (tokens.second.has_value())
//		FAIL();
//	miniplc0::Analyser analyser(tokens.first);
//	auto instructions = analyser.Analyse();
//	if (instructions.second.has_value())
//		FAIL();
//	miniplc0::VM vm(instructions.first);
//	std::vector<int32_t> writes = vm.Run();
//	std::vector<int32_t> output = {};
//	output.emplace_back(-2);
//	REQUIRE((writes == output));
//}