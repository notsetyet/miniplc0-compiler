#pragma once

#include <cstdint>
#include <utility>

namespace miniplc0 {

	enum Operation {
		//loada level_diff(2), offset(4)
		loada,
		//常量加载	loadc index(2)
		loadc,
		//push byte
		bipush,
		//push int
		ipush,
		
		//基于地址的内存读写
		iload,
		dload,
		aload,
		istore,
		dstore,
		cstore,
		astore,
		//数组内存
		iaload,
		daload,
		aaload,
		iastore,
		dastore,
		aastore,
		//heap 
		New,
		//stack	snew count(4)
		snew,
		//注意可以pop多个slot
		pop,
		iadd,
		dadd,
		isub,
		dsub,
		imul,
		dmul,
		idiv,
		ddiv,
		ineg,
		dneg,
		icmp,
		dcmp,
		//强制转换
		i2d,
		i2c,
		d2i,
		/*je：value是0
		jne：value不是0
		jl：value是负数
		jge：value不是负数
		jg：value是正数
		jle：value不是正数*/
		je,
		jne,
		jl,
		jge,
		jg,
		jle,
		jmp,
		call,
		ret,
		iret,
		dret,
		aret,
		iprint,
		dprint,
		cprint,
		sprint,
		//'\n'
		printl,
		iscan,
		dscan,
		cscan,

		nop,
		dup,
		dup2,
	};
	enum RuntimeErr {
		InvalidFile,
		MainNotFound,
		StackOverflow,
		HeapOverflow,
		InvalidMemoryAcc,
		InvalidInstruction,
		DivByZero,
		InvalidTransfer,
		IOErr
	};
	
	class Instruction final {
	private:
		using int32_t = std::int32_t;
		using uint16_t = std::uint16_t;
		using uint32_t = std::uint32_t;
	public:
		friend void swap(Instruction& lhs, Instruction& rhs);
	public:
		Instruction():_opr(Operation::nop){}
		Instruction(Operation opr):_opr(opr),_cnt(1){}
		Instruction(Operation opr, int32_t x) : _opr(opr), _x(x) {}
		Instruction(Operation opr,uint16_t index):_opr(opr),_index(index){}
		Instruction(Operation opr,uint32_t cnt):_opr(opr),_cnt(cnt){}
		Instruction(Operation opr,uint16_t level_diff,int32_t offset):_opr(opr),_offset(offset){}

		Instruction(const Instruction& i) { _opr = i._opr; _x = i._x; }
		Instruction(Instruction&& i) :Instruction() { swap(*this, i); }
		Instruction& operator=(Instruction i) { swap(*this, i); return *this; }
		bool operator==(const Instruction& i) const { return _opr == i._opr && _x == i._x; }

		Operation GetOperation() const { return _opr; }
		int32_t GetX() const { return _x; }
		int32_t GetOffset() const { return _offset; }
		uint16_t GetLevelDiff() const { return _level_diff; }
		uint32_t GetCnt() const { return _cnt; }
		uint16_t GetIndex() const { return _index; }
		uint64_t GetAddr() const { return _addr; }
	private:
		Operation _opr;
		int32_t _x;
		int32_t _offset;
		uint16_t _level_diff;
		uint32_t _cnt;
		uint16_t _index;
		uint64_t _addr;
	};

	inline void swap(Instruction& lhs, Instruction& rhs) {
		using std::swap;
		swap(lhs._opr, rhs._opr);
		swap(lhs._x, rhs._x);
	}

}