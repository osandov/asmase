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
        command : repeat
                | copying
                | help
                | memory
                | print
                | quit
                | registers
                | source
                | warranty
        """
        p[0] = p[1]

    def p_repeat(p):
        "repeat : REPEAT NEWLINE"
        p[0] = ('repeat',)

    def p_copying(p):
        "copying : COPYING NEWLINE"
        p[0] = ('copying',)

    def p_copying_error(p):
        "copying : COPYING error NEWLINE"
        raise CliSyntaxError(p[2].lexpos + 1, 'expected newline')

    def p_help(p):
        """
        help : HELP NEWLINE
             | HELP ID NEWLINE
        """
        if len(p) == 3:
            p[0] = ('help', None)
        else:
            p[0] = ('help', p[2])

    def p_help_error(p):
        """
        help : HELP error NEWLINE
             | HELP ID error NEWLINE
        """
        if len(p) == 4:
            raise CliSyntaxError(p[2].lexpos + 1, 'expected identifier')
        else:
            raise CliSyntaxError(p[3].lexpos + 1, 'expected newline')

    def p_memory(p):
        """
        memory : MEMORY NEWLINE
               | MEMORY primary_expression NEWLINE
               | MEMORY primary_expression primary_expression NEWLINE
               | MEMORY primary_expression primary_expression ID NEWLINE
               | MEMORY primary_expression primary_expression ID INT NEWLINE
        """
        if len(p) == 3:
            p[0] = ('memory', None, None, None, None)
        elif len(p) == 4:
            p[0] = ('memory', p[2], None, None, None)
        elif len(p) == 5:
            p[0] = ('memory', p[2], p[3], None, None)
        elif len(p) == 6:
            p[0] = ('memory', p[2], p[3], p[4], None)
        else:
            p[0] = ('memory', p[2], p[3], p[4], p[5])

    def p_memory_error(p):
        """
        memory : MEMORY error NEWLINE
               | MEMORY primary_expression error NEWLINE
               | MEMORY primary_expression primary_expression error NEWLINE
               | MEMORY primary_expression primary_expression ID error NEWLINE
               | MEMORY primary_expression primary_expression ID INT error NEWLINE
        """
        if len(p) == 4:
            raise CliSyntaxError(p[2].lexpos + 1, 'expected primary expression')
        elif len(p) == 5:
            raise CliSyntaxError(p[3].lexpos + 1, 'expected primary expression')
        elif len(p) == 6:
            raise CliSyntaxError(p[4].lexpos + 1, 'expected identifier')
        elif len(p) == 7:
            raise CliSyntaxError(p[5].lexpos + 1, 'expected integer')
        else:
            raise CliSyntaxError(p[6].lexpos + 1, 'expected newline')

    def p_print(p):
        """
        print : PRINT NEWLINE
              | PRINT expression_list NEWLINE
        """
        if len(p) == 3:
            p[0] = ('print', [])
        else:
            p[0] = ('print', p[2])

    def p_print_error(p):
        "print : PRINT error NEWLINE"
        raise CliSyntaxError(p[2].lexpos + 1, 'expected primary expression')

    def p_quit(p):
        """
        quit : QUIT NEWLINE
             | QUIT primary_expression NEWLINE
        """
        if len(p) == 3:
            p[0] = ('quit', None)
        else:
            p[0] = ('quit', p[2])

    def p_quit_error(p):
        """
        quit : QUIT error NEWLINE
             | QUIT primary_expression error NEWLINE
        """
        if len(p) == 4:
            raise CliSyntaxError(p[2].lexpos + 1, 'expected primary expression')
        else:
            raise CliSyntaxError(p[3].lexpos + 1, 'expected newline')

    def p_registers(p):
        """
        registers : REGISTERS NEWLINE
                  | REGISTERS identifier_list NEWLINE
        """
        if len(p) == 3:
            p[0] = ('registers', None)
        else:
            p[0] = ('registers', p[2])

    def p_registers_error(p):
        "registers : REGISTERS error NEWLINE"
        raise CliSyntaxError(p[2].lexpos + 1, 'expected identifier')

    def p_source(p):
        "source : SOURCE STRING NEWLINE"
        p[0] = ('source', p[2])

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
        p[0] = ('warranty',)

    def p_warranty_error(p):
        "warranty : WARRANTY error NEWLINE"
        raise CliSyntaxError(p[2].lexpos + 1, 'expected newline')

    def p_identifier_list(p):
        """
        identifier_list : identifier_list ID
                        | ID
        """
        if len(p) == 2:
            p[0] = [p[1]]
        else:
            p[0] = p[1] + [p[2]]

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
        p[0] = ('binary_op', p[2], p[1], p[3])

    def p_unary_expression(p):
        """
        primary_expression : '+' expression %prec UPLUS
                           | '-' expression %prec UMINUS
                           | '!' expression
                           | '~' expression
        """
        p[0] = ('unary_op', p[1], p[2])

    def p_literal_expression(p):
        """
        primary_expression : INT
                           | STRING
        """
        p[0] = p[1]

    def p_variable_expression(p):
        "primary_expression : VAR"
        p[0] = ('variable', p[1])

    def p_parenthetical_expression(p):
        "primary_expression : '(' expression ')'"
        p[0] = p[2]

    return yacc.yacc()
