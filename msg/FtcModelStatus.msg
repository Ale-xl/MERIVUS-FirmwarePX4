# Online propulsion and rigid-body model observation
uint64 timestamp
uint64 timestamp_sample

bool valid
bool effectiveness_valid
bool mass_valid
bool inertia_valid
bool cg_valid

uint8 estimator_type
uint8 motor_count
float32 excitation
float32 model_quality
float32 rotational_residual

uint8 NUM_MOTORS = 12
float32[12] motor_effectiveness
float32[12] motor_confidence

float32 mass
float32[3] inertia
float32[3] cg_offset
