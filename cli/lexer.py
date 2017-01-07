import ply.lex as lex

tokens = (
    'ID',
)


def Lexer():
    t_ignore = ' \t'

    t_ID = r'[a-zA-Z_][a-zA-Z_0-9]*'

    literals = ':'

    return lex.lex()
