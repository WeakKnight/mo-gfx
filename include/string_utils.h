#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <stdarg.h>

namespace StringUtils
{
	std::string& ltrim(std::string& str, const std::string& chars = "\t\n\v\f\r ");


	std::string& rtrim(std::string& str, const std::string& chars = "\t\n\v\f\r ");


	std::string& trim(std::string& str, const std::string& chars = "\t\n\v\f\r ");


    static inline std::string Format(const std::string fmt_str, ...)
    {
        auto finaln = 0;
        auto n = ((int)fmt_str.size()) * 2; // reserve 2 times as much as the length of the fmt_str
        std::unique_ptr<char[]> formatted;
        va_list ap;
        while (true)
        {
            //wrap the plain char array into the unique_ptr
            formatted.reset(new char[n]);
            strcpy(&formatted[0], fmt_str.c_str());
            va_start(ap, fmt_str);
            finaln = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
            va_end(ap);
            
            if (finaln < 0 || finaln >= n)
            {
                n += abs(finaln - n + 1);
            }
            else
            {
                break;
            }
        }
        
        return std::string(formatted.get());
    }
    
    static inline std::vector<std::string> Split(const std::string &str, const std::string &delim, const bool trim_empty = false){
        size_t pos, last_pos = 0, len;
        std::vector<std::string> tokens;
        
        while(true) {
            pos = str.find(delim, last_pos);
            if (pos == std::string::npos) {
                pos = str.size();
            }
            
            len = pos-last_pos;
            if ( !trim_empty || len != 0) {
                tokens.push_back(str.substr(last_pos, len));
            }
            
            if (pos == str.size()) {
                break;
            } else {
                last_pos = pos + delim.size();
            }
        }
        
        return tokens;
    }
    
    static inline std::string ReadFile(const std::string &filepath) {
        std::ifstream ifs(filepath.c_str());
        std::string content( (std::istreambuf_iterator<char>(ifs) ),
                            (std::istreambuf_iterator<char>()    ) );
        ifs.close();
        return content;
    }
    
    static inline void WriteFile(const std::string &filepath, const std::string &content) {
        std::ofstream ofs(filepath.c_str());
        ofs << content;
        ofs.close();
        return;
    }

    static inline std::string ParseDirectory(const std::string& path)
    {
        std::string result;
        auto tokens = StringUtils::Split(path, "/");
        bool start = false;
        for (int i = 0; i < tokens.size() - 1; i++)
        {
            if (tokens[i] == "assets")
            {
                start = true;
                continue;
            }

            if (start)
            {
                result += (tokens[i] + "/");
            }
        }

        if (!start)
        {
            return path;
        }

        return result;
    }

    static inline void ParseFileName(const std::string& name, std::string& nameWithoutSuffix, std::string& suffix, bool& hasSuffix)
    {

        auto tokens = StringUtils::Split(name, ".");
        if (tokens.size() >= 2)
        {
            hasSuffix = true;
            nameWithoutSuffix = tokens[0];
            suffix = tokens[tokens.size() - 1];
        }
        else
        {
            hasSuffix = false;
        }
    }
}
