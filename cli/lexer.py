import codecs
import ply.lex as lex


class LexerError(Exception):
    def __init__(self, pos, msg):
        self.pos = pos
        self.msg = msg


tokens = (
    'ID',
    'STRING',
    'VAR',
)


def Lexer():
    def t_error(t):
        raise LexerError(t.lexpos + 1, 'illegal character {!r}'.format(t.value[0]))

    t_ignore = ' \t'

    t_ID = r'[a-zA-Z_][a-zA-Z_0-9]*'

    def t_VAR(t):
        r'\$[a-zA-Z_0-9]+'
        t.value = t.value[1:]
        return t

    def t_STRING(t):
        r'"(\\.|[^"\\])*"'
        try:
            t.value = codecs.escape_decode(t.value[1:-1].encode())[0].decode()
        except ValueError as e:
            raise LexerError(t.lexpos + 1, str(e))
        return t

    literals = ':'

    return lex.lex()
