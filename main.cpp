#include <cstdio>
#include <cstring>
#include <string>
#include <cstdlib>
#include <cassert>
#include <cctype>
#include <unordered_map>
#include <filesystem>

// Windows specific
#include <conio.h>

// NOTE(Aiden): There might be a different way to parse flags without map and enum.
// TODO(Aiden): Clean-up the code so that it's more readable

enum class ERROR_TYPE {
    ERROR_BAD_BEGIN = 0,
    ERROR_MISSING_EQUAL,
    ERROR_VALUE_EMPTY,
    ERROR_RANGE,
    ERROR_NO_VALUE_NEEDED,
    ERROR_ALLOC,
    ERROR_SIZE,
    ERROR_FILE,
};

enum class FLAG_TYPE {
    FLAG_LENGTH = 0,
    FLAG_SEARCH,
    FLAG_OUTPUT,
    FLAG_DISPLAY,
    FLAG_REC,
};

static const std::unordered_map<std::string, FLAG_TYPE> VALUED_FLAGS = {
    { "n", FLAG_TYPE::FLAG_LENGTH },
    { "s", FLAG_TYPE::FLAG_SEARCH },
};

static const std::unordered_map<std::string, FLAG_TYPE> VALUELESS_FLAGS = {
    { "o", FLAG_TYPE::FLAG_OUTPUT },

    { "d",        FLAG_TYPE::FLAG_DISPLAY },
    { "-display", FLAG_TYPE::FLAG_DISPLAY },

    { "r", FLAG_TYPE::FLAG_REC },
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
    unsigned int LINE_NUMBER = 0;

    bool REQ_RECURSION = false;
    unsigned int MAX_SUBDIRS = 16;
} context;

struct slurped_file {
    const char* data;
    size_t size;

    ~slurped_file() { if (data) delete[] data; }
};

struct slurped_strings {
    std::string* data;
    size_t size;

    ~slurped_strings() { if (data) delete[] data; }
};

slurped_file slurp_file_whole(const char* filename, size_t data_size);
size_t get_file_size(const char* filename);
void parse_and_execute_flag(const char* flag);
void execute_flag(flag_iterator flag, const std::string& value);
void execute_flag(flag_iterator flag);
void throw_error(ERROR_TYPE err, const std::string& flag_name = std::string());
slurped_strings get_strings_from_file(const slurped_file& slurped_file);
void parse_file_in_chunks(const char* filename);
void output_to_file(const slurped_strings& strings, const char* filename);
void output_based_on_context(const slurped_strings& strings, const char* filename);
bool file_exists(const char* filename);
void parse_linear(const char* filename);
void parse_recursive(const char* filename);
void usage();

inline bool should_be_added(const std::string& current)
{
    return !current.empty()
	&& current.length() >= context.SEARCH_LEN
	&& current.find(context.SEARCH_STR) != std::string::npos;
}

inline void add_based_on_context(slurped_strings& strings, std::string& current)
{
    if (context.REQ_DISPLAY) {
	current += (" -> line: " + std::to_string(context.LINE_NUMBER + 1));
    }

    strings.data[strings.size++] = current;
}

int main(int argc, char* argv[])
{
    const bool help_flag_set = (argc == 2 && (std::strcmp(argv[1], "--help") == 0 || std::strcmp(argv[1], "-h") == 0));
    if (argc == 1 || help_flag_set) {
	usage();
	exit(1);
    }

    for (int i = 0; i < argc - 2; ++i) {
	parse_and_execute_flag(argv[i + 2]);
    }
    
    if (context.REQ_RECURSION) {
	parse_recursive(argv[1]);
    } else {
	parse_linear(argv[1]);
    }
  
    return 0;
}

slurped_file slurp_file_whole(const char* filename, size_t data_size)
{
    FILE* in = fopen(filename, "rb");

    if (in == nullptr) {
	throw_error(ERROR_TYPE::ERROR_FILE);
	exit(1);
    }
    
    char* buffer = new char[data_size];

    if (buffer == nullptr) {
	throw_error(ERROR_TYPE::ERROR_ALLOC);
	fclose(in);
	exit(1);
    }

    fread(buffer, 1, data_size, in);
    fclose(in);
    
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
	throw_error(ERROR_TYPE::ERROR_FILE);
	exit(1);
    }

    fseek(in, 0, SEEK_END);
    size_t data_size = ftell(in);
    fclose(in);

    if (data_size == 0) {
	throw_error(ERROR_TYPE::ERROR_SIZE);
	exit(1);
    }
    
    return data_size;
}

void parse_and_execute_flag(const char* flag)
{
    if (flag[0] != '-') {
	throw_error(ERROR_TYPE::ERROR_BAD_BEGIN, flag);
	exit(1);
    }

    std::string flag_name = std::string(flag).substr(1);
    flag_iterator valueless_flag_it = VALUELESS_FLAGS.find(flag_name);

    size_t equal_sign_pos = flag_name.find('=');
    bool eq_sign_found = equal_sign_pos != std::string::npos;

    if (!eq_sign_found && valueless_flag_it == VALUELESS_FLAGS.end()) {
	throw_error(ERROR_TYPE::ERROR_MISSING_EQUAL, flag_name);
	exit(1);
    }
    
    if (eq_sign_found) {
	std::string valued_flag_name = flag_name.substr(0, equal_sign_pos);
	flag_iterator valued_flag_it = VALUED_FLAGS.find(valued_flag_name);
	std::string flag_value = flag_name.substr(equal_sign_pos + 1);

	if (valued_flag_it == VALUED_FLAGS.end()) {
	    throw_error(ERROR_TYPE::ERROR_NO_VALUE_NEEDED, flag_name);
	    exit(1);
	}
	
	if (flag_value.empty()) {
	    throw_error(ERROR_TYPE::ERROR_VALUE_EMPTY, flag_name);
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
		throw_error(ERROR_TYPE::ERROR_RANGE, flag->first);
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

	case FLAG_TYPE::FLAG_REC:
	    context.REQ_RECURSION = true;
	    break;
	    
	default:
	    assert(false && "Unrecognized flag provided.\n");
	    exit(1);
    }
}

void parse_file_in_chunks(const char* filename)
{
    const size_t full_size = get_file_size(filename);
    
    bool is_running = true;
    FILE* file = fopen(filename, "rb");
    int iteration_count = 0;

    if (file == nullptr) {
	throw_error(ERROR_TYPE::ERROR_FILE);
	exit(1);
    }    
    
    char* buffer = new char[context.MAX_STRINGS_CAP];

    if (buffer == nullptr) {
	throw_error(ERROR_TYPE::ERROR_ALLOC);
	fclose(file);
	exit(1);
    }

    slurped_file slurped_file = {
	buffer,
	context.MAX_STRINGS_CAP
    };
    
    while ((fread(buffer, 1, context.MAX_STRINGS_CAP, file) > 0) && is_running) {
	slurped_strings strings = get_strings_from_file(slurped_file);
	output_based_on_context(strings, filename);
	
	if (!context.REQ_OUTPUT) {
	    printf("File too large, displaying to console/terminal, press [ANY KEY] to continue and [Q] to exit.\n");
	    char c = _getch();
	
	    switch (c) {
		case 113:
		case 81:
		    is_running = false;
		    break;
	    }
	} else {
	    // Thank you C++ very cool.
	    printf("\rWritten: %d%% of the file",
		   static_cast<int>((static_cast<double>(++iteration_count * context.MAX_STRINGS_CAP) / full_size) * 100));
	}
    }

    fclose(file);
}

slurped_strings get_strings_from_file(const slurped_file& slurped_file)
{
    slurped_strings strings = {};
    std::string current = std::string();
    strings.data = new std::string[slurped_file.size];
	
    for (size_t i = 0; i < slurped_file.size; ++i) {
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

void output_to_file(const slurped_strings& strings, const char* filename)
{
    FILE* file = fopen(filename, "a");
    
    for (size_t i = 0; i < strings.size; ++i) {
	fprintf(file, "%s\n", strings.data[i].c_str());
    }

    fclose(file);
}

void output_based_on_context(const slurped_strings& strings, const char* filename)
{
    if (context.REQ_OUTPUT) {
	std::string output_file = std::string(filename) + "_out.txt";
	output_to_file(strings, output_file.c_str());
    } else {
	for (size_t i = 0; i < strings.size; ++i) {
	    printf("%s\n", strings.data[i].c_str());
	}
    }
}

bool file_exists(const char* filename)
{
    if (FILE* file = fopen(filename, "rb")) {
	fclose(file);
	return true;
    }
    
    return false;
}

void parse_linear(const char* filename)
{
    size_t file_size = get_file_size(filename);

    std::string out = std::string(filename) + "_out.txt";
    if (context.REQ_OUTPUT && file_exists(out.c_str())) {
	printf("INFO: Requested output but corresponding '_out.txt' file already exists, it's contents will be replaced.\n");
	remove(out.c_str());
    }
    
    if (file_size > context.MAX_STRINGS_CAP) {
	parse_file_in_chunks(filename);
    } else {
	slurped_file slurped_file = slurp_file_whole(filename, file_size);
	slurped_strings strings = get_strings_from_file(slurped_file);
    
	output_based_on_context(strings, filename);
    }
}

void parse_recursive(const char* filename)
{
    if (context.MAX_SUBDIRS == 0)
	return;
    
    std::filesystem::path path = std::string(filename);
    if (path.has_extension()) {
	path = path.parent_path();
    }

    for (const auto& entry : std::filesystem::directory_iterator(path)) {
	const std::string filepath = entry.path().string();
	const std::string file = entry.path().filename().string();

	if (file[0] == '.')
	    continue;
	
	if (entry.is_directory()) {
	    if (context.MAX_SUBDIRS > 0)
		context.MAX_SUBDIRS--;

	    parse_recursive(filepath.c_str());
	} else {
	    parse_linear(filepath.c_str());
	}
    }
}

void throw_error(ERROR_TYPE err, const std::string& flag_name)
{
    switch (err)
    {
	case ERROR_TYPE::ERROR_BAD_BEGIN:
	    fprintf(stderr, "ERROR: One of the provided flags does not begin with minus sign -> ('-')\n");
	    fprintf(stderr, "    FLAG: %s\n", flag_name.c_str());
	    break;
	    
	case ERROR_TYPE::ERROR_MISSING_EQUAL:
	    fprintf(stderr, "ERROR: Could not find equal sign -> ('=') in one of the provided flags, possible invalid flag.\n");
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
	    fprintf(stderr, "ERROR: Provided flag most likely does not need an equal sign -> ('=')/value, possible invalid flag.\n");
	    fprintf(stderr, "    FLAG: %s\n", flag_name.c_str());
	    break;

	case ERROR_TYPE::ERROR_ALLOC:
	    fprintf(stderr, "ERROR: Could not allocate enough memory.\n");
	    break;

	case ERROR_TYPE::ERROR_SIZE:
	    fprintf(stderr, "ERROR: Provided file's size is not valid. (<= 0)\n");
	    break;

	case ERROR_TYPE::ERROR_FILE:
	    fprintf(stderr, "ERROR: Provided file is invalid or does not exist.\n");
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
