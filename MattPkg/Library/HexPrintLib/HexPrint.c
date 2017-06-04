/** @file
  Functions here allow for a buffer of arbitrary length to be printed in a format similar
  to a hex editor. This is useful for visualizing hex and possible ASCII side by side.
  
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
#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>

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
HexPrintToDebug (
  VOID* InputBuffer,
  UINTN InputBufferSize
  )
{
  UINTN  NumLines;
  UINTN  CurrentLine;
  UINTN  i;
  UINTN  RemaingBytes;
  CHAR8* BufferPointer;
  
  // 
  // 16 bytes per line
  //
  NumLines = InputBufferSize / 16;
  
  //
  // The amount of data to print on a shortened last line
  //
  RemaingBytes = InputBufferSize % 16;
 
  CurrentLine = 0;
  BufferPointer = (CHAR8*)InputBuffer;
  
  DEBUG ((DEBUG_ERROR, "Buffer @ 0x%X, %d bytes\n",(UINTN)InputBuffer,InputBufferSize));
  while (CurrentLine < NumLines) {
    DEBUG ((DEBUG_ERROR, "%07X0: ", CurrentLine));
    //
    // 16 digits of output
    //
    for (i = 0; i < 16; i++) {
      DEBUG ((DEBUG_ERROR, "%02X ", BufferPointer[i]));
    }
    //
    // then output 16 characters or a place holder for non-printable chars
    //
    DEBUG ((DEBUG_ERROR, "\""));
    for (i = 0; i < 16; i++) {
      if(BufferPointer[i] >= 0x20 && BufferPointer[i] <= 0x7e) {
        DEBUG ((DEBUG_ERROR, "%c",BufferPointer[i]));
      }
      else
      {
        DEBUG ((DEBUG_ERROR, "."));
      }
    }
    DEBUG ((DEBUG_ERROR, "\"\n"));
    BufferPointer += 16;
    CurrentLine++;
  }
  //
  // There should be an incomplete line to print.
  //
  DEBUG ((DEBUG_ERROR, "%07X0: ", CurrentLine));
  for (i = 0; i < RemaingBytes; i++) {
    DEBUG ((DEBUG_ERROR, "%02X ", BufferPointer[i]));
  }
  //
  // pad the rest of the line with spaces
  //
  for (i = RemaingBytes; i < 16; i++) {
    DEBUG ((DEBUG_ERROR, "   ", BufferPointer[i]));
  }
  //
  // now do the char printout
  //

  DEBUG ((DEBUG_ERROR, "\""));
  for (i = 0; i < RemaingBytes; i++) {
    if(BufferPointer[i] >= 0x20 && BufferPointer[i] <= 0x7e) {
      DEBUG ((DEBUG_ERROR, "%c",BufferPointer[i]));
    }
    else
    {
      DEBUG ((DEBUG_ERROR, "."));
    }
  }
  //
  // pad the rest of the line with spaces
  //
  for (i = RemaingBytes; i < 16; i++) {
    DEBUG ((DEBUG_ERROR, " ", BufferPointer[i]));
  }
  DEBUG ((DEBUG_ERROR, "\"\n"));
  
}

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
HexPrintToConsole (
  VOID* InputBuffer,
  UINTN InputBufferSize
  )
{
  UINTN  NumLines;
  UINTN  CurrentLine;
  UINTN  i;
  UINTN  RemaingBytes;
  CHAR8* BufferPointer;
  
  // 
  // 16 bytes per line
  //
  NumLines = InputBufferSize / 16;
  
  //
  // The amount of data to print on a shortened last line
  //
  RemaingBytes = InputBufferSize % 16;
 
  CurrentLine = 0;
  BufferPointer = (CHAR8*)InputBuffer;
  
  while (CurrentLine < NumLines) {
    AsciiPrint ("%07X0: ", CurrentLine);
    //
    // 16 digits of output
    //
    for (i = 0; i < 16; i++) {
      AsciiPrint ("%02X ", BufferPointer[i]);
    }
    //
    // then output 16 characters or a place holder for non-printable chars
    //
    AsciiPrint ("\"");
    for (i = 0; i < 16; i++) {
      if(BufferPointer[i] >= 0x20 && BufferPointer[i] <= 0x7e) {
        AsciiPrint ("%c",BufferPointer[i]);
      }
      else
      {
        AsciiPrint (".");
      }
    }
    AsciiPrint ("\"\n");
    BufferPointer += 16;
    CurrentLine++;
  }
  //
  // There should be an incomplete line to print.
  //
  AsciiPrint ("%07X0: ", CurrentLine);
  for (i = 0; i < RemaingBytes; i++) {
    AsciiPrint ("%02X ", BufferPointer[i]);
  }
  //
  // pad the rest of the line with spaces
  //
  for (i = RemaingBytes; i < 16; i++) {
    AsciiPrint ("   ", BufferPointer[i]);
  }
  //
  // now do the char printout
  //

  AsciiPrint ("\"");
  for (i = 0; i < RemaingBytes; i++) {
    if(BufferPointer[i] >= 0x20 && BufferPointer[i] <= 0x7e) {
      AsciiPrint ("%c",BufferPointer[i]);
    }
    else
    {
      AsciiPrint (".");
    }
  }
  //
  // pad the rest of the line with spaces
  //
  for (i = RemaingBytes; i < 16; i++) {
    AsciiPrint (" ", BufferPointer[i]);
  }
  AsciiPrint ("\"\n");
  
}

