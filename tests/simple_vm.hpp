#pragma once

#include "instruction/instruction.h"

#include <array>
#include <vector>
#include <cstdint>
#include <string>
#include <climits>

namespace miniplc0 {
	// This is a simplified version of miniplc0 vm implementation for testing.
	// include error handling??
	class VM {
	private:
		using uint64_t = std::uint64_t;
		using int32_t = std::int32_t;
		using int64_t = std::int64_t;
	public:
		VM(std::vector<Instruction> v) : _codes(std::move(v)), _stack(2048, 0), _ip(0), _sp(0), _bp(0) {}
		VM(const VM&) = delete;
		VM(VM&&) = delete;
		VM& operator=(VM) = delete;

		// If it crashes, let it crash.
		std::vector<int32_t> Run() {
			std::vector<int32_t> v;
			for (auto& it : _codes) {
				auto x = it.GetX();
				auto index = it.GetIndex();
				auto offset = it.GetOffset();
				auto level_diff = it.GetLevelDiff();
				auto cnt = it.GetCnt();
				auto addr = it.GetAddr();
				Operation opr = it.GetOperation();
				switch (opr)
				{
				case miniplc0::nop:
					throw std::out_of_range("nop");
					break;
					//problems
					//沿SL链向前移动level_diff次（移动到当前栈帧层次差为level_diff的栈帧中），加载该栈帧中栈偏移为offset的内存的栈地址值address。
				case miniplc0::loada:
					v.emplace_back(_sp + level_diff * 2048 + offset);
					break;
				//加载常量里index的常量
				case miniplc0::loadc:
					_stack[_sp] = _consts.at(offset);
					break;
				case miniplc0::bipush:
					_stack[_sp] = x;
					_sp += 1;
					break;
				case miniplc0::ipush:
					_stack[_sp] = x;
					_sp += 1;
					break;
				case miniplc0::iload:
					_stack[_sp] = _stack[addr];
					break;
				case miniplc0::dload:
					_stack[_sp] = _stack[addr];
					break;
				case miniplc0::aload:
					break;
				case miniplc0::istore:
					break;
				case miniplc0::dstore:
					break;
				case miniplc0::cstore:
					break;
				case miniplc0::astore:
					break;
				case miniplc0::iaload:
					break;
				case miniplc0::daload:
					break;
				case miniplc0::aaload:
					break;
				case miniplc0::iastore:
					break;
				case miniplc0::dastore:
					break;
				case miniplc0::aastore:
					break;
				case miniplc0::New:
					cnt = _stack[_sp--];
					for (int i = 0; i < cnt; i++) {
						_stack[_sp++] = 0;
					}
					_stack[_sp] = _sp;
					break;
				case miniplc0::snew:
					for (int i = 0; i < cnt; i++) {
						//不被初始化为0
						_stack[_sp++] = 0xff;
					}
					break;
				case miniplc0::pop:
					//pop n
					for (int i = 0; i < cnt; i++) {
						v.emplace_back(_stack[_sp-1]);
						_sp--;
					}
					break;
				case miniplc0::iadd:
					break;
				case miniplc0::dadd:
					break;
				case miniplc0::isub:
					break;
				case miniplc0::dsub:
					break;
				case miniplc0::imul:
					break;
				case miniplc0::dmul:
					break;
				case miniplc0::idiv:
					break;
				case miniplc0::ddiv:
					break;
				case miniplc0::ineg:
					break;
				case miniplc0::dneg:
					break;
				case miniplc0::icmp:
					break;
				case miniplc0::dcmp:
					break;
				case miniplc0::i2d:
					break;
				case miniplc0::i2c:
					break;
				case miniplc0::d2i:
					break;
				case miniplc0::je:
					break;
				case miniplc0::jne:
					break;
				case miniplc0::jl:
					break;
				case miniplc0::jge:
					break;
				case miniplc0::jg:
					break;
				case miniplc0::jle:
					break;
				case miniplc0::jmp:
					break;
				case miniplc0::call:
					break;
				case miniplc0::ret:
					break;
				case miniplc0::iret:
					break;
				case miniplc0::dret:
					break;
				case miniplc0::aret:
					break;
				case miniplc0::iprint:
					break;
				case miniplc0::dprint:
					break;
				case miniplc0::cprint:
					break;
				case miniplc0::sprint:
					break;
				case miniplc0::printl:
					break;
				case miniplc0::iscan:
					break;
				case miniplc0::dscan:
					break;
				case miniplc0::cscan:
					break;
				
				case miniplc0::dup:
					_stack[++_sp] = _stack[_sp];
					break;
				case miniplc0::dup2:
					_stack[++_sp] = _stack[_sp];
					_stack[++_sp] = _stack[_sp];
					break;
				default:
					break;
				}
			}
			return v;
		}
	private:
		int32_t add(int32_t lhs, int32_t rhs) {
			int64_t r = (int64_t)lhs + (int64_t)rhs;
			if (r < INT_MIN || r > INT_MAX)
				throw std::out_of_range("addition out of range");
			return lhs + rhs;
		}

		int32_t sub(int32_t lhs, int32_t rhs) {
			int64_t r = (int64_t) lhs - (int64_t) rhs;
			if (r < INT_MIN || r > INT_MAX)
				throw std::out_of_range("subtraction out of range");
			return lhs - rhs;
		}

		// See CSAPP Chapter.2
		int32_t mul(int32_t lhs, int32_t rhs) {
			int32_t r = lhs * rhs;
			if (!lhs || r / lhs == rhs)
				return r;
			else
				throw std::out_of_range("multiplication out of range");
		}

		int32_t div(int32_t lhs, int32_t rhs) {
			if (rhs == 0)
				throw std::out_of_range("divide by zero");
			if (rhs == -1 && lhs == INT_MIN)
				throw std::out_of_range("INT_MIN/-1");
			else
				return lhs / rhs;
		}

	private:
		std::vector<Instruction> _codes;
		std::vector<int32_t> _stack;
		std::vector<int32_t> _heap;
		uint64_t _ip;
		uint64_t _sp;
		uint64_t _bp;
		std::vector<int32_t> _consts;
		uint64_t _prev;
	};
}