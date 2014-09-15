#ifndef CMDPARSER_HPP
#define CMDPARSER_HPP

#include <vector>
#include <map>
#include <sstream>
#include <stdexcept>
#include "clany_macros.h"

_CLANY_BEGIN
class ParseError : public logic_error {
public:
    ParseError(const std::string& err_msg)
        : logic_error(err_msg) {
    }
};

enum { no_argument = 0, required_argument, optional_argument };

struct LongOption {
    string name;
    int    has_arg;
    int    val;
};

class CmdLineParser {
private:
    struct Option {
        char name;
        int has_arg;
    };

public:
    CmdLineParser(int argc, const char* const argv[], const string& options)
        : arg_vec(argv + 1, argv + argc), pos(0), curr_idx(0, false), next_idx(0) {
        if (argc != 1) initParser(options);
    }

    CmdLineParser(int argc, const char* const argv[],
                  const string& options,
                  const vector<LongOption>& long_options)
        : CmdLineParser(argc, argv, options) {
        // Store user provided long options
        for (const auto& lopt : long_options) {
            long_opt_map.emplace(lopt.name, lopt);
        }
    }

    // Return next option character, if no more option characters available, return -1
    char get() {
        if (pos == opt_idx_vec.size()) return -1;
        curr_idx = opt_idx_vec[pos++];

        string option = arg_vec[curr_idx.first];
        if (!curr_idx.second) {     // Short option
            if (opt_map.find(option[1]) == opt_map.end()) return '?';
            opt_char = option[1];
        } else {                    // Long option
            option = option.substr(2, option.find('=') - 2);
            if (long_opt_map.find(option) == long_opt_map.end()) return '?';
            opt_char = long_opt_map.at(option).val;
        }
        bool next_is_arg = parseArg();
        if (opt_arg == ":") return ':';

        // Update the index of next option to be processed
        if (pos != opt_idx_vec.size()) {
            next_idx = opt_idx_vec[pos].first;
        } else {
            next_idx = opt_idx_vec.back().first + (next_is_arg ? 2 : 1);
        }

        return opt_char;
    }

    // Get current option argument
    template<typename T>
    T getArg() const {
        istringstream ss(opt_arg);
        T parsed_arg;
        ss >> parsed_arg;
        return parsed_arg;
    }

    // Get the index of next option to be processed in argv
    int getIndex() const {
        return next_idx + 1;    // Program name counts 1
    }

    // Get argument of custom option
    template<typename T>
    T parse(const string& name) {
        // Save current state
        auto old_curr_idx = curr_idx;
        auto old_opt_arg  = opt_arg;

        if (!findArg(name)) throw ParseError("Argument not found or not required!");

        istringstream ss(opt_arg);
        T parsed_arg;
        ss >> parsed_arg;

        curr_idx = old_curr_idx;
        opt_arg  = old_opt_arg;

        return parsed_arg;
    }

private:
    void initParser(const string& options) {
        // Get all possible options
        char last_opt;
        for (const auto& opt : options) {
            if (opt != ':') {
                opt_map.emplace(opt, Option {opt, 0}), last_opt = opt;
            } else {
                opt_map.at(last_opt).has_arg++;
            }
        }
        // Extract all options and long options in arg_vec, store index
        for (size_t i = 0; i < arg_vec.size(); ++i) {
            const string& arg = arg_vec[i];
            if (arg[0] == '-') opt_idx_vec.emplace_back(i, arg[1] == '-');
        }
    }

    // Return true if parsed argument is the next argument in argv, else return false
    bool parseArg() {
        string option   = arg_vec[curr_idx.first];
        string next_arg = curr_idx.first + 1 == arg_vec.size() ? "" : arg_vec[curr_idx.first + 1];
        if (curr_idx.second) {    // Long option
            option.erase(0, 2);
            opt_arg = parseArg(option, next_arg, long_opt_map.at(
                               option.substr(0, option.find('='))).has_arg);
        } else {                  // Short option
            option.erase(0, 1);
            opt_arg = parseArg(option, next_arg,
                               opt_map.at(option[0]).has_arg);
        }
        return !opt_arg.empty() && opt_arg == next_arg;
    }

    string parseArg(const string& option, const string& next, int has_argument) const {
        // Option does not require an argument
        if (!has_argument) return NULL_CHARACTER;

        size_t eq_sign_pos = option.find('=');
        string arg = eq_sign_pos == string::npos ? option.substr(1) : option.substr(eq_sign_pos + 1);
        string name = option.substr(0, eq_sign_pos);

        if (!arg.empty()) return arg;
        if (!next.empty() && next[0] != '-') return next;

        // Option require an argument but not found
        if (has_argument == 1) return ":";

        return "";
    }

    bool findArg(const string& name) {
        // Traverse all possible options
        for (const auto& idx : opt_idx_vec) {
            auto option = arg_vec[idx.first];
            // Check if match any short/long option
            if (name == option.substr(1, 1) ||
                name == option.substr(2, option.find('=') - 2)) {
                curr_idx = idx;
                parseArg();
                if (opt_arg == NULL_CHARACTER || opt_arg == ":") return false;
            }
        }
        return true;
    }

private:
    // All commandline arguments
    vector<string> arg_vec;
    // Option index in arg_vec, second element is true if it's a long option
    vector<pair<int, bool>> opt_idx_vec;
    int pos;
    pair<int, bool> curr_idx;

    // Index of next option to be processed
    int next_idx;
    // Current option
    char opt_char;
    // Argument of current option
    string opt_arg;

    // User defined available options
    map<char, Option>       opt_map;
    map<string, LongOption> long_opt_map;

    const string NULL_CHARACTER = string("\0", 1);
};

template<>
inline string CmdLineParser::getArg<string>() const {
    return opt_arg;
}

template<>
inline string CmdLineParser::parse<string>(const string& name) {
    // Save current state
    auto old_curr_idx = curr_idx;
    auto old_opt_arg  = opt_arg;

    if (!findArg(name)) throw ParseError("Argument not found or not required!");
    string parsed_arg = opt_arg;

    curr_idx = old_curr_idx;
    opt_arg  = old_opt_arg;

    return parsed_arg;
}

_CLANY_END

#endif // __CMDPARSER_HPP__