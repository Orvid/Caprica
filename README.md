# Caprica
An MIT licensed compiler for the Papyrus scripting language used by the Creation Engine.

This was embarked upon because I was impatient and didn't want to wait for the Creation Kit to be released. It is currently able to successfully compile all of the scripts in Fallout 4, as decompiled by [Champollion](https://github.com/Orvid/Champollion) with some minor modifications to places where Champollion failed miserably at decompiling the script correctly.

Once mature, it also has the possibility of furthering the development of the language, and solving various annoyances in the official implementation.

Please note that this is developed against MSVC 2015, so other compilers may not work, and earlier versions of MSVC almost certainly won't due to C++11 support.

This will currently only compile under MSVC due to the use of the PPL library to handle the parallel compiling of directories.
