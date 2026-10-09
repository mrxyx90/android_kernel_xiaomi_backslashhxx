/*
 * Simple GPU frequency algorithm for Qualcomm Adreno devfreq.
 *
 * Based on the simple GPU algorithm by Paul Reioux, with bounds checks
 * adapted for the KGSL/Adreno devfreq profile used by this kernel.
 *
 * This is optional and is not enabled by default. The TrustZone DCVS
 * algorithm remains in use unless simple_gpu_activate is set to 1.
 *
 * SPDX-License-Identifier: GPL-2.0
 */
#include <linux/module.h>
#include <linux/devfreq.h>
#include <linux/errno.h>
#include <linux/msm_adreno_devfreq.h>

static int default_laziness = 2;
module_param_named(simple_laziness, default_laziness, int, 0664);

static int ramp_up_threshold = 3000;
module_param_named(simple_ramp_threshold, ramp_up_threshold, int, 0664);

/* Keep the existing TrustZone DCVS algorithm as the safe default. */
int simple_gpu_active;
module_param_named(simple_gpu_activate, simple_gpu_active, int, 0664);

static int laziness;

/*
 * Return a frequency-level delta in *val. A negative delta raises the
 * GPU frequency (lower power-level index); a positive delta lowers it.
 */
int simple_gpu_algorithm(int level, int max_level, int *val,
			 struct devfreq_msm_adreno_tz_data *priv)
{
	int ret = 0;

	if (!val || !priv || max_level <= 0 ||
	    level < 0 || level >= max_level) {
		if (val)
			*val = 0;
		return -EINVAL;
	}

	/* Busy GPU: step up one level, if not already at the maximum. */
	if (priv->bin.busy_time > ramp_up_threshold) {
		if (level > 0)
			ret = -1;
	} else if (level < max_level - 1) {
		/* Delay downscaling to avoid oscillating during brief idle gaps. */
		if (laziness > 0) {
			laziness--;
		} else {
			ret = 1;
			laziness = max(default_laziness, 0);
		}
	}

	*val = ret;
	return 0;
}
EXPORT_SYMBOL(simple_gpu_algorithm);

MODULE_AUTHOR("Paul Reioux; adapted for KGSL/Adreno devfreq");
MODULE_DESCRIPTION("Optional simple Adreno GPU frequency algorithm");
MODULE_LICENSE("GPL v2");
