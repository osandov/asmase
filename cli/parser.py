from collections import namedtuple
import ply.yacc as yacc

from lexer import tokens


Command = namedtuple('Command', ['name'])


def Parser():
    def p_command(p):
        "command : ':' ID"
        p[0] = Command(name=p[2])

    return yacc.yacc()
