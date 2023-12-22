// di_video_buffer.h - Function declarations for painting video scan lines
//
// A a video buffer is a set of 1-pixel-high video scan lines that are equal
// in length (number of pixels) to the total width of the video screen and
// horizontal synchronization pixels.
//
// Copyright (c) 2023 Curtis Whitley
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// 

#pragma once
#include "di_timing.h"

// Holds the DMA scan line buffer for a single visible line.
class DiVideoScanLine {
  protected:

  uint32_t* m_scan_line;

  public:

  DiVideoScanLine();
  ~DiVideoScanLine();

  static uint32_t get_buffer_size();

  volatile uint32_t* get_active_pixels();
  volatile uint32_t* get_hfp_pixels();
  volatile uint32_t* get_hs_pixels();
  volatile uint32_t* get_hbp_pixels();

  inline volatile uint32_t * get_buffer_ptr() volatile {
    return (volatile uint32_t *) m_scan_line;
  }

  void init_to_black();

  void init_for_vsync();
};

// Holds the DMA scan line buffers for two visible lines.
class DiVideoBuffer {
  protected:

  DiVideoScanLine m_line[NUM_LINES_PER_BUFFER];

  public:

  DiVideoBuffer();
  ~DiVideoBuffer();

  inline int32_t get_buffer_size() volatile {
    return sizeof(m_line);
  }

  inline volatile uint32_t * get_buffer_ptr_0() volatile {
    return m_line[0].get_buffer_ptr();
  }

  inline volatile uint32_t * get_buffer_ptr_1() volatile {
    return m_line[1].get_buffer_ptr();
  }

  void init_to_black();

  void init_for_vsync();
};
