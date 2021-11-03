#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <cstdlib>
#include <cassert>
#include <conio.h>
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

static const std::unordered_map<std::string, FLAG_TYPE> VALUED_FLAGS = {
    { "n", FLAG_TYPE::FLAG_LENGTH },
    { "s", FLAG_TYPE::FLAG_SEARCH },
};

static const std::unordered_map<std::string, FLAG_TYPE> VALUELESS_FLAGS = {
    { "o", FLAG_TYPE::FLAG_OUTPUT },

    { "d",        FLAG_TYPE::FLAG_DISPLAY },
    { "-display", FLAG_TYPE::FLAG_DISPLAY },
};

typedef std::unordered_map<std::string, FLAG_TYPE>::const_iterator flag_iterator;

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

slurped_file slurp_file_whole(const char* filename);
size_t get_file_size(const char* filename);
void parse_and_execute_flag(const char* flag);
void execute_flag(flag_iterator flag, const std::string& value);
void execute_flag(flag_iterator flag);
void flag_throw_error(ERROR_TYPE err, const std::string& flag_name);
std::vector<std::string> get_strings_from_file(const slurped_file& slurped_file, size_t size);
void parse_file_in_chunks(const char* filename);
void output_to_file(const std::vector<std::string>& strings, const char* file_base);
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
    
    size_t file_size = get_file_size(argv[1]);
    for (int i = 0; i < argc - 2; ++i) {
	parse_and_execute_flag(argv[i + 2]);
    }

    if (file_size > context.MAX_STRINGS_CAP) {
	parse_file_in_chunks(argv[1]);
    } else {
	const slurped_file slurped_file = slurp_file_whole(argv[1]);
	std::vector<std::string> strings = get_strings_from_file(slurped_file, slurped_file.size);
	delete[] slurped_file.data;

	if (context.REQ_OUTPUT) {
	    output_to_file(strings, argv[1]);
	} else {
	    for (const std::string& str : strings) {
		printf("%s\n", str.c_str());
	    }
	}
    }

    return 0;
}

slurped_file slurp_file_whole(const char* filename)
{
    FILE* in = fopen(filename, "rb");

    if (in == nullptr) {
	fprintf(stderr, "ERROR: Invalid file provided, make sure the file exists and is valid.\n");
	exit(1);
    }

    fseek(in, 0, SEEK_END);
    size_t data_size = ftell(in);
    fseek(in, 0, SEEK_SET);
    
    char* buffer = new char[data_size];

    if (buffer == nullptr) {
	fprintf(stderr, "ERROR: Could not allocate enough memory for buffer.\n");
	fclose(in);
	exit(1);
    }
    
    if (fread(buffer, 1, data_size, in) == 0) {
	fprintf(stderr, "ERROR: Could not read file into memory, make sure the provided file is valid.\n");
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

size_t get_file_size(const char* filename)
{
    FILE* in = fopen(filename, "rb");

    if (in == nullptr) {
	fprintf(stderr, "ERROR: Invalid file provided, make sure the file exists and is valid.\n");
	exit(1);
    }

    fseek(in, 0, SEEK_END);
    size_t data_size = ftell(in);
    fseek(in, 0, SEEK_SET);

    fclose(in);
    return data_size;
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

void parse_file_in_chunks(const char* filename)
{
    size_t current_size_read = 0;
    bool is_running = true;
    char c;
    
    FILE* file = fopen(filename, "rb");
    char* buffer = new char[context.MAX_STRINGS_CAP];

    if (buffer == nullptr) {
	fprintf(stderr, "ERROR: Could not allocate enough memory for buffer.\n");
	exit(1);
    }
    
    while ((current_size_read = fread(buffer, 1, context.MAX_STRINGS_CAP, file) > 0) && is_running) {
	slurped_file slurped_file = {
	    buffer,
	    context.MAX_STRINGS_CAP
	};
	std::vector<std::string> strings = get_strings_from_file(slurped_file, context.MAX_STRINGS_CAP);
	
	// Contents of large files are only displayed in console/terminal
	for (const std::string& str : strings) {
	    printf("%s\n", str.c_str());
	}
 
	printf("File too large, displaying to console/terminal, press [ANY KEY] to continue and [Q] to exit.\n");
        c = _getch();

	switch (c) {
	    case 113:
	    case 81:
		is_running = false;
		break;
	}
    }

    fclose(file);
    delete[] buffer;
}

std::vector<std::string> get_strings_from_file(const slurped_file& slurped_file, size_t size)
{
    std::vector<std::string> strings;
    strings.reserve(size);
    
    std::string current = std::string();
	
    for (size_t i = 0; i < size; ++i) {
	const char c = slurped_file.data[i];
	
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

void output_to_file(const std::vector<std::string>& strings, const char* file_base)
{
    const std::string out_filename = std::string(file_base) + "_out.txt";
    FILE* file = fopen(out_filename.c_str(), "w");
	
    for (const std::string& str : strings) {
	fprintf(file, "%s\n", str.c_str());
    }

    fclose(file);
}

void flag_throw_error(ERROR_TYPE err, const std::string& flag_name)
{
    switch (err)
    {
	case ERROR_TYPE::ERROR_BAD_BEGIN:
	    fprintf(stderr, "ERROR: One of the provided flags does not begin with minus sign -> (\'-\')\n");
	    fprintf(stderr, "    FLAG: %s\n", flag_name.c_str());
	    break;
	    
	case ERROR_TYPE::ERROR_MISSING_EQUAL:
	    fprintf(stderr, "ERROR: Could not find equal sign -> (\'=\') in one of the provided flags, possible invalid flag.\n");
	    fprintf(stderr, "    FLAG: %s\n", flag_name.c_str());
	    break;
	    
	case ERROR_TYPE::ERROR_VALUE_EMPTY:
	    fprintf(stderr, "ERROR: One of the provided flags does not have a value associated with it.\n");
	    fprintf(stderr, "    FLAG: %s\n", flag_name.c_str());
	    break;
	    
	case ERROR_TYPE::ERROR_RANGE:
	    fprintf(stderr, "ERROR: Invalid value provided to flag: \"%s\" valid range is between %u and %u\n", flag_name.c_str(), context.VAL_MIN, context.VAL_MAX);
	    break;

	case ERROR_TYPE::ERROR_NO_VALUE_NEEDED:
	    fprintf(stderr, "ERROR: Provided flag most likely does not need an equal sign -> (\'=\')/value, possible invalid flag.\n");
	    fprintf(stderr, "    FLAG: %s\n", flag_name.c_str());
	    break;
	    
	default:
	    assert(false && "Unknown error thrown.\n");
	    exit(1);
    }
}

void usage()
{
    printf("Usage: ./strings [FILE] [OPTIONS]\n");
    printf("    -n=<number>   -> minimum size of a string to display (min: %u, max: %u)\n", context.VAL_MIN, context.VAL_MAX);
    printf("    -s=<string>   -> search for specified string in file\n");
    printf("    -o            -> outputs everything to \".txt\" file with the name [FILE]_out.txt\n");
    printf("                     (does not output to terminal/console window)\n");
    printf("    -d --display  -> display line number, outputs at which line a particular string was found.\n");
}
