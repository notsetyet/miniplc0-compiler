#include "tokenizer/tokenizer.h"

#include <cctype>
#include <sstream>

namespace miniplc0 {

	std::pair<std::optional<Token>, std::optional<CompilationError>> Tokenizer::NextToken() {
		if (!_initialized)
			readAll();
		if (_rdr.bad())
			return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(0, 0, ErrorCode::ErrStreamError));
		if (isEOF())
			return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(0, 0, ErrorCode::ErrEOF));
		auto p = nextToken();
		if (p.second.has_value())
			return std::make_pair(p.first, p.second);
		auto err = checkToken(p.first.value());//检查了是否是以数字开头的非法identifier
		if (err.has_value())
			return std::make_pair(p.first, err.value());
		return std::make_pair(p.first, std::optional<CompilationError>());
	}

	std::pair<std::vector<Token>, std::optional<CompilationError>> Tokenizer::AllTokens() {
		std::vector<Token> result;
		while (true) {
			auto p = NextToken();
			if (p.second.has_value()) {
				if (p.second.value().GetCode() == ErrorCode::ErrEOF)
					return std::make_pair(result, std::optional<CompilationError>());
				else
					return std::make_pair(std::vector<Token>(), p.second);
			}
			result.emplace_back(p.first.value());
		}
	}	
	
	// 注意：这里的返回值中 Token 和 CompilationError 只能返回一个，不能同时返回。
	std::pair<std::optional<Token>, std::optional<CompilationError>> Tokenizer::nextToken() {
		// 用于存储已经读到的组成当前token字符
		std::stringstream ss;
		ss.clear();
		// 分析token的结果，作为此函数的返回值
		std::pair<std::optional<Token>, std::optional<CompilationError>> result;
		// <行号，列号>，表示当前token的第一个字符在源代码中的位置
		std::pair<int64_t, int64_t> pos;
		// 记录当前自动机的状态，进入此函数时是初始状态
		DFAState current_state = DFAState::INITIAL_STATE;
		bool zeroFlag = false;	
		bool numFlag = false;
		std::string s;
		// 这是一个死循环，除非主动跳出
		// 每一次执行while内的代码，都可能导致状态的变更
		while (true) {
			auto current_char = nextChar();
			switch (current_state) {
			case INITIAL_STATE: {
				// 已经读到了文件尾
				if (!current_char.has_value())
					// 返回一个空的token，和编译错误ErrEOF：遇到了文件尾
					return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(0, 0, ErrEOF));

				// 获取读到的字符的值，注意auto推导出的类型是char
				auto ch = current_char.value();
				// 标记是否读到了不合法的字符，初始化为否
				auto invalid = false;

				// 使用了自己封装的判断字符类型的函数，定义于 tokenizer/utils.hpp
				// see https://en.cppreference.com/w/cpp/string/byte/isblank
				if (miniplc0::isspace(ch)) // 读到的字符是空白字符（空格、换行、制表符等）
					current_state = DFAState::INITIAL_STATE; // 保留当前状态为初始状态，此处直接break也是可以的
				else if (!miniplc0::isprint(ch)) { // control codes and backspace
					invalid = true;
				}
				else if (miniplc0::isdigit(ch)) { // 读到的字符是数字			
					if (ch == '0')
						current_state = DFAState::ZERO_STATE;
					else
						current_state = DFAState::DECIMAL_LITERAL_STATE;
				}
				else if (miniplc0::isalpha(ch)) // 读到的字符是英文字母		
					current_state = DFAState::IDENTIFIER_STATE; // 切换到16进制
				else {
					switch (ch) {
					case '=': 
						current_state = DFAState::EQUAL_STATE;
						break;
					case '-':
						// 请填空：切换到减号的状态
						current_state = DFAState::MINUS_SIGN_STATE;
						break;
					case '+':
						// 请填空：切换到加号的状态
						current_state = DFAState::PLUS_SIGN_STATE;
						break;
					case '*':
						// 请填空：切换状态
						current_state = DFAState::MULTIPLICATION_SIGN_STATE;
						break;
					case '/':
						// 请填空：切换状态
						current_state = DFAState::DIVISION_SIGN_STATE;
						break;
						
					case ')':
						current_state = DFAState::RIGHTBRACKET_STATE;
						break;
					case '(':
						current_state = DFAState::LEFTBRACKET_STATE;
						break;
					case ';':
						current_state = DFAState::SEMICOLON_STATE;
						break;
					case '<':
						current_state = DFAState::LESS_STATE;
						break;
					case '>':
						current_state = DFAState::MORE_STATE;
						break;
					case '!':
						current_state = DFAState::NOT_EQUAL_SIGN_STATE;
						break;
						// 不接受的字符导致的不合法的状态
					case ':':
						current_state = DFAState::MAO_STATE;
						break;
					case '.':
						current_state = DFAState::DOT_STATE;
						break;
					case '{':
						current_state = DFAState::LEFT_MID_BRACKET_STATE;
						break;
					case '}':
						current_state = DFAState::RIGHT_MID_BRACKET_STATE;
						break;
					case '\'':
						current_state = DFAState::CHAR_LITERAL_STATE;
						break;
					case '\"':
						current_state = DFAState::STRING_LITERAL_STATE;
						break;
					case ',':
						current_state = DFAState::COMMA_STATE;
						break;
					default: {
						invalid = true;
						break;
					}
					}
				}
				// 如果读到的字符导致了状态的转移，说明它是一个token的第一个字符
				if (current_state != DFAState::INITIAL_STATE)
					pos = previousPos(); // 记录该字符的的位置为token的开始位置
				// 读到了不合法的字符
				if (invalid) {
					// 回退这个字符
					unreadLast();
					// 返回编译错误：非法的输入
					return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidInput));
				}
				// 如果读到的字符导致了状态的转移，说明它是一个token的第一个字符
				if (current_state != DFAState::INITIAL_STATE) // ignore white spaces	
					ss << ch; // 存储读到的字符
				break;
			}

			case ZERO_STATE: {
				auto ch = current_char.value();
				if (!current_char.has_value()) {
					return std::make_pair(std::make_optional<Token>(TokenType::UNSIGNED_INTEGER, std::string("0"), pos, currentPos()), std::optional<CompilationError>());
				}
				else if (ch == 'x' || ch == 'X') {
					ss << ch;
					current_state = DFAState::HEXADECIMAL_LITERAL_STATE;
				}
				else if (ch == '.') {//'.'报错
					ss << ch;
					current_char = nextChar();
					if (!current_char.has_value()) {
						return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidInput));
					}
					ch = current_char.value();
					if (miniplc0::isdigit(ch)) {
						ss << ch;
						current_state = DFAState::DOUBLE_STATE;
					}
					else if (ch == 'e' || ch == 'E') {
						ss << ch;
						current_state = DFAState::EXPONENT_STATE;
					}
					else {
						return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidInput));
					}
					//float||double
				}
				else if (miniplc0::isdigit(ch)) {
					return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrTooManyZeros));
				}
				else if (miniplc0::isalpha(ch)) {
					return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidIdentifier));
				}
				else {
					unreadLast();
					return std::make_pair(std::make_optional<Token>(TokenType::UNSIGNED_INTEGER, std::string("0"), pos, currentPos()), std::optional<CompilationError>());
				}
				break;
			}

			case DOT_STATE: {
				if (!current_char.has_value()) {
					return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidInput));
				}
				auto ch = current_char.value();
				if (miniplc0::isdigit(ch)) {
					ss << ch;
					current_state = DFAState::DOUBLE_STATE;
				}
				else if (ch == 'e' || ch == 'E') {
					ss << ch;
					current_state = DFAState::EXPONENT_STATE;
				}
				else {
					return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidInput));
				}
				break;
			}

			case DECIMAL_LITERAL_STATE: {				
				if (!current_char.has_value()) {
					s = ss.str();
					int32_t tmp;
					ss >> tmp;
					if (ss.eof() && !ss.fail()) {
						return std::make_pair(std::make_optional<Token>(TokenType::UNSIGNED_INTEGER, tmp, pos, currentPos()), std::optional<CompilationError>());
					}
					return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrIntegerOverflow));
				}
				// 如果读到的字符是数字，则存储读到的字符
				auto ch = current_char.value();
				if (miniplc0::isdigit(ch)) {					
					ss << ch;
				}
				else if (ch == '.') {
					ss << ch;
					current_char = nextChar();
					if (!current_char.has_value()) {
						return std::make_pair(std::make_optional<Token>(TokenType::DOUBLE, ss.str(), pos, currentPos()), std::optional<CompilationError>());
					}
					ch = current_char.value();
					if (miniplc0::isdigit(ch)) {
						ss << ch;
						current_state = DFAState::DOUBLE_STATE;
					}
					else if (ch == 'e' || ch == 'E') {
						ss << ch;
						current_state = DFAState::EXPONENT_STATE;
					}
					else {
						return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidInput));
					}
					//current_state = DFAState::DOUBLE_STATE;
					//float||double
				}
				else if (ch == 'e' || ch == 'E') {
					ss << ch;
					current_state = DFAState::EXPONENT_STATE;
				}
				else if (miniplc0::isalpha(ch)) {
					//不能num开头
					return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidInput));
				}
				// 如果读到的字符不是上述情况之一，则回退读到的字符，并解析已经读到的字符串为整数
				else {
					unreadLast();
					//     解析成功则返回无符号整数类型的token，否则返回编译错误
					s = ss.str();
					int32_t tmp;
					ss >> tmp;
					if (ss.eof() && !ss.fail()) {
						return std::make_pair(std::make_optional<Token>(TokenType::UNSIGNED_INTEGER, tmp, pos, currentPos()), std::optional<CompilationError>());
					}
					return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrIntegerOverflow));
				}
				break;
			}
			case HEXADECIMAL_LITERAL_STATE: {
				if (!current_char.has_value()) {
					if (ss.str().length() > 6) {
						return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrIntegerOverflow));
					}
					std::string str = ss.str();
					int32_t tmp = 0;
					for (int i = 2; i < str.length(); i++) {
						tmp *= 16;
						if (miniplc0::isdigit(str[i])) {
							tmp += str[i] - '0';
						}
						else {
							if (isupper(str[i])) {
								tmp += str[i] - 'A' + 10;
							}
							else {
								tmp += str[i] - 'a' + 10;
							}
						}						
					}
					//std::cout << tmp << "\n";
					return std::make_pair(std::make_optional<Token>(TokenType::UNSIGNED_INTEGER, tmp, pos, currentPos()), std::optional<CompilationError>());
				}
				auto ch = current_char.value();
				if (miniplc0::isdigit(ch)) {
					ss << ch;
				}
				else if (miniplc0::isalpha(ch)) {
					if (ch >= 'a' && ch <= 'f') {
						ss << ch;
					}
					else if (ch >= 'A' && ch <= 'F') {
						ss << ch;
					}
					else {
						return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidInput));
					}
				}
				else {
					unreadLast();
					if (ss.str().length() > 6) {
						return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrIntegerOverflow));
					}
					std::string str = ss.str();
					int32_t tmp = 0;
					for (int i = 2; i < str.length(); i++) {
						tmp *= 16;
						if (miniplc0::isdigit(str[i])) {
							tmp += str[i] - '0';
						}
						else {
							if (isupper(str[i])) {
								tmp += str[i] - 'A' + 10;
							}
							else {
								tmp += str[i] - 'a' + 10;
							}
						}
					}
					//std::cout << tmp << "\n";
					return std::make_pair(std::make_optional<Token>(TokenType::UNSIGNED_INTEGER, tmp, pos, currentPos()), std::optional<CompilationError>());
				}
				break;
			}
			case DOUBLE_STATE: {
				if (!current_char.has_value()) {
					return std::make_pair(std::make_optional<Token>(TokenType::DOUBLE, ss.str(), pos, currentPos()), std::optional<CompilationError>());
				}
				auto ch = current_char.value();
				if (ch == 'e' || ch == 'E') {
					ss << ch;
					current_state = DFAState::EXPONENT_STATE;
				}
				else if (miniplc0::isdigit(ch)) {
					ss << ch;
				}
				else {
					unreadLast();
					return std::make_pair(std::make_optional<Token>(TokenType::DOUBLE, ss.str(), pos, currentPos()), std::optional<CompilationError>());
				}
				break;
			}
			case EXPONENT_STATE: {
				if (!current_char.has_value()) {
					return std::make_pair(std::make_optional<Token>(TokenType::DOUBLE, ss.str(), pos, currentPos()), std::optional<CompilationError>());
				}
				auto ch = current_char.value();
				if (ch == '+' || ch == '-') {
					ss << ch;
				}
				else if (miniplc0::isdigit(ch)) {
					ss << ch;
				}
				else {
					unreadLast();
					return std::make_pair(std::make_optional<Token>(TokenType::DOUBLE, ss.str(), pos, currentPos()), std::optional<CompilationError>());
				}
				break;
			}
			case IDENTIFIER_STATE: {
				// 请填空：
				// 如果当前已经读到了文件尾，则解析已经读到的字符串
				if (!current_char.has_value()) {
					//     如果解析结果是关键字，那么返回对应关键字的token，否则返回标识符的token					
					std::string s = ss.str();
					if (s == "const") {
						return std::make_pair(std::make_optional<Token>(TokenType::CONST, std::string("const"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "print") {
						return std::make_pair(std::make_optional<Token>(TokenType::PRINT, std::string("print"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "scan") {
						return std::make_pair(std::make_optional<Token>(TokenType::SCAN, std::string("scan"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "void") {
						return std::make_pair(std::make_optional<Token>(TokenType::VOID, std::string("void"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "int") {
						return std::make_pair(std::make_optional<Token>(TokenType::INT, std::string("int"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "char") {
						return std::make_pair(std::make_optional<Token>(TokenType::CHAR, std::string("char"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "double") {
						return std::make_pair(std::make_optional<Token>(TokenType::DOUBLE, std::string("double"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "struct") {
						return std::make_pair(std::make_optional<Token>(TokenType::STRUCT, std::string("struct"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "if") {
						return std::make_pair(std::make_optional<Token>(TokenType::IF, std::string("if"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "else") {
						return std::make_pair(std::make_optional<Token>(TokenType::ELSE, std::string("else"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "switch") {
						return std::make_pair(std::make_optional<Token>(TokenType::SWITCH, std::string("switch"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "case") {
						return std::make_pair(std::make_optional<Token>(TokenType::CASE, std::string("case"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "default") {
						return std::make_pair(std::make_optional<Token>(TokenType::DEFAULT, std::string("default"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "while") {
						return std::make_pair(std::make_optional<Token>(TokenType::WHILE, std::string("while"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "for") {
						return std::make_pair(std::make_optional<Token>(TokenType::FOR, std::string("for"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "do") {
						return std::make_pair(std::make_optional<Token>(TokenType::DO, std::string("do"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "return") {
						return std::make_pair(std::make_optional<Token>(TokenType::RETURN, std::string("return"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "break") {
						return std::make_pair(std::make_optional<Token>(TokenType::BREAK, std::string("break"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "continue") {
						return std::make_pair(std::make_optional<Token>(TokenType::CONTINUE, std::string("continue"), pos, currentPos()), std::optional<CompilationError>());
					}
					return std::make_pair(std::make_optional<Token>(TokenType::IDENTIFIER, s, pos, currentPos()), std::optional<CompilationError>());
				}
				auto ch = current_char.value();
				// 如果读到的是字符或字母，则存储读到的字符
				if (miniplc0::isalpha(ch) || miniplc0::isdigit(ch)) {
					ss << ch;
				}
				// 如果读到的字符不是上述情况之一，则回退读到的字符，并解析已经读到的字符串
				else {
					unreadLast();
					//     如果解析结果是关键字，那么返回对应关键字的token，否则返回标识符的token
					std::string s = ss.str();
					if (s == "const") {
						return std::make_pair(std::make_optional<Token>(TokenType::CONST, std::string("const"), pos, currentPos()), std::optional<CompilationError>());
					}					
					else if (s == "print") {
						return std::make_pair(std::make_optional<Token>(TokenType::PRINT, std::string("print"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "scan") {
						return std::make_pair(std::make_optional<Token>(TokenType::SCAN, std::string("scan"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "void") {
						return std::make_pair(std::make_optional<Token>(TokenType::VOID, std::string("void"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "int") {
						return std::make_pair(std::make_optional<Token>(TokenType::INT, std::string("int"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "char") {
						return std::make_pair(std::make_optional<Token>(TokenType::CHAR, std::string("char"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "double") {
						return std::make_pair(std::make_optional<Token>(TokenType::DOUBLE, std::string("double"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "struct") {
						return std::make_pair(std::make_optional<Token>(TokenType::STRUCT, std::string("struct"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "if") {
						return std::make_pair(std::make_optional<Token>(TokenType::IF, std::string("if"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "else") {
						return std::make_pair(std::make_optional<Token>(TokenType::ELSE, std::string("else"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "switch") {
						return std::make_pair(std::make_optional<Token>(TokenType::SWITCH, std::string("switch"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "case") {
						return std::make_pair(std::make_optional<Token>(TokenType::CASE, std::string("case"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "default") {
						return std::make_pair(std::make_optional<Token>(TokenType::DEFAULT, std::string("default"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "while") {
						return std::make_pair(std::make_optional<Token>(TokenType::WHILE, std::string("while"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "for") {
						return std::make_pair(std::make_optional<Token>(TokenType::FOR, std::string("for"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "do") {
						return std::make_pair(std::make_optional<Token>(TokenType::DO, std::string("do"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "return") {
						return std::make_pair(std::make_optional<Token>(TokenType::RETURN, std::string("return"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "break") {
						return std::make_pair(std::make_optional<Token>(TokenType::BREAK, std::string("break"), pos, currentPos()), std::optional<CompilationError>());
					}
					else if (s == "continue") {
						return std::make_pair(std::make_optional<Token>(TokenType::CONTINUE, std::string("continue"), pos, currentPos()), std::optional<CompilationError>());
					}
					return std::make_pair(std::make_optional<Token>(TokenType::IDENTIFIER, s, pos, currentPos()), std::optional<CompilationError>());
				}
				break;
			}

								 // 如果当前状态是加号
			case PLUS_SIGN_STATE: {
				// 请思考这里为什么要回退，在其他地方会不会需要		多读了一个
				unreadLast(); // Yes, we unread last char even if it's an EOF.
				return std::make_pair(std::make_optional<Token>(TokenType::PLUS_SIGN, std::string("+"), pos, currentPos()), std::optional<CompilationError>());
			}
								// 当前状态为减号的状态
			case MINUS_SIGN_STATE: {
				// 请填空：回退，并返回减号token
				unreadLast(); // Yes, we unread last char even if it's an EOF.
				return std::make_pair(std::make_optional<Token>(TokenType::MINUS_SIGN, std::string("-"), pos, currentPos()), std::optional<CompilationError>());
			}

								 // 请填空：
								 // 对于其他的合法状态，进行合适的操作
								 // 比如进行解析、返回token、返回编译错误
			case MULTIPLICATION_SIGN_STATE: {				
				unreadLast();
				return std::make_pair(std::make_optional<Token>(TokenType::MULTIPLICATION_SIGN, std::string("*"), pos, currentPos()), std::optional<CompilationError>());
			}
			case DIVISION_SIGN_STATE: {
				if (!current_char.has_value()) {
					unreadLast();
					return std::make_pair(std::make_optional<Token>(TokenType::DIVISION_SIGN, std::string("/"), pos, currentPos()), std::optional<CompilationError>());
				}
				auto ch = current_char.value();
				if (ch == '*') {
					ss.clear();
					ss.str("");
					current_state = DFAState::LEFT_MULTI_COMMENT_STATE;					
				}
				else if (ch == '/') {
					ss.clear();
					ss.str("");
					current_state = DFAState::SINGLE_COMMENT_STATE;					
				}
				else {
					unreadLast();
					return std::make_pair(std::make_optional<Token>(TokenType::DIVISION_SIGN, std::string("/"), pos, currentPos()), std::optional<CompilationError>());
				}		
				break;
			}
			case EQUAL_STATE: {
				if (!current_char.has_value()) {
					unreadLast();
					return std::make_pair(std::make_optional<Token>(TokenType::EQUAL_SIGN, std::string("="), pos, currentPos()), std::optional<CompilationError>());
				}
				if (current_char.value() == '=') {
					return std::make_pair(std::make_optional<Token>(TokenType::TOTAL_EQUAL_SIGN, std::string("=="), pos, currentPos()), std::optional<CompilationError>());
				}
				unreadLast();
				return std::make_pair(std::make_optional<Token>(TokenType::EQUAL_SIGN, std::string("="), pos, currentPos()), std::optional<CompilationError>());
			}
			case LEFTBRACKET_STATE: {
				unreadLast();
				return std::make_pair(std::make_optional<Token>(TokenType::LEFT_BRACKET, std::string("("), pos, currentPos()), std::optional<CompilationError>());
			}
			case RIGHTBRACKET_STATE: {
				unreadLast();
				return std::make_pair(std::make_optional<Token>(TokenType::RIGHT_BRACKET, std::string(")"), pos, currentPos()), std::optional<CompilationError>());
			}
			case SEMICOLON_STATE: {
				unreadLast();
				return std::make_pair(std::make_optional<Token>(TokenType::SEMICOLON, std::string(";"), pos, currentPos()), std::optional<CompilationError>());
			}
			case MORE_STATE: {
				if (!current_char.has_value()) {
					unreadLast();
					return std::make_pair(std::make_optional<Token>(TokenType::MORE_SIGN, std::string(">"), pos, currentPos()), std::optional<CompilationError>());
				}
				if (current_char.value() == '=') {
					return std::make_pair(std::make_optional<Token>(TokenType::MORE_EQUAL_SIGN, std::string(">="), pos, currentPos()), std::optional<CompilationError>());
				}
				unreadLast();
				return std::make_pair(std::make_optional<Token>(TokenType::MORE_SIGN, std::string(">"), pos, currentPos()), std::optional<CompilationError>());
			}
			case LESS_STATE: {
				if (!current_char.has_value()) {
					unreadLast();
					return std::make_pair(std::make_optional<Token>(TokenType::LESS_SIGN, std::string("<"), pos, currentPos()), std::optional<CompilationError>());
				}
				if (current_char.value() == '=') {
					return std::make_pair(std::make_optional<Token>(TokenType::LESS_EQAUL_SIGN, std::string("<="), pos, currentPos()), std::optional<CompilationError>());
				}
				unreadLast();
				return std::make_pair(std::make_optional<Token>(TokenType::LESS_SIGN, std::string("<"), pos, currentPos()), std::optional<CompilationError>());
			}
			case NOT_EQUAL_SIGN_STATE:
				if (!current_char.has_value() || current_char.value() != '=') {
					return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidInput));
				}
				return std::make_pair(std::make_optional<Token>(TokenType::NOT_EQUAL_SIGN, std::string("!="), pos, currentPos()), std::optional<CompilationError>());
			case MAO_STATE:
				unreadLast();
				return std::make_pair(std::make_optional<Token>(TokenType::MAO_SIGN, std::string(":"), pos, currentPos()), std::optional<CompilationError>());
			case LEFT_MULTI_COMMENT_STATE:
				if (current_char.value() == '*') {
					current_char = nextChar();
					if (current_char.value() == '/') {
						current_state = DFAState::INITIAL_STATE;
					}
				}
				break;
			case SINGLE_COMMENT_STATE:
				if (current_char.value() == '\r' || current_char.value() == '\n') {
					unreadLast();
					current_state = DFAState::INITIAL_STATE;
				}
				break;
			case LEFT_MID_BRACKET_STATE:
				unreadLast();
				return std::make_pair(std::make_optional<Token>(TokenType::LEFT_MID_BRACKET, std::string("{"), pos, currentPos()), std::optional<CompilationError>());
			case RIGHT_MID_BRACKET_STATE:
				unreadLast();
				return std::make_pair(std::make_optional<Token>(TokenType::RIGHT_MID_BRACKET, std::string("}"), pos, currentPos()), std::optional<CompilationError>());
			//会读到引号结束
			case CHAR_LITERAL_STATE:
				//escape
				if (current_char.value() == '\\') {
					ss << current_char.value();
					current_char = nextChar();
					if (current_char.value() == 'r'
						|| current_char.value() == 'n'
						|| current_char.value() == 't'
						|| current_char.value() == '\''
						|| current_char.value() == '\"'
						|| current_char.value() == '\\') {
						ss << current_char.value();
						current_char = nextChar();
						if (current_char.value() == '\'') {
							//ret
							return std::make_pair(std::make_optional<Token>(TokenType::C_CHAR, ss.str().substr(1), pos, currentPos()), std::optional<CompilationError>());
						}
						else {
							return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidInput));
						}
					}
					//hexa
					else if (current_char == 'x') {
						ss << current_char.value();
						current_char = nextChar();
						if (miniplc0::isdigit(current_char.value()) 
							|| (current_char.value() >= 'a' 
							&& current_char.value() <= 'f')) {
							ss << current_char.value();
						}
						else {
							return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidInput));
						}
						current_char = nextChar();
						if (miniplc0::isdigit(current_char.value())
							|| (current_char.value() >= 'a'
								&& current_char.value() <= 'f')) {
							ss << current_char.value();
						}
						else {
							return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidInput));
						}
						current_char = nextChar();
						if (current_char.value() != '\'') {
							return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidInput));
						}
						return std::make_pair(std::make_optional<Token>(TokenType::C_CHAR, ss.str().substr(1), pos, currentPos()), std::optional<CompilationError>());
					}
					else {
						return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidInput));
					}
					
				}
				else {
					if (current_char.value() == '\''
						|| current_char.value() == '\\'
						|| current_char.value() == '\n'
						|| current_char.value() == '\r') {
						return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidInput));
					}
					ss << current_char.value();
					current_char = nextChar();
					if (current_char.value() != '\'') {
						return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidInput));
					}
					return std::make_pair(std::make_optional<Token>(TokenType::C_CHAR, ss.str().substr(1), pos, currentPos()), std::optional<CompilationError>());
				}
				break;
			case STRING_LITERAL_STATE:
				//escape
				if (current_char.value() == '\\') {
					ss << current_char.value();
					current_char = nextChar();
					if (current_char.value() == 'r'
						|| current_char.value() == 'n'
						|| current_char.value() == 't'
						|| current_char.value() == '\''
						|| current_char.value() == '\"'
						|| current_char.value() == '\\') {
						ss << current_char.value();
						current_char = nextChar();
						if (current_char.value() == '\"') {
							//ret
							return std::make_pair(std::make_optional<Token>(TokenType::S_CHAR, ss.str().substr(1), pos, currentPos()), std::optional<CompilationError>());
						}
						else {
							return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidInput));
						}
					}
					//hexa
					else if (current_char == 'x') {
						ss << current_char.value();
						current_char = nextChar();
						if (miniplc0::isdigit(current_char.value())
							|| (current_char.value() >= 'a'
								&& current_char.value() <= 'f')) {
							ss << current_char.value();
						}
						else {
							return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidInput));
						}
						current_char = nextChar();
						if (miniplc0::isdigit(current_char.value())
							|| (current_char.value() >= 'a'
								&& current_char.value() <= 'f')) {
							ss << current_char.value();
						}
						else {
							return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidInput));
						}
						current_char = nextChar();
						if (current_char.value() != '\"') {
							return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidInput));
						}
						return std::make_pair(std::make_optional<Token>(TokenType::S_CHAR, ss.str().substr(1), pos, currentPos()), std::optional<CompilationError>());
					}
					else {
						return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidInput));
					}

				}
				else {
					if (current_char.value() == '\"') {
						return std::make_pair(std::make_optional<Token>(TokenType::S_CHAR, ss.str().substr(1), pos, currentPos()), std::optional<CompilationError>());
					}
					if (current_char.value() == '\\'
						|| current_char.value() == '\n'
						|| current_char.value() == '\r') {
						return std::make_pair(std::optional<Token>(), std::make_optional<CompilationError>(pos, ErrorCode::ErrInvalidInput));
					}
					auto ch = current_char.value();
					ss << ch;
				}
				break;
			case COMMA_STATE:
				unreadLast();
				return std::make_pair(std::make_optional<Token>(TokenType::COMMA, std::string(","), pos, currentPos()), std::optional<CompilationError>());
			default:
				DieAndPrint("unhandled state.");
				break;
			}
		}
		// 预料之外的状态，如果执行到了这里，说明程序异常
		return std::make_pair(std::optional<Token>(), std::optional<CompilationError>());
	}

	std::optional<CompilationError> Tokenizer::checkToken(const Token& t) {
		switch (t.GetType()) {
		case IDENTIFIER: {
			auto val = t.GetValueString();
			if (miniplc0::isdigit(val[0]))
				return std::make_optional<CompilationError>(t.GetStartPos().first, t.GetStartPos().second, ErrorCode::ErrInvalidIdentifier);
			break;
		}
		default:
			break;
		}
		return {};
	}

	void Tokenizer::readAll() {
		if (_initialized)
			return;
		for (std::string tp; std::getline(_rdr, tp);) {
			_lines_buffer.emplace_back(std::move(tp + "\n"));
			//std::cout << tp << "\n";
		}
		_initialized = true;
		_ptr = std::make_pair<int64_t, int64_t>(0, 0);
		return;
	}

	// Note: We allow this function to return a postion which is out of bound according to the design like std::vector::end().
	std::pair<uint64_t, uint64_t> Tokenizer::nextPos() {
		if (_ptr.first >= _lines_buffer.size())
			DieAndPrint("advance after EOF");
		if (_ptr.second == _lines_buffer[_ptr.first].size() - 1)
			return std::make_pair(_ptr.first + 1, 0);
		else
			return std::make_pair(_ptr.first, _ptr.second + 1);
	}

	std::pair<uint64_t, uint64_t> Tokenizer::currentPos() {
		return _ptr;
	}

	std::pair<uint64_t, uint64_t> Tokenizer::previousPos() {
		if (_ptr.first == 0 && _ptr.second == 0)
			DieAndPrint("previous position from beginning");
		if (_ptr.second == 0)
			return std::make_pair(_ptr.first - 1, _lines_buffer[_ptr.first - 1].size() - 1);
		else
			return std::make_pair(_ptr.first, _ptr.second - 1);
	}

	std::optional<char> Tokenizer::nextChar() {
		if (isEOF())
			return {}; // EOF
		auto result = _lines_buffer[_ptr.first][_ptr.second];
		_ptr = nextPos();
		return result;
	}

	bool Tokenizer::isEOF() {
		return _ptr.first >= _lines_buffer.size();
	}

	// Note: Is it evil to unread a buffer?
	void Tokenizer::unreadLast() {
		_ptr = previousPos();
	}
}