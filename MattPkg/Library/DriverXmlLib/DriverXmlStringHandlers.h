/** @file
  Internal function prototypes for the string handling functions used in the XML parser.
  
  Copyright (c) 2016, Matthew Lazarowitz. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the MIT License that accompanies this distribution.

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
**/

#ifndef _DRIVER_XML_STR_HANDLERS_H_
#define _DRIVER_XML_STR_HANDLERS_H_

#include <uefi.h>
#include <Library/DriverXmlLib.h>


EFI_STATUS
AsciiExtractMarkupOrText (
  XML_DOCUMENT*  XmlDoc,
  CHAR8**        XmlString,
  XML_DATA_TYPE* ExtractedType
);

EFI_STATUS
AsciiGetTagNameFromElement (
  CHAR8*  TagData,
  CHAR8** ElementName
);

BOOLEAN
IsAsciiXmlTagWithAttributes (
  CHAR8*  XmlString,
  CHAR8** FirstAttribute
);

EFI_STATUS
AsciiExtractAttribute (
  CHAR8** InOutStr,
  CHAR8** Attribute,
  CHAR8** AttribData
);

EFI_STATUS
AsciiGetPIData(
  CHAR8*  PiData,
  CHAR8** PiTargetName,
  CHAR8** PiTargetData
);

#endif
