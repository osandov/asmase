from collections import namedtuple
import ply.yacc as yacc

from lexer import tokens


Command = namedtuple('Command', ['name', 'args'])
Identifier = namedtuple('Identifier', ['name'])


def Parser():
    def p_command(p):
        """
        command : ':' ID identifier_list
                | ':' ID
        """
        if len(p) == 4:
            p[0] = Command(name=p[2], args=p[3])
        else:
            p[0] = Command(name=p[2], args=[])

    def p_identifier_list(p):
        """
        identifier_list : identifier_list identifier
                        | identifier
        """
        if len(p) == 2:
            p[0] = [p[1]]
        else:
            p[0] = p[1].copy()
            p[0].append(p[2])

    def p_identifier(p):
        """
        identifier : ID
        """
        p[0] = Identifier(p[1])

    return yacc.yacc()
