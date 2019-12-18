The two functions you want to focus on are:

EventButtonRunPredictiveDTW - Line 685
This function sets up the timewarp by doing the following:
a:	Setting up the timewarp curve and assigning the end effectors of the reference motion to the timewarp curve.
b:	Setting the playback to the beginning of the take
c:	Setting up the an update function to be called every frame.

EventHandler - Line 775
This is an update function called every frame, that updates the warp  The function does the following:
a:	Using the current warp speed, determine the time of the current frame and the next two frames in the reference motion. Frames n, n+1, n+2.
b:	Loop through each effector doing the following
	i:	get the postion of the live input motions end effectors
	ii:	predict the position of the live input motion on the next.  This is done using a simple dead rekoning techinique of projecting the current velocity one frame ahead.
	iii. 	measure the simularity of the of the predicted live input motion to each of the reference motion frames (n, n+1, n+2).  
			Uses a simple destance based appraoch and sums the distances between coresponding end effectors using local space. 
c:	depending on which reference framen the live input frame is most simular two, either:
	frame n is most simular - warp the reference motion to speed it up, 
	frame n+1 is most simular - leave the reference motion to play at it's current speed - no warping
	frame n+2 is most simular - warp the reference motion to slow it down.
d:	remove this update function is the end fo the reference motion has been reached.

A more detailed discussion of this appraoch can be found int he journal article also uploaded to gitHub.  You may find the OPW algorithm particularly useful.

	
	
	