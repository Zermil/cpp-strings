#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>
#include <cstdlib>
#include <cassert>
#include <cctype>
#include <unordered_map>

// NOTE(Aiden): There might be a different way to parse flags without map and enum.

enum class ERROR_TYPE {
    ERROR_BAD_BEGIN = 0,
    ERROR_MISSING_EQUAL,
    ERROR_VALUE_EMPTY,
    ERROR_RANGE,
    ERROR_NO_VALUE_NEEDED,
};

enum class FLAG_TYPE {
    FLAG_LENGTH = 0,
    FLAG_SEARCH,
    FLAG_OUTPUT,
    FLAG_DISPLAY,
};

static const std::unordered_map<std::string_view, FLAG_TYPE> VALUED_FLAGS = {
    { "n", FLAG_TYPE::FLAG_LENGTH },
    { "s", FLAG_TYPE::FLAG_SEARCH },
};

static const std::unordered_map<std::string_view, FLAG_TYPE> VALUELESS_FLAGS = {
    { "o", FLAG_TYPE::FLAG_OUTPUT },

    { "d",        FLAG_TYPE::FLAG_DISPLAY },
    { "-display", FLAG_TYPE::FLAG_DISPLAY },
};

typedef std::unordered_map<std::string_view, FLAG_TYPE>::const_iterator flag_iterator;

struct global_context {
    const unsigned int VAL_MIN = 4;
    const unsigned int VAL_MAX = 256;

    unsigned int SEARCH_LEN = 4;
    std::string SEARCH_STR = std::string();
    
    bool REQ_OUTPUT = false;

    bool REQ_DISPLAY = false;
    size_t LINE_NUMBER = 0;
} context;

std::vector<char> slurp(const char* filename);
void parse_and_execute_flag(const char* flag);
void execute_flag(flag_iterator flag, const std::string& value);
void execute_flag(flag_iterator flag);
void flag_throw_error(ERROR_TYPE err, std::string_view flag_name);
void usage();

inline bool should_be_added(const std::string& current)
{
    return !current.empty()
	&& current.length() >= context.SEARCH_LEN
	&& current.find(context.SEARCH_STR) != std::string::npos;
}

inline void add_based_on_context(std::vector<std::string>& strings, std::string& current) noexcept
{
    if (context.REQ_DISPLAY) {
	current += (" -> line: " + std::to_string(context.LINE_NUMBER + 1));
    }
	
    strings.emplace_back(current);
}

int main(int argc, char* argv[])
{
    const bool help_flag_set = (argc == 2 && (std::strcmp(argv[1], "--help") == 0 || std::strcmp(argv[1], "-h") == 0));
    if (argc == 1 || help_flag_set) {
	usage();
	exit(1);
    }
    
    const std::vector<char> slurped_file = slurp(argv[1]);
    for (int i = 0; i < argc - 2; ++i) {
	parse_and_execute_flag(argv[i + 2]);
    }
    
    std::vector<std::string> strings;
    std::string current = std::string();
	
    for (const char& c : slurped_file) {
	if (c == '\n' && context.REQ_DISPLAY) {
	    context.LINE_NUMBER++;
	}

	if (std::isprint(c)) {
	    current += c;
	    continue;
	}
    
	if (should_be_added(current)) {
	    add_based_on_context(strings, current);
	}
	
	current = std::string();
    }

    // Catch at EOF
    if (should_be_added(current)) {
	add_based_on_context(strings, current);
    }

    if (context.REQ_OUTPUT) {
	const std::string out_filename = std::string(argv[1]) + "_out.txt";
	std::ofstream out(out_filename);
	
	for (const std::string& str : strings) {
	    out << str << '\n';
	}

	out.close();
    } else {
	for (const std::string& str : strings) {
	    std::cout << str << '\n';
	}	
    }

    return 0;
}

std::vector<char> slurp(const char* filename)
{
    std::ifstream in(filename, std::ios::binary | std::ios::ate);

    if (!in.is_open()) {
	std::cerr << "ERROR: Invalid file provided, make sure the file exists and is valid.\n";
	exit(1);
    }

    std::streamsize data_size = in.tellg();
    std::vector<char> buffer(data_size);
    in.seekg(0, std::ios::beg);

    if (!in.read(buffer.data(), data_size)) {
	std::cerr << "ERROR: Could not read file into memory, make sure the provided file is valid.\n";
	in.close();
	exit(1);
    }
    
    in.close();
    
    return buffer;
}

void parse_and_execute_flag(const char* flag)
{
    if (flag[0] != '-') {
	flag_throw_error(ERROR_TYPE::ERROR_BAD_BEGIN, flag);
	exit(1);
    }

    std::string flag_name = std::string(flag).substr(1);
    flag_iterator valueless_flag_it = VALUELESS_FLAGS.find(flag_name);

    size_t equal_sign_pos = flag_name.find('=');
    bool eq_sign_found = equal_sign_pos != std::string::npos;

    if (!eq_sign_found && valueless_flag_it == VALUELESS_FLAGS.end()) {
	flag_throw_error(ERROR_TYPE::ERROR_MISSING_EQUAL, flag_name);
	exit(1);
    }
    
    if (eq_sign_found) {
	std::string valued_flag_name = flag_name.substr(0, equal_sign_pos);
	flag_iterator valued_flag_it = VALUED_FLAGS.find(valued_flag_name);
	std::string flag_value = flag_name.substr(equal_sign_pos + 1);

	if (valued_flag_it == VALUED_FLAGS.end()) {
	    flag_throw_error(ERROR_TYPE::ERROR_NO_VALUE_NEEDED, flag_name);
	    exit(1);
	}
	
	if (flag_value.empty()) {
	    flag_throw_error(ERROR_TYPE::ERROR_VALUE_EMPTY, flag_name);
	    exit(1);
	}
	
	execute_flag(valued_flag_it, flag_value);
    } else {
	execute_flag(valueless_flag_it);
    }
}

void execute_flag(flag_iterator flag, const std::string& value)
{
    switch (flag->second)
    {
	case FLAG_TYPE::FLAG_LENGTH: {
	    unsigned int new_len = std::atoi(value.c_str());
	    
	    if (new_len < context.VAL_MIN || new_len > context.VAL_MAX) {
		flag_throw_error(ERROR_TYPE::ERROR_RANGE, flag->first);
		exit(1);
	    }

	    context.SEARCH_LEN = new_len;
	} break;

	case FLAG_TYPE::FLAG_SEARCH:
	    context.SEARCH_STR = value;
	    break;

	default:
	    assert(false && "Unrecognized flag provided.\n");
	    exit(1);
    }
}

void execute_flag(flag_iterator flag)
{
    switch (flag->second)
    {
	case FLAG_TYPE::FLAG_OUTPUT:
	    context.REQ_OUTPUT = true;
	    break;

	case FLAG_TYPE::FLAG_DISPLAY:
	    context.REQ_DISPLAY = true;
	    break;
	    
	default:
	    assert(false && "Unrecognized flag provided.\n");
	    exit(1);
    }
}

void flag_throw_error(ERROR_TYPE err, std::string_view flag_name)
{
    switch (err)
    {
	case ERROR_TYPE::ERROR_BAD_BEGIN:
	    std::cerr << "ERROR: One of the provided flags does not begin with minus sign -> (\'-\')\n";
	    std::cerr << "    FLAG: " << flag_name << '\n';
	    break;
	    
	case ERROR_TYPE::ERROR_MISSING_EQUAL:
	    std::cerr << "ERROR: Could not find equal sign -> (\'=\') in one of the provided flags, possible invalid flag.\n";
	    std::cerr << "    FLAG: " << flag_name << '\n';
	    break;
	    
	case ERROR_TYPE::ERROR_VALUE_EMPTY:
	    std::cerr << "ERROR: One of the provided flags does not have a value associated with it.\n";
	    std::cerr << "    FLAG: " << flag_name << '\n';
	    break;
	    
	case ERROR_TYPE::ERROR_RANGE:
	    std::cerr << "ERROR: Invalid value provided to flag: \"" << flag_name << "\" valid range is between " << context.VAL_MIN << " and " << context.VAL_MAX << '\n';
	    break;

	case ERROR_TYPE::ERROR_NO_VALUE_NEEDED:
	    std::cerr << "ERROR: Provided flag most likely does not need an equal sign -> (\'=\')/value, possible invalid flag.\n";
	    std::cerr << "    FLAG: " << flag_name << '\n';
	    break;
	    
	default:
	    assert(false && "Unknown error thrown.\n");
	    exit(1);
    }
}

void usage()
{
    std::cout << "Usage: ./strings [FILE] [OPTIONS]\n";
    std::cout << "    -n=<number>   -> minimum size of a string to display (min: " << context.VAL_MIN <<  ", max: " << context.VAL_MAX << ")\n";
    std::cout << "    -s=<string>   -> search for specified string in file\n";
    std::cout << "    -o            -> outputs everything to \".txt\" file with the name [FILE]_out.txt\n";
    std::cout << "                     (does not output to terminal/console window)\n";
    std::cout << "    -d --display  -> display line number, outputs at which line a particular string was found.\n";
}
