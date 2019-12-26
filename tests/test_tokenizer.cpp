#include "catch2/catch.hpp"
#include "tokenizer/tokenizer.h"
#include "fmt/core.h"

#include <sstream>
#include <vector>

////TEST_CASE("invalid_characters", "[invalid]") {
////	std::string input = "!\"#$%&',.:<>?@[\\]^_`{|}~";
////	std::stringstream ss;
////	ss.str(input);
////	miniplc0::Tokenizer tkz(ss);
////	auto result = tkz.AllTokens();
////	REQUIRE((result.second.has_value()));
////}
////
////TEST_CASE("toooooo_big_integers", "[invalid]") {
////	std::string input =
////		"2147483648\n"
////		"4000000000\n"
////		"18446744073709551616\n"
////		"1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111\n";
////	std::stringstream ss;
////	ss.str(input);
////	miniplc0::Tokenizer tkz(ss);
////	auto result = tkz.AllTokens();
////	REQUIRE((result.second.has_value()));
////}
////
//TEST_CASE("empty_file", "[valid]") {
//	std::string input =
//		"\"\\x20\"\n"
//		;
//	std::stringstream ss;
//	ss.str(input);
//	miniplc0::Tokenizer tkz(ss);
//	auto result = tkz.AllTokens();	
//	if (result.second.has_value())
//		FAIL();
//	std::vector<miniplc0::Token> output = {};
//	/*for (std::vector<miniplc0::Token>::iterator it = result.first.begin();
//		it != result.first.end(); it++) {
//		miniplc0::Token tk = *it;
//		std::cout << tk.GetValueString();
//	}*/
//	std::cout << result.first.at(0).GetValueString() << "\n";
//	REQUIRE((!result.second.has_value()));
//}
//
////TEST_CASE("identifiers", "[invalid]") {
////	std::string input =
////		//"=\n"
////		//"==\n"
////		//">\n"
////		".\n"
////		;
////	std::stringstream ss;
////	ss.str(input);
////	miniplc0::Tokenizer tkz(ss);
////	auto result = tkz.AllTokens();
////	REQUIRE((result.second.has_value()));
////}
////
//TEST_CASE("integers", "[invalid]") {
//	std::string input =
//		"\'a\'\n"
//		;
//		
//	std::stringstream ss;
//	ss.str(input);
//	std::ifstream in;
//	in.open("D:\\study\\编译原理\\C0\\miniplc0-compiler\\build\\in.txt");
//	miniplc0::Tokenizer tkz(in);
//	auto result = tkz.AllTokens();
//	//std::cout << result.second.value().GetPos().first << result.second.value().GetPos().second << "\n";
//	if (result.second.has_value())
//		FAIL();
//	std::vector<miniplc0::Token> output = {};
//	output.emplace_back(miniplc0::Token(miniplc0::TokenType::C_CHAR, std::string("a"), 0, 0, 0, 3));
//	//std::cout << result.first.at(0).GetValueString() << "\n";
//	REQUIRE((result.first==output));
//}
//TEST_CASE("comma", "[invalid]") {
//	std::string input =
//		",\n"
//		;
//
//	std::stringstream ss;
//	ss.str(input);
//	std::ifstream in;
//	in.open("D:\\study\\编译原理\\C0\\miniplc0-compiler\\build\\in.txt");
//	miniplc0::Tokenizer tkz(in);
//	auto result = tkz.AllTokens();
//	if (result.second.has_value())
//		FAIL();
//	std::vector<miniplc0::Token> output = {};
//	output.emplace_back(miniplc0::Token(miniplc0::TokenType::COMMA, std::string(","), 0, 0, 0, 1));
//	//std::cout << result.first.at(0).GetValueString() << "\n";
//	REQUIRE((result.first == output));
//}
////
////TEST_CASE("keywords", "[valid]") {
////	std::string input =
////		"char\n"
////		"void\n"
////		"const\n"
////		"struct\n"
////		"print";
////	std::stringstream ss;
////	ss.str(input);
////	miniplc0::Tokenizer tkz(ss);
////	auto result = tkz.AllTokens();
////	if (result.second.has_value())
////		FAIL();
////	std::vector<miniplc0::Token> output = {};
////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::CHAR, std::string("char"), 0, 0, 0, 4));
////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::VOID, std::string("void"), 1, 0, 1, 4));
////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::CONST, std::string("const"), 2, 0, 2, 5));
////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::STRUCT, std::string("struct"), 3, 0, 3, 6));
////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::PRINT, std::string("print"), 4, 0, 4, 5));
////	REQUIRE((result.first == output));
////}
////
//TEST_CASE("no_ws_at_the_end1", "[valid]") {
//	std::string input =
//		"int a=1;\n"
//		"int func(){}\n";
//	std::stringstream ss;
//	ss.str(input);
//	std::ifstream in;
//	in.open("D:\\study\\编译原理\\C0\\miniplc0-compiler\\build\\in.txt");
//	miniplc0::Tokenizer tkz(in);
//	auto result = tkz.AllTokens();
//	
//	if (result.second.has_value())
//		FAIL();
//	std::vector<miniplc0::Token> output = {};
//	output.emplace_back(miniplc0::Token(miniplc0::TokenType::INT, std::string("int"), 0, 0, 0, 3));
//	output.emplace_back(miniplc0::Token(miniplc0::TokenType::IDENTIFIER, std::string("a"), 0, 4, 0, 5));
//	output.emplace_back(miniplc0::Token(miniplc0::TokenType::EQUAL_SIGN, std::string("="), 0, 5, 0, 6));
//	output.emplace_back(miniplc0::Token(miniplc0::TokenType::UNSIGNED_INTEGER, std::string("1"), 0, 6, 0, 7));
//	output.emplace_back(miniplc0::Token(miniplc0::TokenType::SEMICOLON, std::string(";"), 0, 7, 0, 8));
//	output.emplace_back(miniplc0::Token(miniplc0::TokenType::INT, std::string("int"), 1, 0, 1, 3));
//	output.emplace_back(miniplc0::Token(miniplc0::TokenType::IDENTIFIER, std::string("func"), 1, 4, 1, 8));
//	output.emplace_back(miniplc0::Token(miniplc0::TokenType::LEFT_BRACKET, std::string("("), 1, 8, 1, 9));
//	output.emplace_back(miniplc0::Token(miniplc0::TokenType::RIGHT_BRACKET, std::string(")"), 1, 9, 1, 10));
//	output.emplace_back(miniplc0::Token(miniplc0::TokenType::LEFT_MID_BRACKET, std::string("{"), 1, 10, 1, 11));
//	output.emplace_back(miniplc0::Token(miniplc0::TokenType::RIGHT_MID_BRACKET, std::string("}"), 1, 11, 1, 12));
//	REQUIRE((result.first == output));
//}
//////
//////TEST_CASE("no_ws_at_the_end2", "[valid]") {
//////	std::string input =
//////		";";
//////	std::stringstream ss;
//////	ss.str(input);
//////	miniplc0::Tokenizer tkz(ss);
//////	auto result = tkz.AllTokens();
//////	if (result.second.has_value())
//////		FAIL();
//////	std::vector<miniplc0::Token> output = {};
//////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::SEMICOLON, ';', 0, 0, 0, 1));
//////	REQUIRE((result.first == output));
//////}
//////
////////why error??
////TEST_CASE("operators", "[valid]") {
////	std::string input =
////		"/*abc*/\n"
////		"//abc\n";
////		//"/**/\n"
////		//"/*/";
////	std::stringstream ss;
////	ss.str(input);
////	miniplc0::Tokenizer tkz(ss);
////	auto result = tkz.AllTokens();
////	if (result.second.has_value())
////		FAIL();
////	std::vector<miniplc0::Token> output = {};
////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::RIGHT_MULTI_COMMENT, std::string("*/"), 0, 0, 0, 7));
////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::SINGLE_COMMENT, std::string("//"), 1, 0, 1, 5));
////	//output.emplace_back(miniplc0::Token(miniplc0::TokenType::MULTIPLICATION_SIGN, '*', 5, 1, 5, 2));
////	//output.emplace_back(miniplc0::Token(miniplc0::TokenType::LEFT_MULTI_COMMENT, std::string("/*"), 6, 0, 6, 2));
////	//output.emplace_back(miniplc0::Token(miniplc0::TokenType::RIGHT_MULTI_COMMENT, std::string("*/"), 6, 2, 6, 4));
////	//output.emplace_back(miniplc0::Token(miniplc0::TokenType::LEFT_MULTI_COMMENT, std::string("/*"), 7, 0, 7, 2));
////	std::cout <<  result.first.at(1).GetEndPos().first << "\n";
////	REQUIRE((result.first == output));
////}
//////
//////TEST_CASE("parentheses", "[valid]") {
//////	std::string input =
//////		"(\n"
//////		")\n"
//////		"(()))(";
//////	std::stringstream ss;
//////	ss.str(input);
//////	miniplc0::Tokenizer tkz(ss);
//////	auto result = tkz.AllTokens();
//////	if (result.second.has_value())
//////		FAIL();
//////	std::vector<miniplc0::Token> output = {};
//////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::LEFT_BRACKET, '(', 0, 0, 0, 1));
//////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::RIGHT_BRACKET, ')', 1, 0, 1, 1));
//////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::LEFT_BRACKET, '(', 2, 0, 2, 1));
//////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::LEFT_BRACKET, '(', 2, 1, 2, 2));
//////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::RIGHT_BRACKET, ')', 2, 2, 2, 3));
//////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::RIGHT_BRACKET, ')', 2, 3, 2, 4));
//////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::RIGHT_BRACKET, ')', 2, 4, 2, 5));
//////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::LEFT_BRACKET, '(', 2, 5, 2, 6));
//////	REQUIRE((result.first == output));
//////}
//////
//////TEST_CASE("semicolons", "[valid]") {
//////	std::string input =
//////		";\n"
//////		";;;";
//////	std::stringstream ss;
//////	ss.str(input);
//////	miniplc0::Tokenizer tkz(ss);
//////	auto result = tkz.AllTokens();
//////	if (result.second.has_value())
//////		FAIL();
//////	std::vector<miniplc0::Token> output = {};
//////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::SEMICOLON, ';', 0, 0, 0, 1));
//////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::SEMICOLON, ';', 1, 0, 1, 1));
//////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::SEMICOLON, ';', 1, 1, 1, 2));
//////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::SEMICOLON, ';', 1, 2, 1, 3));
//////	REQUIRE((result.first == output));
//////}
//////
//////TEST_CASE("test", "[valid]") {
//////	std::string input =
//////		"int intMin = -2147483647-1;\n"
//////		"const intMIN = - 2147483647 - 1;";
//////	std::stringstream ss;
//////	ss.str(input);
//////	miniplc0::Tokenizer tkz(ss);
//////	auto result = tkz.AllTokens();
//////	if (result.second.has_value())
//////		FAIL();
//////	std::vector<miniplc0::Token> output = {};
//////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::INT, std::string("int"), 0, 0, 0, 3));
//////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::IDENTIFIER, std::string("intMin"), 0, 4, 0, 10));
//////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::EQUAL_SIGN, '=', 0, 11, 0, 12));
//////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::MINUS_SIGN, '-', 0, 13, 0, 14));
//////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::UNSIGNED_INTEGER, 2147483647, 0, 14, 0, 24));
//////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::MINUS_SIGN, '-', 0, 24, 0, 25));
//////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::UNSIGNED_INTEGER, 1, 0, 25, 0, 26));
//////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::SEMICOLON, ';', 0, 26, 0, 27));
//////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::CONST, std::string("const"), 1, 0, 1, 5));
//////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::IDENTIFIER, std::string("intMIN"), 1, 6, 1, 12));
//////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::EQUAL_SIGN, '=', 1, 13, 1, 14));
//////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::MINUS_SIGN, '-', 1, 15, 1, 16));
//////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::UNSIGNED_INTEGER, 2147483647, 1, 17, 1, 27));
//////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::MINUS_SIGN, '-', 1, 28, 1, 29));
//////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::UNSIGNED_INTEGER, 1, 1, 30, 1, 31));
//////	output.emplace_back(miniplc0::Token(miniplc0::TokenType::SEMICOLON, ';', 1, 31, 1, 32));
//////	REQUIRE((result.first == output));
//////}
//////
//TEST_CASE("whitespaces", "[valid]") {
//	std::string input =
//		"//int a=1;   	     \n"
//		"/*aaaasafwef%%%   \n"
//		"int*/\n"
//		"\n"
//		" \n"
//		"                     \n"
//		"       \n";
//	std::stringstream ss;
//	ss.str(input);
//	std::ifstream in;
//	in.open("D:\\study\\编译原理\\C0\\miniplc0-compiler\\build\\in.txt");
//	miniplc0::Tokenizer tkz(in);
//	auto result = tkz.AllTokens();
//	if (result.second.has_value())
//		FAIL();
//	std::vector<miniplc0::Token> output = {};
//	//output.emplace_back(miniplc0::Token(miniplc0::TokenType::INT, std::string("int"), 2, 0, 2, 3));
//	//std::cout << result.first.at(0).GetStartPos().first << "\n";
//	REQUIRE((result.first == output));
//}
////
////TEST_CASE("invalid_ids", "[invalid]") {
////	std::string input =
////		"a\n"
////		"A\n"
////		"abc\n"
////		"ABC\n"
////		"Abc\n"
////		"aBc\n"
////		"aaawiogfpiusaGPIFsbfbpiweifgbpIAEGPFIewpifgpibpijgbpijbgpbijpbIPJabipPDP\n"
////		"a1\n"
////		"0989852\n"
////		"A5\n"
////		"A21646452\n"
////		"a2431A\n"
////		"5s6sa89sa9asf5asf98asf5789asf5789asf9587\n"
////		"a7dt b87TR8D sr780sA7D089 TS87tdxb08 TX08tn\n"
////		"d70SADT087 satdx697R  NX9X2141sga2asfEN08qw\n"
////		"\n"
////		"\n"
////		"BEGIN\n"
////		"END\n"
////		"CONST\n"
////		"VAR\n"
////		"PRINT\n"
////		"\n"
////		"BeGiN\n"
////		"eNd\n"
////		"CONst\n"
////		"vaR\n"
////		"priNT\n"
////		"\n"
////		"beginend\n"
////		"beginEND\n"
////		"CONSTvar\n"
////		"begin123456end\n"
////		"print987654321\n"
////		"const0\n"
////		"var1\n"
////		;
////	std::stringstream ss;
////	ss.str(input);
////	miniplc0::Tokenizer tkz(ss);
////	auto result = tkz.AllTokens();
////	REQUIRE((result.second.has_value()));
////}
