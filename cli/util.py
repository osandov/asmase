def escape_character(c, escape_single_quote=False, escape_double_quote=False,
                     escape_backslash=False):
    if c == 0:
        return r'\0'
    elif c == 7:
        return r'\a'
    elif c == 8:
        return r'\b'
    elif c == 9:
        return r'\t'
    elif c == 10:
        return r'\n'
    elif c == 11:
        return r'\v'
    elif c == 12:
        return r'\f'
    elif c == 13:
        return r'\r'
    elif escape_double_quote and c == 34:
        return r'\"'
    elif escape_single_quote and c == 39:
        return r"\'"
    elif escape_backslash and c == 92:
        return r'\\'
    elif 32 <= c <= 126:
        return f'{chr(c)}'
    else:
        return f'\\x{c:02x}'
