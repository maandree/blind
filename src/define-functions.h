/* See LICENSE file for copyright and license details. */

#define PROCESS process_lf
#define TYPE double
#include FILE
#undef PROCESS
#undef TYPE

#define PROCESS process_f
#define TYPE float
#include FILE
#undef PROCESS
#undef TYPE
