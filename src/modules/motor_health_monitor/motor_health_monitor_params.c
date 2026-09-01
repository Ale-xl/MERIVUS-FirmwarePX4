/****************************************************************************
 *
 * Copyright (c) 2026 Merivus Industrial. All rights reserved.
 *
 ****************************************************************************/

/**
 * Enable the fault-tolerant motor health monitor
 *
 * The module only observes and publishes diagnostics. It does not alter
 * actuator commands or controller setpoints.
 *
 * @boolean
 * @reboot_required true
 * @group Fault Tolerant Control
 */
PARAM_DEFINE_INT32(FTC_MON_EN, 0);

/**
 * Minimum mean motor command used for estimation
 *
 * @unit norm
 * @min 0.05
 * @max 0.8
 * @decimal 2
 * @group Fault Tolerant Control
 */
PARAM_DEFINE_FLOAT(FTC_MIN_THR, 0.15f);

/**
 * Input and angular acceleration low-pass time constant
 *
 * @unit s
 * @min 0.02
 * @max 2.0
 * @decimal 2
 * @group Fault Tolerant Control
 */
PARAM_DEFINE_FLOAT(FTC_LPF_TC, 0.20f);

/**
 * Minimum per-motor command excitation standard deviation
 *
 * @unit norm
 * @min 0.005
 * @max 0.3
 * @decimal 3
 * @group Fault Tolerant Control
 */
PARAM_DEFINE_FLOAT(FTC_EXC_MIN, 0.025f);

/**
 * Healthy baseline learning duration with sufficient excitation
 *
 * @unit s
 * @min 1.0
 * @max 30.0
 * @decimal 1
 * @group Fault Tolerant Control
 */
PARAM_DEFINE_FLOAT(FTC_BASE_T, 5.0f);

/**
 * Maximum effectiveness estimate change rate
 *
 * @unit 1/s
 * @min 0.02
 * @max 2.0
 * @decimal 2
 * @group Fault Tolerant Control
 */
PARAM_DEFINE_FLOAT(FTC_EST_RATE, 0.30f);

/**
 * Maximum normalized model residual for a valid estimate
 *
 * @min 0.05
 * @max 5.0
 * @decimal 2
 * @group Fault Tolerant Control
 */
PARAM_DEFINE_FLOAT(FTC_RES_THR, 1.0f);

/**
 * Health threshold for degraded classification
 *
 * @min 0.1
 * @max 0.95
 * @decimal 2
 * @group Fault Tolerant Control
 */
PARAM_DEFINE_FLOAT(FTC_HLTH_MIN, 0.70f);

/**
 * Health threshold for failed classification
 *
 * @min 0.0
 * @max 0.7
 * @decimal 2
 * @group Fault Tolerant Control
 */
PARAM_DEFINE_FLOAT(FTC_FAIL_MIN, 0.25f);

/**
 * Minimum estimator confidence for fault classification
 *
 * @min 0.0
 * @max 1.0
 * @decimal 2
 * @group Fault Tolerant Control
 */
PARAM_DEFINE_FLOAT(FTC_CONF_MIN, 0.60f);

/**
 * Fault threshold persistence time
 *
 * @unit s
 * @min 0.1
 * @max 10.0
 * @decimal 1
 * @group Fault Tolerant Control
 */
PARAM_DEFINE_FLOAT(FTC_FAIL_T, 1.0f);

/**
 * Enable adaptive allocation shadow calculation
 *
 * Shadow mode publishes a candidate output for logging only and never writes
 * to the actuator pipeline.
 *
 * @boolean
 * @group Fault Tolerant Control
 */
PARAM_DEFINE_INT32(FTC_CA_SHADOW, 0);
