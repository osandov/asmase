from collections import namedtuple
import ply.yacc as yacc

from cli.lexer import tokens


class ParserError(Exception):
    def __init__(self, pos, msg):
        self.pos = pos
        self.msg = msg


Command = namedtuple('Command', ['name', 'args'])
Identifier = namedtuple('Identifier', ['name'])
Variable = namedtuple('Variable', ['name'])


def Parser():
    def p_error(p):
        if p is None:
            raise ParserError(None, 'unexpected EOL while parsing')
        else:
            raise ParserError(p.lexpos + 1, 'unexpected token')

    def p_command(p):
        """
        command : ':' ID argument_list
                | ':' ID
        """
        if len(p) == 4:
            p[0] = Command(name=p[2], args=p[3])
        else:
            p[0] = Command(name=p[2], args=[])

    def p_argument_list(p):
        """
        argument_list : argument_list argument
                      | argument
        """
        if len(p) == 2:
            p[0] = [p[1]]
        else:
            p[0] = p[1].copy()
            p[0].append(p[2])

    def p_argument(p):
        """
        argument : identifier
                 | expression
        """
        p[0] = p[1]

    def p_identifier(p):
        "identifier : ID"
        p[0] = Identifier(p[1])

    def p_expression(p):
        """
        expression : string_expression
                   | variable_expression
        """
        p[0] = p[1]

    def p_string_expression(p):
        "string_expression : STRING"
        p[0] = p[1]

    def p_variable_expression(p):
        "variable_expression : VAR"
        p[0] = Variable(p[1])

    return yacc.yacc()
