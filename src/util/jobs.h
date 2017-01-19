/* See LICENSE file for copyright and license details. */

#define efork_jobs(...) enfork_jobs(1, __VA_ARGS__)
#define ejoin_jobs(...) enjoin_jobs(1, __VA_ARGS__)

int enfork_jobs(int status, size_t *start, size_t *end, size_t jobs, pid_t **pids);
void enjoin_jobs(int status, int is_master, pid_t *pids);
