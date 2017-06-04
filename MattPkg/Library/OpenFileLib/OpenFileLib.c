/** @file
  Open a file based on a string that could be just a file name, a path and a filename, 
  or a volume, path, and filename.
  
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
  
  @par WC3 XML specification https://www.w3.org/TR/REC-xml/
  
**/
#include <uefi.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/DevicePath.h>
#include <Protocol/LoadedImage.h>
#include <Guid/FileInfo.h>


/**
  Get a path to the executable image from the loaded image protocol.
  There is an assumption that there will be only one file path node, 
  and it will contain the complete path.
  
  @param [in]  InputDevicePath     The device path to examine for a file path node.
  @param [out] ImageDirectoryPath  The directory path with the image file name removed.
  
  @retval EFI_SUCCESS   ImageDirectoryPath will contain a valid path string
  @retval EFI_NOT_FOUND The device path did not contain a file path node
**/
EFI_STATUS
GetDirectoryOfImage (
  EFI_DEVICE_PATH_PROTOCOL* ImageDevicePath,
  CHAR16**                  ImageDirectory
  )
{
  UINTN                 InputPathLength;
  UINTN                 DirectoryPathLength;
  FILEPATH_DEVICE_PATH* FilePath;
  EFI_DEVICE_PATH*      TestNode;
  
  //
  // Functions used on the input path will modify the pointer.
  // Make a copy to ensure we do not modify the original
  //
  TestNode = ImageDevicePath;
  FilePath = NULL;

  //
  // walk the path until we get to a media device path node or the end of the path
  // note that the media device path node may be the the first node.
  //
  while (!IsDevicePathEnd (TestNode)) {

    if (DevicePathType (TestNode) == MEDIA_DEVICE_PATH 
      && DevicePathSubType (TestNode) == MEDIA_FILEPATH_DP)
    {
      FilePath = (FILEPATH_DEVICE_PATH*)TestNode;
      break;
    }
    TestNode = NextDevicePathNode (TestNode);
  }
  
  if (IsDevicePathEnd (TestNode) 
      || FilePath == NULL) 
  {
    return EFI_NOT_FOUND;
  }

  InputPathLength = StrSize (FilePath->PathName);
  *ImageDirectory = AllocateZeroPool (InputPathLength);
  gBS->CopyMem  (
         *ImageDirectory,
         FilePath->PathName,
         InputPathLength
       );
  //
  // Remove the <filename>.efi from the data we copied
  //
  PathRemoveLastItem (*ImageDirectory);
  DirectoryPathLength = StrSize (*ImageDirectory);
  return EFI_SUCCESS;
}

/**
  Tries to open a file from the file system on a file system handle provided by the caller.  
  The caller must also provide the full path relative to the root of the file system.

  @param[in]  FilePathFromRoot  The full path from the file system root.
  @paran[in]  FileSystemHandle  The file system handle to check for the supplied file name.
  @param[out] FileBuffer     The buffer for the file data to be returned to the caller.
  @param[out] FileSize       The size of the file as it was read from disk.
  
  @retval EFI_SUCCESS        The file was found, and read into memory.
  
**/
EFI_STATUS
OpenFullPathOnFileSystem (
  CHAR16*    FilePathFromRoot,
  EFI_HANDLE FileSystemHandle,
  VOID**     FileBuffer,
  UINTN*     FileSize
  )
{
  EFI_STATUS                       Status;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* SimpleFileSystemProtocol;
  EFI_FILE_PROTOCOL*               FileSystemRoot;
  EFI_FILE_PROTOCOL*               RequestedFile;
  EFI_FILE_INFO*                   RequestedFileInfo;
  UINTN                            Size;

  //
  // Open the file system
  //
  Status = gBS->OpenProtocol (
                  FileSystemHandle,
                  &gEfiSimpleFileSystemProtocolGuid,  
                  &SimpleFileSystemProtocol,
                  NULL,
                  NULL,
                  EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
                );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Unable to get file system from the loaded image handle\n"));
    return EFI_NOT_FOUND;
  }
  //
  // Open volume, get the root
  //
  Status = SimpleFileSystemProtocol->OpenVolume (
                                       SimpleFileSystemProtocol,
                                       &FileSystemRoot
                                     );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Could not open the volume\n"));
    return EFI_NOT_FOUND;
  }

  //
  // Try opening the file relative to the root
  //
  Status = FileSystemRoot->Open (
                             FileSystemRoot,
                             &RequestedFile,
                             FilePathFromRoot,
                             EFI_FILE_MODE_READ,
                             0
                           );


  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Unable to open file (%r).\n",Status));

    switch (Status) {
      case EFI_VOLUME_FULL:
      case EFI_VOLUME_CORRUPTED:
      case EFI_DEVICE_ERROR:
        DEBUG ((DEBUG_ERROR, "Disk Error\n"));
        break;
      case EFI_ACCESS_DENIED:
      case EFI_WRITE_PROTECTED: 
      case EFI_MEDIA_CHANGED:
      case EFI_NO_MEDIA:
        DEBUG ((DEBUG_ERROR, "Access Denied\n"));
        break;
      default:
        DEBUG ((DEBUG_ERROR, "Invalid File Name\n"));
    }
    return Status;
  }
    
  //
  // The EFI_FILE_INFO has a variable size based on the 
  // path supplied. Allocate for the path and the file info structure
  //
  Size = StrLen (FilePathFromRoot) * sizeof (CHAR16); 
  Size += sizeof (EFI_FILE_INFO);
  RequestedFileInfo = AllocateZeroPool (Size);

  Status = RequestedFile->GetInfo (
                            RequestedFile,
                            &gEfiFileInfoGuid,
                            &Size,
                            RequestedFileInfo
                            );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Unable to get file info %r\n",Status));
    RequestedFile->Close (RequestedFile);
    gBS->FreePool (RequestedFileInfo);
    return Status; 
  }
    //
    // Try to allocate a buffer now that we have a file size
    //
    *FileBuffer = AllocatePool ((UINTN)RequestedFileInfo->FileSize);

    if (*FileBuffer == NULL) {
      DEBUG ((DEBUG_ERROR, "no buffer, out of resources.\n"));
      RequestedFile->Close (RequestedFile);
        gBS->FreePool (*FileBuffer);
        FileBuffer = NULL;
        *FileSize = 0;
        return EFI_OUT_OF_RESOURCES;
    }
    //
    // Read the file into memory
    //
    Size = (UINTN)RequestedFileInfo->FileSize;

    Status = RequestedFile->Read (
                              RequestedFile,
                              &Size,
                              *FileBuffer
                            );

    if (EFI_ERROR(Status)) {
      DEBUG ((DEBUG_ERROR, "Failed to read from the file %r.\n",Status));
      RequestedFile->Close (RequestedFile);
      gBS->FreePool (*FileBuffer);
      gBS->FreePool (RequestedFileInfo);
      FileBuffer = NULL;
      *FileSize = 0;
      return Status;
    }

    *FileSize = (UINTN)RequestedFileInfo->FileSize;

    RequestedFile->Close (RequestedFile);
    gBS->FreePool (RequestedFileInfo);
    return EFI_SUCCESS;
}

/**
  Only a file name was provided (no path). Assume the file is in the same location 
  as <image name>.efi.
  Build a complete path the file based on that assumption.
  
  @param [in]  LoadedImageProtocol  The EFI_LOADED_IMAGE_PROTOCOL instance to extract path data from
  @param [in]  FileName             The file name to append to the path.
  @param [out] FullPath             The complete path of the file to open.
  
  @retval EFI_SUCCESS   ImageDirectoryPath will contain a valid path string
  @retval EFI_OUT_OF_RESOURCES No memory could be allocated for the concatenated path.
**/

EFI_STATUS
FullPathFromImageDirectory (
  EFI_LOADED_IMAGE_PROTOCOL* LoadedImageProtocol,
  CHAR16*                    FileName,
  CHAR16**                   Path
  )
{
  //
  // Get the path from the image information
  //
  GetDirectoryOfImage (LoadedImageProtocol->FilePath, Path);
  //
  // Need to make space for both the filename and path.
  //
  *Path = ReallocatePool (
            StrSize (*Path),
            StrLen (*Path) + StrSize (FileName),
            *Path
          );
  if (*Path == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  //
  // concatenate the strings
  // 
  StrCat (*Path, FileName);

  return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS                 Status;
  EFI_LOADED_IMAGE_PROTOCOL* LoadedImageProtocol;
  CHAR16*                    CompletePath;
  
  Status = EFI_NOT_FOUND;
  //
  // There is a colon, assume a map name is specified.
  //
  if (StrStr (FileString, L":") != NULL ) {
    return Status;
  }

  //
  // Get the loaded image protocol since that will contain path info about our .efi executable.
  //
  Status = gBS->OpenProtocol (
                  gImageHandle,
                  &gEfiLoadedImageProtocolGuid,  
                  &LoadedImageProtocol,
                  NULL,
                  NULL,
                  EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL
                );
  if (EFI_ERROR(Status)) {
    DEBUG ((DEBUG_ERROR, "error getting the loaded image protocol. %r\n", Status));
    return Status;
  }

  //
  // No colon, but there is a backslash. 
  // Assume this is a path on the same volume as our .efi file
  //
  if (StrStr (FileString, L"\\") !=NULL ){
    Status = OpenFullPathOnFileSystem (
               FileString,
               LoadedImageProtocol->DeviceHandle,
               FileBuffer,
               FileSize
             );
    return Status;
  }
  //
  // Neither the colon or backslash cases were true.
  // Assume the file is in the same place as our .efi file
  //
  Status = FullPathFromImageDirectory (
             LoadedImageProtocol,
             FileString,
             &CompletePath
           );

  if (EFI_ERROR (Status)) {
    return Status;
  }
  Status = OpenFullPathOnFileSystem (
             CompletePath,
             LoadedImageProtocol->DeviceHandle,
             FileBuffer,
             FileSize
           );
  gBS->FreePool (CompletePath);
  return Status;
}


