import codecs
import ply.lex as lex
from ply.lex import TOKEN

import cli
from cli import CliSyntaxError


command_tokens = {':' + command: command.upper() for command in cli.commands}


tokens = (
    'NEWLINE',
    'ID',
    'INT',
    'STRING',
    'VAR',
) + tuple(sorted(command_tokens.values()))


states = (
    ('args', 'exclusive'),
)


def Lexer():
    def t_ANY_error(t):
        raise CliSyntaxError(t.lexpos + 1, f'illegal character {t.value[0]!r}')

    t_ANY_ignore = ' \t'

    t_ANY_NEWLINE = r'\n'

    # In the INITIAL state, we are looking for a command name starting with
    # ':'. We then transition immediately into the args state.
    def t_COMMAND(t):
        r':[a-zA-Z_0-9]+'
        try:
            t.type = command_tokens[t.value]
        except KeyError:
            raise CliSyntaxError(t.lexpos + 1, f'unknown command {t.value!r}')
        t.lexer.begin('args')
        return t

    t_args_ID = r'[a-zA-Z_][a-zA-Z_0-9]*'

    def t_args_INT(t):
        r'(0x)?[0-9]+'
        if t.value.startswith('0x'):
            t.value = int(t.value, 16)
        elif t.value.startswith('0'):
            t.value = int(t.value, 8)
        else:
            t.value = int(t.value, 10)
        return t

    def t_args_STRING(t):
        r'"(\\.|[^"\\])*"'
        try:
            t.value = codecs.escape_decode(t.value[1:-1].encode())[0].decode()
        except ValueError as e:
            raise CliSyntaxError(t.lexpos + 1, str(e))
        return t

    def t_args_VAR(t):
        r'\$[a-zA-Z_0-9]+'
        t.value = t.value[1:]
        return t

    return lex.lex()
