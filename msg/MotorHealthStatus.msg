# Continuous motor effectiveness and health observation
uint64 timestamp
uint64 timestamp_sample

uint8 STATE_DISABLED = 0
uint8 STATE_CALIBRATING = 1
uint8 STATE_VALID = 2
uint8 STATE_INVALID = 3
uint8 state

uint8 motor_count
bool model_valid
bool esc_data_available

uint16 degraded_mask
uint16 failed_mask

float32 model_residual

uint8 NUM_MOTORS = 12
float32[12] effectiveness
float32[12] health
float32[12] confidence
float32[12] residual
