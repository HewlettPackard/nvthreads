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
#ifndef TESTS_H
#define TESTS_H

#include <logger.h>
#include <matrix.h>
#include <crsmatrix.h>
#include <vector.h>
#include <algorithm.h>
#include <utils.h>

int tests_run = 0;

static char* test_vector_sort()
{
    vector_f v;
    vector_f_init(&v, 3);

    v.elements[0] = 1.5;
    v.elements[1] = -0.5;
    v.elements[2] = 0.8;

    vector_f_sort(&v);

    mu_assert("vector_sort failed", compare_floats(v.elements[0], -0.5));
    mu_assert("vector_sort failed", compare_floats(v.elements[1], 0.8));

    mu_assert("vector_sort failed", compare_floats(v.elements[2], 1.5));

    vector_f_free(&v);

    return 0;
}

static char* test_compare_floats()
{
    mu_assert("compare_floats failed", compare_floats(0.0, 0.0));
    mu_assert("compare_floats failed", compare_floats(0.001, 0.001));
    mu_assert("compare_floats failed", !compare_floats(0.0, 0.150));
    return 0;
}

static char* test_vector_copy()
{
    size_t size = 3;

    vector_f vec;
    vector_f_init(&vec, size);

    vector_f tmp_vec;
    vector_f_init(&tmp_vec, size);

    for (size_t i = 0; i < size; ++i)
    {
	vec.elements[i] = 0.0;
	tmp_vec.elements[i] = 6.66;
    }

    for (size_t i = 0; i < size; ++i)
    {
	mu_assert("vector init failed", compare_floats(vec.elements[i], 0.0));
	mu_assert("vector init failed", compare_floats(tmp_vec.elements[i], 6.66));
    }

    for (size_t i = 0; i < size; ++i)
    {
	vec.elements[i] = tmp_vec.elements[i];
	tmp_vec.elements[i] = 0.0;
    }

    for (size_t i = 0; i < size; ++i)
    {
	mu_assert("vector copy failed", compare_floats(vec.elements[i], 6.66));
	mu_assert("vector copy failed", compare_floats(tmp_vec.elements[i], 0.0));
    }

    vector_f_free(&vec);
    vector_f_free(&tmp_vec);

    return 0;
}

static char* test_vector_normalize()
{
    vector_f v;
    vector_f_init(&v, 3);

    v.elements[0] = 2.5;
    v.elements[1] = -0.5;
    v.elements[2] = -0.5;

    vector_f_normalize(&v);

    mu_assert("vector_normalize failed", compare_floats(v.elements[0], 1.66));
    mu_assert("vector_normalize failed", compare_floats(v.elements[1], -0.33));
    mu_assert("vector_normalize failed", compare_floats(v.elements[2], -0.33));

    vector_f_free(&v);

    return 0;
}

static char* test_matrix_multiply()
{
    size_t problem_size = 2;
    matrix_f g;

    matrix_f_init(&g, problem_size);

    g.elements[0][0] = 0;	
    g.elements[0][1] = 0.5;	
    g.elements[1][0] = 1;	
    g.elements[1][1] = 0.5;	

    vector_f output;
    vector_f input;
    vector_f_init(&output, problem_size);
    vector_f_init(&input, problem_size);

    input.elements[0] = 1.0;
    input.elements[1] = 1.0;

    matrix_f_multiply_vector_f(&output, &g, &input);

    mu_assert("matrix multiply failed", compare_floats(output.elements[0], 0.5 ));
    mu_assert("matrix multiply failed", compare_floats(output.elements[1], 1.5 ));

    matrix_f_free(&g);
    vector_f_free(&output);
    vector_f_free(&input);

    return 0;
}

static char* test_matrix_compress()
{
	size_t problem_size = 3;
	
	matrix_i *m = malloc(sizeof(matrix_i));
	matrix_i_init_set(m, problem_size, 0);
	
	m->elements[0][0] = 0;
        m->elements[0][1] = 4;
	m->elements[0][2] = 1;
        
	m->elements[1][0] = 5;
        m->elements[1][1] = 6;
	m->elements[1][2] = 0;
	
	m->elements[2][0] = 0;
        m->elements[2][1] = 123;
        m->elements[2][2] = 0;
	
	matrix_crs_i *dst = malloc(sizeof(matrix_crs_i));
	mcrs_i_init(dst, 0);

	mcrs_err e;
	
	if((e = mcrs_i_from_matrix_i(dst, m)) != MCRS_ERR_NONE)
		return 1;
	
	printf("\n");
	matrix_i_display(LOGD_L, m);
	printf("\n");
	mcrs_i_display(LOGD_L, dst);
	printf("\n");	

	mcrs_i_free(dst);
	//free(dst);
	matrix_i_free(m);
	
	return 0;
}

static char* test_matrix_vector_multiply()
{
	size_t problem_size = 2;
	
	matrix_f *m = (matrix_f*)malloc(sizeof(matrix_f));
	vector_f *v = (vector_f*)malloc(sizeof(vector_f));
	vector_f *res = (vector_f*)malloc(sizeof(vector_f));	

	matrix_f_init_set(m, problem_size, 0.0);
	vector_f_init_set(v, problem_size, 0.0);
	vector_f_init_set(res, problem_size, 0.0);
	
	m->elements[0][0] = 3;
	m->elements[0][1] = 4;
	m->elements[1][0] = 5;
	m->elements[1][1] = 6;
	
	v->elements[0] = 1;
	v->elements[1] = 2;
	
	matrix_f_multiply_vector_f(res, m, v);

	vector_f_display(LOGD_L, res);	

	mu_assert("m*v-multiplication failed", compare_floats(res->elements[0], 11.0));
	mu_assert("m*v-multiplication failed", compare_floats(res->elements[1], 17.0));

	vector_f_free(res);
	vector_f_free(v);
	matrix_f_free(m);
	
	return 0;
}

static char* test_matrix_transpose()
{
    size_t problem_size = 2;
    matrix_f problem;

    matrix_f_init(&problem, problem_size);

    problem.elements[0][0] = 1.0;
    problem.elements[0][1] = 2.0;
    problem.elements[1][0] = 3.0;
    problem.elements[1][1] = 4.0;

    matrix_f_transpose(&problem);

    mu_assert("matrix_tranpose failed", compare_floats(problem.elements[0][0], 1.0));
    mu_assert("matrix_tranpose failed", compare_floats(problem.elements[0][1], 3.0));
    mu_assert("matrix_tranpose failed", compare_floats(problem.elements[1][0], 2.0));
    mu_assert("matrix_tranpose failed", compare_floats(problem.elements[1][1], 4.0));

    matrix_f_free(&problem);

    return 0;
}

static char* test_matrix_multi()
{
    size_t problem_size = 2;

    matrix_f *problem_a = (matrix_f*)malloc(sizeof(matrix_f));
    matrix_f *problem_b = (matrix_f*)malloc(sizeof(matrix_f));

    matrix_f_init_set(problem_a, problem_size, 0.0);
    matrix_f_init_set(problem_b, problem_size, 0.0);

    problem_a->elements[0][0] = 1;
    problem_a->elements[0][1] = 2;
    problem_a->elements[1][0] = 3;
    problem_a->elements[1][1] = 4;
    
    problem_b->elements[0][0] = 5;
    problem_b->elements[0][1] = 6;
    problem_b->elements[1][0] = 7;
    problem_b->elements[1][1] = 8;

    matrix_f *result = (matrix_f*)malloc(sizeof(matrix_f));
    
    matrix_f_init_set(result, problem_size, 0.0);

    matrix_f_multiply_matrix_f(result, problem_a, problem_b);

    matrix_f_display(LOGD_L, result);

    matrix_f_free(result);
    matrix_f_free(problem_b);
    matrix_f_free(problem_a);

    return 0;
}

static char* test_matrix_solve()
{
    size_t problem_size = 3;
    matrix_f problem;

    matrix_f_init(&problem, problem_size);

    problem.elements[0][0] = 1.5;
    problem.elements[0][1] = 0.0;
    problem.elements[0][2] = 1.0;
    problem.elements[1][0] = -0.5;
    problem.elements[1][1] = 0.5;
    problem.elements[1][2] = -0.5;
    problem.elements[2][0] = -0.5;
    problem.elements[2][1] = 0.0;
    problem.elements[2][2] = 0.0;

    vector_f v;
    vector_f_init(&v, problem_size);
    matrix_f_solve(&v, &problem);

    mu_assert("matrix_solve failed", compare_floats(v.elements[0], 9.66));
    mu_assert("matrix_solve failed", compare_floats(v.elements[1], -4.33));
    mu_assert("matrix_solve failed", compare_floats(v.elements[2], -4.33));

    matrix_f_free(&problem);
    vector_f_free(&v);

    return 0;
}

static char* test_calculate_links()
{
    size_t problem_size = 2;
    matrix_f problem;

    matrix_f_init(&problem, problem_size);

    problem.elements[0][0] = 0;
    problem.elements[0][1] = 0;
    problem.elements[1][0] = 1;
    problem.elements[1][1] = 0;

    mu_assert("calculate_link failed", calculate_links(&problem, 0) == 1);
    mu_assert("caclulate_link failed", calculate_links(&problem, 1) == 2);

    matrix_f_free(&problem);

    return 0;
}

static char* test_gen_web_matrix()
{
    size_t size = 10;
    matrix_f w;
    matrix_f_init(&w, size);
    gen_web_matrix(&w); // random generated links matrix
    //TODO: add some smart asserts
    return 0;
}

static char* test_calculate_probability()
{
    size_t problem_size = 3;
    matrix_f problem;

    matrix_f_init(&problem, problem_size);

    problem.elements[0][0] = 0;
    problem.elements[0][1] = 0;
    problem.elements[0][2] = 0;
    problem.elements[1][0] = 1.0;
    problem.elements[1][1] = 0;
    problem.elements[1][2] = 0;
    problem.elements[2][0] = 1.0;
    problem.elements[2][1] = 0;
    problem.elements[2][2] = 1.0;

    mu_assert("calculate_probability failed", compare_floats(calculate_probability(&problem, 0, 0), 0));
    mu_assert("calculate_probability failed", compare_floats(calculate_probability(&problem, 0, 1), 0));
    mu_assert("calculate_probability failed", compare_floats(calculate_probability(&problem, 0, 2), 0));

    mu_assert("calculate_probability failed", compare_floats(calculate_probability(&problem, 1, 0), 0.33));
    mu_assert("calculate_probability failed", compare_floats(calculate_probability(&problem, 1, 1), 0));
    mu_assert("calculate_probability failed", compare_floats(calculate_probability(&problem, 1, 2), 0));

    mu_assert("calculate_probability failed", compare_floats(calculate_probability(&problem, 2, 0), 1));
    mu_assert("calculate_probability failed", compare_floats(calculate_probability(&problem, 2, 1), 0));
    mu_assert("calculate_probability failed", compare_floats(calculate_probability(&problem, 2, 2), 1));

    matrix_f_free(&problem);

    return 0;
}

static char* test_full_algorithm()
{
    size_t web_size = 10;

    matrix_f w;
    matrix_f_init(&w, web_size);
    gen_web_matrix(&w); // random generated links matrix

    //w.elements[0][0] = 0.5;	
    //w.elements[0][1] = 1;	
    //w.elements[1][0] = 0.5;	
    //w.elements[1][1] = 0;	

    //printf("web matrix\n");
    //matrix_display(&w);

    matrix_f g;
    matrix_f_init(&g, web_size);
    gen_google_matrix(&g, &w);

    //printf("google matrix\n");
    //matrix_display(&g);

    vector_f p;
    vector_f_init(&p, web_size);

    matrix_f_transpose(&g);

    matrix_f_solve(&p, &g);

    vector_f_sort(&p);

    float sum = 0.0;
    for (size_t i = 0; i < web_size; ++i)
    {
	logd(LOGD_L, "PageRank = %f\n", p.elements[i]);
	sum += p.elements[i];
    }
    logd(LOGD_L, "sum = %f\n", sum);

    matrix_f_free(&w);
    matrix_f_free(&g);
    vector_f_free(&p);

    return 0;
}

#endif // TESTS_Hu

