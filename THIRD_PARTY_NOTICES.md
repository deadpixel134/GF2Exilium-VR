# GF2VR third-party components

The Clean and Dirty v1.3.13 installation ZIPs contain the components listed
below. Their license texts also apply to the corresponding third-party
sources distributed with this repository.

| Component | Upstream | License material in the installation ZIP |
| --- | --- | --- |
| Khronos OpenXR Loader and headers 1.1.63 | [Khronos release 1.1.63](https://github.com/KhronosGroup/OpenXR-SDK-Source/releases/tag/release-1.1.63) | Apache-2.0 is the option used by this distribution. The text is at payload/vrmod/runtime/openxr-LICENSE.txt. Copyright The Khronos Group Inc. |
| MinHook v1.3.4 and Hacker Disassembler Engine | [MinHook v1.3.4](https://github.com/TsudaKageyu/minhook/tree/v1.3.4) | BSD 2-Clause conditions and copyright notices are at payload/vrmod/runtime/minhook-LICENSE.txt. |
| Microsoft .NET Core and Windows Desktop 10.0.12 for Windows x64 | [Core runtime pack](https://www.nuget.org/packages/Microsoft.NETCore.App.Runtime.win-x64/10.0.12) and [Desktop runtime pack](https://www.nuget.org/packages/Microsoft.WindowsDesktop.App.Runtime.win-x64/10.0.12) | [DOTNET-NOTICES.txt](DOTNET-NOTICES.txt) at the ZIP root contains the MIT license texts and .NET Core third-party notices. |

The controller guide uses artwork under the
[Meta Art Attribution License 1.0](https://developers.meta.com/vr/licenses/art-1.0/).
See the [controller artwork credits](docs/en/CONTROLLER_ART.md).

## Corresponding source for binary releases

For each Clean and Dirty binary release under GPL-3.0, give recipients access
to the complete Corresponding Source for that exact build from the release
page. Include the covered build and installation scripts with the source.
See [GPLv3 section 6](https://www.gnu.org/licenses/gpl-3.0.html#section6).

## JsonCpp in the OpenXR Loader

OpenXR Loader 1.1.63 includes JsonCpp. Include this upstream notice alongside
openxr-LICENSE.txt when preparing a Clean or Dirty distribution.
[Khronos vendored JsonCpp license](https://github.com/KhronosGroup/OpenXR-SDK-Source/blob/release-1.1.63/src/external/jsoncpp/LICENSE)

~~~~text
The JsonCpp library's source code, including accompanying documentation,
tests and demonstration applications, are licensed under the following
conditions...

Baptiste Lepilleur and The JsonCpp Authors explicitly disclaim copyright in all
jurisdictions which recognize such a disclaimer. In such jurisdictions,
this software is released into the Public Domain.

In jurisdictions which do not recognize Public Domain property (e.g. Germany as of
2010), this software is Copyright (c) 2007-2010 by Baptiste Lepilleur and
The JsonCpp Authors, and is released under the terms of the MIT License (see below).

In jurisdictions which recognize Public Domain property, the user of this
software may choose to accept it either as 1) Public Domain, 2) under the
conditions of the MIT License (see below), or 3) under the terms of dual
Public Domain/MIT License conditions described here, as they choose.

The MIT License is about as close to Public Domain as a license can get, and is
described in clear, concise terms at:

   http://en.wikipedia.org/wiki/MIT_License

The full text of the MIT License follows:

========================================================================
Copyright (c) 2007-2010 Baptiste Lepilleur and The JsonCpp Authors

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use, copy,
modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
========================================================================
(END LICENSE TEXT)

The MIT license is compatible with both the GPL and commercial
software, affording one all of the rights of Public Domain with the
minor nuisance of being required to keep the above copyright notice
and license text in the source code. Note also that by accepting the
Public Domain "license" you can re-license your copy using whatever
license you like.
~~~~
