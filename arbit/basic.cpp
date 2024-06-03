#include "arbit.hpp"
#include <sstream>

namespace wc
{
	void arbit::zero()
	{
		if (fixed_len > 1)
			shrink(fixed_len - 1);
		else if (fixed_len == 0)
			grow(1);
		fixed_ptr[0] = 0;

		if (decimal_len > 0)
		{
			if (decimal_len > 1)
				shrink(decimal_len - 1);
			decimal_ptr[0] = 0;
		}
	}

	bool arbit::is_zero() const
	{
		for (size_t i=0; i < fixed_len; i++)
			if (fixed_ptr[i] != 0)
				return false;
		for (size_t i=0; i < decimal_len; i++)
			if (decimal_ptr[i] != 0)
				return false;
		return true;
	}

	bool arbit::is_negative() const
	{
		if (fixed_len > 0)
			return fixed_ptr[fixed_len-1] >> (base_bits - 1);
		return false;
	}

	bool arbit::is_negative(base_t n)
	{
		return n >> (base_bits - 1);
	}

	void arbit::shift_right_units(size_t n)
	{
		if (n == 0 || n > fixed_len) return;

		const auto old_decimal_len = decimal_len;

		grow_decimal(n);

		for (ssize_t i = old_decimal_len-1; i >= 0; i--)
		{
			const ssize_t dest = i + ssize_t(n);
			decimal_ptr[dest] = decimal_ptr[i];
		}

		for (size_t i=0; i < n; i++)
		{
			const size_t dest = n - 1 - i;
			decimal_ptr[dest] = fixed_ptr[i];
		}

		for (size_t i=n; i < fixed_len; i++)
		{
			const size_t dest = i - n;
			fixed_ptr[dest] = fixed_ptr[i];
		}

		shrink(n);
	}
	
	void arbit::shrink_if_can_raw(bool fixed_not_decimal)
	{
		const auto len = fixed_not_decimal ? fixed_len : decimal_len;
		const auto ptr = fixed_not_decimal ? fixed_ptr : decimal_ptr;

		const auto neg = is_negative();
		const auto check = neg && fixed_not_decimal ? base_max : 0;

		ssize_t i = (ssize_t)len - 1;
		for (; i >= 1; i--)
		{
			if (ptr[i] == check)
			{
				if (fixed_not_decimal)
				{
					if (is_negative(ptr[i-1]) == neg)
						continue;
					else break;
				}
			} else break;
		}

		const ssize_t by = (ssize_t)len - 1 - i;
		if (fixed_not_decimal)
		{
			if (by > 0)
				shrink(by);
		}
		else
		{
			if (by > 0)
				shrink_decimal(by);
		}
	}

	void arbit::shrink_if_can()
	{
		shrink_if_can_raw(true);
		shrink_if_can_raw(false);
	}

	size_t arbit::bytes() const
	{
		return fixed_len * sizeof(base_t);
	}

	size_t arbit::bytes_decimal() const
	{
		return decimal_len * sizeof(base_t);
	}

	size_t arbit::bytes_total() const
	{
		return bytes() + bytes_decimal();
	}

	std::string arbit::raw_format(char way) const
	{
		std::ostringstream oss;

		if (fixed_len > 0)
		{
			oss << "F(" << actual_fixed_len << "," << fixed_len << "): ";
			if (is_negative()) oss << "!";

			for (unsigned i=0; i < fixed_len; i++)
			{
				auto unit = fixed_ptr[i];
				switch(way)
				{
				case 'b': oss << std::format("0b{:0>32b}", unit);
					break;
				case 'x': oss << std::hex << "0x" << unit;
					break;
				case 's': oss << sbase_t(unit);
					break;
				default: oss << unit;
					break;
				}
				if (i != fixed_len-1)
					oss << " ";
			}
		}

		if (decimal_len > 0)
		{
			if (fixed_len != 0) oss << ", ";
			oss << "D(" << actual_decimal_len << "," << decimal_len << "): ";

			for (unsigned i=0; i < decimal_len; i++)
			{
				auto unit = decimal_ptr[i];
				switch(way)
				{
				case 'b': oss << std::format("0b{:0>32b}", unit);
					break;
				case 'x': oss << std::hex << "0x" << unit;
					break;
				case 's': oss << sbase_t(unit);
					break;
				default: oss << unit;
					break;
				}
				if (i != decimal_len-1)
					oss << " ";
			}
		}

		if (fixed_len == 0 && decimal_len == 0)
			oss << "EMPTY ARBIT!";

		return oss.str();
	}

	std::string arbit::format() const
	{
		WC_STD_EXCEPTION("{}:{}: print() is broken", __FILE__, __LINE__);
		/*
		  std::string digits;

		  const bool neg = is_negative();

		  {
		  arbit copy;
		  const arbit* source = this;
		  if (neg)
		  {
		  source = &copy;
		  copy = *this;
		  copy.negate();
		  }

		  for (size_t i=0; i < source->fixed_len; i++)
		  {
		  base_t n = source->fixed_ptr[i];
		  while (n != 0)
		  {
		  digits += std::to_string(n % 10);
		  n /= 10;
		  }
		  }
		  }

		  if (digits.empty()) digits += "0";
		  std::reverse(digits.begin(), digits.end());

		  if (neg) std::print("-");
		  std::print("{}", digits);

		  if (decimal_len > 0)
		  WC_STD_EXCEPTION("{}:{}: Decimal printing is broken", __FILE__, __LINE__);
		*/
		return "";
	}

	void arbit::parse(std::string_view both)
	{
		if (both.size() == 0)
			WC_ARBIT_EXCEPTION(parse, "Empty string");

		bool is_decimal = false;
		std::string_view::iterator fixed_end = both.end();

		bool neg = false;
		auto it = both.begin();
		if (both[0] == '-')
		{
			neg = true;
			it = both.begin()+1;
		}

		for (; it != both.end(); it++)
		{
			char c = *it;
			if (c == '.')
			{
				if (is_decimal)
					WC_ARBIT_EXCEPTION(parse, "Encountered a second decimal point in '{}'", both);
				if (it != both.end()-1)
					is_decimal = true;
				fixed_end = it;
				continue;
			}
			else if (!isdigit(c))
			{
				WC_ARBIT_EXCEPTION(parse, "'{}' is not a digit in '{}'", c, both);
			}
		}

		std::string_view fixed(both.begin() + (neg ? 1 : 0), fixed_end), decimal;
		if (is_decimal)
			decimal = std::string_view(std::next(fixed_end), both.end());
		parse(fixed, decimal, neg);
	}

	void arbit::parse(std::string_view fixed, std::string_view decimal, bool neg)
	{
		arbit multiplier(1);

		for (auto it = fixed.rbegin(); it != fixed.rend(); it++)
		{
			arbit cur((*it) - '0');
			cur *= multiplier;
			multiplier *= 10;

			*this += cur;
		}

		if (neg)
			negate();

		if (decimal.size() > 0)
		{
			WC_STD_EXCEPTION("{}:{}: Decimal parsing is broken", __FILE__, __LINE__);
		}
	}
};
