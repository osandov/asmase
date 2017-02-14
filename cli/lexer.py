import codecs
import ply.lex as lex

import cli
from cli import CliSyntaxError


command_tokens = {':' + command: command.upper() for command in cli.commands}


tokens = (
    'NEWLINE',
    'LEFT_OP',
    'RIGHT_OP',
    'LE_OP',
    'GE_OP',
    'EQ_OP',
    'NE_OP',
    'AND_OP',
    'OR_OP',
    'ID',
    'INT',
    'STRING',
    'VAR',
    'REPEAT',
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
        r':[a-zA-Z_0-9]*'

        if t.value == ':':
            t.type = 'REPEAT'
            return t

        # Naive linear search for matching commands. We won't bother with a
        # more complicated solution until we have more commands.
        matches = []
        for command in command_tokens:
            if command.startswith(t.value):
                matches.append(command)
        matches.sort()

        if len(matches) == 1:
            t.type = command_tokens[matches[0]]
        elif len(matches) > 1:  # pragma: no cover
            raise CliSyntaxError(
                t.lexpos + 1, f"ambiguous command {t.value!r}: {', '.join(matches)}")
        else:
            raise CliSyntaxError(t.lexpos + 1, f'unknown command {t.value!r}')
        t.lexer.begin('args')
        return t

    literals = '!%&()*+-/<>^|~'

    t_ANY_LEFT_OP = r'<<'
    t_ANY_RIGHT_OP = r'>>'
    t_ANY_LE_OP = r'<='
    t_ANY_GE_OP = r'>='
    t_ANY_EQ_OP = r'=='
    t_ANY_NE_OP = r'!='
    t_ANY_AND_OP = r'&&'
    t_ANY_OR_OP = r'\|\|'

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
