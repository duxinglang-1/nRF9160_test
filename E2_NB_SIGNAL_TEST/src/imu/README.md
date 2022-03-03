# IMU Functions

To activate IMU functions (step counter, fall detection, wrist tilt detection), you need to initialize IMU first.

After Initialisation, single tap and wrist tilt event will send an interrupt to pin INT2 of the IMU, which will make `int2_event = true`.

Step counter will send an interrupt to pin INT1 of the IMU (10 continuous steps must be detected to activate step counter), which will make `int1_event = true`.



### Function Description

`void IMU_init(void);`  ---> Initialize IMU



`void fall_detection(void);`   ---> Run fall-analyse（only run this function when a single tap is detected）

`bool fall_result;`   ---> fall_result = true when a fall happens

`bool int2_event;`   ---> Both single-tap an wrist-tilt will make int2_event = true/



`void GetImuSteps(void);`   

`void ReSetImuSteps(void);`   

`void GetSportData(u16_t *steps, u16_t *calorie, u16_t *distance);`   



`void is_tilt(void);`   ---> Check whether a wrist-tilt happened.

`bool wrist_tilt;`   ---> wrist_tilt = true when a wrist-tilt happened.

`void disable_tilt_detection(void);`   ---> Disable wrist-tilt detection.

`void enable_tilt_detection(void);`   ---> Re-enable wrist-tilt detection.











