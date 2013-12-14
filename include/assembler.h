#ifndef ASIP_ASSEMBLER_H
#define ASIP_ASSEMBLER_H

/** An opaque handle for an assembler. */
struct assembler;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the assembler system, including parsing command line parameters.
 */
int init_assemblers(int argc, char **argv);

/** Shutdown and clean up for the assembler system. */
void shutdown_assemblers();

/** Create a new assembler. */
struct assembler *create_assembler();

/** Destroy an assembler. */
void destroy_assembler(struct assembler *ctx);

/**
 * Assemble an instruction to machine code.
 * @param in The assembly instruction.
 * @param in_size The length of the assembly instruction in characters.
 * @param out A pointer to a buffer in which to return the machine code. This
 * buffer may be realloc'd if it is not big enough to hold the machine code.
 * @param out_size A pointer to the size of the out buffer. This may be updated
 * to reflect a resizing of the buffer.
 * @return The length in bytes of the machine code generated.
 */
ssize_t assemble_instruction(struct assembler *ctx, const char *in,
                             unsigned char **out, size_t *out_size);
#ifdef __cplusplus
}
#endif
#endif /* ASIP_ASSEMBLER_H */
