#ifndef ASMASE_INPUTTER_H
#define ASMASE_INPUTTER_H

#include <iostream>
#include <memory>
#include <utility>
#include <vector>

class Inputter {
    struct InputFile {
        std::string filename;
        int lineno;
        std::unique_ptr<std::istream> stream;
    };

    std::vector<InputFile> files;

public:
    static const int MAX_FILES = 128;

    Inputter();
    ~Inputter();

    int redirectInput(const std::string &filename);

    /** Get line including the newline character or empty string on EOF. */
    std::string readLine(const std::string &prompt);

    const std::string &currentFilename() const;
    int currentLineno() const;
};

#endif /* ASMASE_INPUTTER_H */
