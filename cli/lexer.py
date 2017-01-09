import ply.lex as lex


class LexerError(Exception):
    def __init__(self, pos, msg):
        self.pos = pos
        self.msg = msg


tokens = (
    'ID',
)


def Lexer():
    def t_error(t):
        raise LexerError(t.lexpos + 1, 'illegal character {!r}'.format(t.value[0]))

    t_ignore = ' \t'

    t_ID = r'[a-zA-Z_][a-zA-Z_0-9]*'

    literals = ':'

    return lex.lex()
