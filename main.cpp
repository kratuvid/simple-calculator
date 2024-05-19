#include <iostream>
#include <string>
#include <sstream>
#include <cctype>
#include <string_view>
#include <cstring>
#include <exception>
#include <regex>
#include <vector>
#include <stack>
#include <array>
#include <cmath>

#include <readline/readline.h>

#include "utility.hpp"

class SimpleCalculator
{
public:
	using number_t = long double;
	enum class operator_t
	{
		add, subtract, multiply, divide,
		stack, current, quit, set, clear,
		zero, one,
		help,
		sin, cos
	};
	union element_t {
		number_t n;
		operator_t o;
	};

private:
	static constexpr size_t operations_size = 14;
	static constexpr std::array<std::string_view, operations_size> operations {
		"+", "-", "*", "/",
		"stack", "current", "quit", "set", "clear",
		"zero", "one",
		"help",
		"sin", "cos"
	};
	static constexpr std::array<unsigned, operations_size> operand_size {
		1, 1, 1, 1,
		0, 0, 0, 1, 0,
		0, 0,
		0,
		0, 0
	};
	
	std::vector<std::pair<element_t, bool>> elements;
	number_t current = 0;

private:
	void show_help(char* name)
	{
		std::cerr << name << ": Arbitrary length calculator" << std::endl
				  << "\t-h, --help: Show this" << std::endl
				  << "\t-e, --expr=[EXPRESSION]: Calculates EXPRESSION and quits"
				  << "\t-r, --repl: Start the REPL"
				  << std::endl;
	}
	
	void parse_arguments(int argc, char** argv)
	{
		if (argc == 1)
		{
			repl();
		}
		else
		{
			for (int i=1; i < argc; i++)
			{
				if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
				{
					show_help(argv[0]);
					throw sc::exception("", sc::error_type::init_help);
				}
			
				else if (strncmp(argv[i], "--expr", 2+4) == 0 || strncmp(argv[i], "-e", 1+1) == 0)
				{
					std::regex reg(R"(--expr=(.+))"), reg2(R"(-e=(.+))");
					std::cmatch match;
					if (std::regex_match(argv[i], match, reg) || std::regex_match(argv[i], match, reg2))
					{
						auto e = match[1].str();
						expr(e);
						std::cout << current << std::endl;
					}
					else
					{
						throw sc::exception("Please supply an expression to calculate", sc::error_type::init);
					}
				}

				else if (strcmp(argv[i], "--repl") == 0 || strcmp(argv[i], "-r") == 0)
				{
					repl();
					return;
				}
			
				else
				{
					std::ostringstream oss;
					oss << "Unknown argument: '" << argv[i] << "'";
					show_help(argv[0]);
					throw sc::exception(oss.str(), sc::error_type::init);
				}
			}
		}
	}

	void perform_operation(operator_t operation, std::stack<number_t>& operands)
	{
		auto& result = current;
		
		switch (operation)
		{
		case operator_t::add: {			
			auto o = operands.top();
			operands.pop();
			result += o;
		}
			break;
			
		case operator_t::subtract: {
			auto o = operands.top();
			operands.pop();
			result -= o;
		}
			break;
			
		case operator_t::multiply: {
			auto o = operands.top();
			operands.pop();
			result *= o;
		}
			break;
			
		case operator_t::divide: {
			auto o = operands.top();
			operands.pop();
			if (o == 0)
				throw sc::exception("Cannot divide by 0", sc::error_type::expr_divide_by_zero);
			result /= o;
		}
			break;

		case operator_t::stack: {
			std::cout << "Stack size: " << elements.size() << std::endl;
			for (int i = elements.size()-1; i >= 0; i--)
			{
				const auto& e = elements[i];
				if (e.second)
					std::cout << "N: " << e.first.n;
				else
					std::cout << "O: " << operations[static_cast<int>(e.first.o)];
				std::cout << std::endl;
			}
		}
			break;
			
		case operator_t::current: {
			std::cout << current << std::endl;
		}
			break;
			
		case operator_t::quit: {
			throw sc::exception("", sc::error_type::repl_quit);
		}
			break;
			
		case operator_t::set: {
			auto o = operands.top();
			operands.pop();
			current = o;
		}
			break;
			
		case operator_t::clear: {
			elements.clear();
		}
			break;
			
		case operator_t::zero: {
			current = 0.0l;
		}
			break;
			
		case operator_t::one: {
			current = 1.0l;
		}
			break;
			
		case operator_t::help: {
			std::cerr << R"(Operation: operand size: description:
-------------------------------------
+: 1: current = current + operand
-: 1: current = current - operand
*: 1: current = current * operand
/: 1: current = current / operand
sin: 1: current = sin(current)
cos: 1: current = cos(current)
stack: 0: list the stack
current: 0: display the current value
quit: 0: quit the REPL
set: 1: current = operand
clear: 0: empty the stack
zero: 0: current = 0
one: 0: current = 1
help: 0: show this screen)" << std::endl;
		}
			break;
			
		case operator_t::sin: {
			current = std::sin(current);
		}
			break;
			
		case operator_t::cos: {
			current = std::cos(current);
		}
			break;
		}
	}

	void reverse_polish()
	{
		unsigned i = 0;
		while (i < elements.size())
		{
			auto e = elements.back();

			if (!e.second)
			{
				elements.pop_back();
			
				int i = static_cast<int>(e.first.o);
				if (elements.size() < operand_size[i])
				{
					std::ostringstream oss;
					oss << "Operation '" << operations[i] << "' requires "
						<< operand_size[i] << " elements but only "
						<< elements.size() << " are left";
					throw sc::exception(oss.str(), sc::error_type::expr);
				}
				else
				{
					std::stack<number_t> operands;
					for (unsigned j=0; j < operand_size[i]; j++)
					{
						auto e = elements.back();
						elements.pop_back();
						i++;
						
						if (!e.second)
						{
							std::ostringstream oss;
							oss << "Unexpected operation '" << operations[static_cast<int>(e.first.o)]
								<< "' while processing current operation '" << operations[i]
								<< "'";
							throw sc::exception(oss.str(), sc::error_type::expr);
						}
						else
						{
							operands.push(e.first.n);
						}
					}
					
					perform_operation(e.first.o, operands);
				}
			}

			else
			{
			}

			i++;
		}
	}
	
	void expr(std::string_view what)
	{
		std::vector<std::string> subs;
		{
			std::string tmp;
			for (char c : what)
			{
				if (isspace(c))
				{
					if (!tmp.empty())
						subs.push_back(std::move(tmp));
				}
				else
				{
					tmp += c;
				}
			}
			if (!tmp.empty())
				subs.push_back(std::move(tmp));
		}

		bool okay = true;
		for (const auto& sub : subs)
		{
			okay = true;
			auto elem {std::make_pair(static_cast<element_t>(0), false)};
			
			elem.second = false;
			bool is_op = false;
			for (unsigned i=0; i < operations.size(); i++)
			{
				if (sub == operations[i])
				{
					elem.first.o = static_cast<operator_t>(i);
					is_op = true;
					break;
				}
			}

			if (!is_op)
			{
				try
				{
					elem.second = true;
					elem.first.n = std::stold(sub);
				}
				catch (const std::out_of_range& e) {
					okay = false;
				}
				catch (const std::invalid_argument& e) {
					okay = false;
				}
			}

			if (okay)
				elements.push_back(std::move(elem));
			else
			{
				std::ostringstream oss; 
				oss << "Garbage sub-expression: '" << sub << "'";
				throw sc::exception(oss.str(), sc::error_type::expr);
			}
		}

		reverse_polish();
	}
	
	void repl()
	{
	  begin:
		try
		{
			while (true)
			{
				if (false)
				{					
					std::string what;

					std::cerr << "> ";
					std::getline(std::cin, what);
					
					expr(what);
				}
				else
				{
					char* what = nullptr;
					try {
						what = readline("> ");
						if (!what) throw sc::exception("", sc::error_type::repl_quit);
							
						expr(what);
					} catch (...) {
						if (what) free(what);
						throw;
					}
					if (what) free(what);
				}
				
				std::cout << current << std::endl;
			}
		}
		catch (const sc::exception& e)
		{
			std::ostringstream oss;
			oss << "Error: ";
			oss << sc::error_type_str[static_cast<int>(e.type)] << ": ";
			oss << e.what();
			
			switch (e.type)
			{
			case sc::error_type::expr_divide_by_zero:
			case sc::error_type::expr:
				break;
			case sc::error_type::repl_quit:
				return;
			default:
				throw;
			}
			
			std::cerr << oss.str() << std::endl;
			
			goto begin;
		}
	}

public:
	SimpleCalculator(int argc, char** argv)
	{
		parse_arguments(argc, argv);
	}
};

int main(int argc, char** argv)
{
	try
	{
		SimpleCalculator sc(argc, argv);
	}
	catch (const sc::exception& e)
	{
		if (e.type == sc::error_type::init_help)
			return 0;
		
		std::ostringstream oss;
		oss << "Fatal exception: ";
		oss << sc::error_type_str[static_cast<int>(e.type)] << ": ";
		std::cerr << oss.str() << e.what() << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cerr << "Fatal standard exception";
		if (e.what()[0] != '\0')
			std::cerr << ": " << e.what();
		std::cerr << std::endl;
		return 1;
	}
}
