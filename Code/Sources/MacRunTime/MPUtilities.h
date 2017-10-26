// =================================================================================// MPUtilities.h// =================================================================================// General utilities to assist with multi-processor execution#pragma once// Defines#define				MPU_PRINT_BUF_SIZE	2048							// Size of MP printf buffer// Global variablesextern	long		MPU_num_processors;									// No. of processors availableextern	long		MPU_task_is_running;								// True if MP task is runningextern	long 		MPU_task_should_stop;								// True to terminate taskextern	long 		MPU_task_should_yield;								// True if task should yieldextern	long 		MPU_task_has_terminated;							// True if task has terminatedextern	char		MPU_print_out_buf[MPU_PRINT_BUF_SIZE];				// Holds chars printed from MP or TM Taskextern	long		MPU_print_buf_rd;									// Read pointerextern	long		MPU_print_buf_wr;									// Write pointerextern	void		(*MPU_synchronize_io)();							// Points to empty routine, or SynchronizeIO if DSL available MP lib availableextern	void	 	(*MPU_task)();										// What gets executed// Routinesextern	void 		MPU_post_output_character(char it);										// MP   ONLY (!!): post a character for later outputextern	void 		MPU_post_output_character_from_TM_or_DT(char it);						// HOST ONLY (!!): post a character for later outputextern	void 		MPU_dump_posted_MP_output_characters(void (*out_routine)(char it));		// HOST ONLY (!!): flush posted charactersextern	void		MPU_initialize_for_mp(void (*theTask)(), long force_mp_api);			// HOST ONLY (!!): initializeextern	void		MPU_terminate();														// HOST ONLY (!!): force termination