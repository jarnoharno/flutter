#ifndef OPT_PARSER_H
#define OPT_PARSER_H

#include <functional>
#include <utility>
#include <memory>
#include <stdexcept>
#include <vector>
#include <type_traits>
#include <cstring>
#include <cstdlib>

namespace opt
{

struct opt_error: public std::exception
{
	inline opt_error()
	{
	}

	inline opt_error(std::string opt):
		opt(std::move(opt))
	{
	}

	inline char const* what() const throw()
	{
		return opt.c_str();
	}

	std::string opt;
};

struct parse_error: public opt_error
{
	inline parse_error():
		opt_error()
	{
	}
	inline parse_error(std::string opt):
		opt_error(std::move(opt))
	{
	}
};

struct no_argument_error: public opt_error
{
	inline no_argument_error():
		opt_error()
	{
	}
	inline no_argument_error(std::string opt):
		opt_error(std::move(opt))
	{
	}
};

struct required_argument_error: public opt_error
{
	inline required_argument_error():
		opt_error()
	{
	}
	inline required_argument_error(std::string opt):
		opt_error(std::move(opt))
	{
	}
};

struct unknown_option_error: public opt_error
{
	inline unknown_option_error():
		opt_error()
	{
	}
	inline unknown_option_error(std::string opt):
		opt_error(std::move(opt))
	{
	}
};

enum argument_type {
	no_argument,
	required_argument
};

template <typename T>
T parse(char const* arg);

template <>
int parse(char const* arg)
{
	if (!*arg)
		throw parse_error();
	char *end;
	int val = std::strtol(arg, &end, 10);
	if (*end)
		throw parse_error();
	return val;
}

template <>
float parse(char const* arg)
{
	if (!*arg)
		throw parse_error();
	char *end;
	float val = std::strtof(arg, &end);
	if (*end)
		throw parse_error();
	return val;
}

template <>
std::string parse(char const* arg)
{
	return std::string(arg);
}

struct opt_placeholder {
	virtual void handle(char const* arg) const = 0;
	virtual argument_type arg_type() const = 0;
	virtual ~opt_placeholder() {}
};

template <typename T>
struct opt_placeholder_fn: public opt_placeholder
{
	typedef std::function<void(T)> F;
	typedef typename std::remove_const<
		typename std::remove_reference<T>::type
		>::type
		plain_type;
	F handler;
	inline opt_placeholder_fn(F handler):
		handler(handler)
	{
	}
	inline void handle(char const* arg) const
	{
		handler(parse<plain_type>(arg));
	}
	inline argument_type arg_type() const
	{
		return required_argument;
	}
};

struct opt_placeholder_fn_void: public opt_placeholder
{
	typedef std::function<void()> F;
	F handler;
	inline opt_placeholder_fn_void(F handler):
		handler(handler)
	{
	}
	inline void handle(char const* arg) const
	{
		if (*arg)
			throw no_argument_error();
		handler();
	}
	inline argument_type arg_type() const
	{
		return no_argument;
	}
};

template <typename T>
struct opt_placeholder_val: public opt_placeholder
{
	T* value;
	opt_placeholder_val(T* value):
		value(value)
	{
	}
	inline void handle(char const* arg) const
	{
		*value = parse<T>(arg);
	}
	inline argument_type arg_type() const
	{
		return required_argument;
	}
};

template <>
struct opt_placeholder_val<bool>: public opt_placeholder
{
	bool* value;
	opt_placeholder_val(bool* value):
		value(value)
	{
	}
	inline void handle(char const* arg) const
	{
		*value = true;
	}
	inline argument_type arg_type() const
	{
		return no_argument;
	}
};

template <>
struct opt_placeholder_val<std::string>: public opt_placeholder
{
	std::string* value;
	opt_placeholder_val(std::string* value):
		value(value)
	{
	}
	inline void handle(char const* arg) const
	{
		*value = arg;
	}
	inline argument_type arg_type() const
	{
		return required_argument;
	}
};

template <typename F>
struct function_traits: public function_traits<decltype(&F::operator())>
{};

template <typename C, typename R, typename A>
struct function_traits<R(C::*)(A) const>
{
    typedef R result_type;
    typedef A arg_type;
    typedef std::function<R(A)> function_type;
    static constexpr bool arg_function = true;
};

template <typename C, typename R>
struct function_traits<R(C::*)(void) const>
{
    typedef R result_type;
    typedef std::function<R()> function_type;
    static constexpr bool void_function = true;
};

template <
	typename F,
	typename T = function_traits<F>,
	typename A = typename T::arg_type,
	typename P = opt_placeholder_fn<A>
>
typename std::enable_if<T::arg_function, std::unique_ptr<P>>::type
get_opt_placeholder_fn_ptr(F f)
{
	return std::make_unique<P>(
		typename T::function_type(std::forward<F>(f)));
}

template <
	typename F,
	typename T = function_traits<F>
>
typename std::enable_if<T::void_function,
	std::unique_ptr<opt_placeholder_fn_void>>::type
get_opt_placeholder_fn_ptr(F f)
{
	return std::make_unique<opt_placeholder_fn_void>(
		typename T::function_type(std::forward<F>(f)));
}

template <
	typename T,
	typename P = opt_placeholder_val<T>
>
std::unique_ptr<P>
get_opt_placeholder_val_ptr(T* value)
{
	return std::make_unique<P>(value);
}

struct opt_entry {
	char opt;
	std::string long_opt;
	std::unique_ptr<opt_placeholder> handler;

	template <typename T>
	opt_entry(char opt, std::string long_opt, T* value):
		opt(opt),
		long_opt(std::move(long_opt)),
		handler(get_opt_placeholder_val_ptr(value))
	{
	}

	template <typename F>
	opt_entry(char opt, std::string long_opt, F&& f):
		opt(opt),
		long_opt(std::move(long_opt)),
		handler(get_opt_placeholder_fn_ptr(std::forward<F>(f)))
	{
	}
};

inline bool prefix_match(char const* arg, char const* opt, int& len)
{
	len = 0;
	while (arg[len] != '\0' && opt[len] != '\0' && arg[len] == opt[len])
		++len;
	return opt[len] == '\0';
}

inline char const* find_first(char const* arg, char c)
{
	char const* f = arg;
	while (*f && *f != c)
		++f;
	return f;
}

inline bool match(char const* b, char const* e, char const* opt)
{
	for (; b != e && *opt != '\0' && *b == *opt; ++b, ++opt);
	return b == e && *opt == '\0';
}

struct parser {
	std::vector<opt_entry> opts;
	std::vector<std::string> pos_args;
	template <typename T>
	inline void add(char opt, std::string long_opt, T&& arg)
	{
		opts.emplace_back(opt, move(long_opt), std::forward<T>(arg));
	}
	inline void parse(int argc, char const* const* argv)
	{
		bool only_pos_args = false;
		for (int i = 1; i < argc; ++i) {
			char const* arg = argv[i];
			if (!arg[0])
				continue;
			if (!arg[1] || arg[0] != '-' || only_pos_args) {
				pos_args.emplace_back(arg);
				continue;
			}
			if (arg[1] == '-') {
				only_pos_args = parse_long_opt(arg);
				continue;
			}
			i += parse_short_opt(arg+1, argv[i+1]);
		}
	}
	inline int parse_short_opt(char const* opt, char const* next)
	{
		if (!*opt)
			return 0;
		for (const opt_entry& e: opts) {
			if (*opt != e.opt)
				continue;
			argument_type arg_type = e.handler->arg_type();
			if (arg_type == no_argument) {
				e.handler->handle("");
				return parse_short_opt(opt+1, next);
			}
			if (opt[1] == '\0' && !next)
				throw required_argument_error(std::string("-") + *opt);
			if (opt[1] != '\0')
				next = opt+1;
			try {
				e.handler->handle(next);
			} catch (opt_error& err) {
				err.opt = std::string("-") + *opt;
				throw;
			}
			return 1;
		}
		throw unknown_option_error(std::string("-") + *opt);
	}
	inline bool parse_long_opt(char const* const opt)
	{
		if (!opt[2])
			return true;
		for (const opt_entry& e: opts) {
			char const* arg = find_first(opt+2, '=');
			argument_type arg_type = e.handler->arg_type();
			if (!match(opt+2, arg, e.long_opt.c_str()))
				continue;
			if (!*arg && arg_type == required_argument) {
				throw required_argument_error(opt);
			}
			if (*arg == '=' && arg_type == no_argument) {
				throw no_argument_error(opt);
			}
			if (!*arg || *arg++ == '=') {
				try {
					e.handler->handle(arg);
				} catch (opt_error& err) {
					err.opt = opt;
					throw;
				}
				return false;
			}
			break;
		}
		throw unknown_option_error(opt);
	}
};

} // opt

#endif // OPT_PARSER_H
