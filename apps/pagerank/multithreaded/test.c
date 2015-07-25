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

