#include <matrix.h>

extern void matrix_f_init(matrix_f* m, size_t size)
{
	matrix_f_init_set(m, size, 0.0);
}

extern void matrix_f_init_set(matrix_f* m, size_t size, float init)
{
	assert(m != NULL);

	m->size = size;
    printf("allocating %zu bytes\n", sizeof(float *) * size);
    m->elements = (float **)malloc(sizeof(float *) * size);

	assert(m->elements != NULL);

	for (size_t i = 0; i < size; ++i)
	{
        
        printf("allocating %zu bytes\n", sizeof(float *) * size);
        m->elements[i] = malloc(sizeof(float) * size);

		for(size_t j = 0; j < size; j++) {
			m->elements[i][j] = init;
		}
	}
}

extern void matrix_i_init(matrix_i *m, size_t size)
{
	matrix_i_init_set(m, size, 0);
}

extern void matrix_i_init_set(matrix_i *m, size_t size, int init)
{
	assert(m != NULL);

    printf("allocating %zu bytes\n", sizeof(float *) * size); 

	m->size = size;
	m->elements = (int**)malloc(sizeof(int*) * size);
	
	assert(m->elements != NULL);

	for(size_t i = 0; i < size; i++) {
        printf("allocating %zu bytes\n", sizeof(float *) * size);
        m->elements[i] = malloc(sizeof(int) * size);
		
		for(size_t j = 0; j < size; j++) {
			m->elements[i][j] = init;
		}
	}
}

extern void matrix_f_copy(matrix_f *out, const matrix_f *in)
{
	assert(out->size == in->size);
	
	for(size_t i = 0; i < in->size; i++)
		for(size_t j = 0; j < in->size; j++)
			out->elements[i][j] = in->elements[i][j];
}

extern void matrix_i_to_f(const matrix_i *a, matrix_f *b)
{
	assert(a->size == b->size);

	for(size_t i = 0; i < a->size; i++)
		for(size_t j = 0; j < a->size; j++)
			b->elements[i][j] = (float)a->elements[i][j];
}

extern int matrix_i_load(const char* path, matrix_i *m, const char col, const char row)
{
	FILE *f = fopen(path, "rb");

    	char *buffer = 0;
    	long length = 0;

    	if(f) {
      		fseek(f, 0, SEEK_END);
	        length = ftell(f);
		rewind(f);
		
	        buffer = (char*)calloc(length + 1, sizeof(char));
		
	        if(buffer) {
        		length = fread(buffer, 1, length, f);
			buffer[++length] = '\0';
	        }
		
        	fclose(f);
    	}

    	if(buffer) {
		char *el1 = 0;
		int i1 = 0;
		char *el2 = 0;
		int i2 = 0;
	
		int new_sz;

		el1 = strtok(buffer, &col);

		while(el1) {
			el2 = strtok(NULL, &row);

			assert(el2);

			i1 = atoi(el1);
			i2 = atoi(el2);

			if(i1 + 1 > m->size || i2 + 1 > m->size) { // resize if too small
				new_sz = (i1 > i2 ? i1 + 1 : i2 + 1);
				
				if(matrix_i_realloc(m, new_sz, 0))
					return 1;
			}	

			m->elements[i1][i2]++;

			el1 = strtok(NULL, &col);
		}
	}

	return 0;
}

extern int matrix_f_realloc(matrix_f *m, size_t newsize, float init) 
{
	m->elements = (float**)realloc(m->elements, sizeof(float*) * newsize);
	
	if(m->elements == NULL)
		return 1;

	for(size_t i = 0; i < m->size && i < newsize; i++) {
		m->elements[i] = realloc(m->elements[i], sizeof(float) * newsize);
		
		if(m->elements[i] == NULL)
			return 2;

		for(size_t j = m->size; j < newsize; j++) {
			m->elements[i][j] = init;
		}
	}

	for(size_t i = m->size; i < newsize; i++) {
        printf("allocating %zu bytes\n", sizeof(float *) * newsize);
        m->elements[i] = malloc(sizeof(float) * newsize);

		if(m->elements[i] == NULL)
			return 3;

		for(size_t j = 0; j < newsize; j++) {
			m->elements[i][j] = init;
		}
	}

	m->size = newsize;

	return 0;
}

extern int matrix_i_realloc(matrix_i *m, size_t newsize, int init)
{
	m->elements = (int**)realloc(m->elements, sizeof(int*) * newsize);

	if(m->elements == NULL)
		return 1;

	for(size_t i = 0; i < m->size && i < newsize; i++) {
		m->elements[i] = realloc(m->elements[i], sizeof(int) * newsize);
	
		if(m->elements[i] == NULL)
			return 2;
	
		for(size_t j = m->size; j < newsize; j++) {
			m->elements[i][j] = init;
		}
	}
	
	for(size_t i = m->size; i < newsize; i++) {
        printf("allocating %zu bytes\n", sizeof(float *) * newsize);
        m->elements[i] = malloc(sizeof(int) * newsize);
		
		if(m->elements[i] == NULL)
			return 3;

		for(size_t j = 0; j < newsize; j++) {
			m->elements[i][j] = init;
		}
	}

	m->size = newsize;

	return 0;
}

extern void matrix_f_free(matrix_f* m)
{
	assert(m != NULL);
	for (size_t i = 0; i < m->size; ++i)
	{
		free(m->elements[i]);
	}

	free(m->elements);
	m->elements = NULL;
	m->size = 0;
	m = NULL;
}

extern void matrix_i_free(matrix_i *m) {
	assert(m != NULL);

	for(size_t i = 0; i < m->size; i++) {
		free(m->elements[i]);
	}

	free(m->elements);
	
	m->elements = NULL;
	m->size = 0;

	m = NULL;
}

extern void matrix_f_transpose(const matrix_f *m)
{
	assert(m != NULL);
	float tmp = 0.0;

	for (size_t i = 0; i < m->size; ++i)
	{
		for (size_t j = i+1; j < m->size; ++j)
		{
			tmp = m->elements[i][j];
			m->elements[i][j] = m->elements[j][i];
			m->elements[j][i] = tmp;
		}
	}
}

extern void matrix_i_transpose(const matrix_i *m)
{
	assert(m != NULL);

	int tmp = 0;

	for (size_t i = 0; i < m->size; ++i)
        {
                for (size_t j = i+1; j < m->size; ++j)
                {
                        tmp = m->elements[i][j];
                        m->elements[i][j] = m->elements[j][i];
                        m->elements[j][i] = tmp;
                }
        }
}

extern void matrix_f_multiply_matrix_f(matrix_f *out, const matrix_f *a, const matrix_f *b)
{
	matrix_f_multiply_matrix_f_rng(out, a, b, 0, out->size, 0, out->size);
}

extern void matrix_f_multiply_matrix_f_rng(matrix_f *out, const matrix_f *a, const matrix_f *b, const size_t xoffs, const size_t xlength, const size_t yoffs, const size_t ylength)
{
	assert(out != NULL);
	assert(a != NULL);
	assert(b != NULL);
	assert(out->size == a->size);
	assert(a->size == b->size);
	assert(xoffs + xlength <= out->size);
	assert(yoffs + ylength <= out->size);

	float sum;

	for(size_t x = xoffs; x < (xoffs + xlength); x++) {
		for(size_t y = yoffs; y < (yoffs + ylength); y++) {
			sum = 0.0;

			for(size_t i = 0; i < out->size; i++) {
				sum += a->elements[x][i] * b->elements[i][y];
			}

			out->elements[x][y] = sum;
		}
	}
}

extern void matrix_f_multiply_vector_f(vector_f *tmp, const matrix_f *m, const vector_f *v)
{
	assert(tmp != NULL);
	assert (m != NULL);
	assert(v!= NULL);
	assert(tmp->size == v->size);
	assert(m->size == v->size);

	size_t size = tmp->size;

	float sum;

	for (size_t i = 0; i < size; ++i) {
		sum = 0.0;
		
		for (size_t j = 0; j < size; ++j)
		{
			sum += m->elements[i][j] * v->elements[j]; 
		}

		tmp->elements[i] = sum;
	}
}

