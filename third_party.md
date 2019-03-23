## Third Party Libraries and Stuff
This project depends on the amazing work of others. Each work's name, license and a 
description of what it is used for is below.


### [Dear ImGui](https://github.com/nlohmann/json) 
Used for creating and rendering the debug screen.

> Copyright (c) 2014-2018 Omar Cornut
> 
> Permission is hereby granted, free of charge, to any person obtaining a copy
> of this software and associated documentation files (the "Software"), to deal
> in the Software without restriction, including without limitation the rights
> to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
> copies of the Software, and to permit persons to whom the Software is
> furnished to do so, subject to the following conditions:
> 
> The above copyright notice and this permission notice shall be included in all
> copies or substantial portions of the Software.
> 
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
> IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
> FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
> AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
> LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
> OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
> SOFTWARE.

### [stb](https://github.com/nothings/stb) 
Dear ImGui uses public domain headers from Sean Barrett and others.

### [JSON for Modern C++](https://github.com/nlohmann/json) 
This is used to read the shader meta data embedded by the compiler using the same library. The meta data is used to 
identify and make sense of the game's render process. It is stored in the [MessagePack](https://msgpack.org) format.

> Copyright © 2013-2018 Niels Lohmann
> 
> Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
> 
> The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
> 
> THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


### [DirectXTex](https://github.com/Microsoft/DirectXTex) 
DirectXTex is (unsurprisingly) used to load textures.

> Copyright (c) 2018 Microsoft Corp
> 
> Permission is hereby granted, free of charge, to any person obtaining a copy of this 
> software and associated documentation files (the "Software"), to deal in the Software 
> without restriction, including without limitation the rights to use, copy, modify, 
> merge, publish, distribute, sublicense, and/or sell copies of the Software, and to 
> permit persons to whom the Software is furnished to do so, subject to the following 
> conditions: 
> 
> The above copyright notice and this permission notice shall be included in all copies 
> or substantial portions of the Software.  
> 
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
> INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
> PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
> HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
> CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
> OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

### OpenEXR
Used by DirectXTex for `.exr` support.

> Copyright (c) 2004, Industrial Light & Magic, a division of Lucasfilm
> Entertainment Company Ltd.  Portions contributed and copyright held by
> others as indicated.  All rights reserved.
> 
> Redistribution and use in source and binary forms, with or without
> modification, are permitted provided that the following conditions are
> met:
> 
>     * Redistributions of source code must retain the above
>       copyright notice, this list of conditions and the following
>       disclaimer.
> 
>     * Redistributions in binary form must reproduce the above
>       copyright notice, this list of conditions and the following
>       disclaimer in the documentation and/or other materials provided with
>       the distribution.
> 
>     * Neither the name of Industrial Light & Magic nor the names of
>       any other contributors to this software may be used to endorse or
>       promote products derived from this software without specific prior
>       written permission.
> 
> THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
> IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
> THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
> PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
> CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
> EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
> PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
> PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
> LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
> NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
> SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

### zlib
Used by DirectXTex for `.exr` support.

>   version 1.2.11, January 15th, 2017
> 
>   Copyright (C) 1995-2017 Jean-loup Gailly and Mark Adler
> 
>   This software is provided 'as-is', without any express or implied
>   warranty.  In no event will the authors be held liable for any damages
>   arising from the use of this software.
> 
>   Permission is granted to anyone to use this software for any purpose,
>   including commercial applications, and to alter it and redistribute it
>   freely, subject to the following restrictions:
> 
>   1. The origin of this software must not be misrepresented; you must not
>      claim that you wrote the original software. If you use this software
>      in a product, an acknowledgment in the product documentation would be
>      appreciated but is not required.
>   2. Altered source versions must be plainly marked as such, and must not be
>      misrepresented as being the original software.
>   3. This notice may not be removed or altered from any source distribution.
> 
>   Jean-loup Gailly        Mark Adler
>   jloup@gzip.org          madler@alumni.caltech.edu

### [ISPCTextureCompressor](https://github.com/GameTechDev/ISPCTextureCompressor)
Used to compress textures.

> Copyright (c) 2016-2019, Intel Corporation
>
> Permission is hereby granted, free of charge, to any person obtaining a copy
> of this software and associated documentation files (the "Software"), to
> deal in the Software without restriction, including without limitation the
> rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
> sell copies of the Software, and to permit persons to whom the Software is
> furnished to do so, subject to the following conditions:
>
> The above copyright notice and this permission notice shall be included in
> all copies or substantial portions of the Software.
>
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
> IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
> FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
> AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
> LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
> FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
> IN THE SOFTWARE.

### [OpenGL Mathematics](https://github.com/g-truc/glm) 
Despite the patch being focused on Direct3D 9 glm is used as the mathematics library for the patch. 
It's interfaces and structure make it really nice to use and preferably (for me) to alternatives.

> Copyright (c) 2005 - 2017 G-Truc Creation
> 
> Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
> 
> The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
> 
> Restrictions: By making use of the Software for military purposes, you choose to make a Bunny unhappy.
> 
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

### [Guideline Support Library](https://github.com/Microsoft/GSL)
This one's usefulness should be fairly self explanatory.

> Copyright (c) 2015 Microsoft Corporation. All rights reserved. 
>  
> This code is licensed under the MIT License (MIT). 
> 
> Permission is hereby granted, free of charge, to any person obtaining a copy 
> of this software and associated documentation files (the "Software"), to deal 
> in the Software without restriction, including without limitation the rights 
> to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
> of the Software, and to permit persons to whom the Software is furnished to do 
> so, subject to the following conditions: 
> 
> The above copyright notice and this permission notice shall be included in all 
> copies or substantial portions of the Software. 
> 
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
> IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
> FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
> AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
> LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
> OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
> THE SOFTWARE. 

### [clara](https://github.com/catchorg/Clara) 
A nice and simple command line parser. Used for any form of sophisticated argument parsing
in Shader Patch's supporting tools.

> Boost Software License - Version 1.0 - August 17th, 2003
> 
> Permission is hereby granted, free of charge, to any person or organization
> obtaining a copy of the software and accompanying documentation covered by
> this license (the "Software") to use, reproduce, display, distribute,
> execute, and transmit the Software, and to prepare derivative works of the
> Software, and to permit third-parties to whom the Software is furnished to
> do so, all subject to the following:
> 
> The copyright notices in the Software and this entire statement, including
> the above license grant, this restriction and the following disclaimer,
> must be included in all copies of the Software, in whole or in part, and
> all derivative works of the Software, unless such copies or derivative
> works are solely in the form of machine-executable object code generated by
> a source language processor.
> 
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
> IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
> FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
> SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
> FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
> ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
> DEALINGS IN THE SOFTWARE.

### [yaml-cpp](https://github.com/jbeder/yaml-cpp) 
YAML is an excellent format for human editable config files. This fine library is used for reading
in user defined material files and the like.

> Copyright (c) 2008-2015 Jesse Beder.
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
> OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
> THE SOFTWARE.

### [Martins Upitis's Film Grain Shader](http://devlog-martinsh.blogspot.com/2013/05/image-imperfections-and-film-grain-post.html) 
Nifty as Film Grain shader present in Shader Patch's Effects system. It has been modified slightly for use in HLSL and SP, 
but remains largely unchanged. Used under the Creative Commons Attribution 3.0 Unported License which you can see 
bygoing here https://creativecommons.org/licenses/by/3.0/

### [John Hable](https://twitter.com/FilmicWorlds) 
For his incredibly useful posts and examples on [Filmic Tonemapping](http://filmicworlds.com/blog/filmic-tonemapping-with-piecewise-power-curves/)
and [Color Grading](http://filmicworlds.com/blog/minimal-color-grading-tools/).

### [DirectXMesh](https://github.com/Microsoft/DirectXMesh) 
Used for mesh optimization by `material_munge`.

>                               The MIT License (MIT)
>
> Copyright (c) 2014-2019 Microsoft Corp
> 
> Permission is hereby granted, free of charge, to any person obtaining a copy of this 
> software and associated documentation files (the "Software"), to deal in the Software 
> without restriction, including without limitation the rights to use, copy, modify, 
> merge, publish, distribute, sublicense, and/or sell copies of the Software, and to 
> permit persons to whom the Software is furnished to do so, subject to the following 
> conditions: 
> 
> The above copyright notice and this permission notice shall be included in all copies 
> or substantial portions of the Software.  
> 
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
> INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
> PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
> HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
> CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
> OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

### mikktspace
Used to generate tangent vectors by `material_munge`.

>
>  Copyright (C) 2011 by Morten S. Mikkelsen
>
>  This software is provided 'as-is', without any express or implied
>  warranty.  In no event will the authors be held liable for any damages
>  arising from the use of this software.
>
>  Permission is granted to anyone to use this software for any purpose,
>  including commercial applications, and to alter it and redistribute it
>  freely, subject to the following restrictions:
>
>  1. The origin of this software must not be misrepresented; you must not
>     claim that you wrote the original software. If you use this software
>     in a product, an acknowledgment in the product documentation would be
>     appreciated but is not required.
>  2. Altered source versions must be plainly marked as such, and must not be
>     misrepresented as being the original software.
>  3. This notice may not be removed or altered from any source distribution.

### [Detours](https://github.com/Microsoft/Detours) 
Used for API hooking in Shader Patch to live edit `.lvl` files.

> # Copyright (c) Microsoft Corporation
> 
> All rights reserved.
> 
> # MIT License
> 
> Permission is hereby granted, free of charge, to any person obtaining a copy of
> this software and associated documentation files (the "Software"), to deal in
> the Software without restriction, including without limitation the rights to
> use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
> of the Software, and to permit persons to whom the Software is furnished to do
> so, subject to the following conditions:
> 
> The above copyright notice and this permission notice shall be included in all
> copies or substantial portions of the Software.
> 
> THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
> IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
> FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
> AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
> LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
> OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
> SOFTWARE.