/*
* Copyright (C) 2013 - 2016  Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person
* obtaining a copy of this software and associated documentation
* files (the "Software"), to deal in the Software without restriction,
* including without limitation the rights to use, copy, modify, merge,
* publish, distribute, sublicense, and/or sell copies of the Software,
* and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or (b) that interact
* with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in this
* Software without prior written authorization from Xilinx.
*
*/

#include <stdio.h>
#include <malloc.h>
#include <stdio.h>

#define TOTAL_SIZE 1114112
#define CHUNK_MAX 14280
#define BUFFER_SIZE (TOTAL_SIZE/CHUNK_MAX) + 1

int main(int argc, char **argv)
{
    printf("Hello World!\n");
    void* buf[BUFFER_SIZE];
    while (1)
    {
       int i=0;
       int size=TOTAL_SIZE;
       while (size > 0)
       {
          int chunkSize = size;
          if (chunkSize > CHUNK_MAX)
             chunkSize = CHUNK_MAX;
          buf[i] = malloc(chunkSize);
          i++;
          size -= chunkSize;
       }
       for (int j=0; j<i; j++)
          free(buf[j]);
       malloc_trim(0);
       // printf("Loop finished\n");
    }
  
    return 0;
}
