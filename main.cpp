#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cassert>
#include <cctype>
#include <unordered_map>

// TODO(Aiden): There might be a better way to parse flags without map and enum.
// TODO(Aiden): Better error throwing, accroding to whether or not flag requires an equal sign.
// TODO(Aiden): Better flag parsing, find flag at the beginning and treat accordingly.
// TODO(Aiden): Would be better to have something like "flag_desc" structure.

enum class ERROR_TYPE {
    ERROR_BEGIN = 0,
    ERROR_EQUAL,
    ERROR_RECOGNIZE,
    ERROR_VALUE,
    ERROR_RANGE,
};

enum class FLAG_TYPE {
    FLAG_LEN = 0,
    FLAG_SER,
    FLAG_OUT,
};

struct global_context {
    const unsigned int VAL_MIN = 4;
    const unsigned int VAL_MAX = 256;

    unsigned int SEARCH_LEN = 4;
    std::string SEARCH_STR = "";
    bool REQ_OUTPUT = false;
} context;

static const std::unordered_map<std::string, FLAG_TYPE> VALUED_FLAGS = {
    { "n"  , FLAG_TYPE::FLAG_LEN },
    { "ser", FLAG_TYPE::FLAG_SER },
};

static const std::unordered_map<std::string, FLAG_TYPE> VALUELESS_FLAGS = {
    { "o", FLAG_TYPE::FLAG_OUT },
};

typedef std::unordered_map<std::string, FLAG_TYPE>::const_iterator flag_iterator;

std::vector<char> slurp(const char* filename);
void flag_throw_error(ERROR_TYPE err, const char* flag_name);
void parse_execute_flag(const char* flag);
void execute_flag(flag_iterator flag, const std::string& value);
void execute_flag(flag_iterator flag);
void usage();

int main(int argc, char* argv[])
{
    if (argc == 1) {
	usage();
	exit(1);
    }

    if (argc > 2) {
	for (int i = 0; i < argc - 2; ++i) {
	    parse_execute_flag(argv[2 + i]);
	}
    }
    
    const std::vector<char> characters = slurp(argv[1]);
    std::vector<std::string> strings;
    std::string current = "";
    
    for (const char& c : characters) {
	if (std::isprint(c)) {
	    current += c;
	    continue;
	}

	if (!current.empty()
	    && current.length() >= context.SEARCH_LEN
	    && current.find(context.SEARCH_STR) != std::string::npos)
	{
	    strings.push_back(current);
	}
	
	current = "";
    }

    // Catch at EOF
    if (!current.empty()
	&& current.length() >= context.SEARCH_LEN
	&& current.find(context.SEARCH_STR) != std::string::npos)
    {
	strings.push_back(current);
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

    std::streamsize size = in.tellg();
    std::vector<char> buffer(size);
    in.seekg(0, std::ios::beg);
    
    if (!in.read(buffer.data(), size)) {
	std::cerr << "ERROR: Could not read file into memory, make sure the provided file is valid.\n";
	in.close();
	exit(1);
    }

    in.close();
    
    return buffer;
}

void parse_execute_flag(const char* flag)
{
    if (flag[0] != '-') {
	flag_throw_error(ERROR_TYPE::ERROR_BEGIN, flag);
	exit(1);
    }
    
    const std::string flag_str = std::string(flag).substr(1);
    const size_t eq_sign = flag_str.find('=');
    
    if (eq_sign == std::string::npos) {	
	// NOTE(Aiden): Flag without associated value, try to parse it/find it. 
	const flag_iterator valueless_flag = VALUELESS_FLAGS.find(flag_str);

	if (valueless_flag != VALUELESS_FLAGS.end()) {
	    execute_flag(valueless_flag);
	    return;
	}
	
	flag_throw_error(ERROR_TYPE::ERROR_EQUAL, flag);
	exit(1);
    }    

    const std::string flag_name = flag_str.substr(0, eq_sign);
    const flag_iterator valued_flag = VALUED_FLAGS.find(flag_name);

    if (valued_flag == VALUED_FLAGS.end()) {
	flag_throw_error(ERROR_TYPE::ERROR_RECOGNIZE, flag);
	exit(1);    	
    }
    	     
    const std::string value = flag_str.substr(eq_sign + 1);
    
    if (value.empty()) {
	flag_throw_error(ERROR_TYPE::ERROR_VALUE, flag);
	exit(1);	
    }

    execute_flag(valued_flag, value);
}

void execute_flag(flag_iterator flag, const std::string& value)
{
    switch (flag->second)
    {
	case FLAG_TYPE::FLAG_LEN: {
	    unsigned int new_len = std::atoi(value.c_str());
	    
	    if (new_len < context.VAL_MIN || new_len > context.VAL_MAX) {
		flag_throw_error(ERROR_TYPE::ERROR_RANGE, flag->first.c_str());
		exit(1);
	    }

	    context.SEARCH_LEN = new_len;
	} break;

	case FLAG_TYPE::FLAG_SER:
	    context.SEARCH_STR = value;
	    break;

	default:
	    assert(false && "Unrecognized flag provided.\n");
    }
}

void execute_flag(flag_iterator flag)
{
    switch (flag->second)
    {
	case FLAG_TYPE::FLAG_OUT:
	    context.REQ_OUTPUT = true;
	    break;
	    
	default:
	    assert(false && "Unrecognized flag provided.\n");
    }
}

void flag_throw_error(ERROR_TYPE err, const char* flag_name)
{
    switch (err)
    {
	case ERROR_TYPE::ERROR_BEGIN:
	    std::cerr << "ERROR: One of the provided flags does not begin with minus sign -> (\'-\')\n";
	    std::cerr << "    FLAG: " << flag_name << '\n';
	    break;
	    
	case ERROR_TYPE::ERROR_EQUAL:
	    std::cerr << "ERROR: Could not find equal sign -> (\'=\') in one of the provided flags.\n";
	    std::cerr << "    FLAG: " << flag_name;
	    break;
	    
	case ERROR_TYPE::ERROR_RECOGNIZE:
	    std::cerr << "ERROR: Provided flag does not exists or was not recognized as valid flag with associated value,\ncheck if provided flag requires value.\n";
	    std::cerr << "    FLAG: " << flag_name;
	    break;
	    
	case ERROR_TYPE::ERROR_VALUE:
	    std::cerr << "ERROR: One of the provided flags does not have a value associated with it.\n";
	    std::cerr << "    FLAG: " << flag_name;
	    break;
	    
	case ERROR_TYPE::ERROR_RANGE:
	    std::cerr << "ERROR: Invalid value provided to flag: \"" << flag_name << "\" valid range is between " << context.VAL_MIN << " and " << context.VAL_MAX << '\n';
	    break;
	    
	default:
	    assert(false && "Unknown error thrown.\n");
    }
}

void usage()
{
    std::cout << "Usage: ./strings [FILE] [OPTIONS]\n";
    std::cout << "    -n=<number>   -> minimum size of a string to display (min: " << context.VAL_MIN <<  ", max: " << context.VAL_MAX << ")\n";
    std::cout << "    -ser=<string> -> search for specified string in file\n";
    std::cout << "    -o            -> outputs everything to \".txt\" file with the name [FILE]_out.txt\n";
    std::cout << "                     (does not output to terminal/console window)\n";
}
