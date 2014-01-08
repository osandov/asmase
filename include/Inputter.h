/*
 * Inputter class.
 *
 * Copyright (C) 2013-2014 Omar Sandoval
 *
 * This file is part of asmase.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ASMASE_INPUTTER_H
#define ASMASE_INPUTTER_H

#include <cstdio>
#include <memory>
#include <string>
#include <utility>
#include <vector>

/** Class for reading line-by-line input from either a file or stdin. */
class Inputter {
    /** Input file and position in it. */
    class InputFile {
    public:
        std::string filename;
        int lineno;
        FILE *file;

        InputFile(const std::string &filename, int lineno, FILE *file)
            : filename{filename}, lineno{lineno}, file{file} {}

        ~InputFile()
        {
            if (file)
                fclose(file);
        }
    };

    /**
     * Input file stack. The back is the current file and the front represents
     * stdin.
     */
    std::vector<InputFile> files;

    /** Size of line for getline. */
    size_t lineBufferSize;

    /** Line for getline. */
    char *lineBuffer;

public:
    /** The maximum depth of the redirection stack. */
    static const size_t MAX_FILES = 128;

    Inputter();
    ~Inputter();

    /**
     * Redirect input to read from the given filename.
     * @return Zero on success, nonzero on failure.
     */
    int redirectInput(const std::string &filename);

    /**
     * Get a line from the current file. The newline character is included. On
     * EOF of the last file or an error, an empty string is returned.
     */
    std::string readLine(const std::string &prompt);

    /** Return the name of the file we last read from. */
    const std::string &currentFilename() const;

    /** Return the line number of the line we just read. */
    int currentLineno() const;
};

#endif /* ASMASE_INPUTTER_H */
