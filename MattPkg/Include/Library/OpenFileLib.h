/** @file
  Library definition for a library that will open a file at a specified path, 
  or try to use what information has been provided to locate a viable candidate
  based on the input a user provides.
  
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
#ifndef __OPEN_FILE_LIB_H__
#define __OPEN_FILE_LIB_H__
/**
  This function tries to figure out the path to a file specified by a user.
  If there is a : then this assumes a map name is specified along with a complete path to a file.
  If there is no : but there is a \ then this assumes a path is specified on the same device as the 
  <image name>.efi file.
  If neither of those are true, then this assumes the file must exist in the same place as <image name>.efi  
  
  @param [in]     FileString     the string supplied via a command line argument
  @param [in out] FileBuffer     The opened file. Callee allocates, but caller must free.
  @param [in out] FileSize       The size of the file buffer.
  
  @retval EFI_SUCCESS  The file was found, opened, and returned.
  @return Error codes from worker functions.
**/

EFI_STATUS
OpenFileFromArgument (
    CHAR16* FileString,
    CHAR8** FileBuffer,
    UINTN*  FileSize
);

#endif
