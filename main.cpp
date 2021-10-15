#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cctype>
#include <unordered_map>

// TODO(Aiden): There might be a better way to parse flags without this.
enum class FLAG_TYPE {
    FLAG_LEN = 0,
    FLAG_SER,
};

static const std::unordered_map<std::string, FLAG_TYPE> FLAGS = {
    { "n"  , FLAG_TYPE::FLAG_LEN },
    { "ser", FLAG_TYPE::FLAG_SER },
};

static unsigned int SEARCH_LEN = 4;
static std::string SEARCH_STR = "";

std::vector<char> slurp(const char* filename);
void parse_flag(const char* flag);
void exec_flag(std::unordered_map<std::string, FLAG_TYPE>::const_iterator flag, const std::string& value);

int main(int argc, char* argv[])
{
    if (argc == 1) {
	std::cerr << "Usage: strings [FILE] [OPTIONS]\n";
	std::cerr << "    -n=<number>   -> minimum size of a string to display (min: 4, max: 1024)\n";
	std::cerr << "    -ser=<string> -> search for specified string in file";
	exit(1);
    }
    
    if (argc > 2) {
	for (int i = 0; i < argc - 2; ++i) {
	    parse_flag(argv[2 + i]);
	}
    }
    
    std::vector<char> characters = slurp(argv[1]);
    std::vector<std::string> strings;
    std::string current = "";
    
    for (const char& c : characters) {
	if (std::isprint(c)) {
	    current += c;
	    continue;
	}

	if (!current.empty()
	    && current.length() >= SEARCH_LEN
	    && current.find(SEARCH_STR) != std::string::npos)
	{
	    strings.push_back(current);
	}
	
	current = "";
    }

    // Catch at EOF
    if (!current.empty()
	&& current.length() >= SEARCH_LEN
	&& current.find(SEARCH_STR) != std::string::npos)
    {
	strings.push_back(current);
    }

    for (const std::string& str : strings) {
	std::cout << str << '\n';
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

void parse_flag(const char* flag)
{
    if (flag[0] != '-') {
	std::cerr << "ERROR: One of the provided flags does not begin with minus sign -> (\'-\')\n";
	std::cerr << "    FLAG: " << flag;
	exit(1);
    }
    
    const std::string flag_str = std::string(flag).substr(1);
    const size_t eq_sign = flag_str.find('=');
    
    if (eq_sign == std::string::npos) {
	std::cerr << "ERROR: Could not find equal sign -> (\'=\') in one of the provided flags.\n";
	std::cerr << "    FLAG: " << flag;
	exit(1);
    }    

    const std::string flag_name = flag_str.substr(0, eq_sign);
    const auto flag_it = FLAGS.find(flag_name);

    if (flag_it == FLAGS.end()) {
	std::cerr << "ERROR: Provided flag does not exists or was not recognized.\n";
	std::cerr << "    FLAG: " << flag;
	exit(1);    	
    }
    	     
    const std::string value = flag_str.substr(eq_sign + 1);
    
    if (value.empty()) {
	std::cerr << "ERROR: One of the provided flags does not have a value associated with it.\n";
	std::cerr << "    FLAG: " << flag;
	exit(1);	
    }

    exec_flag(flag_it, value);
}

void exec_flag(std::unordered_map<std::string, FLAG_TYPE>::const_iterator flag, const std::string& value)
{
    switch (flag->second)
    {
	case FLAG_TYPE::FLAG_LEN: {
	    unsigned int new_len = std::atoi(value.c_str());
	    
	    if (new_len < 4 || new_len > 1024) {
		std::cerr << "ERROR: Invalid value provided to flag: \"" << flag->first << "\" valid range is between 4 and 1024\n";
		exit(1);
	    }

	    SEARCH_LEN = new_len;
	} break;

	case FLAG_TYPE::FLAG_SER:
	    SEARCH_STR = value;
	    break;
    }
}
