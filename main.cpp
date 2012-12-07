#include <iostream>
#include <cassert>

#include <string.h>
#include <malloc.h>
#include <papi.h>

extern "C" {
#include <cblas.h>
}

#include <cublas.h>

using namespace std;

template<typename DotFunction>
float time_dot(DotFunction f, int N, float *A, float *B) {
    auto start = PAPI_get_real_usec();

    auto dot = f(N, A, B);

    auto stop = PAPI_get_real_usec();

    // A relatively harmless call to keep the compiler from realizing
    // we don't actually do any useful work with the dot product.
    srand48(dot);

    return float(stop - start) / 1e6;    
}

// Generate a random vector
float *generate_vector(int N) {
    float *v = (float *)memalign(32,
                                 sizeof(float) * N);

    for(int i = 0; i < N; ++i) {
        v[i] = drand48();
    }

    return v;
}

float simple_dot(int, float *, float *);
float sse_dot(int, float *, float *);
float avx_dot(int, float *, float *);
float blas_dot(int, float *, float *);
float cublas_dot(int, float *, float *);

int main() {
    auto N = (128 << 20) / sizeof(float);

    cout << "Generating " << N << " element vectors." << endl;

    auto A = generate_vector(N);
    auto B = generate_vector(N);

    assert(A);
    assert(B);
    
    cout << "Simple\t" << time_dot(simple_dot, N, A, B) << endl;;
    cout << "SSE\t" << time_dot(sse_dot, N, A, B) << endl;;
    cout << "AVX\t" << time_dot(avx_dot, N, A, B) << endl;;
    cout << "BLAS\t" << time_dot(blas_dot, N, A, B) << endl;;
    cout << "CUBLAS\t" << time_dot(cublas_dot, N, A, B) << endl;;


    cublasInit();


    cublasShutdown();
    free(A);
    free(B);
}

// Returns the time in seconds to compute the dot product.
//
// This is the usual implementation.
float simple_dot(int N, float *A, float *B) {
    float dot = 0;
    for(int i = 0; i < N; ++i) {
        dot += A[i] * B[i];
    }

    return dot;
}

float sse_dot(int N, float *A, float *B) {
    const int VECTOR_SIZE = 4;

    typedef float vec
        __attribute__ ((vector_size (sizeof(float) * VECTOR_SIZE)));

    vec temp = {0};

    N /= VECTOR_SIZE;

    vec *Av = (vec *)A;
    vec *Bv = (vec *)B;

    for(int i = 0; i < N; ++i) {
        temp += *Av * *Bv;

        Av++;
        Bv++;
    }

    union {
        vec tempv;
        float tempf[VECTOR_SIZE];
    };

    tempv = temp;

    float dot = 0;
    for(int i = 0; i < VECTOR_SIZE; ++i) {
        dot += tempf[i];
    }

    return dot;
}

float avx_dot(int N, float *A, float *B) {
    const int VECTOR_SIZE = 8;

    typedef float vec
        __attribute__ ((vector_size (sizeof(float) * VECTOR_SIZE)));

    vec temp = {0};

    N /= VECTOR_SIZE;

    vec *Av = (vec *)A;
    vec *Bv = (vec *)B;

    for(int i = 0; i < N; ++i) {
        temp += *Av * *Bv;

        Av++;
        Bv++;
    }

    union {
        vec tempv;
        float tempf[VECTOR_SIZE];
    };

    tempv = temp;

    float dot = 0;
    for(int i = 0; i < VECTOR_SIZE; ++i) {
        dot += tempf[i];
    }

    return dot;
}

float blas_dot(int N, float *A, float *B) {
    return cblas_sdot(N, A, 1, B, 1);
}

float cublas_dot(int N, float *A, float *B) {
    return cublasSdot(N, A, 1, B, 1);
}
