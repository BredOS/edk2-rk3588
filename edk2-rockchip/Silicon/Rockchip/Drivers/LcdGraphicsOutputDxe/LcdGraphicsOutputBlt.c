/** @file

  Copyright (c) 2025, Mario Bălănică <mariobalanica02@gmail.com>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseMemoryLib.h>

#include "LcdGraphicsOutputDxe.h"

STATIC
EFI_STATUS
LcdGraphicsBltCheckParameters (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL       *This,
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *BltBuffer  OPTIONAL,
  IN EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
  IN UINTN                              SourceX,
  IN UINTN                              SourceY,
  IN UINTN                              DestinationX,
  IN UINTN                              DestinationY,
  IN UINTN                              Width,
  IN UINTN                              Height,
  IN UINTN                              Delta       OPTIONAL
  )
{
  UINT32  HorizontalResolution = This->Mode->Info->HorizontalResolution;
  UINT32  VerticalResolution   = This->Mode->Info->VerticalResolution;

  if ((Width == 0) || (Height == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((BltBuffer == NULL) &&
      ((BltOperation == EfiBltVideoFill) ||
       (BltOperation == EfiBltBufferToVideo) ||
       (BltOperation == EfiBltVideoToBltBuffer)))
  {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  //
  // If we are reading data out of the video buffer,
  // check that the source area is within the display limits.
  //
  if ((BltOperation == EfiBltVideoToBltBuffer) ||
      (BltOperation == EfiBltVideoToVideo))
  {
    if ((SourceY + Height > VerticalResolution) ||
        (SourceX + Width > HorizontalResolution))
    {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // If we are writing data into the video buffer,
  // check that the destination area is within the display limits.
  //
  if ((BltOperation == EfiBltVideoFill) ||
      (BltOperation == EfiBltBufferToVideo) ||
      (BltOperation == EfiBltVideoToVideo))
  {
    if ((DestinationY + Height > VerticalResolution) ||
        (DestinationX + Width > HorizontalResolution))
    {
      return EFI_INVALID_PARAMETER;
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
BltVideoFill90 (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL        *This,
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *EfiSourcePixel,     OPTIONAL
  IN UINTN                               SourceX,
  IN UINTN                               SourceY,
  IN UINTN                               DestinationX,
  IN UINTN                               DestinationY,
  IN UINTN                               Width,
  IN UINTN                               Height,
  IN UINTN                               Delta           OPTIONAL   // Number of BYTES in a row of the BltBuffer
  )
{
  EFI_STATUS         Status;
  UINT32             FrameBufferHeight;
  VOID            *FrameBufferBase;
  VOID            *DestinationAddr;

  Status           = EFI_SUCCESS;
  FrameBufferBase = (UINTN *)((UINTN)(This->Mode->FrameBufferBase));
  FrameBufferHeight = This->Mode->Info->VerticalResolution;

  for (UINTN y = 0; y < Height; ++y) {
    for (UINTN x = 0; x < Width; ++x) {
      DestinationAddr = (VOID *)((UINT32 *)FrameBufferBase + (DestinationX + x) * FrameBufferHeight + (FrameBufferHeight - 1 - (DestinationY + y)));

      *(UINT32 *)DestinationAddr = *((UINT32 *)EfiSourcePixel);
    }
  }

  return Status;
}

STATIC
EFI_STATUS
BltVideoToBltBuffer90 (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL        *This,
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *BltBuffer,     OPTIONAL
  IN UINTN                               SourceX,
  IN UINTN                               SourceY,
  IN UINTN                               DestinationX,
  IN UINTN                               DestinationY,
  IN UINTN                               Width,
  IN UINTN                               Height,
  IN UINTN                               Delta           OPTIONAL   // Number of BYTES in a row of the BltBuffer
  )
{
  EFI_STATUS         Status;
  UINT32             FrameBufferHeight;
  VOID               *FrameBufferBase;
  VOID               *SourceAddr;
  VOID               *DestinationAddr;
  UINT32             BltBufferHorizontalResolution;

  Status = EFI_SUCCESS;
  FrameBufferHeight = This->Mode->Info->VerticalResolution;
  FrameBufferBase = (UINTN *)((UINTN)(This->Mode->FrameBufferBase));

  if(( Delta != 0 ) && ( Delta != Width * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL))) {
    // Delta is not zero and it is different from the width.
    // Divide it by the size of a pixel to find out the buffer's horizontal resolution.
    BltBufferHorizontalResolution = (UINT32) (Delta / sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
  } else {
    BltBufferHorizontalResolution = Width;
  }

  for (UINTN y = 0; y < Height; ++y) {
    for (UINTN x = 0; x < Width; ++x) {
      SourceAddr = (VOID *)((UINT32 *)FrameBufferBase + (SourceX + x) * FrameBufferHeight + (FrameBufferHeight - 1 - (SourceY + y)));
      DestinationAddr = (VOID *)((UINT32 *)BltBuffer + (DestinationY + y) * BltBufferHorizontalResolution + (DestinationX + x));

      *(UINT32 *)DestinationAddr = *(UINT32 *)SourceAddr;
    }
  }

  return Status;
}

STATIC
EFI_STATUS
BltBufferToVideo90 (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL        *This,
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *BltBuffer,     OPTIONAL
  IN UINTN                               SourceX,
  IN UINTN                               SourceY,
  IN UINTN                               DestinationX,
  IN UINTN                               DestinationY,
  IN UINTN                               Width,
  IN UINTN                               Height,
  IN UINTN                               Delta           OPTIONAL   // Number of BYTES in a row of the BltBuffer
  )
{
  EFI_STATUS         Status;
  UINT32             FrameBufferHeight;
  VOID   *FrameBufferBase;
  VOID            *SourceAddr;
  VOID            *DestinationAddr;
  UINT32          BltBufferHorizontalResolution;

  Status = EFI_SUCCESS;
  FrameBufferHeight = This->Mode->Info->VerticalResolution;
  FrameBufferBase = (UINTN *)((UINTN)(This->Mode->FrameBufferBase));

  if(( Delta != 0 ) && ( Delta != Width * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL))) {
    // Delta is not zero and it is different from the width.
    // Divide it by the size of a pixel to find out the buffer's horizontal resolution.
    BltBufferHorizontalResolution = (UINT32) (Delta / sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
  } else {
    BltBufferHorizontalResolution = Width;
  }

  for (UINTN y = 0; y < Height; ++y) {
    for (UINTN x = 0; x < Width; ++x) {
      SourceAddr      = (VOID *)((UINT32 *)BltBuffer + (SourceY + y) * BltBufferHorizontalResolution + (SourceX + x));
      DestinationAddr = (VOID *)((UINT32 *)FrameBufferBase + (DestinationX + x) * FrameBufferHeight + (FrameBufferHeight - 1 - (DestinationY + y)));

      *(UINT32 *)DestinationAddr = *(UINT32 *)SourceAddr;
    }
  }

  return Status;
}

STATIC
EFI_STATUS
VideoCopyNoHorizontalOverlap90 (
  IN UINTN          BitsPerPixel,
  IN volatile VOID  *FrameBufferBase,
  IN UINT32         VerticalResolution,
  IN UINTN          SourceX,
  IN UINTN          SourceY,
  IN UINTN          DestinationX,
  IN UINTN          DestinationY,
  IN UINTN          Width,
  IN UINTN          Height
)
{
  EFI_STATUS    Status;
  UINTN         SourceLine;
  UINTN         DestinationLine;
  UINTN         LineCount;
  INTN          Step;
  VOID          *SourceAddr;
  VOID          *DestinationAddr;

  Status = EFI_SUCCESS;

  if (DestinationY <= SourceY) {
    // scrolling up (or horizontally but without overlap)
    SourceLine       = SourceY;
    DestinationLine  = DestinationY;
    Step             = 1;
  } else {
    // scrolling down
    SourceLine       = SourceY + Height;
    DestinationLine  = DestinationY + Height;
    Step             = -1;
  }

  for (LineCount = 0; LineCount < Height; LineCount++) {
    for (UINTN x = 0; x < Width; ++x) {
      SourceAddr      = (VOID *)((UINT32 *)FrameBufferBase + (SourceX + x) * VerticalResolution + (VerticalResolution - 1 - SourceLine));
      DestinationAddr = (VOID *)((UINT32 *)FrameBufferBase + (DestinationX + x) * VerticalResolution + (VerticalResolution - 1 - DestinationLine));

      *(UINT32 *)DestinationAddr = *(UINT32 *)SourceAddr;
    }

    SourceLine      += Step;
    DestinationLine += Step;
  }

  return Status;
}

STATIC
EFI_STATUS
VideoCopyHorizontalOverlap90 (
  IN UINTN          BitsPerPixel,
  IN volatile VOID  *FrameBufferBase,
  IN UINT32         VerticalResolution,
  IN UINTN          SourceX,
  IN UINTN          SourceY,
  IN UINTN          DestinationX,
  IN UINTN          DestinationY,
  IN UINTN          Width,
  IN UINTN          Height
)
{
  EFI_STATUS      Status;

  UINT32 *PixelBuffer32bit;
  UINT32 *SourcePixel32bit;
  UINT32 *DestinationPixel32bit;

  Status = EFI_SUCCESS;

  // Allocate a temporary buffer
  PixelBuffer32bit = (UINT32 *) AllocatePool((Height * Width) * sizeof(UINT32));

  if (PixelBuffer32bit == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  // Copy from the video ram (source region) to a temp buffer
  for (UINTN y = 0; y < Height; ++y) {
    for (UINTN x = 0; x < Width; ++x) {
      SourcePixel32bit = (UINT32 *)FrameBufferBase + (SourceX + x) * VerticalResolution + (VerticalResolution - 1 - (SourceY + y));

      PixelBuffer32bit[y * Width + x] = *SourcePixel32bit;
    }
  }

  // Copy from the temp buffer to the video ram (destination region)
  for (UINTN y = 0; y < Height; ++y) {
    for (UINTN x = 0; x < Width; ++x) {
      DestinationPixel32bit = (UINT32 *)FrameBufferBase + (DestinationX + x) * VerticalResolution + (VerticalResolution - 1 - (DestinationY + y));

      *DestinationPixel32bit = PixelBuffer32bit[y * Width + x];
    }
  }

  // Free up the allocated memory
  FreePool((VOID *) PixelBuffer32bit);

EXIT:
  return Status;
}

STATIC
EFI_STATUS
BltVideoToVideo90 (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL        *This,
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL   *BltBuffer,     OPTIONAL
  IN UINTN                               SourceX,
  IN UINTN                               SourceY,
  IN UINTN                               DestinationX,
  IN UINTN                               DestinationY,
  IN UINTN                               Width,
  IN UINTN                               Height,
  IN UINTN                               Delta           OPTIONAL   // Number of BYTES in a row of the BltBuffer
  )
{
  EFI_STATUS         Status;
  UINT32             VerticalResolution;
  LCD_BPP            BitsPerPixel;
  VOID   *FrameBufferBase;

  VerticalResolution = This->Mode->Info->VerticalResolution;
  FrameBufferBase = (UINTN *)((UINTN)(This->Mode->FrameBufferBase));

  //
  // BltVideo to BltVideo:
  //
  //  Source is the Video Memory,
  //  Destination is the Video Memory

  LcdGraphicsGetBpp (This->Mode->Mode, &BitsPerPixel);
  FrameBufferBase = (UINTN *)((UINTN)(This->Mode->FrameBufferBase));

  // The UEFI spec currently states:
  // "There is no limitation on the overlapping of the source and destination rectangles"
  // Therefore, we must be careful to avoid overwriting the source data
  if( SourceY == DestinationY ) {
    // Copying within the same height, e.g. horizontal shift
    if( SourceX == DestinationX ) {
      // Nothing to do
      Status = EFI_SUCCESS;
    } else if( ((SourceX>DestinationX)?(SourceX - DestinationX):(DestinationX - SourceX)) < Width ) {
      // There is overlap
      Status = VideoCopyHorizontalOverlap90 (BitsPerPixel, FrameBufferBase, VerticalResolution, SourceX, SourceY, DestinationX, DestinationY, Width, Height );
    } else {
      // No overlap
      Status = VideoCopyNoHorizontalOverlap90 (BitsPerPixel, FrameBufferBase, VerticalResolution, SourceX, SourceY, DestinationX, DestinationY, Width, Height );
    }
  } else {
    // Copying from different heights
    Status = VideoCopyNoHorizontalOverlap90 (BitsPerPixel, FrameBufferBase, VerticalResolution, SourceX, SourceY, DestinationX, DestinationY, Width, Height );
  }

  return Status;
}

/***************************************
 * GraphicsOutput Protocol function, mapping to
 * EFI_GRAPHICS_OUTPUT_PROTOCOL.Blt
 *
 * PRESUMES: 1 pixel = 4 bytes (32bits)
 *  ***************************************/
EFI_STATUS
EFIAPI
LcdGraphicsBlt (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL       *This,
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *BltBuffer  OPTIONAL,
  IN EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
  IN UINTN                              SourceX,
  IN UINTN                              SourceY,
  IN UINTN                              DestinationX,
  IN UINTN                              DestinationY,
  IN UINTN                              Width,
  IN UINTN                              Height,
  IN UINTN                              Delta       OPTIONAL
  )
{
  EFI_STATUS  Status;
  UINT32      *FrameBuffer;
  UINT32      HorizontalResolution;
  UINTN       WidthInBytes;
  UINT32      *SourceBuffer;
  UINT32      *DestinationBuffer;
  UINTN       Y;

  FrameBuffer          = (UINT32 *)This->Mode->FrameBufferBase;
  HorizontalResolution = This->Mode->Info->HorizontalResolution;
  WidthInBytes         = Width * RK_BYTES_PER_PIXEL;

  Status = LcdGraphicsBltCheckParameters (
             This,
             BltBuffer,
             BltOperation,
             SourceX,
             SourceY,
             DestinationX,
             DestinationY,
             Width,
             Height,
             Delta
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((BltOperation == EfiBltVideoFill) || (BltOperation == EfiBltBufferToVideo) || (BltOperation == EfiBltVideoToBltBuffer)) {
    ASSERT( BltBuffer != NULL);
  }

  /*if ((DestinationX >= HorizontalResolution) || (DestinationY >= VerticalResolution)) {
    DEBUG((DEBUG_ERROR, "LcdGraphicsBlt: ERROR - Invalid destination.\n" ));
    Status = EFI_INVALID_PARAMETER;
    goto EXIT;
  }*/

  // If we are reading data out of the video buffer, check that the source area is within the display limits
  if ((BltOperation == EfiBltVideoToBltBuffer) || (BltOperation == EfiBltVideoToVideo)) {
    if ((SourceY + Height > VerticalResolution) || (SourceX + Width > HorizontalResolution)) {
      DEBUG((DEBUG_INFO, "LcdGraphicsBlt: ERROR - Invalid source resolution.\n" ));
      DEBUG((DEBUG_INFO, "                      - SourceY=%d + Height=%d > VerticalResolution=%d.\n", SourceY, Height, VerticalResolution ));
      DEBUG((DEBUG_INFO, "                      - SourceX=%d + Width=%d > HorizontalResolution=%d.\n", SourceX, Width, HorizontalResolution ));
      Status = EFI_INVALID_PARAMETER;
      goto EXIT;
    }
  }

  // If we are writing data into the video buffer, that the destination area is within the display limits
  if ((BltOperation == EfiBltVideoFill) || (BltOperation == EfiBltBufferToVideo) || (BltOperation == EfiBltVideoToVideo)) {
    if ((DestinationY + Height > VerticalResolution) || (DestinationX + Width > HorizontalResolution)) {
      DEBUG((DEBUG_INFO, "LcdGraphicsBlt: ERROR - Invalid destination resolution.\n" ));
      DEBUG((DEBUG_INFO, "                      - DestinationY=%d + Height=%d > VerticalResolution=%d.\n", DestinationY, Height, VerticalResolution ));
      DEBUG((DEBUG_INFO, "                      - DestinationX=%d + Width=%d > HorizontalResolution=%d.\n", DestinationX, Width, HorizontalResolution ));
      Status = EFI_INVALID_PARAMETER;
      goto EXIT;
    }
  }

  //
  // Perform the Block Transfer Operation
  //

  BOOLEAN LandscapeMode = FixedPcdGetBool (PcdLcdRotateToLandscape);

  switch (BltOperation) {
  case EfiBltVideoFill:
    if (LandscapeMode) {
      Status = BltVideoFill90 (This, BltBuffer, SourceX, SourceY, DestinationX, DestinationY, Width, Height, Delta);
    } else {
      Status = BltVideoFill (This, BltBuffer, SourceX, SourceY, DestinationX, DestinationY, Width, Height, Delta);
    }
    break;

  case EfiBltVideoToBltBuffer:
    if (LandscapeMode) {
      Status = BltVideoToBltBuffer90 (This, BltBuffer, SourceX, SourceY, DestinationX, DestinationY, Width, Height, Delta);
    } else {
      Status = BltVideoToBltBuffer (This, BltBuffer, SourceX, SourceY, DestinationX, DestinationY, Width, Height, Delta);
    }
    break;

  case EfiBltBufferToVideo:
    if (LandscapeMode) {
      Status = BltBufferToVideo90 (This, BltBuffer, SourceX, SourceY, DestinationX, DestinationY, Width, Height, Delta);
    } else {
      Status = BltBufferToVideo (This, BltBuffer, SourceX, SourceY, DestinationX, DestinationY, Width, Height, Delta);
    }
    break;

  case EfiBltVideoToVideo:
    if (LandscapeMode) {
      Status = BltVideoToVideo90 (This, BltBuffer, SourceX, SourceY, DestinationX, DestinationY, Width, Height, Delta);
    } else {
      Status = BltVideoToVideo (This, BltBuffer, SourceX, SourceY, DestinationX, DestinationY, Width, Height, Delta);
    }
    break;

    case EfiBltVideoToBltBuffer:
      if (Delta == 0) {
        Delta = WidthInBytes;
      }

      for (Y = 0; Y < Height; Y++) {
        SourceBuffer = FrameBuffer +
                       (SourceY + Y) * HorizontalResolution +
                       SourceX;

        DestinationBuffer = (UINT32 *)((UINTN)BltBuffer +
                                       (DestinationY + Y) * Delta +
                                       DestinationX * RK_BYTES_PER_PIXEL);

        CopyMem (DestinationBuffer, SourceBuffer, WidthInBytes);
      }

      break;

    case EfiBltBufferToVideo:
      if (Delta == 0) {
        Delta = WidthInBytes;
      }

      for (Y = 0; Y < Height; Y++) {
        SourceBuffer = (UINT32 *)((UINTN)BltBuffer +
                                  (SourceY + Y) * Delta +
                                  SourceX * RK_BYTES_PER_PIXEL);

        DestinationBuffer = FrameBuffer +
                            (DestinationY + Y) * HorizontalResolution +
                            DestinationX;

        CopyMem (DestinationBuffer, SourceBuffer, WidthInBytes);
      }

      break;

    case EfiBltVideoToVideo:
      if (SourceY < DestinationY) {
        for (Y = Height; Y-- > 0;) {
          SourceBuffer = FrameBuffer +
                         (SourceY + Y) * HorizontalResolution +
                         SourceX;

          DestinationBuffer = FrameBuffer +
                              (DestinationY + Y) * HorizontalResolution +
                              DestinationX;

          CopyMem (DestinationBuffer, SourceBuffer, WidthInBytes);
        }
      } else {
        for (Y = 0; Y < Height; Y++) {
          SourceBuffer = FrameBuffer +
                         (SourceY + Y) * HorizontalResolution +
                         SourceX;

          DestinationBuffer = FrameBuffer +
                              (DestinationY + Y) * HorizontalResolution +
                              DestinationX;

          CopyMem (DestinationBuffer, SourceBuffer, WidthInBytes);
        }
      }

      break;

    default:
      return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
LcdGraphicsBlt90 (
  IN EFI_GRAPHICS_OUTPUT_PROTOCOL       *This,
  IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL  *BltBuffer  OPTIONAL,
  IN EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
  IN UINTN                              SourceX,
  IN UINTN                              SourceY,
  IN UINTN                              DestinationX,
  IN UINTN                              DestinationY,
  IN UINTN                              Width,
  IN UINTN                              Height,
  IN UINTN                              Delta       OPTIONAL
  )
{
  EFI_STATUS  Status;
  UINT32      *FrameBuffer;
  UINT32      HorizontalResolution;
  UINTN       WidthInBytes;
  UINT32      *SourceBuffer;
  UINT32      *DestinationBuffer;
  UINTN       Y;
  UINTN       X;

  FrameBuffer          = (UINT32 *)This->Mode->FrameBufferBase;
  HorizontalResolution = This->Mode->Info->VerticalResolution;
  WidthInBytes         = Width * RK_BYTES_PER_PIXEL;

  Status = LcdGraphicsBltCheckParameters (
             This,
             BltBuffer,
             BltOperation,
             SourceX,
             SourceY,
             DestinationX,
             DestinationY,
             Width,
             Height,
             Delta
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  switch (BltOperation) {
    case EfiBltVideoFill:
      SourceBuffer = (UINT32 *)BltBuffer;

      for (Y = 0; Y < Height; Y++) {
        for (X = 0; X < Width; X++) {
          DestinationBuffer = FrameBuffer +
                              (HorizontalResolution - 1 - (DestinationY + Y)) +
                              (DestinationX + X) * HorizontalResolution;

          *DestinationBuffer = *SourceBuffer;
        }
      }

      break;

    case EfiBltVideoToBltBuffer:
      if (Delta == 0) {
        Delta = WidthInBytes;
      }

      for (Y = 0; Y < Height; Y++) {
        for (X = 0; X < Width; X++) {
          SourceBuffer = FrameBuffer +
                         (HorizontalResolution - 1 - (SourceY + Y)) +
                         (SourceX + X) * HorizontalResolution;

          DestinationBuffer = (UINT32 *)((UINTN)BltBuffer +
                                         (DestinationY + Y) * Delta +
                                         (DestinationX + X) * RK_BYTES_PER_PIXEL);

          *DestinationBuffer = *SourceBuffer;
        }
      }

      break;

    case EfiBltBufferToVideo:
      if (Delta == 0) {
        Delta = WidthInBytes;
      }

      for (Y = 0; Y < Height; Y++) {
        for (X = 0; X < Width; X++) {
          SourceBuffer = (UINT32 *)((UINTN)BltBuffer +
                                    (SourceY + Y) * Delta +
                                    (SourceX + X) * RK_BYTES_PER_PIXEL);

          DestinationBuffer = FrameBuffer +
                              (HorizontalResolution - 1 - (DestinationY + Y)) +
                              (DestinationX + X) * HorizontalResolution;

          *DestinationBuffer = *SourceBuffer;
        }
      }

      break;

    case EfiBltVideoToVideo:
      if (SourceY < DestinationY) {
        for (Y = Height; Y-- > 0;) {
          for (X = 0; X < Width; X++) {
            SourceBuffer = FrameBuffer +
                           (HorizontalResolution - 1 - (SourceY + Y)) +
                           (SourceX + X) * HorizontalResolution;

            DestinationBuffer = FrameBuffer +
                                (HorizontalResolution - 1 - (DestinationY + Y)) +
                                (DestinationX + X) * HorizontalResolution;

            *DestinationBuffer = *SourceBuffer;
          }
        }
      } else {
        for (Y = 0; Y < Height; Y++) {
          for (X = 0; X < Width; X++) {
            SourceBuffer = FrameBuffer +
                           (HorizontalResolution - 1 - (SourceY + Y)) +
                           (SourceX + X) * HorizontalResolution;

            DestinationBuffer = FrameBuffer +
                                (HorizontalResolution - 1 - (DestinationY + Y)) +
                                (DestinationX + X) * HorizontalResolution;

            *DestinationBuffer = *SourceBuffer;
          }
        }
      }

      break;

    default:
      return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}
