# Licenses

## Passgen license

Passgen itself is licensed under the MIT license.

> **Copyright (c) 2023 Patrick M. Elsen**
> 
> Permission is hereby granted, free of charge, to any person obtaining a copy
> of this software and associated documentation files (the "Software"), to deal
> in the Software without restriction, including without limitation the rights
> to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
> copies of the Software, and to permit persons to whom the Software is
> furnished to do so, subject to the following conditions:
> 
> The above copyright notice and this permission notice shall be included in
> all copies or substantial portions of the Software.
> 
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
> IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
> FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
> AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
> LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
> OUT OF OR IN

## JuliaStrings utf8proc license

Passgen vendors the [utf8proc](https://github.com/JuliaStrings/utf8proc)
library which it uses to decode and encode UTF-8. This library is licensed
under the MIT license.

> **utf8proc** is a software package originally developed by Jan Behrens and
> the rest of the Public Software Group, who deserve nearly all of the credit
> for this library, that is now maintained by the Julia-language developers.
> Like the original utf8proc, whose copyright and license statements are
> reproduced below, all new work on the utf8proc library is licensed under the
> [MIT "expat" license](http://opensource.org/licenses/MIT):
>
> *Copyright &copy; 2014-2021 by Steven G. Johnson, Jiahao Chen, Tony Kelman,
> Jonas Fonseca, and other contributors listed in the git history.*
>
> Permission is hereby granted, free of charge, to any person obtaining a copy
> of this software and associated documentation files (the "Software"), to deal
> in the Software without restriction, including without limitation the rights
> to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
> copies of the Software, and to permit persons to whom the Software is
> furnished to do so, subject to the following conditions:
>
> The above copyright notice and this permission notice shall be included in
> all copies or substantial portions of the Software.
>
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
> IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
> FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
> AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
> LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
> OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
> SOFTWARE.

## Original utf8proc license

The utf8proc library vendored by Passgen is a fork of a library that is
licensed under the MIT license.

> *Copyright (c) 2009, 2013 Public Software Group e. V., Berlin, Germany*
> 
> Permission is hereby granted, free of charge, to any person obtaining a copy
> of this software and associated documentation files (the "Software"), to deal
> in the Software without restriction, including without limitation the rights
> to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
> copies of the Software, and to permit persons to whom the Software is
> furnished to do so, subject to the following conditions:
> 
> The above copyright notice and this permission notice shall be included in
> all copies or substantial portions of the Software.
> 
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
> IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
> FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
> AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
> LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
> OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
> SOFTWARE.

## Unicode data license

This software contains data (`extern/utf8proc/utf8proc_data.c`) derived from
processing the Unicode data files. The following license applies to that data:

> **COPYRIGHT AND PERMISSION NOTICE**
> 
> *Copyright (c) 1991-2007 Unicode, Inc. All rights reserved. Distributed under
> the Terms of Use in http://www.unicode.org/copyright.html.*
> 
> Permission is hereby granted, free of charge, to any person obtaining a copy
> of the Unicode data files and any associated documentation (the "Data Files")
> or Unicode software and any associated documentation (the "Software") to deal
> in the Data Files or Software without restriction, including without
> limitation the rights to use, copy, modify, merge, publish, distribute,
> and/or sell copies of the Data Files or Software, and to permit persons to
> whom the Data Files or Software are furnished to do so, provided that (a) the
> above copyright notice(s) and this permission notice appear with all copies
> of the Data Files or Software, (b) both the above copyright notice(s) and
> this permission notice appear in associated documentation, and (c) there is
> clear notice in each modified Data File or in the Software as well as in the
> documentation associated with the Data File(s) or Software that the data or
> software has been modified.
> 
> THE DATA FILES AND SOFTWARE ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
> KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
> MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
> THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
> INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT OR
> CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
> DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
> TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
> OF THE DATA FILES OR SOFTWARE.
> 
> Except as contained in this notice, the name of a copyright holder shall not
> be used in advertising or otherwise to promote the sale, use or other
> dealings in these Data Files or Software without prior written authorization
> of the copyright holder.
> 
> Unicode and the Unicode logo are trademarks of Unicode, Inc., and may be
> registered in some jurisdictions. All other trademarks and registered
> trademarks mentioned herein are the property of their respective owners.

## Others

Passgen depends on several other open-source projects. As they are not included
in the final build, their license does not affect Passgen. These projects are:

| Project | Usage | License |
| --- | --- | --- |
| CMake | Build system generator | [BSD 3-clause license](https://gitlab.kitware.com/cmake/cmake/raw/master/Copyright.txt) |
| [Ninja build](https://ninja-build.org/) | Build system | [Apache License, Version 2](https://github.com/ninja-build/ninja/blob/master/COPYING) |
| [GNU Make](https://www.gnu.org/software/make/) | Build system | [GPLv3](https://www.gnu.org/software/make/#license) |
| [GCC](https://gcc.gnu.org/) | Compiler | [GPLv3](https://gcc.gnu.org/onlinedocs/gcc-12.2.0/gcc/Copying.html) |
| [Clang](https://clang.llvm.org/) | Compiler | [LLVM Apache 2 license](https://github.com/llvm/llvm-project/blob/main/clang/LICENSE.TXT) |
| [GitLab](https://about.gitlab.com/) | Code host, CI system | Commercial and [MIT](https://docs.gitlab.com/ee/development/licensing.html) |
| [Ruby](https://www.ruby-lang.org/en/) | Scripting language | [Ruby license](https://www.ruby-lang.org/en/about/license.txt) |
| [Python](https://python.org/) | Scripting language | [Python Software Foundation License](https://docs.python.org/3/license.html) |
| [Doxygen](https://www.doxygen.org) | Documentation generator | [GPLv2](https://github.com/doxygen/doxygen/blob/master/LICENSE) |
| [MkDocs](https://www.mkdocs.org/) | Static site generator | [MkDocs License (BSD)](https://www.mkdocs.org/about/license/) |
| [MkDocs Material](https://squidfunk.github.io/mkdocs-material/) | Theme for MkDocs | [MIT](https://github.com/squidfunk/mkdocs-material/blob/master/LICENSE) |



