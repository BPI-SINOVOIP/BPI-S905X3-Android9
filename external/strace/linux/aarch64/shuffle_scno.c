#define shuffle_scno arm_shuffle_scno
#include "../arm/shuffle_scno.c"
#undef shuffle_scno

kernel_ulong_t
shuffle_scno(kernel_ulong_t scno)
{
	if (current_personality == 1)
		return arm_shuffle_scno(scno);

	return scno;
}
