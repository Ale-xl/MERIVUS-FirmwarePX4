# Candidate actuator compensation calculated for observation only
uint64 timestamp
uint64 timestamp_sample

uint8 motor_count
bool valid
uint16 saturated_mask

uint8 NUM_MOTORS = 12
float32[12] nominal
float32[12] candidate
float32[12] effectiveness
