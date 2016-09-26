#ifndef MINUNIT_H
#define MINUNIT_H
//this is the JTN002 - MinUnit minimal unit testing framework.
//will consider changing it up a bit to have per-test failing/passing
//for the time being use as is.
#define mu_assert(message, test) do { if (!(test)) return message; } while (0)
#define mu_run_test(test) do {char * message = test(); tests_run++; \
							if (message) return message;} while (0)
extern int tests_run;
#endif