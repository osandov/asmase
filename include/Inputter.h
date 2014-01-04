#ifndef ASMASE_INPUTTER_H
#define ASMASE_INPUTTER_H

#include <iostream>
#include <memory>
#include <utility>
#include <vector>

/** Class for reading line-by-line input from either a file or stdin. */
class Inputter {
    /** Input file and position in it. */
    class InputFile {
    public:
        std::string filename;
        int lineno;
        std::unique_ptr<std::istream> stream;
    };

    /**
     * Input file stack. The back is the current file and the front represents
     * std::cout.
     */
    std::vector<InputFile> files;

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
