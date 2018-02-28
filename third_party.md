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
DirectXTex is (unsurprisingly) used to load textures for use by the patch.

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