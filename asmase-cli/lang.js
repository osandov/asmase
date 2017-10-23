/*
 * asmase CLI language.
 *
 * Copyright (C) 2017 Omar Sandoval
 *
 * This file is part of asmase.
 *
 * asmase is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * asmase is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with asmase.  If not, see <http://www.gnu.org/licenses/>.
 */

"use strict";

const {createToken, tokenMatcher, Lexer, Parser} = require('chevrotain');
const unescapeJS = require('unescape-js');
const {AsmaseValue} = require('./value.js');

const Whitespace = createToken({
  name: 'Whitespace',
  pattern: /\s+/,
  group: Lexer.SKIPPED,
  line_breaks: true,
});

const Command = createToken({
  name: 'Command',
  pattern: /:[a-zA-Z_0-9]*/,
  push_mode: 'argumentMode',
});

const Identifier = createToken({
  name: 'Identifier',
  pattern: /[a-zA-Z_][a-zA-Z_0-9]*/,
});

const Integer = createToken({
  name: 'Integer',
  pattern: /(?:0x)?[0-9a-fA-F]+/,
});

const String = createToken({
  name: 'String',
  pattern: /"(\\.|[^"\\])*"/,
});

const Variable = createToken({
  name: 'Variable',
  pattern: /\$[a-zA-Z_0-9]+/,
});

const EqualityOperator = createToken({
  name: 'EqualityOperator',
  pattern: Lexer.NA,
});

const RelationalOperator = createToken({
  name: 'RelationalOperator',
  pattern: Lexer.NA,
});

const ShiftOperator = createToken({
  name: 'ShiftOperator',
  pattern: Lexer.NA,
});

const UnaryOperator = createToken({
  name: 'UnaryOperator',
  pattern: Lexer.NA,
});

const AdditiveOperator = createToken({
  name: 'AdditiveOperator',
  pattern: Lexer.NA,
  parent: UnaryOperator,
});

const MultiplicativeOperator = createToken({
  name: 'MultiplicativeOperator',
  pattern: Lexer.NA,
});

const LShift = createToken({name: 'LShift', pattern: /<</, parent: ShiftOperator});
const RShift = createToken({name: 'RShift', pattern: />>/, parent: ShiftOperator});
const Le = createToken({name: 'Le', pattern: /<=/, parent: RelationalOperator});
const Ge = createToken({name: 'Ge', pattern: />=/, parent: RelationalOperator});
const Eq = createToken({name: 'Eq', pattern: /==/, parent: EqualityOperator});
const Ne = createToken({name: 'Ne', pattern: /!=/, parent: EqualityOperator});
const And = createToken({name: 'And', pattern: /&&/});
const Or = createToken({name: 'Or', pattern: /\|\|/});
const Not = createToken({name: 'Not', pattern: /!/, parent: UnaryOperator});
const Lt = createToken({name: 'Lt', pattern: /</, parent: RelationalOperator});
const Gt = createToken({name: 'Gt', pattern: />/, parent: RelationalOperator});
const Plus = createToken({name: 'Plus', pattern: /\+/, parent: AdditiveOperator});
const Minus = createToken({name: 'Minus', pattern: /-/, parent: AdditiveOperator});
const Times = createToken({name: 'Times', pattern: /\*/, parent: MultiplicativeOperator});
const Divide = createToken({name: 'Divide', pattern: /\//, parent: MultiplicativeOperator});
const Mod = createToken({name: 'Mod', pattern: /%/, parent: MultiplicativeOperator});
const BitAnd = createToken({name: 'BitAnd', pattern: /&/});
const BitOr = createToken({name: 'BitOr', pattern: /\|/});
const Xor = createToken({name: 'Xor', pattern: /\^/});
const BitNot = createToken({name: 'BitNot', pattern: /~/, parent: UnaryOperator});
const LParen = createToken({name: 'LParen', pattern: /\(/});
const RParen = createToken({name: 'RParen', pattern: /\)/});

const allTokens = [
  Whitespace,
  Command,
  Identifier,
  Integer,
  String,
  Variable,
  EqualityOperator,
  RelationalOperator,
  ShiftOperator,
  UnaryOperator,
  AdditiveOperator,
  MultiplicativeOperator,
  // The lexer chooses the first match, not the longest match, so these
  // operators must go first.
  LShift,
  RShift,
  Le,
  Ge,
  Eq,
  Ne,
  And,
  Or,
  Not,
  Lt,
  Gt,
  Plus,
  Minus,
  Times,
  Divide,
  Mod,
  BitAnd,
  BitOr,
  Xor,
  BitNot,
  LParen,
  RParen,
];

exports.createAsmaseLexer = function createAsmaseLexer() {
  return new Lexer({
    modes: {
      commandMode: [
        Whitespace,
        Command,
      ],
      argumentMode: allTokens.filter((token) => token !== Command),
    },
    defaultMode: 'commandMode',
  });
};

class AsmaseParser extends Parser {
  constructor(input) {
    super(input, allTokens, {outputCst: true});

    const $ = this;

    $.RULE('command', () => {
      $.CONSUME(Command);
      $.MANY(() => {
        $.SUBRULE($.argument);
      });
    });

    $.RULE('argument', () => {
      $.OR([
        {ALT: () => {$.CONSUME(Identifier)}},
        {ALT: () => {$.SUBRULE($.unaryExpression)}},
      ]);
    });

    $.RULE('expression', () => {
      $.SUBRULE($.logicalOrExpression);
    });

    $.RULE('logicalOrExpression', () => {
      $.SUBRULE($.logicalAndExpression);
      $.MANY(() => {
        $.CONSUME(Or);
        $.SUBRULE2($.logicalAndExpression);
      });
    });

    $.RULE('logicalAndExpression', () => {
      $.SUBRULE($.bitwiseOrExpression);
      $.MANY(() => {
        $.CONSUME(And);
        $.SUBRULE2($.bitwiseOrExpression);
      });
    });

    $.RULE('bitwiseOrExpression', () => {
      $.SUBRULE($.xorExpression);
      $.MANY(() => {
        $.CONSUME(BitOr);
        $.SUBRULE2($.xorExpression);
      });
    });

    $.RULE('xorExpression', () => {
      $.SUBRULE($.bitwiseAndExpression);
      $.MANY(() => {
        $.CONSUME(Xor);
        $.SUBRULE2($.bitwiseAndExpression);
      });
    });

    $.RULE('bitwiseAndExpression', () => {
      $.SUBRULE($.equalityExpression);
      $.MANY(() => {
        $.CONSUME(BitAnd);
        $.SUBRULE2($.equalityExpression);
      });
    });

    $.RULE('equalityExpression', () => {
      $.SUBRULE($.relationalExpression);
      $.MANY(() => {
        $.CONSUME(EqualityOperator);
        $.SUBRULE2($.relationalExpression);
      });
    });

    $.RULE('relationalExpression', () => {
      $.SUBRULE($.shiftExpression);
      $.MANY(() => {
        $.CONSUME(RelationalOperator);
        $.SUBRULE2($.shiftExpression);
      });
    });

    $.RULE('shiftExpression', () => {
      $.SUBRULE($.additiveExpression);
      $.MANY(() => {
        $.CONSUME(ShiftOperator);
        $.SUBRULE2($.additiveExpression);
      });
    });

    $.RULE('additiveExpression', () => {
      $.SUBRULE($.multiplicativeExpression);
      $.MANY(() => {
        $.CONSUME(AdditiveOperator);
        $.SUBRULE2($.multiplicativeExpression);
      });
    });

    $.RULE('multiplicativeExpression', () => {
      $.SUBRULE($.unaryExpression);
      $.MANY(() => {
        $.CONSUME(MultiplicativeOperator);
        $.SUBRULE2($.unaryExpression);
      });
    });

    $.RULE('unaryExpression', () => {
      $.MANY(() => {
        $.CONSUME(UnaryOperator);
      });
      $.SUBRULE($.primaryExpression);
    });

    $.RULE('primaryExpression', () => {
      $.OR([
        {ALT: () => {$.CONSUME(Integer)}},
        {ALT: () => {$.CONSUME(String)}},
        {ALT: () => {$.CONSUME(Variable)}},
        {ALT: () => {$.SUBRULE($.parentheticalExpression)}},
      ]);
    });

    $.RULE('parentheticalExpression', () => {
      $.CONSUME(LParen);
      $.SUBRULE($.expression);
      $.CONSUME(RParen);
    });

    Parser.performSelfAnalysis(this);
  }
}

exports.AsmaseParser = AsmaseParser;

const binaryOperators = {
  [BitOr.tokenType]: AsmaseValue.prototype.or,
  [Xor.tokenType]: AsmaseValue.prototype.xor,
  [BitAnd.tokenType]: AsmaseValue.prototype.and,
  [Eq.tokenType]: AsmaseValue.prototype.eq,
  [Ne.tokenType]: AsmaseValue.prototype.ne,
  [Lt.tokenType]: AsmaseValue.prototype.lt,
  [Le.tokenType]: AsmaseValue.prototype.le,
  [Gt.tokenType]: AsmaseValue.prototype.gt,
  [Ge.tokenType]: AsmaseValue.prototype.ge,
  [LShift.tokenType]: AsmaseValue.prototype.lshift,
  [RShift.tokenType]: AsmaseValue.prototype.rshift,
  [Plus.tokenType]: AsmaseValue.prototype.add,
  [Minus.tokenType]: AsmaseValue.prototype.sub,
  [Times.tokenType]: AsmaseValue.prototype.mul,
  [Divide.tokenType]: AsmaseValue.prototype.div,
  [Mod.tokenType]: AsmaseValue.prototype.mod,
};

const unaryOperators = {
  [Plus.tokenType]: AsmaseValue.prototype.pos,
  [Minus.tokenType]: AsmaseValue.prototype.neg,
  [BitNot.tokenType]: AsmaseValue.prototype.invert,
  [Not.tokenType]: (function () {
    return new AsmaseValue(!this.toBoolean());
  }),
};

exports.createAsmaseVisitor = function createAsmaseVisitor(parser) {
  const BaseAsmaseVisitor = parser.getBaseCstVisitorConstructor();

  class AsmaseVisitor extends BaseAsmaseVisitor {
    constructor() {
      super();
      this.validateVisitor();
    }

    command(ctx) {
      const command = ctx.Command[0].image.substring(1);
      const args = [];
      for (let i = 0; i < ctx.argument.length; i++) {
        args.push(this.visit(ctx.argument[i]));
      }
      return {command, args};
    }

    argument(ctx) {
      if (ctx.Identifier.length > 0) {
        return ctx.Identifier[0].image;
      }
      return this.visit(ctx.unaryExpression);
    }

    expression(ctx) {
      return this.visit(ctx.logicalOrExpression);
    }

    binaryExpression(ctx, children, operators) {
      let lhs = this.visit(children[0]);
      if (children.length > 1) {
        // Left to right
        for (let i = 1; i < children.length; i++) {
          const operator = operators[i - 1];
          // && and || are special cases because of short-circuiting.
          if (tokenMatcher(operator, And)) {
            if (lhs.toBoolean()) {
              const rhs = this.visit(children[i]);
              lhs = new AsmaseValue(rhs.toBoolean());
            } else {
              lhs = new AsmaseValue(false);
            }
          } else if (tokenMatcher(operator, Or)) {
            if (lhs.toBoolean()) {
              lhs = new AsmaseValue(true);
            } else {
              const rhs = this.visit(children[i]);
              lhs = new AsmaseValue(rhs.toBoolean());
            }
          } else {
            const rhs = this.visit(children[i]);
            lhs = binaryOperators[operator.tokenType].call(lhs, rhs);
          }
        }
      }
      return lhs;
    }

    logicalOrExpression(ctx) {
      return this.binaryExpression(ctx, ctx.logicalAndExpression, ctx.Or);
    }

    logicalAndExpression(ctx) {
      return this.binaryExpression(ctx, ctx.bitwiseOrExpression, ctx.And);
    }

    bitwiseOrExpression(ctx) {
      return this.binaryExpression(ctx, ctx.xorExpression, ctx.BitOr);
    }

    xorExpression(ctx) {
      return this.binaryExpression(ctx, ctx.bitwiseAndExpression, ctx.Xor);
    }

    bitwiseAndExpression(ctx) {
      return this.binaryExpression(ctx, ctx.equalityExpression, ctx.BitAnd);
    }

    equalityExpression(ctx) {
      return this.binaryExpression(ctx, ctx.relationalExpression, ctx.EqualityOperator);
    }

    relationalExpression(ctx) {
      return this.binaryExpression(ctx, ctx.shiftExpression, ctx.RelationalOperator);
    }

    shiftExpression(ctx) {
      return this.binaryExpression(ctx, ctx.additiveExpression, ctx.ShiftOperator);
    }

    additiveExpression(ctx) {
      return this.binaryExpression(ctx, ctx.multiplicativeExpression, ctx.AdditiveOperator);
    }

    multiplicativeExpression(ctx) {
      return this.binaryExpression(ctx, ctx.unaryExpression, ctx.MultiplicativeOperator);
    }

    unaryExpression(ctx) {
      let result = this.visit(ctx.primaryExpression);
      // Right to left
      for (let i = ctx.UnaryOperator.length - 1; i >= 0; i--) {
        const operator = ctx.UnaryOperator[i];
        result = unaryOperators[operator.tokenType].call(result);
      }
      return result;
    }

    primaryExpression(ctx) {
      if (ctx.Integer.length > 0) {
        const integer = ctx.Integer[0].image;
        let radix = 10;
        if (integer.startsWith('0x')) {
          radix = 16;
        } else if (integer.startsWith('0')) {
          radix = 8;
        }
        return new AsmaseValue(parseInt(integer, radix));
      } else if (ctx.String.length > 0) {
        const string = ctx.String[0].image;
        return new AsmaseValue(unescapeJS(string.substring(1, string.length - 1)));
      } else if (ctx.Variable.length > 0) {
        return this.getVariable(ctx.Variable[0].image.substring(1));
      } else {
        return this.visit(ctx.parentheticalExpression);
      }
    }

    parentheticalExpression(ctx) {
      return this.visit(ctx.expression);
    }
  }

  return new AsmaseVisitor();
}
