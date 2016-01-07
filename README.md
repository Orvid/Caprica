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
