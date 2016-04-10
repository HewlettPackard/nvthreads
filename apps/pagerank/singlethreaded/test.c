/*
  Copyright 2015-2016 Hewlett Packard Enterprise Development LP
  
  This program is free software; you can redistribute it and/or modify 
  it under the terms of the GNU General Public License, version 2 as 
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, 
  but WITHOUT ANY WARRANTY; without even the implied warranty of 
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License 
  along with this program; if not, write to the Free Software 
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

*/
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>

int main(int argc, char** argv) {
	FILE *f = fopen(argv[1], "rb");
	
	char *buffer = 0;
	long length = 0;
	
	if(f) {
		fseek(f, 0, SEEK_END);
		length = ftell(f);
		rewind(f);
		
		buffer = (char*)calloc(length + 1, sizeof(char));
		
		printf("\nbuffer before='%s'\n", buffer);
		
		if(buffer) {
			length = fread(buffer, sizeof(char), length, f);
			
			buffer[++length] = '\0';
		}
		
		fclose(f);
	}
	
	printf("length=%d\n", length);
	
	for(int i = 0; i < length; i++)
                printf("[%d] '%d'\n", i,(unsigned char) buffer[i]);
}

