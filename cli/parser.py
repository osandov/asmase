import ply.yacc as yacc

import cli
from cli import CliSyntaxError
from cli.lexer import tokens


def Parser():
    def p_error(p):
        if p is None:
            raise CliSyntaxError(None, 'unexpected EOF while parsing')

    def p_command(p):
        """
        command : copying
                | help
                | print
                | source
                | warranty
        """
        p[0] = p[1]

    def p_copying(p):
        "copying : COPYING NEWLINE"
        p[0] = cli.Copying()

    def p_copying_error(p):
        "copying : COPYING error NEWLINE"
        raise CliSyntaxError(p[2].lexpos + 1, 'expected newline')

    def p_help(p):
        """
        help : HELP NEWLINE
             | HELP ID NEWLINE
        """
        if len(p) == 3:
            p[0] = cli.Help(None)
        else:
            p[0] = cli.Help(p[2])

    def p_help_error(p):
        """
        help : HELP error NEWLINE
             | HELP ID error NEWLINE
        """
        if len(p) == 4:
            raise CliSyntaxError(p[2].lexpos + 1, 'expected identifier')
        else:
            raise CliSyntaxError(p[3].lexpos + 1, 'expected newline')

    def p_print(p):
        """
        print : PRINT NEWLINE
              | PRINT expression_list NEWLINE
        """
        if len(p) == 3:
            p[0] = cli.Print([])
        else:
            p[0] = cli.Print(p[2])

    def p_print_error(p):
        """
        print : PRINT error NEWLINE
        """
        raise CliSyntaxError(p[2].lexpos + 1, 'expected expression')

    def p_source(p):
        "source : SOURCE STRING NEWLINE"
        p[0] = cli.Source(p[2])

    def p_source_error(p):
        """
        source : SOURCE error NEWLINE
               | SOURCE STRING error NEWLINE
        """
        if len(p) == 4:
            raise CliSyntaxError(p[2].lexpos + 1, 'expected string')
        else:
            raise CliSyntaxError(p[3].lexpos + 1, 'expected newline')

    def p_warranty(p):
        "warranty : WARRANTY NEWLINE"
        p[0] = cli.Warranty()

    def p_warranty_error(p):
        "warranty : WARRANTY error NEWLINE"
        raise CliSyntaxError(p[2].lexpos + 1, 'expected newline')

    def p_expression_list(p):
        """
        expression_list : expression_list expression
                        | expression
        """
        if len(p) == 2:
            p[0] = [p[1]]
        else:
            p[0] = p[1] + [p[2]]

    def p_expression(p):
        """
        expression : STRING
                   | variable_expression
        """
        p[0] = p[1]

    def p_variable_expression(p):
        "variable_expression : VAR"
        p[0] = cli.Variable(p[1])

    return yacc.yacc()
