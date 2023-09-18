# Caprica
An MIT licensed compiler for the Papyrus scripting language used by the Creation Engine.

This was embarked upon because I was impatient and didn't want to wait for the Creation Kit to be released. It is currently able to successfully compile all of the scripts in Fallout 4, as decompiled by [Champollion](https://github.com/Orvid/Champollion) with some minor modifications to places where Champollion failed miserably at decompiling the script correctly.

Once mature, it also has the possibility of furthering the development of the language, and solving various annoyances in the official implementation.

Please note that this is developed against MSVC 2015, so other compilers may not work, and earlier versions of MSVC almost certainly won't due to C++11 support.

This will currently only compile under MSVC due to the use of the PPL library to handle the parallel compiling of directories.

Deliberate Differences from the Papyrus compiler in the Creation Kit:
 - The CK compiler allows implicit coercion from `None` to `Bool` and `String` as long as it is not a literal `None`. Caprica does not allow implicit coercion from `None` to `Bool` or `String` at all.
 - The CK compiler allows implicit coercion from `None` to arrays and objects, regardless of the origin. Caprica only allows implicit coercion to arrays, objects, structs, and `Var` when the source is a literal `None`.
 - The CK compiler will generate direct reads of the underlying variable for `Auto` properties defined in parent classes. This is not safe, and Caprica will only do this for `Auto` properties defined on the current class.
 - The CK compiler for FO4 appears to not check to see if parameter names conflict with names in a parent scope. Previous versions of the CK compiler did check this correctly, and Caprica does as well, but RobotSelfDestructScript.frenzyFX appears to be a victim of this bug, and does have parameter names that conflict with property names.

And, because it can, Caprica allows a few things that the CK compiler does not. Here's a non-exaustive list:
 - Op-equals operations (`+=`, `-=`, etc.) on array elements are allowed.
 - FloatingPoint literals can be used as the default value for floats, so no more `.0` on the end of those literals.
 - Local variables can be declared as `Auto` and their type will be inferred from the initializer expression.
 - Break/Continue statements with the same semantics as are present in C++, are allowed.
 - Switch-Case statements on `Int` and `String` values are allowed. Case values must be literals.
 - ForEach statements can be used to iterate over an array or Collection.
 - For statements can be used.
 - Do-LoopWhile statements can be used.


# Language Extensions
### For
The statement whose absence has most annoyed Papyrus users working with arrays. Fear not, for your cries have been heard, and your calls answered! The behaviour of it is the same as in [VB.Net](https://msdn.microsoft.com/en-us/library/5z06z1kb.aspx) with the difference that `<identifier>` will never implicitly be defined. If you want to declare it as a variable, you must prefix it with a type.
```
<for> ::= 'For' ['Int'|'Float'|'Auto'] <identifier> '=' <expression> 'To' <expression> ['Step' <expression>]
            <statement>*
          'EndFor'
```
### ForEach
The ForEach statement may be used to iterate over arrays or collections. A collection is defined as any Object which implements a method named either `GetCount` or `GetSize` that accepts no parameters and returns an `Int`, and also implements a method named `GetAt` which accepts a single `Int` argument and returns the value at that index. `<expression>` is evaluated exactly once before the loop begins. `FormList` and `RefCollectionAlias` are two examples of objects in Fallout 4 that are considered collections.
```
<foreach> ::= 'ForEach' (<type>|'Auto') <identifier> 'In' <expression>
                <statement>*
              'EndForEach'
```
### Switch
The Switch statement may only be used on an `Int` or `String`, and the case values must be literals.
```
<switch> ::= 'Switch' <expression>
               ['Case' (<integer>|<string>)
                 <statement>*]*
               ['Default'
                 <statement>*]
             'EndSwitch'
```
### Do-LoopWhile
The Do-LoopWhile statement is the same as a While statement, with the difference that the body is alway executed at least once.
```
<do-loop-while> ::= 'Do'
                      <statement>*
                    'LoopWhile' <expression>
```
### Break/Continue
Break and Continue statements are both also supported as part of extensions to the language. Break will cause control to transfer after the innermost Switch, For, ForEach, Do-LoopWhile, or While statement containing the Break statement. Continue will cause control to tranfer to the next iteration of the innermost For, ForEach, Do-LoopWhile, or While statement containing the Continue statement.
```
<break> ::= 'Break'
<continue> ::= 'Continue'
```
