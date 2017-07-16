/* See LICENSE file for copyright and license details. */

#define PROCESS process_lf
#define TYPE double
#define SCAN_TYPE "lf"
#define PRINT_TYPE "lf"
#define PRINT_CAST double
#include FILE
#undef PROCESS
#undef TYPE
#undef SCAN_TYPE
#undef PRINT_TYPE
#undef PRINT_CAST

#define PROCESS process_f
#define TYPE float
#define SCAN_TYPE "f"
#define PRINT_TYPE "lf"
#define PRINT_CAST double
#include FILE
#undef PROCESS
#undef TYPE
#undef SCAN_TYPE
#undef PRINT_TYPE
#undef PRINT_CAST
