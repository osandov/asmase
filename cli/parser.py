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
                | quit
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
        "print : PRINT error NEWLINE"
        raise CliSyntaxError(p[2].lexpos + 1, 'expected primary expression')

    def p_quit(p):
        """
        quit : QUIT NEWLINE
             | QUIT primary_expression NEWLINE
        """
        if len(p) == 3:
            p[0] = cli.Quit(None)
        else:
            p[0] = cli.Quit(p[2])

    def p_quit_error(p):
        """
        quit : QUIT error NEWLINE
             | QUIT primary_expression error NEWLINE
        """
        if len(p) == 4:
            raise CliSyntaxError(p[2].lexpos + 1, 'expected primary expression')
        else:
            raise CliSyntaxError(p[3].lexpos + 1, 'expected newline')

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
        expression_list : expression_list primary_expression
                        | primary_expression
        """
        if len(p) == 2:
            p[0] = [p[1]]
        else:
            p[0] = p[1] + [p[2]]

    precedence = (
        ('left', 'OR_OP'),
        ('left', 'AND_OP'),
        ('left', '|'),
        ('left', '^'),
        ('left', '&'),
        ('left', 'EQ_OP', 'NE_OP'),
        ('left', '<', '>', 'LE_OP', 'GE_OP'),
        ('left', 'LEFT_OP', 'RIGHT_OP'),
        ('left', '+', '-'),
        ('left', '*', '/', '%'),
        ('right', 'UPLUS', 'UMINUS', '!', '~'),
    )

    def p_expression(p):
        """
        expression : primary_expression
        """
        p[0] = p[1]

    def p_binary_expression(p):
        """
        expression : expression '+' expression
                   | expression '-' expression
                   | expression '*' expression
                   | expression '/' expression
                   | expression '%' expression
                   | expression LEFT_OP expression
                   | expression RIGHT_OP expression
                   | expression '<' expression
                   | expression '>' expression
                   | expression LE_OP expression
                   | expression GE_OP expression
                   | expression EQ_OP expression
                   | expression NE_OP expression
                   | expression '&' expression
                   | expression '^' expression
                   | expression '|' expression
                   | expression AND_OP expression
                   | expression OR_OP expression
        """
        p[0] = cli.BinaryOp(p[2], p[1], p[3])

    def p_unary_expression(p):
        """
        primary_expression : '+' expression %prec UPLUS
                           | '-' expression %prec UMINUS
                           | '!' expression
                           | '~' expression
        """
        p[0] = cli.UnaryOp(p[1], p[2])

    def p_literal_expression(p):
        """
        primary_expression : INT
                           | STRING
        """
        p[0] = p[1]

    def p_variable_expression(p):
        "primary_expression : VAR"
        p[0] = cli.Variable(p[1])

    def p_parenthetical_expression(p):
        "primary_expression : '(' expression ')'"
        p[0] = p[2]

    return yacc.yacc()
