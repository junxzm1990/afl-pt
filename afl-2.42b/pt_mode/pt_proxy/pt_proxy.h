enum proxy_status{
	PROXY_SLEEP = 0,                                /* proxy initial state            */
	PROXY_START,                                    /* upon receiving SCONFIRM_msg    */
	PROXY_FORKSRV,                                  /* upon receiving TCONFIRM_msg    */
	PROXY_FUZZ_RDY,                                 /* upon receiving TOPA_RDY_msg    */
                                                  /*       OR       AFL start cmd   */
	PROXY_FUZZ_ING,                                 /* upon receiving fuzz target pid */
	PROXY_FUZZ_STOP,                                /* upon receiving target status   */
	UNKNOWN
};

enum msg_type{
	START_CONFIRM = 0,
	TARGET_CONFIRM,
	TOPA_RDY,
	PTNEXT,
	ERROR
};


void proxy_recv_msg();
void proxy_send_msg(char *msg);
//inline void
//pt_parse_packet(char *buffer, size_t size, int fd0, int fd1);



