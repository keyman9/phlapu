@ accelerate.s

/* function to alter dragon position, velocity, and gravity relative to one another */
/* given 	r0: dragon->y */
			/*r1: dragon->yvel*/ 
			/*r2: dragon->gravity	*/

.global accelerate
accelerate:
	add r0,r0,r1		/*y += yvel*/
	add r1,r1,r2		/*yvel += gravity*/
	mov pc, lr			

/* function to  calculate score of player */
/* given r0: total score 
		 r1: total laps completed */

.global gameScore
gameScore:
		mul r0,r1,r0	/*total multiplied by laps */
		add r0,r0,#2    /* add 2 */
		mov r2, #3 		/* store 3 for multiplication */
		mov r3, r0
		mul r0, r3, r2	/* multiply by 3 */
		mov pc, lr

@ uppercase.s

/* function to convert a given string to all uppercase */
.global	uppercase
uppercase:
	sub sp, sp, #12 /*Clear space on stack*/
	str r4, [sp,#0]	/*move stuff away*/
	str r5, [sp,#4]	/*move stuff away*/
	str lr, [sp,#8] /*stack pointer*/

	mov r4, r0	/*move input into temp register*/
	mov r5, #0	/*create index*/
	
	.top:
		ldrb r0, [r4,r5]	/*load the letter by index*/
    	cmp r0, #0			/*is it the null terminator?*/
    	beq .done			/*exit*/
    	bl toupper			/*capitalize it*/
    	strb r0, [r4,r5] 	/*store the capitalized letter*/	
        add r5,r5,#1        /*move index*/
    	b .top

    .done:
        mov r0, r4
    	ldr r4, [sp,#0]	/*reload the */
    	ldr r5, [sp,#4]
    	ldr lr, [sp,#8]
        add sp, sp, #12 
    	mov pc, lr
