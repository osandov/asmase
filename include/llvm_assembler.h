/*
 * Copyright (C) 2013 Omar Sandoval
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ASMASE_LLVM_ASSEMBLER_H
#define ASMASE_LLVM_ASSEMBLER_H

/** A container for parsed command line parameters. */
struct Parameters {
    const char *ProgName;
    const std::string &ArchName;
    const std::string &TripleName;
    const cl::list<std::string> &MAttrs;
    const std::string &MCPU;

    Parameters(const char *PN,
               const std::string &AN,
               const std::string &TN,
               const cl::list<std::string> &MA,
               const std::string &MCPU);
};

/**
 * Return a structure containing all parsed command line parameters. This
 * must be called after init_assemblers.
 */
Parameters GetParameters();

#endif /* ASMASE_LLVM_ASSEMBLER_H */
