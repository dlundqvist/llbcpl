// Copyright 2017 Daniel Lundqvist. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace {

// Types and sizes for INTCODE machine
using word = std::int16_t;
using uword = std::make_unsigned<word>::type;
constexpr auto bytesize {8};
constexpr auto wordsize{sizeof(word) * bytesize};
static_assert(wordsize >= 16, "Word size must be at least 16 bits");
constexpr auto bytesperword {wordsize / bytesize};

// Instructions
const uword l_op {0}; // Load
const uword s_op {1}; // Store
const uword a_op {2}; // Add
const uword j_op {3}; // Jump
const uword t_op {4}; // Jump true
const uword f_op {5}; // Jump false
const uword k_op {6}; // Call
const uword x_op {7}; // Execute operation

// Instruction format
constexpr auto f_shift {wordsize - 3u};		
constexpr auto i_bit   {static_cast<uword>(1) << (f_shift - 1)};
constexpr auto p_bit   {i_bit >> 1u};
constexpr auto g_bit   {p_bit >> 1u};
constexpr auto d_bit   {g_bit >> 1u};
constexpr auto d_mask  {d_bit - 1u};

// Store, register and I/O streams for INTCODE machine.
struct machine {
	std::vector<uword> S			= std::vector<uword>(32 * 1024 - 1);
	uword P							{512};
	uword G							{};
	word A							{};
	word B							{};
	uword C							{P};
	uword D							{};
	std::vector<std::FILE *> files	{NULL, stdin, stdout, stderr};
	uword selected_input			{1};
	uword selected_output			{2};
};

// INTCODE assembler.
struct assembler {
	machine &M;
	std::basic_istream<char> &I;
	std::vector<word> L			= std::vector<word>(512);
	int CH						{};
	uword CP					{};
};

// Set label N to current value of register P.
// Before updating label, resolve references to it.
// Unresolved references are kept in a list.
void
setlab(assembler &A, word N)
{
	word K = A.L[static_cast<uword>(N)];
	if (K < 0) {
		std::cerr << "L" << N << " ALREADY SET TO " << -K << " AT P = " << A.M.P << "\n";
	}
	while (K > 0) {
		word D = static_cast<word>(A.M.S[static_cast<uword>(K)]);
		A.M.S[static_cast<uword>(K)] = A.M.P;
		K = D;
	}
	A.L[static_cast<uword>(N)] = -static_cast<word>(A.M.P);
}

// Reference label N at address D.
// If label is unset, append to D to
// list of locations to patch when N is set.
void
labref(assembler &A, word N, uword D)
{
	word K = A.L[static_cast<uword>(N)];
	if (K < 0) {
		K = -K;
	} else {
		A.L[static_cast<uword>(N)] = static_cast<word>(D);
	}
	A.M.S[D] += static_cast<uword>(K);
}

// Read next character.
// If read character is '/', skips until next line.
void
rch(assembler &A)
{
	while (1) {
		A.CH  = A.I.get();
		if (A.CH != '/') {
			return;
		}
		while (A.CH != '\n') {
			A.CH = A.I.get();
		}
	}
}

word
rdn(assembler &A)
{
	word N{};
	bool negative{};
	if (A.CH == '-') {
		negative = true;
		rch(A);
	}
	while ('0' <= A.CH && A.CH <= '9') {
		N = N * 10 + static_cast<word>(A.CH - '0');
		rch(A);
	}
	return negative ? -N : N;
}

void
stw(assembler &A, uword W)
{
	A.M.S[A.M.P++] = static_cast<uword>(W);
	A.CP = 0;
}

void
stc(assembler &A, word C)
{
	if (!A.CP) {
		stw(A, 0);
		A.CP = wordsize;
	}
	A.CP -= bytesize;
	A.M.S[A.M.P - 1] += static_cast<uword>(C << A.CP);
}

void
sts(assembler &A, std::string const &s)
{
	stc(A, static_cast<word>(s.size()));
	for (auto const &c : s) {
		stc(A, c);
	}

}

machine
assemble(std::basic_istream<char> &is)
{
	machine M;
	assembler A {M, is};
	uword F = 0;

	A.I >> std::noskipws;

	// Startup code
	// - Jump to global function 1
	// - Exit
	stw(A, (l_op << f_shift)|i_bit|g_bit|1);
	stw(A, (k_op << f_shift)|2);
	stw(A, (x_op << f_shift)|22);

	// Add panic handler for calling unset global function.
	setlab(A, 1);
	stw(A, (l_op << f_shift)|d_bit);
	stw(A, 0u);
	labref(A, 2, A.M.P - 1);
	stw(A, (x_op << f_shift)|38);
	setlab(A, 2);
	sts(A, "Unset global called!");
	// Set all global pointers to panic handler
	for (auto i = 0u; i < 512; ++i) {
		A.M.S[A.M.G + i] = static_cast<uword>(-A.L[1]);
	}

clear:
	std::fill(A.L.begin(), A.L.end(), 0);
	A.CP = 0;
next:
	rch(A);
sw:
	switch (A.CH) {
		default:
			if (A.CH == std::basic_istream<char>::traits_type::eof()) {
				return A.M;
			}
			std::cerr << "\nBAD CH " << static_cast<char>(A.CH) << "AT P = " << A.M.P << "\n";
			goto next;
		// Label
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9': {
			setlab(A, rdn(A));
			A.CP = 0;
			goto sw;
		// "whitespace"
		case '$': case ' ': case '\n':
			goto next;
		// Instuctons
		case 'L':
			F = l_op;
			break;
		case 'S':
			F = s_op;
			break;
		case 'A':
			F = a_op;
			break;
		case 'J':
			F = j_op;
			break;
		case 'T':
			F = t_op;
			break;
		case 'F':
			F = f_op;
			break;
		case 'K':
			F = k_op;
			break;
		case 'X':
			F = x_op;
			break;
		// Character
		case 'C':
			rch(A);
			stc(A, rdn(A));
			goto sw;
		// Data
		case 'D' :
			rch(A);
			if (A.CH == 'L') {
				rch(A);
				stw(A, 0u);
				labref(A, rdn(A), A.M.P - 1);
			} else {
				stw(A, static_cast<uword>(rdn(A)));
			}
			goto sw;
		case 'G': {
			rch(A);
			uword Ad = static_cast<uword>(rdn(A)) + A.M.G;
			if (A.CH == 'L') {
				rch(A);
			} else {
				std::cerr << "\nBAD CODE AT P = " << A.M.P << "\n";
			}
			A.M.S[Ad] = 0;
			labref(A, rdn(A), Ad);
			goto sw;
		}
		case 'Z':
			for (auto i = 0u; i < A.L.size(); ++i) {
				if (A.L[i] > 0) {
					std::cerr << "L" << i << " UNSET\n";
				}
			}
			goto clear;
		}
	}
	uword W = static_cast<uword>(F << f_shift);
	rch(A);
	if (A.CH == 'I') {
		W |= i_bit;
		rch(A);
	}
	if (A.CH == 'P') {
		W |= p_bit;
		rch(A);
	}
	if (A.CH == 'G') {
		W |= g_bit;
		rch(A);
	}
	if (A.CH == 'L') {
		rch(A);
		stw(A, W | d_bit);
		stw(A, 0u);
		labref(A, rdn(A), A.M.P - 1);
	} else {
		uword D = static_cast<uword>(rdn(A));
		if ((D & d_mask) == D) {
			stw(A, W | D);
		} else {
			stw(A, W | d_bit);
			stw(A, D);
		}
	}
	goto sw;
}

#if 0
std::string idecode(machine &M) {
	static char const * const instrs[] = {"L", "S", "A", "J", "T", "F", "K", "X"};

	uword W = M.S[M.C];
	std::string s = instrs[(W >> f_shift) & 0x7];
	s += W & i_bit ? "I" : "";
	s += W & p_bit ? "P" : "";
	s += W & g_bit ? "G" : "";
	if (W & d_bit) {
		s += std::to_string(M.S[M.C + 1]);
	} else {
		s += std::to_string(W & d_mask);
	}
	return s;
}
#endif

word
icgetbyte(machine &M, uword S, uword I) {
	uint8_t *C = reinterpret_cast<uint8_t *>(&M.S[S + I / bytesperword]);
	I = I & (bytesperword - 1);
	return C[bytesperword - 1 - I] & 0xff;
}

void
icputbyte(machine &M, uword S, uword I, uword CH) {
	uint8_t *C = reinterpret_cast<uint8_t *>(&M.S[S + I / bytesperword]);
	I = I & (bytesperword - 1);
	C[bytesperword - 1 - I] = CH & 0xff;
}

std::string
stringbcpl(machine &M, uword A) {
	auto l = static_cast<std::size_t>(icgetbyte(M, A, 0));
	std::string s(l, 0);
	for(uword i = 0u; i < l; ++i) {
		s[i] = static_cast<char>(icgetbyte(M, A, i + 1));
	}
	return s;
}

word
findslot(machine &M, FILE *f)
{
	auto it = std::find(M.files.begin() + 1, M.files.end(), nullptr);
	if (it == M.files.end()) {
		M.files.push_back(f);
		return static_cast<word>(M.files.size() - 1);
	} else {
		*it = f;
		return static_cast<word>(std::distance(M.files.begin(), it));
	}
}

word
findinput(machine &M, std::string input) {
	if (input == "SYSIN") {
		return 1;
	}

	auto f = std::fopen(input.c_str(), "r");
	if (!f) {
		return 0;
	}

	return findslot(M, f);
}

word
findoutput(machine &M, std::string output) {
	if (output == "SYSPRINT") {
		return 2;
	} else if (output == "SYSERROR") {
		return 3;
	}

	auto f = std::fopen(output.c_str(), "w");
	if (!f) {
		return 0;
	}

	return findslot(M, f);
}

void
panic(machine &M, std::string s) {
	std::cerr << "PANIC C" << std::dec << M.S[M.P + 1] - 1 << " P" << M.S[M.P] << ": " << s << '\n';
	std::exit(1);
}

word
interpret(machine &M)
{
	while (1) {
		//std::cerr << std::dec << M.C << ' ' << idecode(M) << '\n';
		uword W = M.S[M.C++];

		if (W & d_bit) {
			M.D = M.S[M.C++];
		} else {
			M.D = W & d_mask;
		}

		if (W & p_bit) {
			M.D += M.P;
		}

		if (W & g_bit) {
			M.D += M.G;
		}

		if (W & i_bit) {
			M.D = M.S[M.D];
		}

		switch ((W >> f_shift) & 0x7) {
			case l_op:
				M.B = M.A;
				M.A = static_cast<word>(M.D);
				break;
			case s_op:
				M.S[M.D] = static_cast<uword>(M.A);
				break;
			case a_op:
				M.A = M.A + static_cast<word>(M.D);
				break;
			case j_op:
				M.C = M.D;
				break;
			case t_op:
				if (M.A) {
					M.C = M.D;
				}
				break;
			case f_op:
				if (!M.A) {
					M.C = M.D;
				}
				break;
			case k_op:
				M.D = M.P + M.D;
				M.S[M.D + 0] = M.P;
				M.S[M.D + 1] = M.C;
				M.P = M.D;
				M.C = static_cast<uword>(M.A);
				break;
			case x_op: {
				switch (M.D) {
					case 1:
						M.A = static_cast<word>(M.S[static_cast<uword>(M.A)]);
						break;
					case 2:
						M.A = -M.A;
						break;
					case 3:
						M.A = ~M.A;
						break;
					case 4:
						M.C = M.S[M.P + 1];
						M.P = M.S[M.P + 0];
						break;
					case 5:
						M.A = M.B * M.A;
						break;
					case 6:
						M.A = M.B / M.A;
						break;
					case 7:
						M.A = M.B % M.A;
						break;
					case 8:
						M.A = M.B + M.A;
						break;
					case 9:
						M.A = M.B - M.A;
						break;
					case 10:
						M.A = M.B == M.A ? ~0 : 0;
						break;
					case 11:
						M.A = M.B != M.A ? ~0 : 0;
						break;
					case 12:
						M.A = M.B < M.A ? ~0 : 0;
						break;
					case 13:
						M.A = M.B >= M.A ? ~0 : 0;
						break;
					case 14:
						M.A = M.B > M.A ? ~0 : 0;
						break;
					case 15:
						M.A = M.B <= M.A ? ~0 : 0;
						break;
					case 16:
						M.A = static_cast<word>(M.B << M.A);
						break;
					case 17:
						M.A = M.B >> M.A;
						break;
					case 18:
						M.A = M.B & M.A;
						break;
					case 19:
						M.A = M.B | M.A;
						break;
					case 20:
						M.A = M.B ^ M.A;
						break;
					case 21:
						M.A = ~M.B ^ M.A;
						break;
					case 22:
						return 0;
					case 23:
						M.B = static_cast<word>(M.S[M.C + 0]);
						M.D = M.S[M.C + 1];
						while (M.B != 0) {
							M.B = M.B - 1;
							M.C = M.C + 2;
							if (M.A == static_cast<word>(M.S[M.C + 0])) {
								M.D = M.S[M.C + 1];
								break;
							}
						}
						M.C = M.D;
						break;
					case 24:
						M.selected_input = static_cast<uword>(M.A);
						break;
					case 25:
						M.selected_output = static_cast<uword>(M.A);
						break;
					case 26:
						M.A = static_cast<word>(std::fgetc(M.files[M.selected_input]));
						break;
					case 27:
						std::fprintf(M.files[M.selected_output], "%c", static_cast<char>(M.A));
						break;
					case 28:
						M.A = findinput(M, stringbcpl(M, static_cast<uword>(M.A)));
						break;
					case 29:
						M.A = findoutput(M, stringbcpl(M, static_cast<uword>(M.A)));
						break;
					case 30:
						return M.A;
					case 31:
						M.A = static_cast<word>(M.S[M.P]);
						break;
					case 32:
						M.P = static_cast<uword>(M.A);
						M.C = static_cast<uword>(M.B);
						break;
					case 33:
						if (M.selected_input) {
							std::fclose(M.files[M.selected_input]);
							M.files[M.selected_input] = nullptr;
						}
						M.selected_input = 0;
						break;
					case 34:
						if (M.selected_output) {
							std::fclose(M.files[M.selected_output]);
							M.files[M.selected_output] = nullptr;
						}
						M.selected_output = 0;
						break;
					case 35:
						M.D = static_cast<uword>(M.P + static_cast<uword>(M.B) + 1);
						M.S[M.D + 0] = M.S[M.P + 0];
						M.S[M.D + 1] = M.S[M.P + 1];
						M.S[M.D + 2] = M.P;
						M.S[M.D + 3] = static_cast<uword>(M.B);
						M.P = M.D;
						M.C = static_cast<uword>(M.A);
						break;
					case 36:
						M.A = icgetbyte(M, static_cast<uword>(M.A), static_cast<uword>(M.B));
						break;
					case 37:
						icputbyte(M, static_cast<uword>(M.A), static_cast<uword>(M.B), M.S[M.P + 4]);
						break;
					case 38:
						panic(M, stringbcpl(M, static_cast<uword>(M.A)));
						break;
				}
			}
		}
	}
}
}

int
main(int argc, char const *argv[]) {
	if (argc < 2) {
		std::cerr << "ERROR: Missing source filename\n";
		std::exit(1);
	}

	std::ifstream source(argv[1]);
	if (!source) {
		std::cerr << "ERROR: Unable to open source '" << argv[1] << "'\n";
		std::exit(1);
	}

	auto M = assemble(source);
	interpret(M);
}
