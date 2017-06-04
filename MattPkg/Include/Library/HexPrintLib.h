/** @file
  Library interface for the PrintHexLib library.
  
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
#ifndef _HEX_PRINT_LIB_H_
#define _HEX_PRINT_LIB_H_

/**
  Print a buffer of text in a format similar to a hex editor.
  It's the offset followed by the data followed by an attempt 
  to print and ASCII in the data and use a . for any unprintable 
  characters.
  
  This version directs the data to whatever output is enabled by the DEBUG macro.

  @param[in] InputBuffer      The buffer of data to be printed.
  @param[in] InputBufferSize  The size of the buffer to be printed.
**/
VOID
HexPrintToDebug(
  VOID* InputBuffer,
  UINTN InputBufferSize
  );
/**
  Print a buffer of text in a format similar to a hex editor.
  It's the offset followed by the data followed by an attempt 
  to print and ASCII in the data and use a . for any unprintable 
  characters.
  
  This version directs the data to the output used by AsciiPrint 
  which is typically the console.

  @param[in] InputBuffer      The buffer of data to be printed.
  @param[in] InputBufferSize  The size of the buffer to be printed.
**/
VOID
HexPrintToConsole (
  VOID* InputBuffer,
  UINTN InputBufferSize
  );
#endif
