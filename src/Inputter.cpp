#include <iostream>
#include <fstream>

#include <readline/readline.h>
#include <readline/history.h>

#include "Inputter.h"

Inputter::Inputter()
{
    files.push_back({"<stdin>", 0, nullptr});
    using_history();
}

Inputter::~Inputter()
{
    clear_history();
}

/* See Inputter.h. */
int Inputter::redirectInput(const std::string &filename)
{
    if (files.size() >= Inputter::MAX_FILES) {
        std::cerr << "redirection stack too deep\n";
        return 1;
    }

    std::unique_ptr<std::istream> file{new std::ifstream{filename}};

    if (!file->good()) {
        std::cerr << "could not open file\n";
        return 1;
    }

    files.push_back({filename, 0, nullptr});
    files.back().stream.swap(file);
    return 0;
}

/* See Inputter.h. */
std::string Inputter::readLine(const std::string &prompt)
{
    std::string line;
    bool gotLine = false;

    do {
        if (files.size() > 1) {
            std::istream &file = *files.back().stream;
            std::getline(file, line);
            if (file.eof()) // No more input, pop the stack and retry
                files.pop_back();
            else {
                line += '\n';
                gotLine = true;
            }
        } else {
            char *cline;
            if ((cline = readline(prompt.c_str()))) {
                if (*cline) // If the line isn't empty, add it to the history
                    add_history(cline);
                line = cline;
                line += '\n';
                free(cline);
            }
            gotLine = true;
        }
    } while (!gotLine);

    ++files.back().lineno;
    return line;
}

/* See Inputter.h. */
const std::string &Inputter::currentFilename() const
{
    return files.back().filename;
}

/* See Inputter.h. */
int Inputter::currentLineno() const
{
    return files.back().lineno;
}
