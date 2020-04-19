
# mp1.S - life-or-death version
# Good luck, and have fun!

.data
	
    # Constants for accessing the fields of a struct keystroke_args
    # struct keystroke_args is defined in rtc.h and mp1.h

	KA_SIZE	  = 14   # 14 bytes when copying
	KA_STACK  = 16   # use 16 bytes on stack for alignment
	DIRECTION = 0
	SELECTOR  = 2
	GUESS     = 4
	HINT      = 9

    # Constants for accessing the fields of a struct game_status
    # struct game_status is defined in rtc.h and mp1.h

	GS_SIZE   = 8	# 8 bytes
	POP 	  = 0
	INF	  = 4

    # Constants for screen size

    SCR_WIDTH  = 80
	SCR_HEIGHT = 20
	SCR_SIZE   = ((SCR_WIDTH) * (SCR_HEIGHT))

    # ASCII cell images

    CELL_LIVE = 0x78
	CELL_DEAD = 0x20


# Data shared between this file and rtc.c helper functions

# This '.globl' directive makes these symbols visible externally
.globl rand_seed, aggression

			.align 8
rand_seed:		.quad 0x0	# Pseudo-random number seed
aggression:		.long 0x0	# Aggression level of virus


# Data private to this file

current_board:		.long 0x0	# Address of current board

next_board:		.long 0x0	# Address of next board
population:		.long 0x0	# Remaining human population
infection:		.long 0x0	# Count of live virus cells
select_image:		.string "==>"	# Picture of the selector
select_blank:		.string "   "	# Picture to overwrite selector
base_array:             .string "ACGT"  # Array of characters for bases


.text

# void mp1_poke(void);
# You can use this function to write to video memory.
#
# Interface: Register-based arguments (not C-style)
#    Inputs: %cl  - The byte you wish to write
#            %eax - Offset from the start of video memory that you wish
#                   to write to
#   Outputs: Text-mode video screen is written to at location %eax with
#            the byte in %cl
# Registers: Clobbers EDX

mp1_poke:
		movl    vmem_base_addr(,1),%edx
		movb    %cl,(%edx,%eax,1)
		ret

#   ----------------- Exported functions ---------------------

# void mp1_rtc_tasklet(unsigned long ignore);
# Performs three tasks:
#	(1) updates the life game board and calculates infection count
#	(2) swaps the two boards
#	(3) applies population reduction based on infection count
#	(4) redraws the screen
#	(5) notifies the user code by calling mp1_notify_user
# Inputs   : none
# Outputs  : none
# Registers: Standard C calling convention
# ecx :counter for total cell
.globl mp1_rtc_tasklet
mp1_rtc_tasklet:
		pushl	%ebp
		movl	%esp, %ebp

		pushl	%ebx					# counter for width loop
		pushl	%esi					# counter for height loop
		pushl	%edi					# offset for every cell in each boards
		
		xorl	%ecx, %ecx				# counter for the live cell in the next board
				
		movl	current_board, %eax
		cmpl	$0, %eax					# check the value of current_board 
		jz	TASKLET_DONE

		movl	$1,	%ebx
	WIDTH_LOOP:
		cmpl	$SCR_WIDTH-1, %ebx
		jge	WIDTH_LOOP_DONE
		movl	$1, %esi
	HEIGHT_LOOP:
		cmpl	$SCR_HEIGHT-1, %esi
		jge	HEIGHT_LOOP_DONE
		imull	$SCR_WIDTH, %esi, %edi 	
		addl	%ebx, %edi				# edi <- offset
		movl	%edi, %eax
		addl	current_board,%eax		# eax <- unsigned char* cell
	
		pushl 	%ecx					# store the caller saved register

		pushl	%eax
		call	neighbor_count			# call the function neighbor_count for each cell
		addl	$4, %esp

		pushl	%eax					# push int neighbor
		movl	current_board, %edx
		addl	%edi, %edx
		pushl	(%edx)					# push unsigned char cur
		call	tick_result				# call the function tick_result
		addl	$8, %esp

		popl	%ecx					# pop back the caller saved register

		movl	next_board, %edx
		addl 	%edi, %edx				# edi <- pointer to the cell in the next board
		movb	%al,(%edx)
		addl	%eax, %ecx  			# count the live cells' number
		incl	%esi
		jmp	HEIGHT_LOOP
	HEIGHT_LOOP_DONE:
		incl	%ebx
		jmp	WIDTH_LOOP
	WIDTH_LOOP_DONE:
		imull	$10, %ecx				# time the cell number by 10
		movl	%ecx, infection															

	# second step	
		movl	current_board, %eax		# swap the two boards
		movl 	next_board, %ecx
		movl	%eax, next_board
		movl	%ecx, current_board

	# third step
		movl	population, %eax		# subtract the new infection value from the population
		cmpl 	infection, %eax
		jb 		POPULATION_NEGATIVE
		subl	infection,	%eax
		movl	%eax, population
		jmp 	POPULATION_NOT_NEGATIVE
	POPULATION_NEGATIVE:
		movl 	$0, population
	POPULATION_NOT_NEGATIVE:
		
	# fourth step
		call draw_screen				# call the draw_screen

	
	# fifth step
		call mp1_notify_user			
	
	TASKLET_DONE:
		popl	%edi
		popl	%esi
		popl	%ebx	# restore the two registers
		leave
		ret


# int mp1_ioctl(unsigned long arg, unsigned int cmd)
# The dispatch function for the MP1 ioctls - should use the cmd argument
# and a jumptable to execute one of the specific ioctls implemented below.
# Inputs   : unsigned long arg - parameter to the mp1_ioctl_....
#          : unsigned int cmd  - specifies which mp1_ioctl_... function 
#          :                     to execute
# Outputs  : Returns an integer - depends on which ioctl() function is called
# Registers: Standard C calling convention

.globl mp1_ioctl
mp1_ioctl:
		movl	8(%esp), %edx	# %edx	<-	cmd
		cmpl 	$4, %edx
		ja	CMD_FAIL		# check whether cmd is in [0,4] 
		jmp		*jump_table(,%edx,4)		
	CMD_FAIL:
		movl	$-1, %eax
		ret


# ----------------- Functions private to this file -------------------

# void draw_screen(void)
draw_screen:	
	 # For simplicity, your implementation may assume a SCR_WIDTH of 80.
	 	pushl	%ebp
	 	movl	%esp, %ebp
	 	pushl	%edi					# offset for every cell
		pushl	%ebx
		pushl	%esi
	 	movl	$0,	%edx				
	 DRAW_LOOP:
	 	cmpl	$SCR_SIZE, %edx
	 	jge	DRAW_LOOP_DONE

	 	movl 	%edx, %edi				# edi <- offset

	 	imull	$2,%edi,%eax			# eax <- 2*(80*height + width)
	 	addl	current_board, %edi		# edi <- the pointer to each cell
	 	movb	(%edi), %bl			# %edi <- 0 or 1 for cell's condition

	  	cmpb	$0, %bl
	 	jz	CELL_IS_DEAD				# set up the value for cl
		movb $CELL_LIVE, %cl
	  	jmp	CL_SET_DONE
	  CELL_IS_DEAD:
	  	movb $CELL_DEAD, %cl
	  CL_SET_DONE:

	 	pushl	%edx
	 	call 	mp1_poke
	 	popl	%edx					# caller saved edx

	 	incl	%edx
	 	jmp	DRAW_LOOP
	 DRAW_LOOP_DONE:
	 	popl	%esi
		popl	%ebx
	 	popl	%edi
	 	leave
	 	ret

# %ecx counter for filling the board with 0
mp1_ioctl_startgame:
		pushl	%ebp
		movl	%esp, %ebp

		pushl	%edi					# offset for every cell
		pushl	%ebx
		pushl	%esi

		pushl	8(%ebp)				# call the seed_generator funciton 
		call 	seed_generator			
		addl	$4, %esp
		
		pushl	$SCR_SIZE				# allocate the memory for the current_board
		call	mp1_malloc
		addl	$4, %esp
		cmpl	$0, %eax
		jz	MALLOC_FAIL1
		movl	%eax, current_board	

		pushl	$SCR_SIZE				# allocate the memory for the next_board
		call	mp1_malloc
		addl	$4, %esp
		cmpl	$0, %eax
		jz	MALLOC_FAIL2
		movl	%eax, next_board

		xorl	%ecx, %ecx				# the counter used to loop the whole board
	INITIAL0_LOOP:
		cmpl	$SCR_SIZE, %ecx
		jge	INITIAL0_LOOP_DONE
		movl 	%ecx, %eax
		addl	current_board, %eax
		movb	$0, (%eax)				# fill two boards with zero
		movl 	%ecx, %eax
		addl	next_board, %eax
		movb	$0, (%eax)
		incl	%ecx
		jmp	INITIAL0_LOOP

	INITIAL0_LOOP_DONE:
		pushl	current_board			# call init_virus on the current_board
		call	init_virus
		addl	$4, %esp
		movl	%eax, infection			# infection <- the return value 
		movl	$80, aggression				# aggression <- 80
		movl	$8000000, population		# population <- 80	
		movl	$0, %eax
		jmp 	START_DONE

	MALLOC_FAIL2:
		pushl	current_board			# if the second allocation failed, free the first allocation
		call	mp1_free
		addl	$4, %esp
		movl	$-1, %eax
	MALLOC_FAIL1:
		movl	$0, current_board		# set both two board to 0
		movl 	$0, neighbor_count
		movl	$-1, %eax				# return -1

	START_DONE:
		popl	%esi
		popl	%ebx
	 	popl	%edi
		leave
		ret

mp1_ioctl_endgame:
		pushl	%ebp
		movl	%esp, %ebp
		pushl	%edi					# offset for every cell
		pushl	%ebx
		pushl	%esi
		pushl	current_board			# free the current_board	
		call	mp1_free
		addl	$4, %esp
		
		pushl	next_board				# free the next_board
		call 	mp1_free
		addl	$4, %esp
		
		movl	$0, current_board		# set both two board to 0
		movl	$0, next_board
		movl 	$0, %eax				# return 0
		popl	%esi
		popl	%ebx
	 	popl	%edi
		leave
		ret

mp1_ioctl_keystroke:
		pushl	%ebp
		movl	%esp, %ebp
		pushl	%edi					# offset for every cell
		pushl	%ebx
		pushl	%esi
		cmpl	$0,8(%ebp)			# check whether the pointer is null
		jz 		KEYSTROKE_FAIL

		subl 	$KA_STACK, %esp		# leave the space for the structure
		movl	%esp, %edx 			# %edx <- pointer to the structure

		pushl	%edx				# store the caller saved register
		pushl	$KA_SIZE			# push n
		pushl	8(%ebp)				# push *from
		pushl 	%edx				# push *to
		call 	mp1_copy_from_user
		addl	$12, %esp
		popl	%edx


		cmpl	$0,	%eax			# check whether the call is failed
		jnz		KEYSTROKE_FAIL	
	KEYSTROKE_SUCCESS:
		movl	$0, %ecx			
		movl	$0, %eax
		movw	(%edx), %ax			# %ax <- direction
		movw	2(%edx), %cx		# %cx <- selector(S)

		imull 	$10, %ecx, %ebx		# %ebx <- 10S
		addl	$14, %ebx 			# %ebx <- 14+10S
		addl	$1760, %ebx			# %ebx <- 14+10S + 22*80 (because 22*80 = 1760)	

		cmpw	$0, %ax 			# left?
		jz 		LEFT
		cmpw 	$2,	%ax				# right?
		jz 		RIGHT

		addl	$4, %ebx			# %ebx <- 18+10S + 22*80

		cmpw 	$1, %ax 			# down?
		jz 		DOWN
		cmpw	$3, %ax
		jz 	UP						# defalt: UP
		jmp		KEYSTROKE_FAIL
	LEFT:
		# clear the current select
		imull	$2, %ebx, %eax		# eax <- 2*(14+10S + 22*80)

		pushl	%ecx
		pushl	%edx
		movb 	select_blank, %cl	# " "
		call	mp1_poke
		
		addl	$2,%eax
		movb	select_blank+1, %cl	# "  "	
		call 	mp1_poke

		addl	$2,%eax
		movb	select_blank+2, %cl # "   "
		call 	mp1_poke
		popl 	%edx
		popl	%ecx

		# draw the new select
		imull	$2, %ebx, %eax		# eax <- 2*(14+10S + 22*80)
		cmpw	$0,	%cx				# check whether the selector is 0
		jnz		CAN_LEFT
		addw	$5, %cx				# change S from 0 to 5(although there is no "5" state)
		addl	$100, %eax			# for 2*(14+10S + 22*80), S is added 5, so the offeset increases 100
	CAN_LEFT:
		subw 	$1, %cx				# turn left means S -1
		subl	$20, %eax			# change the corresponding offset

		pushl	%ecx
		pushl	%edx
		movb 	select_image, %cl	# draw"-"
		call	mp1_poke
		
		addl	$2,%eax
		movb	select_image+1, %cl	# draw"--"	
		call 	mp1_poke

		addl	$2,%eax
		movb	select_image+2, %cl # draw"-->"
		call 	mp1_poke
		popl 	%edx
		popl	%ecx

		movw	%cx, 2(%edx)
		jmp 	DIRECTION_DONE
	RIGHT:
		# clear the current select
		imull	$2, %ebx, %eax		# eax <- 2*(14+10S + 22*80)

		pushl	%ecx
		pushl	%edx
		movb 	select_blank, %cl	# draw" "
		call	mp1_poke
		
		addl	$2,%eax
		movb	select_blank+1, %cl	# draw"  "	
		call 	mp1_poke

		addl	$2,%eax
		movb	select_blank+2, %cl # draw"   "
		call 	mp1_poke
		popl 	%edx
		popl	%ecx

		# draw the new select
		imull	$2, %ebx, %eax		# eax <- 2*(14+10S + 22*80)
		cmpw	$4,	%cx				# check whether the selector is 4
		jnz		CAN_RIGHT
		subw	$5, %cx				# change S from 0 to 5(although there is no "5" state)
		subl	$100, %eax			# for 2*(14+10S + 22*80), S is added 5, so the offeset increases 100
	CAN_RIGHT:
		addw 	$1, %cx				# turn left means S -1
		addl	$20, %eax			# change the corresponding offset

		pushl	%ecx
		pushl	%edx
		movb 	select_image, %cl	# "-"
		call	mp1_poke
		
		addl	$2,%eax
		movb	select_image+1, %cl	# "--"	
		call 	mp1_poke

		addl	$2,%eax
		movb	select_image+2, %cl # "-->"
		call 	mp1_poke
		popl 	%edx
		popl	%ecx

		movw	%cx, 2(%edx)
		jmp 	DIRECTION_DONE
	UP:
		pushl	%ebx
		pushl	%edx
		pushl	%ecx

		movl	$0,	%eax
		movl	%ecx, %esi
		movb	4(%edx,%esi), %cl		# %cl <- the base from 0 to 3
		movb	9(%edx,%esi), %al 		# %al <- the hint 
		movl	$1, %edx
		# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		shlb	%cl, %dl				# %dl <- 1000 or 0100 or 0010 or 0001
										#			3		2		1		0 
			
	UP_LOOP:
		cmpb	$8, %dl					# check whether the base should return to 1
		jnz	CAN_UP
		movb	$1, %dl					# if the current base is 3, we need turn it to 0
		movl	$0, %ecx
		jmp	UP_TEST
	CAN_UP:
		shlb	$1, %dl
		incl	%ecx					# use %ecx to trace the current base
	UP_TEST:
		testb	%al, %dl 				# whether the guess is in the hint
		jz UP_LOOP

		movzbl 	%cl, %esi				# %esi <- base
		movl 	$0, %ebx
		movb 	%cl, %bl				# %bl <- base
		popl	%ecx
		popl	%edx
		movb 	%bl, 4(%edx,%ecx)		# store back the base
		movb	base_array(%esi), %cl 	# %cl <- ACGT
		popl 	%ebx
		imull	$2, %ebx, %eax				
		pushl	%edx
		call	mp1_poke
		popl	%edx
		jmp 	DIRECTION_DONE
	DOWN:
		pushl	%ebx
		pushl	%edx
		pushl	%ecx

		movl	$0,	%eax
		movl 	%ecx, %esi
		movb	4(%edx,%esi), %cl		# %cl <- the base from 0 to 3
		movb	9(%edx,%esi), %al 		# %al <- the hint 
		movl	$1, %edx
		# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		shlb	%cl, %dl				# %dl <- 1000 or 0100 or 0010 or 0001
										#			3		2		1		0 
			
	DOWN_LOOP:
		cmpb	$1, %dl					# check whether the base should return to 1
		jnz	CAN_DOWN
		movb	$8, %dl					# if the current base is 0, we need turn it to 3
		movl	$3, %ecx
		jmp	DOWN_TEST
	CAN_DOWN:
		shrb	$1, %dl
		subl	$1,	%ecx					# use %ecx to trace the current base
	DOWN_TEST:
		testb	%al, %dl 				# whether the guess is in the hint
		jz DOWN_LOOP

		movzbl 	%cl, %esi				# %esi <- base
		movl 	$0, %ebx
		movb 	%cl, %bl				# %bl <- base
		popl	%ecx
		popl	%edx
		movb 	%bl, 4(%edx,%ecx)		# store back the base
		movb	base_array(%esi), %cl 	# %cl <- ACGT
		popl 	%ebx
		imull	$2, %ebx, %eax				
		pushl	%edx
		call	mp1_poke
		popl	%edx
		jmp 	DIRECTION_DONE
	DIRECTION_DONE:

		# return the structure back
		pushl 	%edx 
		pushl	$KA_SIZE			# push n
		pushl 	%edx				# push *from
		pushl	8(%ebp)				# push *to
		call 	mp1_copy_to_user
		addl	$12, %esp
		popl	%edx

		cmpl	$0,	%eax			# check whether the call is failed
		jnz		KEYSTROKE_FAIL	

	KEYSTROKE_DONE:
		movl	$0, %eax
		popl	%esi
		popl	%ebx
		popl	%edi
		leave
		ret
	KEYSTROKE_FAIL:
		movl	$-1, %eax
		popl	%esi
		popl	%ebx
	 	popl	%edi
		leave
		ret

mp1_ioctl_getstatus:
 		pushl	%ebp
		movl	%esp, %ebp
		cmpl	$0,8(%ebp)			# %eax <- *user_status
		jz	GETSTATUS_FAILED
		subl	$GS_SIZE, %esp		# leave the struct space
		movl 	%esp, %edx			# pointer to the structure
		movl 	population, %ecx	# use to store perameter
		movl	%ecx, (%edx)
		movl 	infection, %ecx
		movl	%ecx, 4(%edx)

		pushl 	$GS_SIZE			# n	
		pushl	%edx
		pushl 	8(%ebp)
		call	mp1_copy_to_user
		addl	$12,%esp
		addl	$GS_SIZE,%esp
		cmpl 	$0, %eax
		jz 	GETSTATUS_SUCCESS
	GETSTATUS_FAILED:
		movl 	$-1, %eax
	GETSTATUS_SUCCESS:
		leave
		ret

mp1_ioctl_vaccinate:
		pushl	%ebp
		movl	%esp, %ebp
		pushl	%edi					# offset for every cell
		pushl	%ebx
		pushl	%esi
		movw 	10(%ebp), %cx			# %cx <- amount should be added to aggression
 		addw	%cx, aggression
		movw	8(%ebp), %dx 			# %dx <- percentage chance

		movl	$0,	%ecx				# use as loop counter				

	VACCINATE_LOOP:
			cmpl	$SCR_SIZE, %ecx
			jge	VACCINATE_LOOP_DONE
			movl	%ecx, %ebx
			addl	current_board, %ebx		# %ebx <- current_board + offset
			movl 	(%ebx), %eax 			# %eax <- 0 or 1 (live of dead)
			cmpl 	$0, %eax
			jz 	CELL_NEEDNOT_CHANGE

			pushl	%edx					# store the caller saved registers ecx and edx
			pushl	%ecx					
			call	generate 				# get a random number
			popl	%ecx
			popl	%edx
			

			pushl 	%edx					# store the registers
			pushl	%ecx
			movl 	$0, %edx
			movl 	$100, %ecx
			idiv	%ecx					# edx <- edx:eax % 100 
			movl	%edx, %eax				# let eax store the remainder
			popl	%ecx
			popl	%edx

			cmpw 	%dx,%ax					# check whether it can kill the cell
			jae 	CELL_NEEDNOT_CHANGE		# if the random number is above or equal than the chance, it can not kill the cell
			movl	$0,	(%ebx)
		CELL_NEEDNOT_CHANGE:
			incl	%ecx
			jmp	VACCINATE_LOOP
	VACCINATE_LOOP_DONE:
		popl	%esi
		popl	%ebx
	 	popl	%edi
		movl 	$0, %eax
		leave
		ret

jump_table:		.long	mp1_ioctl_startgame, mp1_ioctl_endgame, mp1_ioctl_keystroke, mp1_ioctl_getstatus, mp1_ioctl_vaccinate
