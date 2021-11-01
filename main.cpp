#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>
#include <cstdlib>
#include <cassert>
#include <conio.h>
#include <cctype>
#include <unordered_map>

// NOTE(Aiden): There might be a different way to parse flags without map and enum.
// TODO(Aiden): Instead of loading the entire "big file" into memory, read it also in chunks. 

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
    const unsigned int MAX_STRINGS_CAP = 640000; // Should be enough for anybody.

    unsigned int SEARCH_LEN = 4;
    std::string SEARCH_STR = std::string();
    
    bool REQ_OUTPUT = false;

    bool REQ_DISPLAY = false;
    size_t LINE_NUMBER = 0;
} context;

struct slurped_file {
    const char* data;
    size_t size;
};

slurped_file slurp(const char* filename);
void parse_and_execute_flag(const char* flag);
void execute_flag(flag_iterator flag, const std::string& value);
void execute_flag(flag_iterator flag);
void flag_throw_error(ERROR_TYPE err, std::string_view flag_name);
std::vector<std::string> get_strings_from_file(const slurped_file& slurped_file, size_t size, size_t offset);
void parse_file_in_chunks(const slurped_file& slurped_file);
void output_based_on_context(const std::vector<std::string>& strings, const char* file_base);
void usage();

inline bool should_be_added(const std::string& current)
{
    return !current.empty()
	&& current.length() >= context.SEARCH_LEN
	&& current.find(context.SEARCH_STR) != std::string::npos;
}

inline void add_based_on_context(std::vector<std::string>& strings, std::string& current)
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
    
    const slurped_file slurped_file = slurp(argv[1]);
    for (int i = 0; i < argc - 2; ++i) {
	parse_and_execute_flag(argv[i + 2]);
    }

    if (slurped_file.size > context.MAX_STRINGS_CAP) {
	parse_file_in_chunks(slurped_file);
    } else {
	std::vector<std::string> strings = get_strings_from_file(slurped_file, slurped_file.size, 0); 
	delete[] slurped_file.data;

	output_based_on_context(strings, argv[1]);
    }

    return 0;
}

slurped_file slurp(const char* filename)
{
    FILE* in = fopen(filename, "rb");

    if (in == nullptr) {
	std::cerr << "ERROR: Invalid file provided, make sure the file exists and is valid.\n";
	exit(1);
    }

    fseek(in, 0, SEEK_END);
    size_t data_size = ftell(in);
    fseek(in, 0, SEEK_SET);
    
    char* buffer = new char[data_size];

    if (buffer == nullptr) {
	std::cerr << "ERROR: Could not allocate enough memory for buffer.\n";
	fclose(in);
	exit(1);
    }
    
    if (fread(buffer, 1, data_size, in) == 0) {
	std::cerr << "ERROR: Could not read file into memory, make sure the provided file is valid.\n";
	fclose(in);
	delete[] buffer;
	exit(1);
    }

    fclose(in);
    
    buffer[data_size] = '\0';
    slurped_file slurped_file = {
	buffer,
	data_size
    };
    
    return slurped_file;
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

void parse_file_in_chunks(const slurped_file& slurped_file)
{
    size_t current_size_read = 0;
    bool is_running = true;
    char c;
    
    while (current_size_read <= slurped_file.size && is_running) {
	std::vector<std::string> strings = get_strings_from_file(slurped_file, context.MAX_STRINGS_CAP, current_size_read);

	// Contents of large files are only displayed in console/terminal
	for (const std::string& str : strings) {
	    std::cout << str << '\n';
	}
 
	std::cout << "File too large, displaying to console/terminal, press \'[ANY KEY]\' to continue and \'[Q]\' to exit.\n";
        c = _getch();

	switch (c) {
	    case 113:
	    case 81:
		is_running = false;
		break;
	}
	
	current_size_read += (context.MAX_STRINGS_CAP + 1);
    }

    delete[] slurped_file.data;
}

std::vector<std::string> get_strings_from_file(const slurped_file& slurped_file, size_t size, size_t offset)
{
    std::vector<std::string> strings;
    strings.reserve(size);
    
    std::string current = std::string();
	
    for (size_t i = 0; i < size; ++i) {
	const char c = slurped_file.data[offset + i];
	
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

    return strings;
}

void output_based_on_context(const std::vector<std::string>& strings, const char* file_base)
{    
    if (context.REQ_OUTPUT) {
	const std::string out_filename = std::string(file_base) + "_out.txt";
	FILE* file = fopen(out_filename.c_str(), "w");
	
	for (const std::string& str : strings) {
	    fprintf(file, "%s\n", str.c_str());
	}

	fclose(file);
    } else {
	for (const std::string& str : strings) {
	    std::cout << str << '\n';
	}
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
