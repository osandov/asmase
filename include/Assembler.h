/*
 * Assembler class.
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

#ifndef ASMASE_ASSEMBLER_H
#define ASMASE_ASSEMBLER_H

#include <memory>
#include <string>

class Inputter;

/** Opaque handle for an assembler context. */
class AssemblerContext;

/** Class providing assembly of individual instructions. */
class Assembler {
    /** The assembler context for this assembler. */
    const std::shared_ptr<AssemblerContext> context;

public:
    /** Create an assembler in the given context. */
    Assembler(std::shared_ptr<AssemblerContext> &context)
        : context{context} {}

    /**
     * Assemble the given assembly instruction to machine code.
     * @return Zero on success, nonzero on failure.
     */
    int assembleInstruction(const std::string &instruction,
                            std::string &machineCodeOut,
                            const Inputter &inputter);

    /**
     * Create an assembler context which can be used to construct an
     * assembler.
     * @return nullptr on error.
     */
    static std::shared_ptr<AssemblerContext> createAssemblerContext();
};

#endif /* ASMASE_ASSEMBLER_H */
