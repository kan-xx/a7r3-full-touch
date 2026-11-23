 /* Baseline 96px: 107px is below the threshold and 108px is exact. */
 cases++;reset();event(0,100,1,10);event(1,196,1,11);event(1,207,0x80000001,12);fire();
 CK(!dispatch_calls&&!getter_calls&&!hit_calls&&!rt_pb.capture_attempts&&!allocations);
 CK(!rt_pb.timer&&rt_pb.frames==1&&rt_pb.image.valid&&RW(667)==P15D_NO_PLAN);
 event(1,208,0x80000001,13);fire();
 CK(dispatch_calls==1&&rt_pb.capture_attempts==4&&getter_calls==8&&hit_calls==16);
 CK(condition_calls==1&&releases==8&&rt_pb.frames==2&&!rt_pb.timer);

 /* Keep the existing queue retry even on a geometrically negative sample. */
 cases++;reset();event(0,100,1,10);event(1,196,1,11);queue_busy=1;fire();
 CK(!dispatch_calls&&!getter_calls&&rt_pb.timer&&rt_pb.defer_left==1);
 queue_busy=0;event(1,208,0x80000001,12);fire();
 CK(dispatch_calls==1&&rt_pb.capture_attempts==4&&!rt_pb.timer);

 /* A negative sample does not bypass the original M59 result. */
 cases++;reset();event(0,100,1,10);event(1,196,1,11);lookup59_bad=1;fire();
 CK(!dispatch_calls&&!getter_calls&&RW(667)==P15D_M59);
