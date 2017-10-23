const should = require('chai').should();

const {createAsmaseLexer, AsmaseParser, createAsmaseVisitor} = require('../lang.js');
const {AsmaseTypeError, AsmaseValue} = require('../value.js');

const lexer = createAsmaseLexer();
const parser = new AsmaseParser();
const visitor = createAsmaseVisitor(parser);

describe('parser', function() {
  beforeEach(function() {
    this.parse = (function(line) {
      const lexResult = lexer.tokenize(line);
      lexResult.errors.should.be.empty;
      parser.input = lexResult.tokens;
      const cst = parser.command();
      parser.errors.should.be.empty;
      return visitor.visit(cst);
    });
  });

  it('should parse a command with no arguments', function() {
    this.parse(':help').should.eql({command: 'help', args: []});
  });

  it('should parse a command with one argument', function() {
    this.parse(':h foo').should.eql({command: 'h', args: ['foo']});
  });

  it('should parse a command with multiple arguments', function() {
    this.parse(':print foo bar baz').should.eql({command: 'print', args: ['foo', 'bar', 'baz']});
  });

  it('should parse the repeat command', function() {
    this.parse(':').should.eql({command: '', args: []});
  });
});


describe('visitor', function() {
  beforeEach(function() {
    visitor.getVariable = ((varName) => new AsmaseValue(5));
    this.eval = (function(expr) {
      const lexResult = lexer.tokenize(': (' + expr + ')');
      lexResult.errors.should.be.empty;
      parser.input = lexResult.tokens;
      const cst = parser.command();
      parser.errors.should.be.empty;
      return visitor.visit(cst).args[0];
    });
  });

  const testCases = {
    'variable': {
      '$foo': '5',
    },
    'integer': {
      '10': '10',
      '0x10': '16',
      '010': '8',
    },
    'string': {
      '"foo bar"': 'foo bar',
      '"foo\\tbar"': 'foo\tbar',
      '"\\"foo\\"bar\\""': '"foo"bar"',
      '"foo\\x0a"': 'foo\n',
      '"foo\\z"': 'foo\\z',
      '"foo\\xzz"': 'foo\\xzz',
      '"foo\\xzz"': 'foo\\xzz',
    },
    'unary +': {
      '+5': '5',
      '+"foo"': undefined,
    },
    'unary -': {
      '-5': '-5',
      '-"foo"': undefined,
    },
    '!': {
      '!0': '1',
      '!5': '0',
      '!""': '1',
      '!"foo"': '0',
    },
    '~': {
      '~5': (~5).toString(),
      '~"foo"': undefined,
    },
    '*': {
      '2 * 3': '6',
      '"foo" * "bar"': undefined,
      '"foo" * 5': undefined,
      '5 * "foo"': undefined,
    },
    '/': {
      '10 / 3': '3',
      '"foo" / "bar"': undefined,
      '"foo" / 5': undefined,
      '5 / "foo"': undefined,
    },
    '%': {
      '10 % 3': '1',
      '"foo" % "bar"': undefined,
      '"foo" % 5': undefined,
      '5 % "foo"': undefined,
    },
    '+': {
      '1 + 2': '3',
      '"foo" + "bar"': 'foobar',
      '"foo" + 5': undefined,
      '5 + "foo"': undefined,
    },
    '-': {
      '1 - 2': '-1',
      '"foo" - "bar"': undefined,
      '"foo" - 5': undefined,
      '5 - "foo"': undefined,
    },
    '<<': {
      '1 << 10': '1024',
    },
    '>>': {
      '4096 >> 12': '1',
    },
    '<': {
      '1 < 1': '0',
      '1 < 2': '1',
      '"a" < "a"': '0',
      '"a" < "b"': '1',
      '"a" < 1': undefined,
      '1 < "a"': undefined,
    },
    '<=': {
      '1 <= 1': '1',
      '1 <= 2': '1',
      '"a" <= "a"': '1',
      '"a" <= "b"': '1',
      '"a" <= 1': undefined,
      '1 <= "a"': undefined,
    },
    '>': {
      '1 > 1': '0',
      '2 > 1': '1',
      '"a" > "a"': '0',
      '"b" > "a"': '1',
      '"a" > 1': undefined,
      '1 > "a"': undefined,
    },
    '>=': {
      '1 >= 1': '1',
      '2 >= 1': '1',
      '"a" >= "a"': '1',
      '"b" >= "a"': '1',
      '"a" >= 1': undefined,
      '1 >= "a"': undefined,
    },
    '==': {
      '1 == 1': '1',
      '1 == 2': '0',
      '"a" == "a"': '1',
      '"a" == "b"': '0',
      '"a" == 1': '0',
      '1 == "a"': '0',
    },
    '!=': {
      '1 != 1': '0',
      '1 != 2': '1',
      '"a" != "a"': '0',
      '"a" != "b"': '1',
      '"a" != 1': '1',
      '1 != "a"': '1',
    },
    '&': {
      '6 & 3': '2',
      '"foo" & "bar"': undefined,
      '"foo" & 5': undefined,
      '5 & "foo"': undefined,
    },
    '^': {
      '6 ^ 3': '5',
      '"foo" ^ "bar"': undefined,
      '"foo" ^ 5': undefined,
      '5 ^ "foo"': undefined,
    },
    '|': {
      '6 | 3': '7',
      '"foo" | "bar"': undefined,
      '"foo" | 5': undefined,
      '5 | "foo"': undefined,
    },
    '&&': {
      '1 && 1': '1',
      '1 && 0': '0',
      '0 && 1': '0',
      '0 && 0': '0',
      '0 && (1 / 0)': '0',
      '"a" && ""': '0',
    },
    '||': {
      '1 || 1': '1',
      '1 || 0': '1',
      '0 || 1': '1',
      '0 || 0': '0',
      '1 || (1 / 0)': '1',
      '"a" || ""': '1',
    },
    'precedence': {
      '1 || 0 && 0': '1',
      '(1 || 0) && 0': '0',
      '0 && 0 | 1': '0',
      '(0 && 0) | 1': '1',
      '3 | 2 ^ 2': '3',
      '(3 | 2) ^ 2': '1',
      '3 ^ 2 & 2': '1',
      '(3 ^ 2) & 2': '0',
      '1 & 3 == 1': '0',
      '(1 & 3) == 1': '1',
      '0 == 1 < 0': '1',
      '(0 == 1) < 0': '0',
      '1 < 2 << 3': '1',
      '(1 < 2) << 3': '8',
      '2 << 1 + 2': '16',
      '(2 << 1) + 2': '6',
      '2 + 2 * 2': '6',
      '(2 + 2) * 2': '8',
      '~--1': '-2',
    },
  };

  for (const operator in testCases) {
    if (testCases.hasOwnProperty(operator)) {
      describe(operator, function() {
        for (const expr in testCases[operator]) {
          const expected = testCases[operator][expr];
          if (typeof expected === 'undefined') {
            it('should throw AsmaseTypeError for ' + expr, function() {
              (() => this.eval(expr)).should.throw(AsmaseTypeError);
            });
          } else {
            it('should evaluate ' + expr + ' = ' + expected, function() {
              this.eval(expr).toString().should.equal(expected);
            });
          }
        }
      });
    }
  }
});
